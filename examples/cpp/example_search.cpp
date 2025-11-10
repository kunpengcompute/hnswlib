#include "hnswlib/hnswlib.h"


int main() {
    int dim = 16;               // Dimension of the elements
    int max_elements = 10000;   // Maximum number of elements, should be known beforehand
    int M = 16;                 // Tightly connected with internal dimensionality of the data
                                // strongly affects the memory consumption
    int ef_construction = 200;  // Controls index search speed/build speed tradeoff

    // Initing index
#ifdef HNSWLIB_ENABLE_FP16
    hnswlib::L2SpaceHalf space(dim);
    hnswlib::HierarchicalNSWHalf* alg_hnsw = new hnswlib::HierarchicalNSWHalf(&space, max_elements, M, ef_construction);

    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;
    float* data = new float[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data[i] = distrib_real(rng);
    }
    float16_t* data_fp16 = new float16_t[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data_fp16[i] = static_cast<float16_t>(data[i]);
    }

    // Add data to index
    for (int i = 0; i < max_elements; i++) {
        alg_hnsw->addPoint(data_fp16 + i * dim, i);
    }

    // Query the elements for themselves and measure recall
    float correct = 0;
    for (int i = 0; i < max_elements; i++) {
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data_fp16 + i * dim, 1);
        hnswlib::labeltype label = result.top().second;
        if (label == i) correct++;
    }
    float recall = correct / max_elements;
    std::cout << "Recall: " << recall << "\n";

    // Serialize index
    std::string hnsw_path = "hnsw.bin";
    alg_hnsw->saveIndex(hnsw_path);
    delete alg_hnsw;

    // Deserialize index and check recall
    alg_hnsw = new hnswlib::HierarchicalNSWHalf(&space, hnsw_path);
    correct = 0;
    for (int i = 0; i < max_elements; i++) {
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data_fp16 + i * dim, 1);
        hnswlib::labeltype label = result.top().second;
        if (label == i) correct++;
    }
    recall = (float)correct / max_elements;
    std::cout << "Recall of deserialized index: " << recall << "\n";

    delete[] data;
    delete[] data_fp16;

#else
    hnswlib::L2SpaceF space(dim);
    hnswlib::HierarchicalNSWFloat* alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, max_elements, M, ef_construction);

    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;
    float* data = new float[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data[i] = distrib_real(rng);
    }

    // Add data to index
    for (int i = 0; i < max_elements; i++) {
        alg_hnsw->addPoint(data + i * dim, i);
    }

    // Query the elements for themselves and measure recall
    float correct = 0;
    for (int i = 0; i < max_elements; i++) {
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data + i * dim, 1);
        hnswlib::labeltype label = result.top().second;
        if (label == i) correct++;
    }
    float recall = correct / max_elements;
    std::cout << "Recall: " << recall << "\n";

    // Serialize index
    std::string hnsw_path = "hnsw.bin";
    alg_hnsw->saveIndex(hnsw_path);
    delete alg_hnsw;

    // Deserialize index and check recall
    alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, hnsw_path);
    correct = 0;
    for (int i = 0; i < max_elements; i++) {
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data + i * dim, 1);
        hnswlib::labeltype label = result.top().second;
        if (label == i) correct++;
    }
    recall = (float)correct / max_elements;
    std::cout << "Recall of deserialized index: " << recall << "\n";

    delete[] data;
#endif
    delete alg_hnsw;
    return 0;
}
