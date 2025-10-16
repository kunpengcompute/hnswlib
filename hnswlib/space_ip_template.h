/*
   Copyright 2025 Huawei Technologies Co., Ltd.

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
 
#pragma once

#if defined(HNSWLIB_USE_ARM)

#include <arm_sve.h>
#include <arm_neon.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>
#include <memory>
#include <cstring>
#include "hnswlib.h"

#if !defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC) && !defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
typedef __fp16 float16_t;
#else
#ifndef __fp16
typedef __fp16 float16_t;
#endif
#endif
namespace hnswlib {
template<typename T>
static float InnerProductTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    T *pVect1 = (T *)pVect1v;
    T *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    float res = 0;
    for (size_t i = 0; i < qty; i++) {
        res += (float)(*pVect1) * (float)(*pVect2);
        pVect1++;
        pVect2++;
    }
    return -res;
}

#if defined(USE_SVE)

template<typename T>
static float InnerProductSIMD16ExtTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    T *pVect1 = (T *)pVect1v;
    T *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    size_t qty16 = qty >> 4;

    const T *pEnd1 = pVect1 + (qty16 << 4);

    svfloat32_t sum = svdup_f32(0.0f);
    svbool_t pg8_b32 = svwhilelt_b32(0, 8);

    while (pVect1 < pEnd1) {
        svfloat32_t v1, v2;
        
        if constexpr (std::is_same_v<T, float>) {
            v1 = svld1_f32(pg8_b32, pVect1);
            v2 = svld1_f32(pg8_b32, pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)pVect1)));
            v2 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)pVect2)));
        }
        
        pVect1 += 8;
        pVect2 += 8;
        sum = svadd_f32_z(pg8_b32, sum, svmul_f32_z(pg8_b32, v1, v2));

        if constexpr (std::is_same_v<T, float>) {
            v1 = svld1_f32(pg8_b32, pVect1);
            v2 = svld1_f32(pg8_b32, pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)pVect1)));
            v2 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)pVect2)));
        }
        pVect1 += 8;
        pVect2 += 8;
        sum = svadd_f32_z(pg8_b32, sum, svmul_f32_z(pg8_b32, v1, v2));
    }

    return -svaddv_f32(pg8_b32, sum);
}

template<typename T>
static float InnerProductSIMDResidualTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    auto *pVect1 = (T *)pVect1v;
    auto *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    size_t vec_8x = qty >> 3 << 3;
    const T *pEnd1 = pVect1 + vec_8x;
    const T *pEnd2 = pVect2 + vec_8x;
    size_t residual = qty - vec_8x;

    svfloat32_t sum = svdup_f32(0.0f);
    svbool_t pg8_b32 = svwhilelt_b32(0, 8);

    while (pVect1 < pEnd1) {
        svfloat32_t v1, v2;

        if constexpr (std::is_same_v<T, float>) {
            v1 = svld1_f32(pg8_b32, pVect1);
            v2 = svld1_f32(pg8_b32, pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)pVect1)));
            v2 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)pVect2)));
        }
        pVect1 += 8;
        pVect2 += 8;
        sum = svadd_f32_z(pg8_b32, sum, svmul_f32_z(pg8_b32, v1, v2));
    }

    if (residual) {
        T resi_vec[16] = {0};
        memcpy(resi_vec, pEnd1, residual * sizeof(T));
        memcpy(resi_vec + 8, pEnd2, residual * sizeof(T));
        svfloat32_t v1, v2;

        if constexpr (std::is_same_v<T, float>) {
            v1 = svld1_f32(pg8_b32, resi_vec);
            v2 = svld1_f32(pg8_b32, resi_vec + 8);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)resi_vec)));
            v2 = svcvt_f32_f16_z(pg8_b32, svreinterpret_f16_u32(svld1uh_u32(pg8_b32, (const uint16_t *)resi_vec + 8)));
        }
        
        sum = svadd_f32_z(pg8_b32, sum, svmul_f32_z(pg8_b32, v1, v2));
    }

    return -svaddv_f32(pg8_b32, sum);
}

template<typename T>
static float InnerProductSIMD4ExtTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    auto *pVect1 = (T *)pVect1v;
    auto *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    size_t qty4 = qty >> 2;
    const T *pEnd1 = pVect1 + (qty4 << 2);
    svfloat32_t sum = svdup_f32(0.0f);
    svbool_t pg4_b32 = svwhilelt_b32(0, 4);
    while (pVect1 < pEnd1) {
        svfloat32_t v1, v2;

        if constexpr (std::is_same_v<T, float>) {
            v1 = svld1_f32(pg4_b32, pVect1);
            v2 = svld1_f32(pg4_b32, pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = svcvt_f32_f16_z(pg4_b32, svreinterpret_f16_u32(svld1uh_u32(pg4_b32, (const uint16_t *)pVect1)));
            v2 = svcvt_f32_f16_z(pg4_b32, svreinterpret_f16_u32(svld1uh_u32(pg4_b32, (const uint16_t *)pVect2)));
        }

        pVect1 += 4;
        pVect2 += 4;
        sum = svadd_f32_z(pg4_b32, sum, svmul_f32_z(pg4_b32, v1, v2));
    }

    return -svaddv_f32(pg4_b32, sum);
}

template<typename T>
static float InnerProductSIMD4ExtResidualsTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    size_t qty = *((size_t *)qty_ptr);
    size_t qty4 = qty >> 2 << 2;
    float res = InnerProductSIMD4ExtTemplate<T>(pVect1v, pVect2v, &qty4);
    size_t qty_left = qty - qty4;
    T *pVect1 = (T *)pVect1v + qty4;
    T *pVect2 = (T *)pVect2v + qty4;
    float res_tail = InnerProductSIMDResidualTemplate<T>(pVect1, pVect2, &qty_left);
    return (res + res_tail);
}

#elif defined(USE_NEON)

template<typename T>
static float InnerProductSIMD16ExtTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    T *pVect1 = (T *)pVect1v;
    T *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    size_t qty16 = qty >> 4;
    const T *pEnd1 = pVect1 + (qty16 << 4);
    float32x4_t sum = vdupq_n_f32(0.0f);
    while (pVect1 < pEnd1) {
        float32x4_t v1, v2;

        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(pVect1);
            v2 = vld1q_f32(pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(pVect1));
            v2 = vcvt_f32_f16(vld1_f16(pVect2));
        }
        
        pVect1 += 4;
        pVect2 += 4;
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));

        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(pVect1);
            v2 = vld1q_f32(pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(pVect1));
            v2 = vcvt_f32_f16(vld1_f16(pVect2));
        }
        
        pVect1 += 4;
        pVect2 += 4;
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));

        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(pVect1);
            v2 = vld1q_f32(pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(pVect1));
            v2 = vcvt_f32_f16(vld1_f16(pVect2));
        }
        
        pVect1 += 4;
        pVect2 += 4;
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));

        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(pVect1);
            v2 = vld1q_f32(pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(pVect1));
            v2 = vcvt_f32_f16(vld1_f16(pVect2));
        }
        
        pVect1 += 4;
        pVect2 += 4;
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));
    }

    return vaddvq_f32(sum);
}

template<typename T>
static float InnerProductSIMD4ExtTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    auto *pVect1 = (T *)pVect1v;
    auto *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    size_t qty4 = qty >> 2;

    const T *pEnd1 = pVect1 + (qty4 << 2);

    float32x4_t sum = vdupq_n_f32(0.0f);

    while (pVect1 < pEnd1) {
        float32x4_t v1, v2;
        
        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(pVect1);
            v2 = vld1q_f32(pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(pVect1));
            v2 = vcvt_f32_f16(vld1_f16(pVect2));
        }
        
        pVect1 += 4;
        pVect2 += 4;
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));
    }

    return - vaddvq_f32(sum);
}

template<typename T>
static float InnerProductSIMDResidualTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    auto *pVect1 = (T *)pVect1v;
    auto *pVect2 = (T *)pVect2v;
    size_t qty = *((size_t *)qty_ptr);
    size_t vec_4x = qty >> 2 << 2;
    const T *pEnd1 = pVect1 + vec_4x;
    const T *pEnd2 = pVect2 + vec_4x;
    size_t residual = qty - vec_4x;

    float32x4_t sum = vdupq_n_f32(0.0f);

    while (pVect1 < pEnd1) {
        float32x4_t v1, v2;
        
        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(pVect1);
            v2 = vld1q_f32(pVect2);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(pVect1));
            v2 = vcvt_f32_f16(vld1_f16(pVect2));
        }
        
        pVect1 += 4;
        pVect2 += 4;
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));
    }

    if (residual) {
        T resi_vec[16] = {0};
        memcpy(resi_vec, pEnd1, residual * sizeof(T));
        memcpy(resi_vec + 8, pEnd2, residual * sizeof(T));

        float32x4_t v1, v2;
        if constexpr (std::is_same_v<T, float>) {
            v1 = vld1q_f32(resi_vec);
            v2 = vld1q_f32(resi_vec + 8);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            v1 = vcvt_f32_f16(vld1_f16(resi_vec));
            v2 = vcvt_f32_f16(vld1_f16(resi_vec + 8));
        }
        
        sum = vaddq_f32(sum, vmulq_f32(v1, v2));
    }

    return - vaddvq_f32(sum);
}

template<typename T>
static float InnerProductSIMD4ExtResidualsTemplate(const void *pVect1v, const void *pVect2v, const void *qty_ptr) {
    size_t qty = *((size_t *)qty_ptr);
    size_t qty4 = qty >> 2 << 2;

    float res = InnerProductSIMD4ExtTemplate<T>(pVect1v, pVect2v, &qty4);
    size_t qty_left = qty - qty4;

    T *pVect1 = (T *)pVect1v + qty4;
    T *pVect2 = (T *)pVect2v + qty4;
    float res_tail = InnerProductSIMDResidualTemplate<T>(pVect1, pVect2, &qty_left);

    return (res + res_tail);
}

#endif

// 模板化内积空间类
template<typename T>
class InnerProductSpaceTemplate : public SpaceInterface<T> {
    DISTFUNC fstdistfunc_;
    size_t data_size_;
    size_t dim_;

public:
    InnerProductSpaceTemplate(size_t dim) {
        fstdistfunc_ = InnerProductTemplate<T>;
        
#if defined(USE_SVE) || defined(USE_NEON)
        if (dim % 16 == 0)
            fstdistfunc_ = InnerProductSIMD16ExtTemplate<T>;
        else if (dim % 4 == 0)
            fstdistfunc_ = InnerProductSIMD4ExtTemplate<T>;
        else if (dim > 16)
            fstdistfunc_ = InnerProductSIMDResidualTemplate<T>;
        else if (dim > 4)
            fstdistfunc_ = InnerProductSIMD4ExtResidualsTemplate<T>;
#endif
        
        dim_ = dim;
        data_size_ = dim * sizeof(T);
    }

    size_t get_data_size() {
        return data_size_;
    }

    DISTFUNC get_dist_func() {
        return fstdistfunc_;
    }

    void *get_dist_func_param() {
        return &dim_;
    }

    ~InnerProductSpaceTemplate() {}
};

// 类型别名，保持向后兼容
using InnerProductSpaceF = InnerProductSpaceTemplate<float>;
#ifdef HNSWLIB_ENABLE_FP16
using InnerProductSpaceHalf = InnerProductSpaceTemplate<float16_t>;
#endif

} // namespace hnswlib 

#endif // HNSWLIB_USE_ARM