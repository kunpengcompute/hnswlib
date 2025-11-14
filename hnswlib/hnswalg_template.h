/*
   Copyright 2025 Huawei Technologies Co., Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */
 
#pragma once

#if defined(HNSWLIB_USE_ARM)

#include "visited_list_pool.h"
#include "hnswlib.h"
#include "safe_memory.h"
#include <atomic>
#include <random>
#include <stdlib.h>
#include <unordered_set>
#include <list>
#include <memory>
#include <fstream>
#include <iostream>
#include <omp.h>

#if !defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC) && !defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
typedef __fp16 float16_t;
#else
// 如果ARM FP16特性已启用，确保float16_t类型存在
#ifndef __fp16
typedef __fp16 float16_t;
#endif
#endif

using namespace SafeMemory;

namespace hnswlib {

template<typename T>
using PriorityType = typename std::conditional<
    std::is_same<T, float>::value || std::is_same<T, float16_t>::value,
    float,
    int
>::type;

template<typename T>
class HierarchicalNSWTemplate : public AlgorithmInterface<PriorityType<T>> {
public:
    typedef size_t labeltype;
    typedef unsigned int tableint;
    typedef unsigned int linklistsizeint;

    static const tableint MAX_LABEL_OPERATION_LOCKS = 65536;
    static const unsigned char DELETE_MARK = 0x01;

    size_t max_elements_{0};
    mutable std::atomic<size_t> cur_element_count_{0};
    size_t size_data_per_element_{0};
    size_t size_links_per_element_{0};
    mutable std::atomic<size_t> num_deleted_{0};
    size_t M_{0};
    size_t maxM_{0};
    
    size_t maxM0_{0};
    size_t ef_construction_{0};
    size_t ef_{ 0 };

    double mult_{0.0}, revSize_{0.0};
    int maxlevel_{0};

    std::unique_ptr<VisitedListPool> visited_list_pool_{nullptr};
    mutable std::vector<std::mutex> label_op_locks_;
    std::mutex global;
    std::vector<std::mutex> link_list_locks_;

    tableint enterpoint_node_{0};
    size_t size_links_level0_{0};
    size_t offsetData_{0}, offsetLevel0_{0}, label_offset_{ 0 };

    char *data_level0_memory_{nullptr};
    char **linkLists_{nullptr};
    std::vector<int> element_levels_;

    size_t data_size_{0};
    DISTFUNC<PriorityType<T>> fstdistfunc_;
    void *dist_func_param_{nullptr};

    mutable std::mutex label_lookup_lock;
    std::unordered_map<labeltype, tableint> label_lookup_;

    std::default_random_engine level_generator_;
    std::default_random_engine update_probability_generator_;

    mutable std::atomic<long> metric_distance_computations{0};
    mutable std::atomic<long> metric_hops{0};

    bool allow_replace_deleted_ = false;
    std::mutex deleted_elements_lock;
    std::unordered_set<tableint> deleted_elements;

    // 构造函数
    HierarchicalNSWTemplate(SpaceInterface<PriorityType<T>> *s) {
        throw std::runtime_error("Please use the full constructor with max_elements, M, etc.");
    }

    HierarchicalNSWTemplate(
        SpaceInterface<T> *s,
        const std::string &location,
        bool nmslib = false,
        size_t max_elements = 0,
        bool allow_replace_deleted = false)
        : allow_replace_deleted_(allow_replace_deleted) {
        loadIndex(location, s, max_elements);
    }

    HierarchicalNSWTemplate(
        SpaceInterface<T> *s,
        size_t max_elements,
        size_t M = 16,
        size_t ef_construction = 200,
        size_t random_seed = 100,
        bool allow_replace_deleted = false)
        : label_op_locks_(MAX_LABEL_OPERATION_LOCKS),
            link_list_locks_(max_elements),
            element_levels_(max_elements),
            allow_replace_deleted_(allow_replace_deleted) {
        max_elements_ = max_elements;
        num_deleted_ = 0;
        data_size_ = s->get_data_size();
        fstdistfunc_ = s->get_dist_func();
        dist_func_param_ = s->get_dist_func_param();
        if ( M <= 1) {
            throw std::runtime_error("The value of M must be greater than 1.");
        }
        if ( M <= 10000 ) {
            M_ = M;
        } else {
            std::cerr << "warning: M parameter exceeds 10000 which may lead to adverse effects." << std::endl;
            std::cerr << "         Cap to 10000 will be applied for the rest of the processing." << std::endl;
            M_ = 10000;
        }
        maxM_ = M_;
        maxM0_ = M_ * 2;
        ef_construction_ = std::max(ef_construction, M_);
        ef_ = 10;

        level_generator_.seed(random_seed);
        update_probability_generator_.seed(random_seed + 1);

        size_links_level0_ = maxM0_ * sizeof(tableint) + sizeof(linklistsizeint);
        size_data_per_element_ = size_links_level0_ + data_size_ + sizeof(labeltype);
        offsetData_ = size_links_level0_;
        label_offset_ = size_links_level0_ + data_size_;
        offsetLevel0_ = 0;

        data_level0_memory_ = (char *) malloc(max_elements * size_data_per_element_);
        if (data_level0_memory_ == nullptr)
            throw std::runtime_error("Not enough memory");

        cur_element_count_ = 0;

        visited_list_pool_ = std::unique_ptr<VisitedListPool>(new VisitedListPool(1, max_elements));

        // initializations for special treatment of the first node
        enterpoint_node_ = -1;
        maxlevel_ = -1;

        try {
            linkLists_ = (char **)malloc(sizeof(void *) * max_elements);
            if (linkLists_ == nullptr)
                throw std::runtime_error("Not enough memory: HierarchicalNSW failed to allocate linklists");
        } catch (const std::exception &e) {
            free(data_level0_memory_);
            data_level0_memory_ = nullptr;
            throw;
        }
        size_links_per_element_ = maxM_ * sizeof(tableint) + sizeof(linklistsizeint);
        mult_ = 1 / log(1.0 * M_);
        revSize_ = 1.0 / mult_;
    }

    ~HierarchicalNSWTemplate() {
        clear();
    }

    void clear() {
        free(data_level0_memory_);
        data_level0_memory_ = nullptr;
        for (tableint i = 0; i < cur_element_count_; i++) {
            if (element_levels_[i] > 0)
                free(linkLists_[i]);
        }
        free(linkLists_);
        linkLists_ = nullptr;
        cur_element_count_ = 0;
        visited_list_pool_.reset(nullptr);
    }

    // 比较器结构体
    struct CompareByFirst {
        constexpr bool operator()(std::pair<PriorityType<T>, tableint> const& a,
            std::pair<PriorityType<T>, tableint> const& b) const noexcept {
            return a.first < b.first;
        }
    };

    void setEf(size_t ef) {
        ef_ = ef;
    }

    inline std::mutex& getLabelOpMutex(labeltype label) const {
        // calculate hash
        size_t lock_id = label & (MAX_LABEL_OPERATION_LOCKS - 1);
        return label_op_locks_[lock_id];
    }

    inline labeltype getExternalLabel(tableint internal_id) const {
		labeltype return_label;
        CheckAndMemcpy(&return_label,
                       sizeof(labeltype),
                       data_level0_memory_ + (size_t)internal_id * size_data_per_element_ + label_offset_,
                       sizeof(labeltype));
		return return_label;
	}

    inline void setExternalLabel(tableint internal_id, labeltype label) const {
        CheckAndMemcpy((data_level0_memory_ + internal_id * size_data_per_element_ + label_offset_),
                       sizeof(labeltype),
                       &label,
                       sizeof(labeltype));
    }

