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

#include <string>
#include <type_traits>

namespace hnswlib {

//前向声明，避免循环依赖
template<typename T> class L2SpaceTemplate;
template<typename T> class InnerProductSpaceTemplate;

// 相似性度量类型枚举
enum class DistanceType {
    L2,           // 欧几里得距离
    INNER_PRODUCT // 内积相似度
};

// 数据类型枚举
enum class DataType {
    FP32,         // 单精度浮点
    FP16          // 半精度浮点
};

// 模板化空间工厂类
class SpaceFactory {
public:
    // 模板化：根据distance_type字符串和维度创建对应类型空间
    template<typename T>
    static SpaceInterface<T>* createSpace(const std::string& distance_type, size_t dim) {
        if (distance_type == "l2" || distance_type == "L2" || distance_type == "euclidean") {
            return new L2SpaceTemplate<T>(dim);
        } else if (distance_type == "ip" || distance_type == "IP" || 
                   distance_type == "inner_product" || distance_type == "cosine") {
            return new InnerProductSpaceTemplate<T>(dim);
        } else {
            throw std::invalid_argument("Unknown distance type: " + distance_type);
        }
    }

    // 兼容原有字符串接口，返回SpaceInterface<float>*，内部根据data_type分发
    static SpaceInterface<float>* createSpace(
        const std::string& distance_type,
        const std::string& data_type,
        size_t dim
    ) {
        if (data_type == "fp32" || data_type == "FP32" || data_type == "float") {
            return createSpace<float>(distance_type, dim);
        } else if (data_type == "fp16" || data_type == "FP16" || data_type == "half") {
            // 注意：此处返回类型为SpaceInterface<float>*，但实际对象为SpaceInterface<float16_t>*
            // 使用时需确保类型安全
            return reinterpret_cast<SpaceInterface<float>*>(createSpace<float16_t>(distance_type, dim));
        } else {
            throw std::invalid_argument("Unknown data type: " + data_type);
        }
    }

    // 枚举型接口，适用于更严格的类型安全
    template<typename T>
    static SpaceInterface<T>* createSpace(DistanceType distance_type, size_t dim) {
        switch (distance_type) {
            case DistanceType::L2:
                return new L2SpaceTemplate<T>(dim);
            case DistanceType::INNER_PRODUCT:
                return new InnerProductSpaceTemplate<T>(dim);
            default:
                throw std::invalid_argument("Unknown distance type");
        }
    }

    // 获取数据类型对应的模板类型
    template<typename T>
    static DataType getDataType() {
        if constexpr (std::is_same_v<T, float>) {
            return DataType::FP32;
        } else if constexpr (std::is_same_v<T, float16_t>) {
            return DataType::FP16;
        } else {
            throw std::invalid_argument("Unsupported data type");
        }
    }

    // 获取数据类型对应的字节大小
    static size_t getDataTypeSize(DataType data_type) {
        switch (data_type) {
            case DataType::FP32:
                return sizeof(float);
            case DataType::FP16:
                return sizeof(float16_t);
            default:
                throw std::invalid_argument("Unknown data type");
        }
    }

    // 验证数据类型和空间类型的匹配
    template<typename T>
    static bool validateSpaceType(SpaceInterface<T>* space) {
        if constexpr (std::is_same_v<T, float>) {
            return (dynamic_cast<L2SpaceTemplate<float>*>(space) != nullptr ||
                    dynamic_cast<InnerProductSpaceTemplate<float>*>(space) != nullptr);
        } else if constexpr (std::is_same_v<T, float16_t>) {
            return (dynamic_cast<L2SpaceTemplate<float16_t>*>(space) != nullptr ||
                    dynamic_cast<InnerProductSpaceTemplate<float16_t>*>(space) != nullptr);
        }
        return false;
    }

    // 字符串转枚举
    static DistanceType parseDistanceType(const std::string& distance_type) {
        if (distance_type == "l2" || distance_type == "L2" || distance_type == "euclidean") {
            return DistanceType::L2;
        } else if (distance_type == "ip" || distance_type == "IP" || 
                   distance_type == "inner_product" || distance_type == "cosine") {
            return DistanceType::INNER_PRODUCT;
        } else {
            throw std::invalid_argument("Unknown distance type: " + distance_type);
        }
    }
    static DataType parseDataType(const std::string& data_type) {
        if (data_type == "fp32" || data_type == "FP32" || data_type == "float") {
            return DataType::FP32;
        } else if (data_type == "fp16" || data_type == "FP16" || data_type == "half") {
            return DataType::FP16;
        } else {
            throw std::invalid_argument("Unknown data type: " + data_type);
        }
    }
};

// 便捷的创建函数
inline SpaceInterface<float>* createL2Space(size_t dim, bool use_fp16 = false) {
    if (use_fp16) {
        return reinterpret_cast<SpaceInterface<float>*>(new L2SpaceTemplate<float16_t>(dim));
    } else {
        return new L2SpaceTemplate<float>(dim);
    }
}

inline SpaceInterface<float>* createInnerProductSpace(size_t dim, bool use_fp16 = false) {
    if (use_fp16) {
        return reinterpret_cast<SpaceInterface<float>*>(new InnerProductSpaceTemplate<float16_t>(dim));
    } else {
        return new InnerProductSpaceTemplate<float>(dim);
    }
}

} // namespace hnswlib
