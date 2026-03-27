/*
   Copyright 2026 Huawei Technologies Co., Ltd.
  
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

#include <cstring>
#include <vector>
#include <cmath>
#include "../../hnswlib/hnswlib.h"

using namespace hnswlib;

std::vector<float> generate_random_vector(size_t n, float min = -1.0f, float max = 1.0f)
{
    std::vector<float> vec(n);
    for (size_t i = 0; i < n; ++i) {
        float val = min + static_cast<float>(rand()) / RAND_MAX * (max - min);
        vec[i] = val;
    }
    return vec;
}

std::vector<float16_t> generate_random_vector_fp16(size_t n, float min = -1.0f, float max = 1.0f)
{
    std::vector<float16_t> vec(n);
    for (size_t i = 0; i < n; ++i) {
        float val = min + static_cast<float>(rand()) / RAND_MAX * (max - min);
        vec[i] = static_cast<float16_t>(val);
    }
    return vec;
}

#ifdef USE_NEON
bool run_distance_ip_test(size_t N) {
    auto vec1 = generate_random_vector(N);
    auto vec2 = generate_random_vector(N);

    size_t qty = N;
    float expected = InnerProductDistance(vec1.data(), vec2.data(), &qty);

    float result = 0;
    if (N % 16 == 0) {
        result = InnerProductDistanceSIMD16ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N % 4 == 0) {
        result = InnerProductDistanceSIMD4ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 16) {
        result = InnerProductDistanceSIMD16ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 4) {
        result = InnerProductDistanceSIMD4ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else {
        result = InnerProductDistance(vec1.data(), vec2.data(), &qty);
    }

    float diff = std::fabs(result - expected);
    if (diff > 1e-4f) {
        std::cout << "[FAIL] InnerProduct test for N=" << N 
                  << " | Result: " << result 
                  << " | Expected: " << expected 
                  << " | Diff: " << diff << std::endl;
        return false;
    } else {
        std::cout << "[PASS] InnerProduct test for N=" << N << std::endl;
        return true;
    }
}


bool run_distance_l2_test(size_t N) {
    auto vec1 = generate_random_vector(N);
    auto vec2 = generate_random_vector(N);

    size_t qty = N;
    float expected = L2Sqr(vec1.data(), vec2.data(), &qty);

    float result = 0;
    if (N % 16 == 0) {
        result = L2SqrSIMD16ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N % 4 == 0) {
        result = L2SqrSIMD4ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 16) {
        result = L2SqrSIMD16ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 4) {
        result = L2SqrSIMD4ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else {
        result = L2Sqr(vec1.data(), vec2.data(), &qty);
    }

    float diff = std::fabs(result - expected);
    if (diff > 1e-3f) {
        std::cout << "[FAIL] L2 test for N=" << N 
                  << " | Result: " << result 
                  << " | Expected: " << expected 
                  << " | Diff: " << diff << std::endl;
        return false;
    } else {
        std::cout << "[PASS] L2 test for N=" << N << std::endl;
        return true;
    }
}

bool run_distance_ip_fp16_test(size_t N) {
    auto vec1 = generate_random_vector_fp16(N);
    auto vec2 = generate_random_vector_fp16(N);

    size_t qty = N;
    float expected = InnerProductDistancePh(vec1.data(), vec2.data(), &qty);

    float result = 0;
    if (N % 16 == 0) {
        result = InnerProductDistancePhSIMD16ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N % 4 == 0) {
        result = InnerProductDistancePhSIMD4ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 16) {
        result = InnerProductDistancePhSIMD16ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 4) {
        result = InnerProductDistancePhSIMD4ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else {
        result = InnerProductDistancePh(vec1.data(), vec2.data(), &qty);
    }

    float diff = std::fabs(result - expected);
    if (diff > 1e-3f) {
        std::cout << "[FAIL] InnerProduct test for N=" << N 
                  << " | Result: " << result 
                  << " | Expected: " << expected 
                  << " | Diff: " << diff << std::endl;
        return false;
    } else {
        std::cout << "[PASS] InnerProduct test for N=" << N << std::endl;
        return true;
    }
}


bool run_distance_l2_fp16_test(size_t N) {
    auto vec1 = generate_random_vector_fp16(N);
    auto vec2 = generate_random_vector_fp16(N);

    size_t qty = N;
    float expected = L2SqrPh(vec1.data(), vec2.data(), &qty);

    float result = 0;
    if (N % 16 == 0) {
        result = L2SqrPhSIMD16ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N % 4 == 0) {
        result = L2SqrPhSIMD4ExtNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 16) {
        result = L2SqrPhSIMD16ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else if (N > 4) {
        result = L2SqrPhSIMD4ExtResidualsNEON(vec1.data(), vec2.data(), &qty);
    } else {
        result = L2SqrPh(vec1.data(), vec2.data(), &qty);
    }

    float diff = std::fabs(result - expected);
    if (diff > 1e-1f) {
        std::cout << "[FAIL] L2 test for N=" << N 
                  << " | Result: " << result 
                  << " | Expected: " << expected 
                  << " | Diff: " << diff << std::endl;
        return false;
    } else {
        std::cout << "[PASS] L2 test for N=" << N << std::endl;
        return true;
    }
}
#endif

int main() {
#ifdef USE_NEON
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    std::vector<size_t> test_sizes = {1024, 512, 65, 64, 37, 36, 18, 9, 3, 2, 1};
    bool all_tests_passed = true;

    std::cout << "=== Start testing the InnerProduct distance calculation. ===" << std::endl;
    for (size_t size : test_sizes) {
        if (!run_distance_ip_test(size)) {
            all_tests_passed = false;
        }
    }

    std::cout << "\n=== Start testing the L2 distance calculation. ===" << std::endl;
    for (size_t size : test_sizes) {
        if (!run_distance_l2_test(size)) {
            all_tests_passed = false;
        }
    }

    std::cout << "=== Start testing the InnerProductPh distance calculation. ===" << std::endl;
    for (size_t size : test_sizes) {
        if (!run_distance_ip_fp16_test(size)) {
            all_tests_passed = false;
        }
    }

    std::cout << "\n=== Start testing the L2Ph distance calculation. ===" << std::endl;
    for (size_t size : test_sizes) {
        if (!run_distance_l2_fp16_test(size)) {
            all_tests_passed = false;
        }
    }

    std::cout << "\n=== Result Summary ===" << std::endl;
    if (all_tests_passed) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests failed!" << std::endl;
        return 1;
    }
#else
    std::cout << "This test cannot be performed because the NEON macro is not enabled." << std::endl;
    return 1;
#endif;
}