    inline labeltype *getExternalLabeLp(tableint internal_id) const {
		return reinterpret_cast<labeltype *>(
			data_level0_memory_ + (size_t)internal_id * size_data_per_element_ + label_offset_);
	}

    inline char *getDataByInternalId(tableint internal_id) const {
		return data_level0_memory_ + (size_t)internal_id * size_data_per_element_ + offsetData_;
	}


    int getRandomLevel(double reverse_size) {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        double r = -log(distribution(level_generator_)) * reverse_size;
        return (int) r;
    }

    size_t getMaxElements() {
        return max_elements_;
    }

    size_t getCurrentElementCount() {
        return cur_element_count_;
    }

    size_t getDeletedCount() {
        return num_deleted_;
    }

    
    // 搜索基础层
    std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst>
    searchBaseLayer(tableint ep_id, const void *data_point, int layer) {
        VisitedList *vl = visited_list_pool_->getFreeVisitedList();
        vl_type *visited_array = vl->mass;
        vl_type visited_array_tag = vl->curV;

        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> top_candidates;
        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> candidateSet;

        PriorityType<T> lowerBound;
        if (!isMarkedDeleted(ep_id)) {
            PriorityType<T> dist = fstdistfunc_(data_point, getDataByInternalId(ep_id), dist_func_param_);
            top_candidates.emplace(dist, ep_id);
            lowerBound = dist;
            candidateSet.emplace(-dist, ep_id);
        } else {
            lowerBound = std::numeric_limits<PriorityType<T>>::max();
            candidateSet.emplace(-lowerBound, ep_id);
        }
        visited_array[ep_id] = visited_array_tag;

        while (!candidateSet.empty()) {
            std::pair<PriorityType<T>, tableint> curr_el_pair = candidateSet.top();
            if ((-curr_el_pair.first) > lowerBound && top_candidates.size() == ef_construction_) {
                break;
            }
            candidateSet.pop();

            tableint curNodeNum = curr_el_pair.second;

            std::unique_lock <std::mutex> lock(link_list_locks_[curNodeNum]);

            int *data;
            if (layer == 0) {
                data = (int*)get_linklist0(curNodeNum);
            } else {
                data = (int*)get_linklist(curNodeNum, layer);
            }
            size_t size = getListCount((linklistsizeint*)data);
            tableint *datal = (tableint *) (data + 1);
#if defined(USE_SVE) || defined(USE_NEON)
            __builtin_prefetch((visited_array + *(data + 1)), 0, 3);
            __builtin_prefetch((visited_array + *(data + 1) + 64), 0, 3);
            __builtin_prefetch(getDataByInternalId(*datal), 0, 3);
            __builtin_prefetch(getDataByInternalId(*(datal + 1)), 0, 3);
#endif

            for (size_t j = 0; j < size; j++) {
                tableint candidate_id = *(datal + j);
#if defined(USE_SVE) || defined(USE_NEON)
                __builtin_prefetch((visited_array + *(datal + j + 1)), 0, 3);
                __builtin_prefetch(getDataByInternalId(*(datal + j + 1)), 0, 3);
#endif
                if (visited_array[candidate_id] == visited_array_tag) continue;
                visited_array[candidate_id] = visited_array_tag;
                char *currObj1 = (getDataByInternalId(candidate_id));

                PriorityType<T> dist1 = fstdistfunc_(data_point, currObj1, dist_func_param_);
                if (top_candidates.size() < ef_construction_ || lowerBound > dist1) {
                    candidateSet.emplace(-dist1, candidate_id);
#if defined(USE_SVE) || defined(USE_NEON)
                    __builtin_prefetch(getDataByInternalId(candidateSet.top().second), 0, 3);
#endif
                    if (!isMarkedDeleted(candidate_id))
                        top_candidates.emplace(dist1, candidate_id);

                    if (top_candidates.size() > ef_construction_)
                        top_candidates.pop();

                    if (!top_candidates.empty())
                        lowerBound = top_candidates.top().first;
                }
            }
        }
        visited_list_pool_->releaseVisitedList(vl);

        return top_candidates;
    }

    // 搜索基础层（单线程版本）
    template <bool bare_bone_search = true, bool collect_metrics = false>
    std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst>
    searchBaseLayerST(
        tableint ep_id,
        const void *data_point,
        size_t ef,
        BaseFilterFunctor* isIdAllowed = nullptr,
        BaseSearchStopCondition<PriorityType<T>>* stop_condition = nullptr) const {
        VisitedList *vl = visited_list_pool_->getFreeVisitedList();
        vl_type *visited_array = vl->mass;
        vl_type visited_array_tag = vl->curV;

        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> top_candidates;
        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> candidate_set;

        PriorityType<T> lowerBound;
        if (bare_bone_search || 
            (!isMarkedDeleted(ep_id) && ((!isIdAllowed) || (*isIdAllowed)(getExternalLabel(ep_id))))) {
            char* ep_data = getDataByInternalId(ep_id);
            PriorityType<T> dist = fstdistfunc_(data_point, ep_data, dist_func_param_);
            lowerBound = dist;
            top_candidates.emplace(dist, ep_id);
            if (!bare_bone_search && stop_condition) {
                stop_condition->add_point_to_result(getExternalLabel(ep_id), ep_data, dist);
            }
            candidate_set.emplace(-dist, ep_id);
        } else {
            lowerBound = std::numeric_limits<PriorityType<T>>::max();
            candidate_set.emplace(-lowerBound, ep_id);
        }

        visited_array[ep_id] = visited_array_tag;

        while (!candidate_set.empty()) {
            std::pair<PriorityType<T>, tableint> current_node_pair = candidate_set.top();
            PriorityType<T> candidate_dist = -current_node_pair.first;

            bool flag_stop_search;
            if (bare_bone_search) {
                flag_stop_search = candidate_dist > lowerBound;
            } else {
                if (stop_condition) {
                    flag_stop_search = stop_condition->should_stop_search(candidate_dist, lowerBound);
                } else {
                    flag_stop_search = candidate_dist > lowerBound && top_candidates.size() == ef;
                }
            }
            if (flag_stop_search) {
                break;
            }
            candidate_set.pop();

            tableint current_node_id = current_node_pair.second;
            int *data = (int *) get_linklist0(current_node_id);
            size_t size = getListCount((linklistsizeint*)data);
            if (collect_metrics) {
                metric_hops++;
                metric_distance_computations+=size;
            }
#if defined(USE_SVE) || defined(USE_NEON)
            __builtin_prefetch((visited_array + *(data + 1)), 0, 3);
            __builtin_prefetch((visited_array + *(data + 1) + 64), 0, 3);
            __builtin_prefetch((visited_array + *(data + 1)), 0, 3);
            __builtin_prefetch((visited_array + *(data + 1)), 0, 3);
#endif
            for (size_t j = 1; j <= size; j++) {
                int candidate_id = *(data + j);
#if defined(USE_SVE) || defined(USE_NEON)
                __builtin_prefetch((visited_array + *(data + j + 1)), 0, 3);
                __builtin_prefetch(data_level0_memory_ + (*(data + j + 1)) * size_data_per_element_ + offsetData_, 0, 3);
#endif
                if (!(visited_array[candidate_id] == visited_array_tag)) {
                    visited_array[candidate_id] = visited_array_tag;

                    char *currObj1 = (getDataByInternalId(candidate_id));
                    PriorityType<T> dist = fstdistfunc_(data_point, currObj1, dist_func_param_);

                    bool flag_consider_candidate;
                    if (!bare_bone_search && stop_condition) {
                        flag_consider_candidate = stop_condition->should_consider_candidate(dist, lowerBound);
                    } else {
                        flag_consider_candidate = top_candidates.size() < ef || lowerBound > dist;
                    }

                    if (flag_consider_candidate) {
                        candidate_set.emplace(-dist, candidate_id);
#if defined(USE_SVE) || defined(USE_NEON)
                        __builtin_prefetch(data_level0_memory_ + candidate_set.top().second * size_data_per_element_ + 
                                           offsetLevel0_, 0, 3);
#endif
                        if (bare_bone_search || 
                            (!isMarkedDeleted(candidate_id) && ((!isIdAllowed) || (*isIdAllowed)(getExternalLabel(candidate_id))))) {
                            top_candidates.emplace(dist, candidate_id);
                            if (!bare_bone_search && stop_condition) {
                                stop_condition->add_point_to_result(getExternalLabel(candidate_id), currObj1, dist);
                            }
                        }

                        bool flag_remove_extra = false;
                        if (!bare_bone_search && stop_condition) {
                            flag_remove_extra = stop_condition->should_remove_extra();
                        } else {
                            flag_remove_extra = top_candidates.size() > ef;
                        }
                        while (flag_remove_extra) {
                            tableint id = top_candidates.top().second;
                            top_candidates.pop();
                            if (!bare_bone_search && stop_condition) {
                                stop_condition->remove_point_from_result(getExternalLabel(id), getDataByInternalId(id), dist);
                                flag_remove_extra = stop_condition->should_remove_extra();
                            } else {
                                flag_remove_extra = top_candidates.size() > ef;
                            }
                        }

                        if (!top_candidates.empty())
                            lowerBound = top_candidates.top().first;
                    }
                }
            }
        }

        visited_list_pool_->releaseVisitedList(vl);
        return top_candidates;
    }

