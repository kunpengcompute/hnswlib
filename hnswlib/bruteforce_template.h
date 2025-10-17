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

#include <unordered_map>
#include <fstream>
#include <mutex>
#include <algorithm>
#include "hnswlib.h"

// 确保float16_t类型定义
#if !defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC) && !defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
typedef __fp16 float16_t;
#else
// 如果ARM FP16特性已启用，确保float16_t类型存在
#ifndef __fp16
typedef __fp16 float16_t;
#endif
#endif

namespace hnswlib {

template<typename T>
using PriorityType = typename std::conditional<
    std::is_same<T, float>::value || std::is_same<T, float16_t>::value,
    float,
    int
>::type;

template<typename T>
class BruteforceSearch : public AlgorithmInterface<PriorityType<T>> {
public:
    char *data_;
    size_t maxelements_;
    size_t cur_element_count_;
    size_t size_per_element_;

    size_t data_size_;
    DISTFUNC<PriorityType<T>> fstdistfunc_;
    void *dist_func_param_;
    std::mutex index_lock;

    std::unordered_map<labeltype, size_t> dict_external_to_internal;

    BruteforceSearch(SpaceInterface<T> *s)
        : data_(nullptr),
            maxelements_(0),
            cur_element_count_(0),
            size_per_element_(0),
            data_size_(0),
            dist_func_param_(nullptr) {
    }

    BruteforceSearch(SpaceInterface<T> *s, const std::string &location)
        : data_(nullptr),
            maxelements_(0),
            cur_element_count_(0),
            size_per_element_(0),
            data_size_(0),
            dist_func_param_(nullptr) {
        loadIndex(location, s);
    }

    BruteforceSearch(SpaceInterface<T> *s, size_t maxElements) {
        maxelements_ = maxElements;
        data_size_ = s->get_data_size();
        fstdistfunc_ = s->get_dist_func();
        dist_func_param_ = s->get_dist_func_param();
        size_per_element_ = data_size_ + sizeof(labeltype);
        data_ = (char *) malloc(maxElements * size_per_element_);
        if (data_ == nullptr) {
            throw std::runtime_error("Not enough memory: BruteforceSearch failed to allocate data");
        }
        cur_element_count_ = 0;    

    }

    ~BruteforceSearch() {
        free(data_);
        data_ = nullptr;
    }

    void addPoint(const void *datapoint, labeltype label, bool replace_deleted = false) override {
        int idx;
        {
            std::unique_lock<std::mutex> lock(index_lock);

            auto search = dict_external_to_internal.find(label);
            if (search != dict_external_to_internal.end()) {
                idx = search->second;
            } else {
                if (cur_element_count_ >= maxelements_) {
                    throw std::runtime_error("The number of elements exceeds the specified limit\n");
                }
                idx = cur_element_count_;
                dict_external_to_internal[label] = idx;
                cur_element_count_++;
            }
        }
        memcpy(data_ + size_per_element_ * idx + data_size_, &label, sizeof(labeltype));
        memcpy(data_ + size_per_element_ * idx, datapoint, data_size_);
    }

    void removePoint(labeltype cur_external) {
        std::unique_lock<std::mutex> lock(index_lock);

        auto found = dict_external_to_internal.find(cur_external);
        if (found == dict_external_to_internal.end()) {
            return;
        }

        dict_external_to_internal.erase(found);

        size_t cur_c = found->second;
        labeltype label = *((labeltype*)(data_ + size_per_element_ * (cur_element_count_-1) + data_size_));
        dict_external_to_internal[label] = cur_c;
        memcpy(data_ + size_per_element_ * cur_c,
               data_ + size_per_element_ * (cur_element_count_-1),
               data_size_+sizeof(labeltype));
        cur_element_count_--;
    }

