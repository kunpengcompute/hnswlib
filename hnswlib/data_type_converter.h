#pragma once
namespace hnswlib {

class DataTypeConverter {
public:
    // 单个 float -> fp16
    static float16_t float_to_fp16(float value) {
#if defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
        return static_cast<float16_t>(value);
#else
        // 你可以用 union/bit操作或第三方库实现
        // 这里假设 float16_t 有构造函数
        return float16_t(value);
#endif
    }

    // 单个 fp16 -> float
    static float fp16_to_float(float16_t value) {
#if defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC)
        return static_cast<float>(value);
#else
        // 你可以用 union/bit操作或第三方库实现
        return static_cast<float>(value);
#endif
    }

    // 批量转换可复用已有实现
    static void convert_fp32_to_fp16(const float* src, float16_t* dst, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            dst[i] = float_to_fp16(src[i]);
        }
    }
    static void convert_fp16_to_fp32(const float16_t* src, float* dst, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            dst[i] = fp16_to_float(src[i]);
        }
    }

    static bool validate_fp16_data(const float16_t* data, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            if (std::isnan(data[i]) || std::isinf(data[i])) {
                return false;
            }
        }
        return true;
    }
};

} // namespace hnswlib