    void getNeighborsByHeuristic2(
        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> &top_candidates,
        const size_t M) {
        if (top_candidates.size() < M) {
            return;
        }

        std::priority_queue<std::pair<PriorityType<T>, tableint>> queue_closest;
        std::vector<std::pair<PriorityType<T>, tableint>> return_list;
        while (top_candidates.size() > 0) {
            queue_closest.emplace(-top_candidates.top().first, top_candidates.top().second);
            top_candidates.pop();
        }

        while (queue_closest.size()) {
            if (return_list.size() >= M)
                break;
            std::pair<PriorityType<T>, tableint> curent_pair = queue_closest.top();
            PriorityType<T> dist_to_query = -curent_pair.first;
            queue_closest.pop();
            bool good = true;

            for (std::pair<PriorityType<T>, tableint> second_pair : return_list) {
                PriorityType<T> curdist =
                        fstdistfunc_(getDataByInternalId(second_pair.second),
                                        getDataByInternalId(curent_pair.second),
                                        dist_func_param_);
                if (curdist < dist_to_query) {
                    good = false;
                    break;
                }
            }
            if (good) {
                return_list.push_back(curent_pair);
            }
        }

        for (std::pair<PriorityType<T>, tableint> curent_pair : return_list) {
            top_candidates.emplace(-curent_pair.first, curent_pair.second);
        }
    }

    // 获取链接列表
    inline linklistsizeint* get_linklist0(tableint internal_id) const {
		return reinterpret_cast<linklistsizeint*>(
			data_level0_memory_ + (size_t)internal_id * size_data_per_element_ + offsetLevel0_);
	}

    linklistsizeint *get_linklist0(tableint internal_id, char *data_level0_memory_) const {
        return (linklistsizeint *) (data_level0_memory_ + internal_id * size_data_per_element_ + offsetLevel0_);
    }

    linklistsizeint *get_linklist(tableint internal_id, int level) const {
        return (linklistsizeint *) (linkLists_[internal_id] + (level - 1) * size_links_per_element_);
    }

    inline linklistsizeint *get_linklist_at_level(tableint internal_id, int level) const {
		return level == 0 ? get_linklist0(internal_id)
						: reinterpret_cast<linklistsizeint *>(
								linkLists_[internal_id] + (size_t)(level - 1) * size_links_per_element_);
	}


    tableint mutuallyConnectNewElement(
        const void *data_point,
        tableint cur_c,
        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> &top_candidates,
        int level,
        bool isUpdate) {
        size_t Mcurmax = level ? maxM_ : maxM0_;
        getNeighborsByHeuristic2(top_candidates, M_);
        if (top_candidates.size() > M_)
            throw std::runtime_error("Should be not be more than M_ candidates returned by the heuristic");

        std::vector<tableint> selectedNeighbors;
        selectedNeighbors.reserve(M_);
        while (top_candidates.size() > 0) {
            selectedNeighbors.push_back(top_candidates.top().second);
            top_candidates.pop();
        }

        tableint next_closest_entry_point = selectedNeighbors.back();

        {
            std::unique_lock <std::mutex> lock(link_list_locks_[cur_c], std::defer_lock);
            if (isUpdate) {
                lock.lock();
            }
            linklistsizeint *ll_cur;
            if (level == 0)
                ll_cur = get_linklist0(cur_c);
            else
                ll_cur = get_linklist(cur_c, level);

            if (*ll_cur && !isUpdate) {
                throw std::runtime_error("The newly inserted element should have blank link list");
            }
            setListCount(ll_cur, selectedNeighbors.size());
            tableint *data = (tableint *) (ll_cur + 1);
            for (size_t idx = 0; idx < selectedNeighbors.size(); idx++) {
                if (data[idx] && !isUpdate)
                    throw std::runtime_error("Possible memory corruption");
                if (level > element_levels_[selectedNeighbors[idx]])
                    throw std::runtime_error("Trying to make a link on a non-existent level");

                data[idx] = selectedNeighbors[idx];
            }
        }

        for (size_t idx = 0; idx < selectedNeighbors.size(); idx++) {
            std::unique_lock <std::mutex> lock(link_list_locks_[selectedNeighbors[idx]]);

            linklistsizeint *ll_other;
            if (level == 0)
                ll_other = get_linklist0(selectedNeighbors[idx]);
            else
                ll_other = get_linklist(selectedNeighbors[idx], level);

            size_t sz_link_list_other = getListCount(ll_other);

            if (sz_link_list_other > Mcurmax)
                throw std::runtime_error("Bad value of sz_link_list_other");
            if (selectedNeighbors[idx] == cur_c)
                throw std::runtime_error("Trying to connect an element to itself");
            if (level > element_levels_[selectedNeighbors[idx]])
                throw std::runtime_error("Trying to make a link on a non-existent level");

            tableint *data = (tableint *) (ll_other + 1);

            bool is_cur_c_present = false;
            if (isUpdate) {
                for (size_t j = 0; j < sz_link_list_other; j++) {
                    if (data[j] == cur_c) {
                        is_cur_c_present = true;
                        break;
                    }
                }
            }

            if (!is_cur_c_present) {
                if (sz_link_list_other < Mcurmax) {
                    data[sz_link_list_other] = cur_c;
                    setListCount(ll_other, sz_link_list_other + 1);
                } else {
                    PriorityType<T> d_max = fstdistfunc_(getDataByInternalId(cur_c), getDataByInternalId(selectedNeighbors[idx]),
                                                dist_func_param_);
                    std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> candidates;
                    candidates.emplace(d_max, cur_c);

                    for (size_t j = 0; j < sz_link_list_other; j++) {
                        candidates.emplace(
                                fstdistfunc_(getDataByInternalId(data[j]), getDataByInternalId(selectedNeighbors[idx]),
                                                dist_func_param_), data[j]);
                    }

                    getNeighborsByHeuristic2(candidates, Mcurmax);

                    int indx = 0;
                    while (candidates.size() > 0) {
                        data[indx] = candidates.top().second;
                        candidates.pop();
                        indx++;
                    }

                    setListCount(ll_other, indx);
                }
            }
        }

        return next_closest_entry_point;
    }

