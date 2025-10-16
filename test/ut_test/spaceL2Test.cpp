#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cmath>
#include "space_l2_template.h"

using namespace hnswlib;

// 用于测试的测试数据生成辅助函数
template<typename T>
std::vector<T> generate_random_vector(size_t n, float min = -1.0f, float max = 1.0f) {
    std::vector<T> vec(n);
    for (size_t i = 0; i < n; ++i) {
        float val = min + static_cast<float>(rand()) / RAND_MAX * (max - min);
        vec[i] = static_cast<T>(val);
    }
    return vec;
}

// 通用 L2Sqr 计算（用于验证结果）
template<typename T>
float compute_l2sqr(const T* v1, const T* v2, size_t qty) {
    float res = 0.0f;
    for (size_t i = 0; i < qty; ++i) {
        float diff = static_cast<float>(v1[i]) - static_cast<float>(v2[i]);
        res += diff * diff;
    }
    return res;
}

template <typename T>
void test_fstdistfunc_selection(size_t dim, bool expect_simd16, bool expect_simd4, bool expect_residual16, bool expect_residual4) {
    hnswlib::L2SpaceTemplate<T> space(dim);
    hnswlib::DISTFUNC<T> func = space.get_dist_func();

    bool is_simd16 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(L2SqrSIMD16ExtTemplate<T>));
    bool is_simd4 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(L2SqrSIMD4ExtTemplate<T>));
    bool is_residual16 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(L2SqrSIMD16ExtResidualsTemplate<T>));
    bool is_residual4 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(L2SqrSIMD4ExtResidualsTemplate<T>));

    EXPECT_EQ(is_simd16, expect_simd16);
    EXPECT_EQ(is_simd4, expect_simd4);
    EXPECT_EQ(is_residual16, expect_residual16);
    EXPECT_EQ(is_residual4, expect_residual4);
}

// ================== 测试用例 ==================

// 测试：普通模板 float 内积
TEST(L2Test, FloatTemplate_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = L2SqrTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：FP16 内积模板
TEST(L2Test, FloatTemplate_Float16_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = L2SqrTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD16（float）
TEST(L2Test, Float16Ext_SimpleCase) {
    const size_t N = 16;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = L2SqrSIMD16ExtTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD16（float16_t）
TEST(L2Test, Float16Ext_Float16_SimpleCase) {
    const size_t N = 16;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = L2SqrSIMD16ExtTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD4（float）
TEST(L2Test, Float4Ext_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = L2SqrSIMD4ExtTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD4（float16_t）
TEST(L2Test, Float4Ext_Float16_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = L2SqrSIMD4ExtTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}


// 测试：SIMD16 + SIMD4 + 普通模板：float
TEST(L2Test, Float16ExtResiduals_SimpleCase) {
    const size_t N = 24;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = L2SqrSIMD16ExtResidualsTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD16 + SIMD4 + 普通模板：float16_t
TEST(L2Test, Float16ExtResiduals_Float16_SimpleCase) {
    const size_t N = 24;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = L2SqrSIMD16ExtResidualsTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f); // FP16 精度较低
}

// 测试：SIMD4 + 普通模板：float
TEST(L2Test, Float4ExtResiduals_SimpleCase) {
    const size_t N = 9;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = L2SqrSIMD4ExtResidualsTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD4 + 普通模板：float16_t
TEST(L2Test, Float4ExtResiduals_Float16_SimpleCase) {
    const size_t N = 9;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = L2SqrSIMD4ExtResidualsTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_l2sqr(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：边界情况 - 空向量
TEST(L2Test, EmptyVector_ReturnsZero) {
    const size_t N = 0;
    std::vector<float> vec1(N), vec2(N);
    size_t qty = N;

    float result16 = L2SqrSIMD16ExtTemplate<float>(vec1.data(), vec2.data(), &qty);
    float result4 = L2SqrSIMD4ExtTemplate<float>(vec1.data(), vec2.data(), &qty);
    float result = L2SqrTemplate<float>(vec1.data(), vec2.data(), &qty);

    EXPECT_FLOAT_EQ(result16, 0.0f);
    EXPECT_FLOAT_EQ(result4, 0.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST(L2SpaceTemplateTest, ConstructorAndProperties) {
    using T = float16_t;
    const size_t dim = 16;

    L2SpaceTemplate<T> space(dim);

    // 检查成员变量
    EXPECT_EQ(space.get_data_size(), dim * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space.get_dist_func_param()), dim);
    EXPECT_EQ(space.get_dist_func(), L2SqrSIMD16ExtTemplate<T>);

    const size_t dim_4 = 4;
    L2SpaceTemplate<T> space_4(dim_4);
    EXPECT_EQ(space_4.get_data_size(), dim_4 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_4.get_dist_func_param()), dim_4);
    EXPECT_EQ(space_4.get_dist_func(), L2SqrSIMD4ExtTemplate<T>);

    const size_t dim_17 = 17;
    L2SpaceTemplate<T> space_17(dim_17);
    EXPECT_EQ(space_17.get_data_size(), dim_17 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_17.get_dist_func_param()), dim_17);
    EXPECT_EQ(space_17.get_dist_func(), L2SqrSIMD16ExtResidualsTemplate<T>);

    const size_t dim_5 = 5;
    L2SpaceTemplate<T> space_5(dim_5);
    EXPECT_EQ(space_5.get_data_size(), dim_5 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_5.get_dist_func_param()), dim_5);
    EXPECT_EQ(space_5.get_dist_func(), L2SqrSIMD4ExtResidualsTemplate<T>);

    const size_t dim_3 = 3;
    L2SpaceTemplate<T> space_3(dim_3);
    EXPECT_EQ(space_3.get_data_size(), dim_3 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_3.get_dist_func_param()), dim_3);
    EXPECT_EQ(space_3.get_dist_func(), L2SqrTemplate<T>);

}

TEST(L2SpaceTemplateTest, SIMDSelectionLogic) {

#if defined(USE_SVE) || defined(USE_NEON)

    // dim % 16 == 0
    test_fstdistfunc_selection<float>(16, true, false, false, false);
    test_fstdistfunc_selection<float>(32, true, false, false, false);

    // dim % 16 != 0, but dim % 4 == 0
    test_fstdistfunc_selection<float>(8, false, true, false, false);

    // dim > 16, not divisible by 16 → residual
    test_fstdistfunc_selection<float>(17, false, false, true, false);

    // dim > 4, not divisible by 4 → residual
    test_fstdistfunc_selection<float>(7, false, false, false, true);

    test_fstdistfunc_selection<float>(3, false, false, false, false);

    // dim % 16 == 0
    test_fstdistfunc_selection<float16_t>(16, true, false, false, false);
    test_fstdistfunc_selection<float16_t>(32, true, false, false, false);

    // dim % 16 != 0, but dim % 4 == 0
    test_fstdistfunc_selection<float16_t>(8, false, true, false, false);

    // dim > 16, not divisible by 16 → residual
    test_fstdistfunc_selection<float16_t>(17, false, false, true, false);

    // dim > 4, not divisible by 4 → residual
    test_fstdistfunc_selection<float16_t>(7, false, false, false, true);

    test_fstdistfunc_selection<float16_t>(3, false, false, false, false);

#else
    // 如果没有 SIMD，应该始终使用基础版本
    const size_t dim = 16;
    L2SpaceTemplate<float> space(dim);
    EXPECT_EQ(space.get_dist_func(), L2SqrTemplate<float>);
#endif
}