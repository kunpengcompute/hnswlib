#ifndef INDEX_BASE_H
#define INDEX_BASE_H

#include <cstdint>
#include "../hnswlib/hnswlib.h"

class Datasets;

class IndexBase {
public:
    virtual ~IndexBase() = default;
    
    virtual void build(const Datasets* data) = 0;
    virtual void prepare_search() = 0;
#ifdef HNSWLIB_ENABLE_FP16
    virtual void search_single(
        const float16_t* xq, 
        int k, 
        float16_t* distances, 
        int64_t* labels
    ) = 0;
    virtual void search_batch(
        const int nq, 
        const float16_t* xq, 
        int k, 
        float16_t* distances, 
        int64_t* labels
    ) = 0;
#else
    virtual void search_single(
        const float* xq, 
        int k, 
        float* distances, 
        int64_t* labels
    ) = 0;
    virtual void search_batch(
        const int nq, 
        const float* xq, 
        int k, 
        float* distances, 
        int64_t* labels
    ) = 0;
#endif
};

#endif // INDEX_BASE_H