    void AggressiveRenumberInplace(bool place_deleted_last = true,
                               bool reorder_adj_by_newid = true) {
        std::lock_guard<std::mutex> guard(global);

        const tableint N = static_cast<tableint>(cur_element_count_);
        if (N == 0) return;

        // ---------- active mask ----------
        std::vector<char> is_active(N, 1);
        {
            std::unique_lock<std::mutex> lock_deleted(deleted_elements_lock);
            for (tableint id : deleted_elements) if (id < N) is_active[id] = 0;
        }

        // ---------- 1) 生成新顺序：BFS（贴合 HNSW 访游） ----------
        std::vector<tableint> order; order.reserve(N);
        std::vector<char> vis(N, 0);

        auto bfs_push = [&](tableint s) {
            if (s >= N || vis[s]) return;
            std::queue<tableint> q;
            vis[s] = 1; q.push(s);
            while (!q.empty()) {
                tableint u = q.front(); q.pop();
                if (is_active[u]) order.push_back(u);
                // 读 level-0 邻接
                linklistsizeint* ll = get_linklist_at_level(u, 0);
                int deg = getListCount(ll);
                tableint* nb = reinterpret_cast<tableint*>(ll + 1);
                for (int i = 0; i < deg; ++i) {
                    tableint v = nb[i];
                    if (v < N && !vis[v]) { vis[v] = 1; q.push(v); }
                }
            }
        };

        tableint root = (enterpoint_node_ < N && enterpoint_node_ != static_cast<tableint>(-1))
                        ? enterpoint_node_ : 0;
        bfs_push(root);
        for (tableint i = 0; i < N; ++i) bfs_push(i);

        if (place_deleted_last) {
            for (tableint i = 0; i < N; ++i) if (!is_active[i]) order.push_back(i);
        } else {
            std::vector<char> seen(N, 0);
            for (auto x : order) seen[x] = 1;
            for (tableint i = 0; i < N; ++i) if (!seen[i]) order.push_back(i);
        }
        // 现在 order.size()==N

        // ---------- 2) old->new / new->old ----------
        std::vector<tableint> new_id(N);
        std::vector<tableint> old_of_new(N);
        for (tableint newi = 0; newi < N; ++newi) {
            tableint old = order[newi];
            new_id[old] = newi;
            old_of_new[newi] = old;
        }

        // ---------- 3) 分配新 level-0 大块（16B 对齐） ----------
        auto round_up_16 = [](size_t x){ return (x + 15) & ~size_t(15); };
        const size_t bytes = (size_t)max_elements_ * size_data_per_element_;
        void* newp = nullptr;
        if (posix_memalign(&newp, 16, round_up_16(bytes)) != 0 || !newp)
            throw std::runtime_error("posix_memalign failed in AggressiveRenumberInplace");
        char* new_mem = (char*)newp;

        // ---------- 4) 并行复制 level-0 + 改写 level-0 邻接 ID ----------
        #pragma omp parallel for schedule(static)
        for (tableint newi = 0; newi < N; ++newi) {
            tableint old = old_of_new[newi];
            char* dst = new_mem + (size_t)newi * size_data_per_element_;
            char* src = data_level0_memory_ + (size_t)old * size_data_per_element_;
            CheckAndMemcpy(dst, size_data_per_element_, src, size_data_per_element_);

            linklistsizeint* ll = reinterpret_cast<linklistsizeint*>(dst + offsetLevel0_);
            int deg = getListCount(ll);
            tableint* nb = reinterpret_cast<tableint*>(ll + 1);
            for (int j = 0; j < deg; ++j) {
                tableint v = nb[j];
                if (v < N) nb[j] = new_id[v];
            }
            if (reorder_adj_by_newid && deg > 1) {
                std::sort(nb, nb + deg);
            }
        }

        if (N < max_elements_) {
            memset(new_mem + (size_t)N * size_data_per_element_, 0,
                (size_t)(max_elements_ - N) * size_data_per_element_);
        }

        // ---------- 5) 并行重建 >0 层邻接 ----------
        char** new_linkLists = (char**)malloc(sizeof(void*) * max_elements_);
        if (!new_linkLists) { free(new_mem); throw std::runtime_error("OOM: new_linkLists"); }
        #pragma omp parallel for schedule(static)
        for (tableint i = 0; i < max_elements_; ++i) new_linkLists[i] = nullptr;

        std::atomic<bool> oom(false);

        #pragma omp parallel for schedule(static)
        for (tableint newi = 0; newi < N; ++newi) {
            tableint old = old_of_new[newi];
            int level = element_levels_[old];
            if (level > 0) {
                char* buf = (char*)malloc(size_links_per_element_ * level + 1);
                if (!buf) { oom.store(true, std::memory_order_relaxed); continue; }
                memset(buf, 0, size_links_per_element_ * level + 1);
                new_linkLists[newi] = buf;

                for (int l = 1; l <= level; ++l) {
                    unsigned int* old_ll = get_linklist(old, l);
                    int cnt = getListCount(old_ll);
                    unsigned int* new_ll = reinterpret_cast<unsigned int*>(
                        buf + (size_t)(l - 1) * size_links_per_element_);
                    setListCount(new_ll, cnt);

                    tableint* old_nb = (tableint*)(old_ll + 1);
                    tableint* new_nb = (tableint*)(new_ll + 1);
                    for (int k = 0; k < cnt; ++k) {
                        tableint v = old_nb[k];
                        new_nb[k] = (v < N) ? new_id[v] : v;
                    }
                    if (reorder_adj_by_newid && cnt > 1) {
                        std::sort(new_nb, new_nb + cnt);
                    }
                }
            }
        }

        if (oom.load(std::memory_order_relaxed)) {
            for (tableint i = 0; i < N; ++i) if (new_linkLists[i]) free(new_linkLists[i]);
            free(new_linkLists);
            free(new_mem);
            throw std::runtime_error("OOM while rebuilding upper-level linklists");
        }

        // ---------- 6) 释放旧的 >0 层邻接 ----------
        for (tableint oldi = 0; oldi < N; ++oldi) {
            if (element_levels_[oldi] > 0 && linkLists_[oldi]) {
                free(linkLists_[oldi]);
            }
        }
        free(linkLists_);
        linkLists_ = new_linkLists;

        // ---------- 7) 切换 level-0 内存 ----------
        free(data_level0_memory_);
        data_level0_memory_ = new_mem;

        // ---------- 8) 重排 element_levels_ ----------
        std::vector<int> new_levels(max_elements_, 0);
        for (tableint newi = 0; newi < N; ++newi) {
            new_levels[newi] = element_levels_[old_of_new[newi]];
        }
        element_levels_.swap(new_levels);

        // ---------- 9) 更新入口点 ----------
        if (enterpoint_node_ != static_cast<tableint>(-1))
            enterpoint_node_ = new_id[enterpoint_node_];

        // ---------- 10) 重建 label -> id ----------
        {
            std::unique_lock<std::mutex> lock(label_lookup_lock);
            std::unordered_map<labeltype, tableint> new_map;
            new_map.reserve(label_lookup_.size() * 2);
            for (auto &kv : label_lookup_) {
                tableint old = kv.second;
                if (old < N) new_map.emplace(kv.first, new_id[old]);
            }
            label_lookup_.swap(new_map);
        }

        // ---------- 11) 重建 deleted_elements ----------
        {
            std::unique_lock<std::mutex> lock(deleted_elements_lock);
            std::unordered_set<tableint> new_deleted;
            new_deleted.reserve(deleted_elements.size() * 2);
            for (auto id : deleted_elements) if (id < N) new_deleted.insert(new_id[id]);
            deleted_elements.swap(new_deleted);
        }
    }

