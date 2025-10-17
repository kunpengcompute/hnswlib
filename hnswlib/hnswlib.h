/*
  Adapted from https://github.com/nmslib/hnswlib/blob/master/hnswlib/hnswlib.h
 */

#pragma once

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #define HNSWLIB_USE_X86
#elif defined(__aarch64__) || defined(__arm__)
    #ifndef HNSWLIB_USE_ARM
        #define HNSWLIB_USE_ARM
    #endif
#endif

#if defined(HNSWLIB_USE_ARM)
    #if !defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC) && !defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
        typedef __fp16 float16_t;
    #else
        #ifndef __fp16
            typedef __fp16 float16_t;
        #endif
    #endif
#endif

// ================== 标准库与基础类型 ==================
#include <queue>
#include <vector>
#include <iostream>
#include <string.h>
#include <mutex>
#include <atomic>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <memory>
#include <fstream>
#include <cassert>
#include <algorithm>

namespace hnswlib {
// 通用标签类型
typedef size_t labeltype;

class BaseFilterFunctor {
 public:
    virtual bool operator()(hnswlib::labeltype id) { return true; }
    virtual ~BaseFilterFunctor() {};
};

template<typename dist_t>
class BaseSearchStopCondition {
 public:
    virtual void add_point_to_result(labeltype label, const void *datapoint, dist_t dist) = 0;
    virtual void remove_point_from_result(labeltype label, const void *datapoint, dist_t dist) = 0;
    virtual bool should_stop_search(dist_t candidate_dist, dist_t lowerBound) = 0;
    virtual bool should_consider_candidate(dist_t candidate_dist, dist_t lowerBound) = 0;
    virtual bool should_remove_extra() = 0;
    virtual void filter_results(std::vector<std::pair<dist_t, labeltype >> &candidates) = 0;
    virtual ~BaseSearchStopCondition() {}
};

template <typename T>
class pairGreater {
 public:
    bool operator()(const T& p1, const T& p2) {
        return p1.first > p2.first;
    }
};

template<typename T>
static void writeBinaryPOD(std::ostream &out, const T &podRef) {
    out.write((char *) &podRef, sizeof(T));
}

template<typename T>
static void readBinaryPOD(std::istream &in, T &podRef) {
    in.read((char *) &podRef, sizeof(T));
}

#if defined(HNSWLIB_USE_ARM)
template<typename MTYPE>
using DISTFUNC = typename std::conditional<
    std::is_same<MTYPE, float>::value || std::is_same<MTYPE, __fp16>::value,
    float(*)(const void *, const void *, const void *),
    int(*)(const void *, const void *, const void *)
>::type;
#else
template<typename MTYPE>
using DISTFUNC = MTYPE(*)(const void *, const void *, const void *);
#endif

// 空间接口
// 具体空间实现（如L2、IP）需继承并实现
// 支持float32/float16（ARM）
template<typename MTYPE>
class SpaceInterface {
 public:
    virtual size_t get_data_size() = 0;
    virtual DISTFUNC<MTYPE> get_dist_func() = 0;
    virtual void *get_dist_func_param() = 0;
    virtual ~SpaceInterface() {}
};

// 算法接口
// 具体算法（如HNSW、Bruteforce）需继承并实现
// 支持float32/float16（ARM）
template<typename dist_t>
class AlgorithmInterface {
 public:
    virtual void addPoint(const void *datapoint, labeltype label, bool replace_deleted = false) = 0;
    virtual std::priority_queue<std::pair<dist_t, labeltype>>
        searchKnn(const void*, size_t, BaseFilterFunctor* isIdAllowed = nullptr) const = 0;
    virtual std::vector<std::pair<dist_t, labeltype>>
        searchKnnCloserFirst(const void* query_data, size_t k, BaseFilterFunctor* isIdAllowed = nullptr) const;
    virtual void saveIndex(const std::string &location) = 0;
    virtual ~AlgorithmInterface() {}
};

template<typename dist_t>
std::vector<std::pair<dist_t, labeltype>>
AlgorithmInterface<dist_t>::searchKnnCloserFirst(const void* query_data, size_t k,
                                                 BaseFilterFunctor* isIdAllowed) const {
    std::vector<std::pair<dist_t, labeltype>> result;
    auto ret = searchKnn(query_data, k, isIdAllowed);
    {
        size_t sz = ret.size();
        result.resize(sz);
        while (!ret.empty()) {
            result[--sz] = ret.top();
            ret.pop();
            }
    }
    return result;
}

}  // namespace hnswlib

#if defined(HNSWLIB_USE_X86)
    #include "space_l2.h"
    #include "space_ip.h"
    #include "hnswalg.h"
    #include "bruteforce.h"
#elif defined(HNSWLIB_USE_ARM)
    #include "space_l2_template.h"
    #include "space_ip_template.h"
    #include "hnswalg_template.h"
    #include "bruteforce_template.h"
    #include "data_type_converter.h"
#endif

#include "stop_condition.h"
#include "visited_list_pool.h"
