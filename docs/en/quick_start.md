# Quick Start

After installing the hnswlib source code, follow the following instructions to verify the core functions of hnswlib optimized for the Kunpeng platform.

## Source Code Directory Description<a name="section20169143153012"></a>

Assume that the directory where the program runs is `/path/to/hnswlib`. The complete directory structure is as follows:

```text
├── examples/           // Function test files
├── hnswlib/            // hnswlib header files
├── python_bindings/    // Python API files
├── tests/             // Function test files
├── ALGO_PARAMS.md
├── CMakeLists.txt     // CMakeLists.txt file for function tests
├── LICENSE
├── Makefile           // Makefile file for performance tests
├── MANIFEST.in
├── pyproject.toml
├── README.md
├── setup.py
└── TESTING_RECALL.md
```

## Quick Start Code Example<a name="section287635033012"></a>

This section provides an example of using hnswlib, including index creation, element insertion, searching, index saving, and index loading.

```cpp
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include "hnswlib/hnswlib.h"
 
// Define the data dimension and quantity.
const size_t DIM = 128;
const size_t NUM_ELEMENTS = 10000;
const size_t NUM_QUERIES = 10;
const size_t K = 10;  // Number of nearest neighbors to return
 
// Generate random fp32 vectors.
void generateRandomVectors(std::vector<float>& data, size_t num_elements, size_t dim) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    data.resize(num_elements * dim);
    for (size_t i = 0; i < num_elements * dim; ++i) {
        data[i] = dist(gen);
    }
}
 
// Convert fp32 vectors to fp16 vectors.
void convertFp32ToFp16(const std::vector<float>& fp32_data, std::vector<hnswlib::float16_t>& fp16_data) {
    fp16_data.resize(fp32_data.size());
    for (size_t i = 0; i < fp32_data.size(); ++i) {
        fp16_data[i] = static_cast<hnswlib::float16_t>(fp32_data[i]);
    }
}
 
// Demonstrate the usage of FP32 data type.
void demonstrateFp32() {
    std::cout << "=== FP32 example ===\n";
    
    // Generate random data.
    std::vector<float> data;
    generateRandomVectors(data, NUM_ELEMENTS, DIM);
    
    // L2 distance space example
    std::cout << "\n1. L2 distance space: \n";
    {
        hnswlib::L2Space space(DIM);
        hnswlib::HierarchicalNSW<float> index(&space, NUM_ELEMENTS);
        
        // Add data points.
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            index.addPoint(&data[i * DIM], i);
        }
        
        // Search for the nearest neighbors.
        index.setEf(100);  // Set the search parameter.
        size_t correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = index.searchKnn(&data[query_idx * DIM], K);
            
            // Check whether the first result is the query point itself.
            if (!result.empty() && result[0].second == query_idx) {
                correct++;
            }
        }
        std::cout << "   Ratio of correctly identified query points: " << (float)correct / NUM_QUERIES * 100 << "%\n";
        
        // Save the index to a file.
        const std::string index_file = "l2_index.bin";;
        index.saveIndex(index_file);
        
        // Load the index from the file.
        hnswlib::HierarchicalNSW<float> loaded_index(&space, index_file);
        
        // Use the loaded index for search verification.
        loaded_index.setEf(100);
        size_t loaded_correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = loaded_index.searchKnn(&data[query_idx * DIM], K);
            
            // Check whether the first result is the query point itself.
            if (!result.empty() && result[0].second == query_idx) {
                loaded_correct++;
            }
        }
        std::cout << "   Ratio of correctly identified query points by the loaded index: " << (float)loaded_correct / NUM_QUERIES * 100 << "%\n";
        
        // Verify consistency between the original index and the loaded index.
        if (correct == loaded_correct) {
            std::cout << " ✅ Search results match between the original and loaded indexes.\n";
        } else {
            std::cout << " ❌ Search results mismatch between the original and loaded indexes.\n";
        }
    }
}
 
// Demonstrate the usage of FP16 data type (supported on NEON architecture only).
void demonstrateFp16() {
#ifdef USE_NEON
    std::cout << "\n=== FP16 example (NEON architecture only) ===\n";
    
    // Generate random fp32 data.
    std::vector<float> fp32_data;
    generateRandomVectors(fp32_data, NUM_ELEMENTS, DIM);
    
    // Convert the data to fp16 data.
    std::vector<hnswlib::float16_t> fp16_data;
    convertFp32ToFp16(fp32_data, fp16_data);
    
    // L2 distance space example (fp16)
    std::cout << "\n1. L2 distance space (fp16):\n";
    {
        hnswlib::L2SpacePh space(DIM);
        hnswlib::HierarchicalNSW<float> index(&space, NUM_ELEMENTS);
        
        // Add data points.
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            index.addPoint(&fp16_data[i * DIM], i);
        }
        
        // Search for the nearest neighbors.
        index.setEf(100);  // Set the search parameter.
        size_t correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = index.searchKnn(&fp16_data[query_idx * DIM], K);
            
            // Check whether the first result is the query point itself.
            if (!result.empty() && result[0].second == query_idx) {
                correct++;
            }
        }
        std::cout << "   Ratio of correctly identified query points: " << (float)correct / NUM_QUERIES * 100 << "%\n";
        
        // Save the index to a file.
        const std::string index_file = "l2_ph_index.bin";
        index.saveIndex(index_file);
        
        // Load the index from the file.
        hnswlib::HierarchicalNSW<float> loaded_index(&space, index_file);
        
        // Use the loaded index for search verification.
        loaded_index.setEf(100);
        size_t loaded_correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = loaded_index.searchKnn(&fp16_data[query_idx * DIM], K);
            
            // Check whether the first result is the query point itself.
            if (!result.empty() && result[0].second == query_idx) {
                loaded_correct++;
            }
        }
        std::cout << "   Ratio of correctly identified query points by the loaded index: " << (float)loaded_correct / NUM_QUERIES * 100 << "%\n";
        
        // Verify consistency between the original index and the loaded index.
        if (correct == loaded_correct) {
            std::cout << " ✅ Search results match between the original and loaded indexes.\n";
        } else {
            std::cout << " ❌ Search results mismatch between the original and loaded indexes.\n";
        }
    }
#else
    std::cout << "\n=== FP16 example (NEON architecture required)\n";
    std::cout << "   The current compilation environment does not support NEON architecture. Skipping the FP16 example.\n";
#endif
}
 
int main() {
    std::cout << "hnswlib Quick start example\n";
    std::cout << "Data dimension: " << DIM << "\n";
    std::cout << "Data size: " << NUM_ELEMENTS << "\n";
    Number of std::cout << "Query count: " << NUM_QUERIES << "\n";
    std::cout << "Number of nearest neighbors to return: " << K << "\n\n";
    
    // Display the fp32 API.
    demonstrateFp32();
    
    // Display the fp16 API.
    demonstrateFp16();
    
    std::cout << "\n=== Example completed ===\n";
    return 0;
}
```
