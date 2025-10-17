# C++ examples

Creating index, inserting elements, searching and serialization
```cpp
#include "../../hnswlib/hnswlib.h"


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
#else
    hnswlib::L2SpaceF space(dim);
    hnswlib::HierarchicalNSWFloat* alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, max_elements, M, ef_construction);
#endif

    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;
    float* data = new float[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data[i] = distrib_real(rng);
    }
#ifdef HNSWLIB_ENABLE_FP16
    float16_t* data_fp16 = new float16_t[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data_fp16[i] = static_cast<float16_t>(data[i]);
    }
#endif

    // Add data to index
#ifdef HNSWLIB_ENABLE_FP16
    for (int i = 0; i < max_elements; i++) {
        alg_hnsw->addPoint(data_fp16 + i * dim, i);
    }
#else
    for (int i = 0; i < max_elements; i++) {
        alg_hnsw->addPoint(data + i * dim, i);
    }
#endif

    // Query the elements for themselves and measure recall
    float correct = 0;
    for (int i = 0; i < max_elements; i++) {
#ifdef HNSWLIB_ENABLE_FP16
        std::priority_queue<std::pair<float16_t, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data_fp16 + i * dim, 1);
#else
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data + i * dim, 1);
#endif
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
#ifdef HNSWLIB_ENABLE_FP16
    alg_hnsw = new hnswlib::HierarchicalNSWHalf(&space, hnsw_path);
#else
    alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, hnsw_path);
#endif
    correct = 0;
    for (int i = 0; i < max_elements; i++) {
#ifdef HNSWLIB_ENABLE_FP16
        std::priority_queue<std::pair<float16_t, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data_fp16 + i * dim, 1);
#else
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnn(data + i * dim, 1);
#endif
        hnswlib::labeltype label = result.top().second;
        if (label == i) correct++;
    }
    recall = (float)correct / max_elements;
    std::cout << "Recall of deserialized index: " << recall << "\n";

    delete[] data;
#ifdef HNSWLIB_ENABLE_FP16
    delete[] data_fp16;
#endif
    delete alg_hnsw;
    return 0;
}
```

An example of filtering with a boolean function during the search:
```cpp
#include "../../hnswlib/hnswlib.h"


// Filter that allows labels divisible by divisor
class PickDivisibleIds: public hnswlib::BaseFilterFunctor {
unsigned int divisor = 1;
 public:
    PickDivisibleIds(unsigned int divisor): divisor(divisor) {
        assert(divisor != 0);
    }
    bool operator()(hnswlib::labeltype label_id) {
        return label_id % divisor == 0;
    }
};


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
#else
    hnswlib::L2SpaceF space(dim);
    hnswlib::HierarchicalNSWFloat* alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, max_elements, M, ef_construction);
#endif

    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;
    float* data = new float[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data[i] = distrib_real(rng);
    }
#ifdef HNSWLIB_ENABLE_FP16
    float16_t* data_fp16 = new float16_t[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data_fp16[i] = static_cast<float16_t>(data[i]);
    }
#endif

    // Add data to index
    for (int i = 0; i < max_elements; i++) {
#ifdef HNSWLIB_ENABLE_FP16
        alg_hnsw->addPoint(data_fp16 + i * dim, i);
#else
        alg_hnsw->addPoint(data + i * dim, i);
#endif
    }

    // Create filter that allows only even labels
    PickDivisibleIds pickIdsDivisibleByTwo(2);

    // Query the elements for themselves with filter and check returned labels
    int k = 10;
    for (int i = 0; i < max_elements; i++) {
#ifdef HNSWLIB_ENABLE_FP16
        std::vector<std::pair<float16_t, hnswlib::labeltype>> result = alg_hnsw->searchKnnCloserFirst(data_fp16 + i * dim, k, &pickIdsDivisibleByTwo);
#else
        std::vector<std::pair<float, hnswlib::labeltype>> result = alg_hnsw->searchKnnCloserFirst(data + i * dim, k, &pickIdsDivisibleByTwo);
#endif
        for (auto item: result) {
            if (item.second % 2 == 1) std::cout << "Error: found odd label\n";
        }
    }

    delete[] data;
#ifdef HNSWLIB_ENABLE_FP16
    delete[] data_fp16;
#endif
    delete alg_hnsw;
    return 0;
}
```