    void resizeIndex(size_t new_max_elements) {
        if (new_max_elements < cur_element_count_)
            throw std::runtime_error("Cannot resize, max element is less than the current number of elements");

        visited_list_pool_.reset(new VisitedListPool(1, new_max_elements));

        element_levels_.resize(new_max_elements);

        std::vector<std::mutex>(new_max_elements).swap(link_list_locks_);

        char * data_level0_memory_new = (char *) realloc(data_level0_memory_, new_max_elements * size_data_per_element_);
        if (data_level0_memory_new == nullptr)
            throw std::runtime_error("Not enough memory: resizeIndex failed to allocate base layer");
        data_level0_memory_ = data_level0_memory_new;

        try {
            char ** linkLists_new = (char **) realloc(linkLists_, sizeof(void *) * new_max_elements);
            if (linkLists_new == nullptr)
                throw std::runtime_error("Not enough memory: resizeIndex failed to allocate other layers");
            linkLists_ = linkLists_new;
        } catch (const std::exception &e) {
            free(data_level0_memory_new);
            data_level0_memory_new = nullptr;
            throw;
        }

        max_elements_ = new_max_elements;
    }

    size_t indexFileSize() const {
        size_t size = 0;
        size += sizeof(offsetLevel0_);
        size += sizeof(max_elements_);
        size += sizeof(cur_element_count_);
        size += sizeof(size_data_per_element_);
        size += sizeof(label_offset_);
        size += sizeof(offsetData_);
        size += sizeof(maxlevel_);
        size += sizeof(enterpoint_node_);
        size += sizeof(maxM_);

        size += sizeof(maxM0_);
        size += sizeof(M_);
        size += sizeof(mult_);
        size += sizeof(ef_construction_);

        size += cur_element_count_ * size_data_per_element_;

        for (size_t i = 0; i < cur_element_count_; i++) {
            unsigned int linkListSize = element_levels_[i] > 0 ? size_links_per_element_ * element_levels_[i] : 0;
            size += sizeof(linkListSize);
            size += linkListSize;
        }
        return size;
    }

    // 保存索引
    void saveIndex(const std::string &location) override {
        std::ofstream output(location, std::ios::binary);
        std::streampos position;

        writeBinaryPOD(output, offsetLevel0_);
        writeBinaryPOD(output, max_elements_);
        writeBinaryPOD(output, cur_element_count_);
        writeBinaryPOD(output, size_data_per_element_);
        writeBinaryPOD(output, label_offset_);
        writeBinaryPOD(output, offsetData_);
        writeBinaryPOD(output, maxlevel_);
        writeBinaryPOD(output, enterpoint_node_);
        writeBinaryPOD(output, maxM_);

        writeBinaryPOD(output, maxM0_);
        writeBinaryPOD(output, M_);
        writeBinaryPOD(output, mult_);
        writeBinaryPOD(output, ef_construction_);

        output.write(data_level0_memory_, cur_element_count_ * size_data_per_element_);

        for (size_t i = 0; i < cur_element_count_; i++) {
            unsigned int linkListSize = element_levels_[i] > 0 ? size_links_per_element_ * element_levels_[i] : 0;
            writeBinaryPOD(output, linkListSize);
            if (linkListSize)
                output.write(linkLists_[i], linkListSize);
        }
        output.close();
    }

    // 加载索引
    void loadIndex(const std::string &location, SpaceInterface<T> *s, size_t max_elements_i = 0) {
        std::ifstream input(location, std::ios::binary);

        if (!input.is_open())
            throw std::runtime_error("Cannot open file");

        clear();
        // get file size:
        input.seekg(0, input.end);
        std::streampos total_filesize = input.tellg();
        input.seekg(0, input.beg);

        readBinaryPOD(input, offsetLevel0_);
        readBinaryPOD(input, max_elements_);
        readBinaryPOD(input, cur_element_count_);

        size_t max_elements = max_elements_i;
        if (max_elements < cur_element_count_)
            max_elements = max_elements_;
        max_elements_ = max_elements;
        readBinaryPOD(input, size_data_per_element_);
        readBinaryPOD(input, label_offset_);
        readBinaryPOD(input, offsetData_);
        readBinaryPOD(input, maxlevel_);
        readBinaryPOD(input, enterpoint_node_);

        readBinaryPOD(input, maxM_);
        readBinaryPOD(input, maxM0_);
        readBinaryPOD(input, M_);
        readBinaryPOD(input, mult_);
        readBinaryPOD(input, ef_construction_);

        data_size_ = s->get_data_size();
        fstdistfunc_ = s->get_dist_func();
        dist_func_param_ = s->get_dist_func_param();

        auto pos = input.tellg();

        /// Optional - check if index is ok:
        input.seekg(cur_element_count_ * size_data_per_element_, input.cur);
        for (size_t i = 0; i < cur_element_count_; i++) {
            if (input.tellg() < 0 || input.tellg() >= total_filesize) {
                throw std::runtime_error("Index seems to be corrupted or unsupported");
            }

            unsigned int linkListSize;
            readBinaryPOD(input, linkListSize);
            if (linkListSize != 0) {
                input.seekg(linkListSize, input.cur);
            }
        }

        // throw exception if it either corrupted or old index
        if (input.tellg() != total_filesize)
            throw std::runtime_error("Index seems to be corrupted or unsupported");

        input.clear();
        /// Optional check end

        input.seekg(pos, input.beg);

        data_level0_memory_ = (char *) malloc(max_elements * size_data_per_element_);
        if (data_level0_memory_ == nullptr)
            throw std::runtime_error("Not enough memory: loadIndex failed to allocate level0");
        input.read(data_level0_memory_, cur_element_count_ * size_data_per_element_);

        size_links_per_element_ = maxM_ * sizeof(tableint) + sizeof(linklistsizeint);

        size_links_level0_ = maxM0_ * sizeof(tableint) + sizeof(linklistsizeint);
        std::vector<std::mutex>(max_elements).swap(link_list_locks_);
        std::vector<std::mutex>(MAX_LABEL_OPERATION_LOCKS).swap(label_op_locks_);

        visited_list_pool_.reset(new VisitedListPool(1, max_elements));

        try {
            linkLists_ = (char **) malloc(sizeof(void *) * max_elements);
            if (linkLists_ == nullptr)
                throw std::runtime_error("Not enough memory: loadIndex failed to allocate linklists");
        } catch (const std::exception &e) {
            free(data_level0_memory_);
            data_level0_memory_ = nullptr;
            throw;
        }
        element_levels_ = std::vector<int>(max_elements);
        revSize_ = 1.0 / mult_;
        ef_ = 10;
        for (size_t i = 0; i < cur_element_count_; i++) {
            label_lookup_[getExternalLabel(i)] = i;
            unsigned int linkListSize;
            readBinaryPOD(input, linkListSize);
            if (linkListSize == 0) {
                element_levels_[i] = 0;
                linkLists_[i] = nullptr;
            } else {
                element_levels_[i] = linkListSize / size_links_per_element_;
                try {
                    linkLists_[i] = (char *) malloc(linkListSize);
                    if (linkLists_[i] == nullptr)
                        throw std::runtime_error("Not enough memory: loadIndex failed to allocate linklist");
                } catch (const std::exception &e) {
                    free(data_level0_memory_);
                    data_level0_memory_ = nullptr;
                    for (size_t j = 0; j < i; j++) {
                        if (linkLists_[j] != nullptr) {
                            free(linkLists_[j]);
                            linkLists_[j] = nullptr;
                        }
                    }
                    free(linkLists_);
                    linkLists_ = nullptr;
                    throw;
                }
                input.read(linkLists_[i], linkListSize);
            }
        }

        for (size_t i = 0; i < cur_element_count_; i++) {
            if (isMarkedDeleted(i)) {
                num_deleted_ += 1;
                if (allow_replace_deleted_) deleted_elements.insert(i);
            }
        }

        input.close();
        AggressiveRenumberInplace(true, true);

        return;
    }

