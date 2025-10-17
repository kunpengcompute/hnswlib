#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cmath>
#include "space_ip_template.h"

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

// 通用的内积计算（用于验证结果）
template<typename T>
float compute_inner_product(const T* v1, const T* v2, size_t qty) {
    float res = 0.0f;
    for (size_t i = 0; i < qty; ++i) {
        res += static_cast<float>(v1[i]) * static_cast<float>(v2[i]);
    }
    return -res; // 距离为负内积
}

template <typename T>
void test_fstdistfunc_selection(size_t dim, bool expect_simd16, bool expect_simd4, bool expect_residual16, bool expect_residual4) {
    hnswlib::InnerProductSpaceTemplate<T> space(dim);
    hnswlib::DISTFUNC<T> func = space.get_dist_func();

    bool is_simd16 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(InnerProductSIMD16ExtTemplate<T>));
    bool is_simd4 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(InnerProductSIMD4ExtTemplate<T>));
    bool is_residual16 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(InnerProductSIMD16ExtResidualsTemplate<T>));
    bool is_residual4 = (func == reinterpret_cast<hnswlib::DISTFUNC<T>>(InnerProductSIMD4ExtResidualsTemplate<T>));

    EXPECT_EQ(is_simd16, expect_simd16);
    EXPECT_EQ(is_simd4, expect_simd4);
    EXPECT_EQ(is_residual16, expect_residual16);
    EXPECT_EQ(is_residual4, expect_residual4);
}

// ================== 测试用例 ==================

// 测试：普通模板 float 内积
TEST(InnerProductTest, FloatTemplate_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = InnerProductTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：FP16 内积模板
TEST(InnerProductTest, FloatTemplate_Float16_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = InnerProductTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD16（float）
TEST(InnerProductTest, Float16Ext_SimpleCase) {
    const size_t N = 16;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = InnerProductSIMD16ExtTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD16（float16_t）
TEST(InnerProductTest, Float16Ext_Float16_SimpleCase) {
    const size_t N = 16;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = InnerProductSIMD16ExtTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD4（float）
TEST(InnerProductTest, Float4Ext_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = InnerProductSIMD4ExtTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD4（float16_t）
TEST(InnerProductTest, Float4Ext_Float16_SimpleCase) {
    const size_t N = 8;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = InnerProductSIMD4ExtTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}


// 测试：SIMD16 + SIMD4 + 普通模板：float
TEST(InnerProductTest, Float16ExtResiduals_SimpleCase) {
    const size_t N = 24;  // 16 + 8 + 1
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = InnerProductSIMD16ExtResidualsTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD16 + SIMD4 + 普通模板：float16_t
TEST(InnerProductTest, Float16ExtResiduals_Float16_SimpleCase) {
    const size_t N = 24;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = InnerProductSIMD16ExtResidualsTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f); // FP16 精度较低
}

// 测试：SIMD4 + 普通模板：float
TEST(InnerProductTest, Float4ExtResiduals_SimpleCase) {
    const size_t N = 9;  // 8 + 1
    auto vec1 = generate_random_vector<float>(N);
    auto vec2 = generate_random_vector<float>(N);

    size_t qty = N;
    float result = InnerProductSIMD4ExtResidualsTemplate<float>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：SIMD4 + 普通模板：float16_t
TEST(InnerProductTest, Float4ExtResiduals_Float16_SimpleCase) {
    const size_t N = 9;
    auto vec1 = generate_random_vector<float16_t>(N);
    auto vec2 = generate_random_vector<float16_t>(N);

    size_t qty = N;
    float result = InnerProductSIMD4ExtResidualsTemplate<float16_t>(vec1.data(), vec2.data(), &qty);

    float expected = compute_inner_product(vec1.data(), vec2.data(), N);
    EXPECT_NEAR(result, expected, 1e-4f);
}

// 测试：边界情况 - 空向量
TEST(InnerProductTest, EmptyVector_ReturnsZero) {
    const size_t N = 0;
    std::vector<float> vec1(N), vec2(N);
    size_t qty = N;

    float result16 = InnerProductSIMD16ExtResidualsTemplate<float>(vec1.data(), vec2.data(), &qty);
    float result4 = InnerProductSIMD4ExtResidualsTemplate<float>(vec1.data(), vec2.data(), &qty);
    float result = InnerProductTemplate<float>(vec1.data(), vec2.data(), &qty);

    EXPECT_FLOAT_EQ(result16, 0.0f);
    EXPECT_FLOAT_EQ(result4, 0.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

TEST(IPSpaceTemplateTest, ConstructorAndProperties) {
    using T = float16_t;
    const size_t dim = 16;

    InnerProductSpaceTemplate<T> space(dim);

    // 检查成员变量
    EXPECT_EQ(space.get_data_size(), dim * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space.get_dist_func_param()), dim);
    EXPECT_EQ(space.get_dist_func(), InnerProductSIMD16ExtTemplate<T>);

    const size_t dim_4 = 4;
    InnerProductSpaceTemplate<T> space_4(dim_4);
    EXPECT_EQ(space_4.get_data_size(), dim_4 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_4.get_dist_func_param()), dim_4);
    EXPECT_EQ(space_4.get_dist_func(), InnerProductSIMD4ExtTemplate<T>);

    const size_t dim_17 = 17;
    InnerProductSpaceTemplate<T> space_17(dim_17);
    EXPECT_EQ(space_17.get_data_size(), dim_17 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_17.get_dist_func_param()), dim_17);
    EXPECT_EQ(space_17.get_dist_func(), InnerProductSIMD16ExtResidualsTemplate<T>);

    const size_t dim_5 = 5;
    InnerProductSpaceTemplate<T> space_5(dim_5);
    EXPECT_EQ(space_5.get_data_size(), dim_5 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_5.get_dist_func_param()), dim_5);
    EXPECT_EQ(space_5.get_dist_func(), InnerProductSIMD4ExtResidualsTemplate<T>);

    const size_t dim_3 = 3;
    InnerProductSpaceTemplate<T> space_3(dim_3);
    EXPECT_EQ(space_3.get_data_size(), dim_3 * sizeof(T));
    EXPECT_EQ(*reinterpret_cast<const size_t*>(space_3.get_dist_func_param()), dim_3);
    EXPECT_EQ(space_3.get_dist_func(), InnerProductTemplate<T>);

    // // 验证 get_dist_func 返回的函数能正常工作
    // std::vector<T> a(dim, 1.0f);
    // std::vector<T> b(dim, 2.0f);
    // size_t dist = space.get_dist_func()(a.data(), b.data(), &dim);
    // EXPECT_EQ(dist, static_cast<size_t>(dim));
}


TEST(IPSpaceTemplateTest, SIMDSelectionLogic) {

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
    test_fstdistfunc_selection<float_t>(16, true, false, false, false);
    test_fstdistfunc_selection<float_t>(32, true, false, false, false);

    // dim % 16 != 0, but dim % 4 == 0
    test_fstdistfunc_selection<float_t>(8, false, true, false, false);

    // dim > 16, not divisible by 16 → residual
    test_fstdistfunc_selection<float_t>(17, false, false, true, false);

    // dim > 4, not divisible by 4 → residual
    test_fstdistfunc_selection<float_t>(7, false, false, false, true);

    test_fstdistfunc_selection<float_t>(3, false, false, false, false);


#else
    // 如果没有 SIMD，应该始终使用基础版本
    const size_t dim = 16;
    InnerProductSpaceTemplate<float> space(dim);
    EXPECT_EQ(space.get_dist_func(), InnerProductSpaceTemplate<float>);
#endif
}