    std::priority_queue<std::pair<PriorityType<T>, labeltype>>
    searchKnn(const void *query_data, size_t k, BaseFilterFunctor* isIdAllowed = nullptr) const override {
        if (k > cur_element_count_) {
            throw std::runtime_error("Assertion failed: k <= cur_element_count_. k: " +
                                     std::to_string(k) + ", cur_element_count_: " +
                                     std::to_string(cur_element_count_));
        }
        std::priority_queue<std::pair<PriorityType<T>, labeltype>> topResults;
        if (cur_element_count_ == 0) return topResults;
        for (int i = 0; i < k; i++) {
            PriorityType<T> dist = fstdistfunc_(query_data, data_ + size_per_element_ * i, dist_func_param_);
            labeltype label = *((labeltype*) (data_ + size_per_element_ * i + data_size_));
            if ((!isIdAllowed) || (*isIdAllowed)(label)) {
                topResults.emplace(dist, label);
            }
        }
        PriorityType<T> lastdist = topResults.empty() ? std::numeric_limits<PriorityType<T>>::max() : topResults.top().first;
        for (int i = k; i < cur_element_count_; i++) {
            PriorityType<T> dist = fstdistfunc_(query_data, data_ + size_per_element_ * i, dist_func_param_);
            if (dist <= lastdist) {
                labeltype label = *((labeltype *) (data_ + size_per_element_ * i + data_size_));
                if ((!isIdAllowed) || (*isIdAllowed)(label)) {
                    topResults.emplace(dist, label);
                }
                if (topResults.size() > k)
                    topResults.pop();

                if (!topResults.empty()) {
                    lastdist = topResults.top().first;
                }
            }
        }
        return topResults;
    }

    std::vector<std::pair<PriorityType<T>, labeltype>>
    searchKnnCloserFirst(const void* query_data, size_t k, BaseFilterFunctor* isIdAllowed = nullptr) const override {
        auto result = searchKnn(query_data, k, isIdAllowed);
        std::vector<std::pair<PriorityType<T>, labeltype>> result_vec;
        while (!result.empty()) {
            result_vec.push_back(result.top());
            result.pop();
        }
        std::reverse(result_vec.begin(), result_vec.end());
        return result_vec;
    }

    void saveIndex(const std::string &location) override {
        std::ofstream output(location, std::ios::binary);
        std::streampos position;

        writeBinaryPOD(output, maxelements_);
        writeBinaryPOD(output, size_per_element_);
        writeBinaryPOD(output, cur_element_count_);

        output.write(data_, maxelements_ * size_per_element_);

        output.close();
    }

    void loadIndex(const std::string &location, SpaceInterface<PriorityType<T>> *s) {
        std::ifstream input(location, std::ios::binary);
        std::streampos position;

        readBinaryPOD(input, maxelements_);
        readBinaryPOD(input, size_per_element_);
        readBinaryPOD(input, cur_element_count_);

        data_size_ = s->get_data_size();
        fstdistfunc_ = s->get_dist_func();
        dist_func_param_ = s->get_dist_func_param();
        size_per_element_ = data_size_ + sizeof(labeltype);
        data_ = (char *) malloc(maxelements_ * size_per_element_);
        if (data_ == nullptr)
            throw std::runtime_error("Not enough memory: loadIndex failed to allocate data");

        input.read(data_, maxelements_ * size_per_element_);

        input.close();
    }

    // 类型安全的添加点接口
    void addPoint(const T *data_point, labeltype label, bool replace_deleted = false) {
        addPoint(static_cast<const void*>(data_point), label, replace_deleted);
    }

    // 类型安全的搜索接口
    std::priority_queue<std::pair<PriorityType<T>, labeltype>> searchKnn(const T *query_data, size_t k, BaseFilterFunctor* isIdAllowed = nullptr) const {
        return searchKnn(static_cast<const void*>(query_data), k, isIdAllowed);
    }

    std::vector<std::pair<PriorityType<T>, labeltype>> searchKnnCloserFirst(const T *query_data, size_t k, BaseFilterFunctor* isIdAllowed = nullptr) const {
        return searchKnnCloserFirst(static_cast<const void*>(query_data), k, isIdAllowed);
    }

    // 获取当前元素数量
    size_t getCurrentElementCount() const {
        return cur_element_count_;
    }

    // 获取最大元素数量
    size_t getMaxElements() const {
        return maxelements_;
    }
};

// 类型别名
using BruteforceSearchFloat = BruteforceSearch<float>;
using BruteforceSearchHalf = BruteforceSearch<float16_t>;

// 工厂函数
template<typename T>
inline BruteforceSearch<T>* create_bruteforce_index(SpaceInterface<float> *space, size_t max_elements = 0) {
    return new BruteforceSearch<T>(space, max_elements);
}

} // namespace hnswlib