    // 根据标签获取数据
    template<typename data_t>
    std::vector<data_t> getDataByLabel(labeltype label) const {
        std::unique_lock <std::mutex> lock_label(getLabelOpMutex(label));
        std::unique_lock <std::mutex> lock_table(label_lookup_lock);
        auto search = label_lookup_.find(label);
        if (search == label_lookup_.end() || isMarkedDeleted(search->second)) {
            throw std::runtime_error("Label not found");
        }
        tableint internalId = search->second;
        lock_table.unlock();
        char* data_ptrv = getDataByInternalId(internalId);
        size_t dim = *((size_t *) dist_func_param_);
        std::vector<data_t> data;
        data_t* data_ptr = (data_t*) data_ptrv;
        for (size_t i = 0; i < dim; i++) {
            data.push_back(*data_ptr);
            data_ptr += 1;
        }
        return data;
    }

    // 标记删除
    void markDelete(labeltype label) {
        std::unique_lock <std::mutex> lock_label(getLabelOpMutex(label));
        std::unique_lock <std::mutex> lock_table(label_lookup_lock);
        auto search = label_lookup_.find(label);
        if (search == label_lookup_.end()) {
            throw std::runtime_error("Label not found");
        }
        tableint internalId = search->second;
        lock_table.unlock();
        markDeletedInternal(internalId);
    }

    void unmarkDeletedInternal(tableint internalId) {
        if (internalId >= cur_element_count_) {
            throw std::runtime_error("Assertion failed: internalId < cur_element_count_. internalId: " +
                                     std::to_string(internalId) + ", cur_element_count_: " +
                                     std::to_string(cur_element_count_));
        }
        if (isMarkedDeleted(internalId)) {
            unsigned char *ll_cur = ((unsigned char*)get_linklist0(internalId)) + 2;
            *ll_cur &= ~DELETE_MARK;
            num_deleted_ -= 1;
            if (allow_replace_deleted_) {
                std::unique_lock <std::mutex> lock_deleted_elements(deleted_elements_lock);
                deleted_elements.erase(internalId);
            }
        } else {
            throw std::runtime_error("The requested to undelete element is not deleted");
        }
    }

    // 取消标记删除
    void unmarkDelete(labeltype label) {
        std::unique_lock <std::mutex> lock_label(getLabelOpMutex(label));
        std::unique_lock <std::mutex> lock_table(label_lookup_lock);
        auto search = label_lookup_.find(label);
        if (search == label_lookup_.end()) {
            throw std::runtime_error("Label not found");
        }
        tableint internalId = search->second;
        lock_table.unlock();
        unmarkDeletedInternal(internalId);
    }

    void markDeletedInternal(tableint internalId) {
        if (internalId >= cur_element_count_) {
            throw std::runtime_error("Assertion failed: internalId < cur_element_count_. internalId: " +
                                     std::to_string(internalId) + ", cur_element_count_: " +
                                     std::to_string(cur_element_count_));
        }
        if (!isMarkedDeleted(internalId)) {
            unsigned char *ll_cur = ((unsigned char *)get_linklist0(internalId))+2;
            *ll_cur |= DELETE_MARK;
            num_deleted_ += 1;
            if (allow_replace_deleted_) {
                std::unique_lock <std::mutex> lock_deleted_elements(deleted_elements_lock);
                deleted_elements.insert(internalId);
            }
        } else {
            throw std::runtime_error("The requested to delete element is already deleted");
        }
    }

    // 判断是否被标记为删除
    bool isMarkedDeleted(tableint internalId) const {
        unsigned char *ll_cur = ((unsigned char*)get_linklist0(internalId)) + 2;
        return *ll_cur & DELETE_MARK;
    }

    unsigned short int getListCount(linklistsizeint * ptr) const {
        return *((unsigned short int *)ptr);
    }

    void setListCount(linklistsizeint * ptr, unsigned short int size) const {
        *((unsigned short int*)(ptr))=*((unsigned short int *)&size);
    }

    

    // 添加点
    void addPoint(const void *data_point, labeltype label, bool replace_deleted = false) override {
        if ((allow_replace_deleted_ == false) && (replace_deleted == true)) {
            throw std::runtime_error("Replacement of deleted elements is disabled in constructor");
        }

        std::unique_lock <std::mutex> lock_label(getLabelOpMutex(label));
        if (!replace_deleted) {
            addPoint(data_point, label, -1);
            return;
        }
        tableint internal_id_replaced;
        std::unique_lock <std::mutex> lock_deleted_elements(deleted_elements_lock);
        bool is_vacant_place = !deleted_elements.empty();
        if (is_vacant_place) {
            internal_id_replaced = *deleted_elements.begin();
            deleted_elements.erase(internal_id_replaced);
        }
        lock_deleted_elements.unlock();

        if (!is_vacant_place) {
            addPoint(data_point, label, -1);
        } else {
            labeltype label_replaced = getExternalLabel(internal_id_replaced);
            setExternalLabel(internal_id_replaced, label);

            std::unique_lock <std::mutex> lock_table(label_lookup_lock);
            label_lookup_.erase(label_replaced);
            label_lookup_[label] = internal_id_replaced;
            lock_table.unlock();

            unmarkDeletedInternal(internal_id_replaced);
            updatePoint(data_point, internal_id_replaced, 1.0);
        }
    }

