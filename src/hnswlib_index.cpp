#include "hnswlib_index.h"

HNSWLIBIndex::HNSWLIBIndex(const ConfigParser &config)
{
    k_f = config.get_int("k_f", 24);
    efs = config.get_int("efs", 100);
    efc = config.get_int("efc", 200);
    metric_str = config.get_string("metric", "L2");
}

HNSWLIBIndex::~HNSWLIBIndex()
{
    index.reset();
}

void HNSWLIBIndex::build(const Datasets *data)
{
    dim = data->d;
    if (metric_str == "IP") {
        space = std::make_unique<InnerProductSpace>(dim);
    } else {
        space = std::make_unique<L2Space>(dim);
    }
	index = std::make_unique<HierarchicalNSW<float>>(space.get(), data->nb, k_f, efc);
    for (size_t i = 0; i < data->nb; i++) {
        index->addPoint(data->xb + i * data->d, i);
    }
}

void HNSWLIBIndex::prepare_search()
{
    index->setEf(efs);
}

void HNSWLIBIndex::search_single(const float *xq, int k, float *distances, int64_t *labels)
{
    auto result = index->searchKnn(xq, k);
    int j = 0;
    while (!result.empty() && j < k) {
        auto &top = result.top();
        distances[j] = top.first;
        labels[j] = top.second;
        result.pop();
        j++;
    }
}

void HNSWLIBIndex::search_batch(const int nq, const float *xq, int k, float *distances, int64_t *labels)
{
    for (int i = 0; i < nq; i++) {
        auto result = index->searchKnn(xq + i * dim, k);
        int j = 0;
        while (!result.empty() && j < k) {
            auto &top = result.top();
            distances[i * k + j] = top.first;
            labels[i * k + j] = top.second;
            result.pop();
            j++;
        }
    }
}