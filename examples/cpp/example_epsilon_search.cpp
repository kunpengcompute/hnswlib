#include "hnswlib/hnswlib.h"

typedef unsigned int docidtype;
typedef float dist_t;

#ifdef HNSWLIB_ENABLE_FP16
int main() {
    int dim = 16;                  // Dimension of the elements
    int max_elements = 10000;      // Maximum number of elements, should be known beforehand
    int M = 16;                    // Tightly connected with internal dimensionality of the data
                                   // strongly affects the memory consumption
    int ef_construction = 200;     // Controls index search speed/build speed tradeoff
    int min_num_candidates = 100;  // Minimum number of candidates to search in the epsilon region
                                   // this parameter is similar to ef

    int num_queries = 5;
    float epsilon2 = 2.0;          // Squared distance to query

    // Initing index
    hnswlib::L2SpaceHalf space(dim);
    hnswlib::HierarchicalNSWHalf* alg_hnsw = new hnswlib::HierarchicalNSWHalf(&space, max_elements, M, ef_construction);
    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;

    size_t data_point_size = space.get_data_size();
    char* data = new char[data_point_size * max_elements];
    for (int i = 0; i < max_elements; i++) {
        char* point_data = data + i * data_point_size;
        for (int j = 0; j < dim; j++) {
            char* vec_data = point_data + j * sizeof(float16_t);
            float value = distrib_real(rng);
            float16_t value_fp16 = static_cast<float16_t>(value);
            *(float16_t*)vec_data = value_fp16;
        }
    }

    // Add data to index
    for (int i = 0; i < max_elements; i++) {
        hnswlib::labeltype label = i;
        char* point_data = data + i * data_point_size;
        alg_hnsw->addPoint(point_data, label);
    }

    // Query random vectors
    std::cout << "Query random vectors" << std::endl;
    for (int i = 0; i < num_queries; i++) {
        char* query_data = new char[data_point_size];
        for (int j = 0; j < dim; j++) {
            size_t offset = j * sizeof(float16_t);
            char* vec_data = query_data + offset;
            float value = distrib_real(rng);
            float16_t value_fp16 = static_cast<float16_t>(value);
            *(float16_t*)vec_data = value_fp16;
        }
        std::cout << "Query #" << i << "\n";
        hnswlib::EpsilonSearchStopCondition<dist_t> stop_condition(epsilon2, min_num_candidates, max_elements);
        std::vector<std::pair<float, hnswlib::labeltype>> result = 
            alg_hnsw->searchStopConditionClosest(query_data, stop_condition);
        size_t num_vectors = result.size();
        std::cout << "Found " << num_vectors << " vectors\n";
        delete[] query_data;
    }

    delete[] data;
    delete alg_hnsw;
    return 0;
}
#else
int main() {
    int dim = 16;                  // Dimension of the elements
    int max_elements = 10000;      // Maximum number of elements, should be known beforehand
    int M = 16;                    // Tightly connected with internal dimensionality of the data
                                   // strongly affects the memory consumption
    int ef_construction = 200;     // Controls index search speed/build speed tradeoff
    int min_num_candidates = 100;  // Minimum number of candidates to search in the epsilon region
                                   // this parameter is similar to ef

    int num_queries = 5;
    float epsilon2 = 2.0;          // Squared distance to query

    // Initing index
    hnswlib::L2SpaceF space(dim);
    hnswlib::HierarchicalNSWFloat* alg_hnsw = new hnswlib::HierarchicalNSWFloat(&space, max_elements, M, ef_construction);
    // Generate random data
    std::mt19937 rng;
    rng.seed(47);
    std::uniform_real_distribution<> distrib_real;

    size_t data_point_size = space.get_data_size();
    char* data = new char[data_point_size * max_elements];
    for (int i = 0; i < max_elements; i++) {
        char* point_data = data + i * data_point_size;
        for (int j = 0; j < dim; j++) {
            char* vec_data = point_data + j * sizeof(float);
            float value = distrib_real(rng);
            *(float*)vec_data = value;
        }
    }

    // Add data to index
    for (int i = 0; i < max_elements; i++) {
        hnswlib::labeltype label = i;
        char* point_data = data + i * data_point_size;
        alg_hnsw->addPoint(point_data, label);
    }

    // Query random vectors
    std::cout << "Query random vectors" << std::endl;
    for (int i = 0; i < num_queries; i++) {
        char* query_data = new char[data_point_size];
        for (int j = 0; j < dim; j++) {
            size_t offset = j * sizeof(float);
            char* vec_data = query_data + offset;
            float value = distrib_real(rng);
            *(float*)vec_data = value;
        }
        std::cout << "Query #" << i << "\n";
        hnswlib::EpsilonSearchStopCondition<dist_t> stop_condition(epsilon2, min_num_candidates, max_elements);
        std::vector<std::pair<float, hnswlib::labeltype>> result = 
            alg_hnsw->searchStopConditionClosest(query_data, stop_condition);
        size_t num_vectors = result.size();
        std::cout << "Found " << num_vectors << " vectors\n";
        delete[] query_data;
    }

    delete[] data;
    delete alg_hnsw;
    return 0;
}
#endif
