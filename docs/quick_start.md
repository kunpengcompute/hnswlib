# 快速入门<a name="ZH-CN_TOPIC_0000002521702930"></a>

## 快速入门<a name="ZH-CN_TOPIC_0000002551618397"></a>

请在完成hnswlib源码安装后，参考本节示例内容验证基于鲲鹏平台优化后的hnswlib的核心功能。

**源码目录说明<a name="section20169143153012"></a>**

假设程序运行的目录为“"/path/to/hnswlib"“，完整的目录结构应如下所示：

```text
├── examples/           // 功能测试文件
├── hnswlib/            // hnswlib头文件
├── python_bindings/    // python接口文件
├── tests/              // 功能测试文件
├── ALGO_PARAMS.md
├── CMakeLists.txt      // 功能测试CMakeLists.txt文件
├── LICENSE
├── Makefile            // 性能测试Makefile文件
├── MANIFEST.in
├── pyproject.toml
├── README.md
├── setup.py
└── TESTING_RECALL.md
```

**快速入门代码示例<a name="section287635033012"></a>**

本节提供hnswlib入门示例，包含创建索引、插入元素、搜索、保存索引和加载索引的示例。

```cpp
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include "hnswlib/hnswlib.h"
 
// 定义数据维度和数量
const size_t DIM = 128;
const size_t NUM_ELEMENTS = 10000;
const size_t NUM_QUERIES = 10;
const size_t K = 10;  // 搜索最近邻的数量
 
// 生成随机fp32向量
void generateRandomVectors(std::vector<float>& data, size_t num_elements, size_t dim) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    data.resize(num_elements * dim);
    for (size_t i = 0; i < num_elements * dim; ++i) {
        data[i] = dist(gen);
    }
}
 
// 将fp32向量转换为fp16向量
void convertFp32ToFp16(const std::vector<float>& fp32_data, std::vector<hnswlib::float16_t>& fp16_data) {
    fp16_data.resize(fp32_data.size());
    for (size_t i = 0; i < fp32_data.size(); ++i) {
        fp16_data[i] = static_cast<hnswlib::float16_t>(fp32_data[i]);
    }
}
 
// 展示fp32数据类型的使用
void demonstrateFp32() {
    std::cout << "=== FP32 示例 ===\n";
    
    // 生成随机数据
    std::vector<float> data;
    generateRandomVectors(data, NUM_ELEMENTS, DIM);
    
    // L2距离空间示例
    std::cout << "\n1. L2距离空间：\n";
    {
        hnswlib::L2Space space(DIM);
        hnswlib::HierarchicalNSW<float> index(&space, NUM_ELEMENTS);
        
        // 添加数据点
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            index.addPoint(&data[i * DIM], i);
        }
        
        // 搜索最近邻
        index.setEf(100);  // 设置搜索参数
        size_t correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = index.searchKnn(&data[query_idx * DIM], K);
            
            // 检查第一个结果是否是查询点本身
            if (!result.empty() && result[0].second == query_idx) {
                correct++;
            }
        }
        std::cout << "   正确识别查询点的比例: " << (float)correct / NUM_QUERIES * 100 << "%\n";
        
        // 保存索引到文件
        const std::string index_file = "l2_index.bin";;
        index.saveIndex(index_file);
        
        // 从文件加载索引
        hnswlib::HierarchicalNSW<float> loaded_index(&space, index_file);
        
        // 使用加载的索引进行搜索验证
        loaded_index.setEf(100);
        size_t loaded_correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = loaded_index.searchKnn(&data[query_idx * DIM], K);
            
            // 检查第一个结果是否是查询点本身
            if (!result.empty() && result[0].second == query_idx) {
                loaded_correct++;
            }
        }
        std::cout << "   加载的索引正确识别查询点的比例: " << (float)loaded_correct / NUM_QUERIES * 100 << "%\n";
        
        // 验证原始索引和加载的索引结果是否一致
        if (correct == loaded_correct) {
            std::cout << "   ✅ 原始索引和加载的索引搜索结果一致\n";
        } else {
            std::cout << "   ❌ 原始索引和加载的索引搜索结果不一致\n";
        }
    }
}
 
// 展示fp16数据类型的使用（仅支持NEON架构）
void demonstrateFp16() {
#ifdef USE_NEON
    std::cout << "\n=== FP16 示例 (仅支持NEON架构) ===\n";
    
    // 生成随机fp32数据
    std::vector<float> fp32_data;
    generateRandomVectors(fp32_data, NUM_ELEMENTS, DIM);
    
    // 转换为fp16数据
    std::vector<hnswlib::float16_t> fp16_data;
    convertFp32ToFp16(fp32_data, fp16_data);
    
    // L2距离空间示例 (fp16)
    std::cout << "\n1. L2距离空间 (fp16):\n";
    {
        hnswlib::L2SpacePh space(DIM);
        hnswlib::HierarchicalNSW<float> index(&space, NUM_ELEMENTS);
        
        // 添加数据点
        for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
            index.addPoint(&fp16_data[i * DIM], i);
        }
        
        // 搜索最近邻
        index.setEf(100);  // 设置搜索参数
        size_t correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = index.searchKnn(&fp16_data[query_idx * DIM], K);
            
            // 检查第一个结果是否是查询点本身
            if (!result.empty() && result[0].second == query_idx) {
                correct++;
            }
        }
        std::cout << "   正确识别查询点的比例: " << (float)correct / NUM_QUERIES * 100 << "%\n";
        
        // 保存索引到文件
        const std::string index_file = "l2_ph_index.bin";
        index.saveIndex(index_file);
        
        // 从文件加载索引
        hnswlib::HierarchicalNSW<float> loaded_index(&space, index_file);
        
        // 使用加载的索引进行搜索验证
        loaded_index.setEf(100);
        size_t loaded_correct = 0;
        for (size_t i = 0; i < NUM_QUERIES; ++i) {
            size_t query_idx = i * (NUM_ELEMENTS / NUM_QUERIES);
            auto result = loaded_index.searchKnn(&fp16_data[query_idx * DIM], K);
            
            // 检查第一个结果是否是查询点本身
            if (!result.empty() && result[0].second == query_idx) {
                loaded_correct++;
            }
        }
        std::cout << "   加载的索引正确识别查询点的比例: " << (float)loaded_correct / NUM_QUERIES * 100 << "%\n";
        
        // 验证原始索引和加载的索引结果是否一致
        if (correct == loaded_correct) {
            std::cout << "   ✅ 原始索引和加载的索引搜索结果一致\n";
        } else {
            std::cout << "   ❌ 原始索引和加载的索引搜索结果不一致\n";
        }
    }
#else
    std::cout << "\n=== FP16 示例 (需要NEON架构支持) ===\n";
    std::cout << "   当前编译环境不支持NEON架构，跳过FP16示例\n";
#endif
}
 
int main() {
    std::cout << "hnswlib 快速入门示例\n";
    std::cout << "数据维度: " << DIM << "\n";
    std::cout << "数据数量: " << NUM_ELEMENTS << "\n";
    std::cout << "查询数量: " << NUM_QUERIES << "\n";
    std::cout << "搜索最近邻数量: " << K << "\n\n";
    
    // 展示fp32接口
    demonstrateFp32();
    
    // 展示fp16接口
    demonstrateFp16();
    
    std::cout << "\n=== 示例完成 ===\n";
    return 0;
}
```
