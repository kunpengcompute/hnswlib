/**
 * @param k_f               HNSWLIB构建参数：邻居节点数
 * @param efs               HNSWLIB搜索参数：候选节点列表大小
 * @param efc               HNSWLIB构建参数：候选节点列表大小
 * @param dim               HNSWLIB查询参数：向量维度
 * @param index             HNSWLIB索引
 * @param space             HNSWLIB索引空间
 */

#ifndef HNSWLIB_INDEX_H
#define HNSWLIB_INDEX_H

#include "index_base.h"
#include "datasets.h"
#include "config_parser.h"
#include "../hnswlib/hnswlib.h"
#include <memory>

using namespace hnswlib;
using namespace std;

class HNSWLIBIndex : public IndexBase {
public:
	HNSWLIBIndex(const ConfigParser& config);
    ~HNSWLIBIndex();
    virtual void build(const Datasets* data) override;
    virtual void prepare_search() override;
#ifdef HNSWLIB_ENABLE_FP16
    virtual void search_single(const float16_t* xq, int k, float16_t* distances, int64_t* labels) override;
    virtual void search_batch(const int nq, const float16_t* xq, int k, float16_t* distances, int64_t* labels) override;
#else
    virtual void search_single(const float* xq, int k, float* distances, int64_t* labels) override;
    virtual void search_batch(const int nq, const float* xq, int k, float* distances, int64_t* labels) override;
#endif

// private:
    int k_f;
    int efs;
    int efc;
	int dim;
	std::string metric_str;
#ifdef HNSWLIB_USE_ARM
#ifdef HNSWLIB_ENABLE_FP16
    std::unique_ptr<HierarchicalNSWHalf> index;
	std::unique_ptr<SpaceInterface<float16_t>> space;
#else
    std::unique_ptr<HierarchicalNSWFloat> index;
	std::unique_ptr<SpaceInterface<float>> space;
#endif
#else
    std::unique_ptr<HierarchicalNSW<float>> index;
	std::unique_ptr<SpaceInterface<float>> space;
#endif
    std::string index_path;
    std::string save_or_load;
};

#endif // HNSWLIB_INDEX_H