An example with reusing the memory of the deleted elements when new elements are being added (via `allow_replace_deleted` flag):
```cpp
#include "../../hnswlib/hnswlib.h"


int main() {
    int dim = 16;               // Dimension of the elements
    int max_elements = 10000;   // Maximum number of elements, should be known beforehand
    int M = 16;                 // Tightly connected with internal dimensionality of the data
                                // strongly affects the memory consumption
    int ef_construction = 200;  // Controls index search speed/build speed tradeoff

    // Initing index
#ifdef HNSWLIB_ENABLE_FP16
    hnswlib::L2SpaceHalf space(dim);
    hnswlib::HierarchicalNSWHalf* alg_hnsw = new hnswlib::HierarchicalNSWHalf(&space, max_elements, M, ef_construction, 100, true);
#else
    hnswlib::L2SpaceF space(dim);
    hnswlib::HierarchicalNSWFloat* alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, max_elements, M, ef_construction, 100, true);
#endif

    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;
    float* data = new float[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data[i] = distrib_real(rng);
    }
#ifdef HNSWLIB_ENABLE_FP16
    float16_t* data_fp16 = new float16_t[dim * max_elements];
    for (int i = 0; i < dim * max_elements; i++) {
        data_fp16[i] = static_cast<float16_t>(data[i]);
    }
#endif

    // Add data to index
    for (int i = 0; i < max_elements; i++) {
#ifdef HNSWLIB_ENABLE_FP16
        alg_hnsw->addPoint(data_fp16 + i * dim, i);
#else
        alg_hnsw->addPoint(data + i * dim, i);
#endif
    }

    // Mark first half of elements as deleted
    int num_deleted = max_elements / 2;
    for (int i = 0; i < num_deleted; i++) {
        alg_hnsw->markDelete(i);
    }

    float* add_data = new float[dim * num_deleted];
    for (int i = 0; i < dim * num_deleted; i++) {
        add_data[i] = distrib_real(rng);
    }
#ifdef HNSWLIB_ENABLE_FP16
    float16_t* add_data_fp16 = new float16_t[dim * num_deleted];
    for (int i = 0; i < dim * num_deleted; i++) {
        add_data_fp16 = static_cast<float16_t>(add_data[i]);
    }
#endif

    // Replace deleted data with new elements
    // Maximum number of elements is reached therefore we cannot add new items,
    // but we can replace the deleted ones by using replace_deleted=true
    for (int i = 0; i < num_deleted; i++) {
        int label = max_elements + i;
#ifdef HNSWLIB_ENABLE_FP16
        alg_hnsw->addPoint(add_data_fp16 + i * dim, label, true);
#else
        alg_hnsw->addPoint(add_data + i * dim, label, true);
#endif
    }

    delete[] data;
    delete[] add_data;
#ifdef HNSWLIB_ENABLE_FP16
    delete[] data_fp16;
    delete[] add_data_fp16;
#endif
    delete alg_hnsw;
    return 0;
}
```

Multithreaded examples:
* Creating index, inserting elements, searching [example_mt_search.cpp](example_mt_search.cpp)
* Filtering during the search with a boolean function [example_mt_filter.cpp](example_mt_filter.cpp)
* Reusing the memory of the deleted elements when new elements are being added [example_mt_replace_deleted.cpp](example_mt_replace_deleted.cpp)

More examples:
* Multivector search [example_multivector_search.cpp](example_multivector_search.cpp)
* Epsilon search [example_epsilon_search.cpp](example_epsilon_search.cpp)