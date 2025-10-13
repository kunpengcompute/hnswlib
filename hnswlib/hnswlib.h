/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */
#pragma once

// ================== 平台自动检测与宏定义 ==================
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
    #define HNSWLIB_USE_X86
#elif defined(__aarch64__) || defined(__arm__)
    #ifndef HNSWLIB_USE_ARM
        #define HNSWLIB_USE_ARM
    #endif
#endif

// ================== FP16类型定义（仅ARM需要） ==================
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

// ========== 通用接口定义 ========== //
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

// template<typename MTYPE>
using DISTFUNC = float(*)(const void *, const void *, const void *);

// 空间接口
// 具体空间实现（如L2、IP）需继承并实现
// 支持float32/float16（ARM）
template<typename MTYPE>
class SpaceInterface {
 public:
    virtual size_t get_data_size() = 0;
    // virtual DISTFUNC<MTYPE> get_dist_func() = 0;
    virtual DISTFUNC get_dist_func() = 0;
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
    size_t sz = ret.size();
    result.resize(sz);
    while (!ret.empty()) {
        result[--sz] = ret.top();
        ret.pop();
    }
    return result;
}

}  // namespace hnswlib

// ========== 平台相关空间实现自动包含 ========== //
#if defined(HNSWLIB_USE_X86)
    #include "space_l2.h"
    #include "space_ip.h"
    #include "hnswalg.h"
#elif defined(HNSWLIB_USE_ARM)
    #include "space_l2_template.h"
    #include "space_ip_template.h"
    #include "hnswalg_template.h"
#endif

// ========== 通用工具/其它模块 ========== //
#include "stop_condition.h"
#include "bruteforce_template.h"
#include "visited_list_pool.h"
#include "data_type_converter.h"