    void updatePoint(const void *dataPoint, tableint internalId, float updateNeighborProbability) {
        CheckAndMemcpy(getDataByInternalId(internalId), data_size_, dataPoint, data_size_);

        int maxLevelCopy = maxlevel_;
        tableint entryPointCopy = enterpoint_node_;
        if (entryPointCopy == internalId && cur_element_count_ == 1)
            return;

        int elemLevel = element_levels_[internalId];
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        for (int layer = 0; layer <= elemLevel; layer++) {
            std::unordered_set<tableint> sCand;
            std::unordered_set<tableint> sNeigh;
            std::vector<tableint> listOneHop = getConnectionsWithLock(internalId, layer);
            if (listOneHop.size() == 0)
                continue;

            sCand.insert(internalId);

            for (auto&& elOneHop : listOneHop) {
                sCand.insert(elOneHop);

                if (distribution(update_probability_generator_) > updateNeighborProbability)
                    continue;

                sNeigh.insert(elOneHop);

                std::vector<tableint> listTwoHop = getConnectionsWithLock(elOneHop, layer);
                for (auto&& elTwoHop : listTwoHop) {
                    sCand.insert(elTwoHop);
                }
            }

            for (auto&& neigh : sNeigh) {
                std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> candidates;
                size_t size = sCand.find(neigh) == sCand.end() ? sCand.size() : sCand.size() - 1;
                size_t elementsToKeep = std::min(ef_construction_, size);
                for (auto&& cand : sCand) {
                    if (cand == neigh)
                        continue;

                    PriorityType<T> distance = fstdistfunc_(getDataByInternalId(neigh), getDataByInternalId(cand), dist_func_param_);
                    if (candidates.size() < elementsToKeep) {
                        candidates.emplace(distance, cand);
                    } else {
                        if (distance < candidates.top().first) {
                            candidates.pop();
                            candidates.emplace(distance, cand);
                        }
                    }
                }

                getNeighborsByHeuristic2(candidates, layer == 0 ? maxM0_ : maxM_);

                {
                    std::unique_lock <std::mutex> lock(link_list_locks_[neigh]);
                    linklistsizeint *ll_cur;
                    ll_cur = get_linklist_at_level(neigh, layer);
                    size_t candSize = candidates.size();
                    setListCount(ll_cur, candSize);
                    tableint *data = (tableint *) (ll_cur + 1);
                    for (size_t idx = 0; idx < candSize; idx++) {
                        data[idx] = candidates.top().second;
                        candidates.pop();
                    }
                }
            }
        }

        repairConnectionsForUpdate(dataPoint, entryPointCopy, internalId, elemLevel, maxLevelCopy);
    }

    void repairConnectionsForUpdate(
        const void *dataPoint,
        tableint entryPointInternalId,
        tableint dataPointInternalId,
        int dataPointLevel,
        int maxLevel) {
        tableint currObj = entryPointInternalId;
        if (dataPointLevel < maxLevel) {
            PriorityType<T> curdist = fstdistfunc_(dataPoint, getDataByInternalId(currObj), dist_func_param_);
            for (int level = maxLevel; level > dataPointLevel; level--) {
                bool changed = true;
                while (changed) {
                    changed = false;
                    unsigned int *data;
                    std::unique_lock <std::mutex> lock(link_list_locks_[currObj]);
                    data = get_linklist_at_level(currObj, level);
                    int size = getListCount(data);
                    tableint *datal = (tableint *) (data + 1);
                    for (int i = 0; i < size; i++) {
                        tableint cand = datal[i];
                        PriorityType<T> d = fstdistfunc_(dataPoint, getDataByInternalId(cand), dist_func_param_);
                        if (d < curdist) {
                            curdist = d;
                            currObj = cand;
                            changed = true;
                        }
                    }
                }
            }
        }

    if (dataPointLevel > maxLevel)
        throw std::runtime_error("Level of item to be updated cannot be bigger than max level");

    for (int level = dataPointLevel; level >= 0; level--) {
        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> topCandidates = searchBaseLayer(
                currObj, dataPoint, level);

        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> filteredTopCandidates;
        while (topCandidates.size() > 0) {
            if (topCandidates.top().second != dataPointInternalId)
                filteredTopCandidates.push(topCandidates.top());

            topCandidates.pop();
        }

        if (filteredTopCandidates.size() > 0) {
            bool epDeleted = isMarkedDeleted(entryPointInternalId);
            if (epDeleted) {
                filteredTopCandidates.emplace(fstdistfunc_(dataPoint, getDataByInternalId(entryPointInternalId), dist_func_param_), entryPointInternalId);
                if (filteredTopCandidates.size() > ef_construction_)
                    filteredTopCandidates.pop();
            }

            currObj = mutuallyConnectNewElement(dataPoint, dataPointInternalId, filteredTopCandidates, level, true);
        }
    }
    }

    std::vector<tableint> getConnectionsWithLock(tableint internalId, int level) {
        std::unique_lock <std::mutex> lock(link_list_locks_[internalId]);
        unsigned int *data = get_linklist_at_level(internalId, level);
        int size = getListCount(data);
        std::vector<tableint> result(size);
        tableint *ll = (tableint *) (data + 1);
        CheckAndMemcpy(result.data(), size * sizeof(tableint), ll, size * sizeof(tableint));
        return result;
    }

    tableint addPoint(const void *data_point, labeltype label, int level) {
        tableint cur_c = 0;
        {
            std::unique_lock <std::mutex> lock_table(label_lookup_lock);
            auto search = label_lookup_.find(label);
            if (search != label_lookup_.end()) {
                tableint existingInternalId = search->second;
                if (allow_replace_deleted_) {
                    if (isMarkedDeleted(existingInternalId)) {
                        throw std::runtime_error("Can't use addPoint to update deleted elements if replacement of deleted elements is enabled.");
                    }
                }
                lock_table.unlock();

                if (isMarkedDeleted(existingInternalId)) {
                    unmarkDeletedInternal(existingInternalId);
                }
                updatePoint(data_point, existingInternalId, 1.0);

                return existingInternalId;
            }

            if (cur_element_count_ >= max_elements_) {
                throw std::runtime_error("The number of elements exceeds the specified limit");
            }

            cur_c = cur_element_count_;
            cur_element_count_++;
            label_lookup_[label] = cur_c;
        }

        std::unique_lock <std::mutex> lock_el(link_list_locks_[cur_c]);
        int curlevel = getRandomLevel(mult_);
        if (level > 0)
            curlevel = level;

        element_levels_[cur_c] = curlevel;

        std::unique_lock <std::mutex> templock(global);
        int maxlevelcopy = maxlevel_;
        if (curlevel <= maxlevelcopy)
            templock.unlock();
        tableint currObj = enterpoint_node_;
        tableint enterpoint_copy = enterpoint_node_;

        memset(data_level0_memory_ + cur_c * size_data_per_element_ + offsetLevel0_, 0, size_data_per_element_);

        CheckAndMemcpy(getExternalLabeLp(cur_c), sizeof(labeltype), &label, sizeof(labeltype));
        CheckAndMemcpy(getDataByInternalId(cur_c), data_size_, data_point, data_size_);

        if (curlevel) {
            linkLists_[cur_c] = (char *) malloc(size_links_per_element_ * curlevel + 1);
            if (linkLists_[cur_c] == nullptr)
                throw std::runtime_error("Not enough memory: addPoint failed to allocate linklist");
            memset(linkLists_[cur_c], 0, size_links_per_element_ * curlevel + 1);
        }

        if ((signed)currObj != -1) {
            if (curlevel < maxlevelcopy) {
                PriorityType<T> curdist = fstdistfunc_(data_point, getDataByInternalId(currObj), dist_func_param_);
                for (int level = maxlevelcopy; level > curlevel; level--) {
                    bool changed = true;
                    while (changed) {
                        changed = false;
                        unsigned int *data;
                        std::unique_lock <std::mutex> lock(link_list_locks_[currObj]);
                        data = get_linklist(currObj, level);
                        int size = getListCount(data);

                        tableint *datal = (tableint *) (data + 1);
                        for (int i = 0; i < size; i++) {
                            tableint cand = datal[i];
                            if (cand < 0 || cand > max_elements_) {
                                if (curlevel) {
                                    free(linkLists_[cur_c]);
                                    linkLists_[cur_c] = nullptr;
                                }
                                throw std::runtime_error("cand error");
                            }
                            PriorityType<T> d = fstdistfunc_(data_point, getDataByInternalId(cand), dist_func_param_);
                            if (d < curdist) {
                                curdist = d;
                                currObj = cand;
                                changed = true;
                            }
                        }
                    }
                }
            }

            bool epDeleted = isMarkedDeleted(enterpoint_copy);
            for (int level = std::min(curlevel, maxlevelcopy); level >= 0; level--) {
                if (level > maxlevelcopy || level < 0)
                    throw std::runtime_error("Level error");

                std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> top_candidates = searchBaseLayer(
                        currObj, data_point, level);
                if (epDeleted) {
                    top_candidates.emplace(fstdistfunc_(data_point, getDataByInternalId(enterpoint_copy), dist_func_param_), enterpoint_copy);
                    if (top_candidates.size() > ef_construction_)
                        top_candidates.pop();
                }
                try {
                    currObj = mutuallyConnectNewElement(data_point, cur_c, top_candidates, level, false);
                } catch (const std::exception &e) {
                    if (curlevel) {
                        free(linkLists_[cur_c]);
                        linkLists_[cur_c] = nullptr;
                    }
                    throw;
                }
            }
        } else {
            enterpoint_node_ = 0;
            maxlevel_ = curlevel;
        }

        if (curlevel > maxlevelcopy) {
            enterpoint_node_ = cur_c;
            maxlevel_ = curlevel;
        }
        return cur_c;
    }

    // 搜索KNN
    std::priority_queue<std::pair<PriorityType<T>, labeltype>>
    searchKnn(const void *query_data, size_t k, BaseFilterFunctor* isIdAllowed = nullptr) const override {
        std::priority_queue<std::pair<PriorityType<T>, labeltype>> result;
        if (cur_element_count_ == 0) return result;

        tableint currObj = enterpoint_node_;
        PriorityType<T> curdist = fstdistfunc_(query_data, getDataByInternalId(enterpoint_node_), dist_func_param_);

        for (int level = maxlevel_; level > 0; level--) {
            bool changed = true;
            while (changed) {
                changed = false;
                unsigned int *data;

                data = (unsigned int *) get_linklist(currObj, level);
                int size = getListCount(data);
                metric_hops++;
                metric_distance_computations+=size;

                tableint *datal = (tableint *) (data + 1);
                for (int i = 0; i < size; i++) {
                    tableint cand = datal[i];
                    if (cand < 0 || cand > max_elements_)
                        throw std::runtime_error("cand error");
                    PriorityType<T> d = fstdistfunc_(query_data, getDataByInternalId(cand), dist_func_param_);

                    if (d < curdist) {
                        curdist = d;
                        currObj = cand;
                        changed = true;
                    }
                }
            }
        }

        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> top_candidates;
        bool bare_bone_search = !num_deleted_ && !isIdAllowed;
        if (bare_bone_search) {
            top_candidates = searchBaseLayerST<true>(
                    currObj, query_data, std::max(ef_, k), isIdAllowed);
        } else {
            top_candidates = searchBaseLayerST<false>(
                    currObj, query_data, std::max(ef_, k), isIdAllowed);
        }

        while (top_candidates.size() > k) {
            top_candidates.pop();
        }
        while (top_candidates.size() > 0) {
            std::pair<PriorityType<T>, tableint> rez = top_candidates.top();
            result.push(std::pair<PriorityType<T>, labeltype>(rez.first, getExternalLabel(rez.second)));
            top_candidates.pop();
        }
        return result;
    }


    // 停止条件最近邻搜索
    std::vector<std::pair<PriorityType<T>, labeltype>>
    searchStopConditionClosest(
        const void *query_data,
        BaseSearchStopCondition<PriorityType<T>>& stop_condition,
        BaseFilterFunctor* isIdAllowed = nullptr) const {
        std::vector<std::pair<PriorityType<T>, labeltype>> result;
        if (cur_element_count_ == 0) return result;
        tableint currObj = enterpoint_node_;
        PriorityType<T> curdist = fstdistfunc_(query_data, getDataByInternalId(enterpoint_node_), dist_func_param_);
        for (int level = maxlevel_; level > 0; level--) {
            bool changed = true;
            while (changed) {
                changed = false;
                unsigned int *data;
                data = (unsigned int *) get_linklist(currObj, level);
                int size = getListCount(data);
                metric_hops++;
                metric_distance_computations+=size;
                tableint *datal = (tableint *) (data + 1);
                for (int i = 0; i < size; i++) {
                    tableint cand = datal[i];
                    if (cand < 0 || cand > max_elements_)
                        throw std::runtime_error("cand error");
                    PriorityType<T> d = fstdistfunc_(query_data, getDataByInternalId(cand), dist_func_param_);
                    if (d < curdist) {
                        curdist = d;
                        currObj = cand;
                        changed = true;
                    }
                }
            }
        }
        std::priority_queue<std::pair<PriorityType<T>, tableint>, std::vector<std::pair<PriorityType<T>, tableint>>, CompareByFirst> top_candidates;
        top_candidates = searchBaseLayerST<false>(currObj, query_data, 0, isIdAllowed, &stop_condition);
        size_t sz = top_candidates.size();
        result.resize(sz);
        while (!top_candidates.empty()) {
            result[--sz] = top_candidates.top();
            top_candidates.pop();
        }
        stop_condition.filter_results(result);
        return result;
    }

    // 检查索引完整性
    void checkIntegrity() {
        int connections_checked = 0;
        std::vector <int > inbound_connections_num(cur_element_count_, 0);
        for (int i = 0; i < cur_element_count_; i++) {
            for (int l = 0; l <= element_levels_[i]; l++) {
                linklistsizeint *ll_cur = get_linklist_at_level(i, l);
                int size = getListCount(ll_cur);
                tableint *data = (tableint *) (ll_cur + 1);
                std::unordered_set<tableint> s;
                for (int j = 0; j < size; j++) {
                    if (data[j] >= cur_element_count_) {
                        throw std::runtime_error("Assertion failed: data[j] < cur_element_count_. data[j]: " +
                                                 std::to_string(data[j]) + ", cur_element_count_: " +
                                                 std::to_string(cur_element_count_));
                    }
                    if (data[j] == i) {
                        throw std::runtime_error("Assertion failed: data[j] != i. data[j]: " +
                                                 std::to_string(data[j]) + ", i: " +
                                                 std::to_string(i));
                    }
                    inbound_connections_num[data[j]]++;
                    s.insert(data[j]);
                    connections_checked++;
                }
                if (s.size() != size) {
                    throw std::runtime_error("Assertion failed: s.size() == size. s.size(): " +
                                             std::to_string(s.size()) + ", size: " +
                                             std::to_string(size));
                }
            }
        }
        if (cur_element_count_ > 1) {
            int min1 = inbound_connections_num[0], max1 = inbound_connections_num[0];
            for (int i=0; i < cur_element_count_; i++) {
                if (inbound_connections_num[i] <= 0) {
                    throw std::runtime_error("Assertion failed: inbound_connections_num[i] > 0. inbound_connections_num[i]: " +
                                             std::to_string(inbound_connections_num[i]));
                }
                min1 = std::min(inbound_connections_num[i], min1);
                max1 = std::max(inbound_connections_num[i], max1);
            }
            std::cout << "Min inbound: " << min1 << ", Max inbound:" << max1 << "\n";
        }
        std::cout << "integrity ok, checked " << connections_checked << " connections\n";
    }

};

// 类型别名 - 移到类外部
using HierarchicalNSWFloat = HierarchicalNSWTemplate<float>;
using HierarchicalNSWI = HierarchicalNSWTemplate<int>;
#ifdef HNSWLIB_ENABLE_FP16
using HierarchicalNSWHalf = HierarchicalNSWTemplate<float16_t>;
#endif

} // namespace hnswlib

#endif // HNSWLIB_USE_ARM