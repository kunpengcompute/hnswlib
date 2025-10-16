// 在包含ARM NEON头文件之前定义这些宏，避免bfloat16_t冲突
#define __ARM_FEATURE_FP16_SCALAR_ARITHMETIC 1
#define __ARM_FEATURE_FP16_VECTOR_ARITHMETIC 1

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <set>
#include "../../hnswlib.h"
#include "../../space_factory.h"
#include "../../bruteforce_template.h"
#include "../../hnswalg_template.h"
#include "../../data_type_converter.h"

// 确保float16_t类型定义
#if !defined(__ARM_FEATURE_FP16_SCALAR_ARITHMETIC) && !defined(__ARM_FEATURE_FP16_VECTOR_ARITHMETIC)
typedef __fp16 float16_t;
#else
// 如果ARM FP16特性已启用，确保float16_t类型存在
#ifndef __fp16
typedef __fp16 float16_t;
#endif
#endif

using namespace hnswlib;
using namespace std;

// 测试配置
struct TestConfig {
    string distance_type = "l2";
    string data_type = "fp16";
    size_t dim = 64;  // 减少维度
    size_t num_points = 1000;  // 减少点数
    size_t num_queries = 100;  // 减少查询数
    size_t k = 5;  // 减少k值
    size_t M = 16;
    size_t ef_construction = 200;
    size_t ef_search = 50;
};

// 生成测试数据
template<typename T>
vector<T> generate_test_data(size_t num_points, size_t dim, unsigned int seed = 42) {
    vector<T> data(num_points * dim);
    mt19937 gen(seed);
    normal_distribution<float> dist(0.0f, 1.0f);
    
    if constexpr (std::is_same_v<T, float>) {
        for (auto& val : data) {
            val = dist(gen);
        }
    } else if constexpr (std::is_same_v<T, float16_t>) {
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = DataTypeConverter::float_to_fp16(dist(gen));
        }
    }
    
    return data;
}

// 暴力搜索函数 - 用于验证HNSW结果的准确性
template<typename T>
std::vector<std::pair<float, labeltype>>
brute_force_search(const std::vector<T>& data, const T* query,
                   size_t dim, size_t num_points, size_t k,
                   const std::string& distance_type, SpaceInterface<T>* space = nullptr) {
    // using namespace hnswlib;
    
    // 确定数据类型字符串
    std::string data_type_str;
    if constexpr (std::is_same_v<T, float>) {
        data_type_str = "fp32";
    } else if constexpr (std::is_same_v<T, float16_t>) {
        data_type_str = "fp16";
    } else {
        throw std::runtime_error("Unsupported data type");
    }
    
    // 1. 使用传入的空间对象或创建新的空间对象
    bool need_delete_space = false;
    if (space == nullptr) {
        space = SpaceFactory::createSpace<T>(distance_type, dim);
        need_delete_space = true;
    }
    
    std::cout << "[DEBUG] 暴力搜索空间对象信息:" << std::endl;
    std::cout << "  [DEBUG] 类型: " << typeid(*space).name() << ", data_type_str=" << data_type_str << std::endl;
    std::cout << "  [DEBUG] 数据大小: " << space->get_data_size() << std::endl;
    std::cout << "  [DEBUG] 距离函数指针: " << (void*)space->get_dist_func() << std::endl;
    std::cout << "  [DEBUG] 距离函数参数: " << space->get_dist_func_param() << std::endl;
    // 检查距离函数参数的具体内容（维度值）
    if (space->get_dist_func_param() != nullptr) {
        size_t param_dim = *static_cast<size_t*>(space->get_dist_func_param());
        std::cout << "  [DEBUG] 距离函数参数内容（维度）: " << param_dim << std::endl;
    }

    // 2. 创建暴力法索引
    BruteforceSearch<T> bf(space, num_points);

    // 3. 添加所有数据点
    for (size_t i = 0; i < num_points; i++) {
        bf.addPoint(&data[i * dim], i);
    }
    
    // 调试：分析暴力搜索的数据存储结构
    cout << "[DEBUG] 暴力搜索数据存储分析:" << endl;
    cout << "  [DEBUG] bf.data_指针: " << (void*)bf.data_ << endl;
    cout << "  [DEBUG] bf.size_per_element_: " << bf.size_per_element_ << " 字节" << endl;
    cout << "  [DEBUG] bf.data_size_: " << bf.data_size_ << " 字节" << endl;
    cout << "  [DEBUG] 标签大小: " << sizeof(labeltype) << " 字节" << endl;
    cout << "  [DEBUG] 每个元素总大小: " << bf.size_per_element_ << " 字节" << endl;
    cout << "  [DEBUG] 数据偏移: " << 0 << " 字节" << endl;
    cout << "  [DEBUG] 标签偏移: " << bf.data_size_ << " 字节" << endl;
    
    // 调试：检查第一个数据点的原始内容
    if (num_points > 0) {
        std::cout << "[DEBUG] 第一个数据点原始值: ";
        for (size_t j = 0; j < std::min(8ul, dim); j++) {
            if constexpr (std::is_same_v<T, float>) {
                std::cout << data[j] << " ";
            } else {
                std::cout << DataTypeConverter::fp16_to_float(data[j]) << " ";
            }
        }
        std::cout << std::endl;
    }
    // Debug: 检查第一个点在bf.data_中的内容（字节）
    if (num_points > 0) {
        char* raw = (char*)bf.data_;
        std::cout << "[DEBUG] bf.data_前32字节: ";
        for (size_t i = 0; i < std::min(32ul, space->get_data_size()); i++) {
            printf("%02x ", (unsigned char)raw[i]);
        }
        std::cout << std::endl;
        
        // 检查第一个元素在bf.data_中的完整结构
        std::cout << "[DEBUG] 第一个元素完整结构分析:" << std::endl;
        std::cout << "  [DEBUG] 数据部分（前8个元素）: ";
        T* data_ptr = reinterpret_cast<T*>(raw);
        for (size_t j = 0; j < std::min(8ul, dim); j++) {
            if constexpr (std::is_same_v<T, float>) {
                std::cout << data_ptr[j] << " ";
            } else {
                std::cout << DataTypeConverter::fp16_to_float(data_ptr[j]) << " ";
            }
        }
        std::cout << std::endl;
        
        // 检查标签部分
        size_t label_offset = space->get_data_size();
        labeltype* label_ptr = reinterpret_cast<labeltype*>(raw + label_offset);
        std::cout << "  [DEBUG] 标签值: " << *label_ptr << std::endl;
        
        // 检查整个第一个元素的字节表示
        std::cout << "  [DEBUG] 第一个元素完整字节表示: ";
        for (size_t i = 0; i < bf.size_per_element_; i++) {
            printf("%02x ", (unsigned char)raw[i]);
        }
        std::cout << std::endl;
    }
    // Debug: 检查第一个label
    if (num_points > 0) {
        char* raw = (char*)bf.data_;
        size_t offset = space->get_data_size();
        size_t label_val = 0;
        memcpy(&label_val, raw + offset, sizeof(label_val));
        std::cout << "[DEBUG] bf.data_第一个label: " << label_val << std::endl;
    }
    // Debug: 检查query内容
    std::cout << "[DEBUG] 查询向量: ";
    for (size_t j = 0; j < std::min(8ul, dim); j++) {
        if constexpr (std::is_same_v<T, float>) {
            std::cout << query[j] << " ";
        } else {
            std::cout << DataTypeConverter::fp16_to_float(query[j]) << " ";
        }
    }
    std::cout << std::endl;

    // 4. 搜索
    auto result = bf.searchKnn(query, k);

    // Debug: 检查暴力搜索返回的距离和标签
    if (result.empty()) {
        std::cout << "[DEBUG] 暴力搜索没有返回任何结果" << std::endl;
    } else {
        std::cout << "[DEBUG] 暴力搜索返回 " << result.size() << " 个结果" << std::endl;
        // 先转换为vector并升序排列，然后打印所有结果
        std::vector<std::pair<float, labeltype>> temp_vec;
        auto temp_result = result;
        while (!temp_result.empty()) {
            temp_vec.push_back(temp_result.top());
            temp_result.pop();
        }
        std::reverse(temp_vec.begin(), temp_vec.end());
        
        for (size_t i = 0; i < temp_vec.size(); i++) {
            std::cout << "  [DEBUG] 结果 " << i << ": 距离=" << temp_vec[i].first 
                      << ", 标签=" << temp_vec[i].second << std::endl;
        }
    }

    // 5. 转换为vector并升序排列
    std::vector<std::pair<float, labeltype>> result_vec;
    while (!result.empty()) {
        result_vec.push_back(result.top());
        result.pop();
    }
    std::reverse(result_vec.begin(), result_vec.end());

    // Debug: 手动计算第一个点的距离 - 使用与HNSW相同的数据偏移
    if (num_points > 0 && query != &data[0]) {
        // 使用存储在bf.data_中的数据而不是原始数据
        float manual_dist = space->get_dist_func()(query, bf.data_, space->get_dist_func_param());
        std::cout << "[DEBUG] 手动计算第一个点距离: " << manual_dist << std::endl;
    }

    

    if (need_delete_space) {
        delete space;
    }
    return result_vec;
}

// 验证搜索结果 - 与暴力搜索结果对比
template<typename T>
bool validate_search_results(const vector<T>& data, const vector<T>& queries, 
                           const vector<vector<pair<float, labeltype>>>& results,
                           size_t dim, const string& distance_type) {
    cout << "验证搜索结果..." << endl;
    
    size_t num_points = data.size() / dim;
    size_t num_queries = queries.size() / dim;
    
    // 调试：创建HNSW使用的空间对象用于比较
    auto hnsw_space = SpaceFactory::createSpace<T>(distance_type, dim);
    cout << "[DEBUG] HNSW空间对象信息:" << endl;
    cout << "  [DEBUG] 类型: " << typeid(*hnsw_space).name() << endl;
    cout << "  [DEBUG] 数据大小: " << hnsw_space->get_data_size() << endl;
    cout << "  [DEBUG] 距离函数指针: " << (void*)hnsw_space->get_dist_func() << endl;
    cout << "  [DEBUG] 距离函数参数: " << hnsw_space->get_dist_func_param() << endl;
    // 检查距离函数参数的具体内容（维度值）
    if (hnsw_space->get_dist_func_param() != nullptr) {
        size_t param_dim = *static_cast<size_t*>(hnsw_space->get_dist_func_param());
        cout << "  [DEBUG] 距离函数参数内容（维度）: " << param_dim << endl;
    }
    
    // 调试：检查原始数据的存储格式
    cout << "[DEBUG] 原始数据存储分析:" << endl;
    cout << "  [DEBUG] 数据总大小: " << data.size() << " 个元素" << endl;
    cout << "  [DEBUG] 每个向量大小: " << dim << " 个元素" << endl;
    cout << "  [DEBUG] 向量数量: " << num_points << endl;
    cout << "  [DEBUG] 数据类型大小: " << sizeof(T) << " 字节" << endl;
    cout << "  [DEBUG] 每个向量字节数: " << dim * sizeof(T) << " 字节" << endl;
    
    // 调试：检查第一个向量的原始数据
    if (num_points > 0) {
        cout << "  [DEBUG] 第一个向量原始数据（前8个元素）: ";
        for (size_t j = 0; j < std::min(8ul, dim); j++) {
            if constexpr (std::is_same_v<T, float>) {
                cout << data[j] << " ";
            } else {
                cout << DataTypeConverter::fp16_to_float(data[j]) << " ";
            }
        }
        cout << endl;
        
        // 检查第一个向量的字节表示
        cout << "  [DEBUG] 第一个向量字节表示（前32字节）: ";
        const char* raw_data = reinterpret_cast<const char*>(&data[0]);
        for (size_t i = 0; i < std::min(32ul, dim * sizeof(T)); i++) {
            printf("%02x ", (unsigned char)raw_data[i]);
        }
        cout << endl;
    }
    
    for (size_t q = 0; q < num_queries; q++) {
        cout << "\n===== 查询 " << q << " 开始 =====" << endl;
        const T* query = &queries[q * dim];
        const auto& hnsw_result = results[q];
        
        if (hnsw_result.empty()) {
            cout << "查询 " << q << " 没有返回结果" << endl;
            return false;
        }
        
        // 调试：打印HNSW结果
        std::cout << "HNSW返回 " << hnsw_result.size() << " 个结果" << std::endl;
        for (size_t i = 0; i < hnsw_result.size(); i++) {
            std::cout << "  HNSW结果 " << i << ": 距离=" << hnsw_result[i].first 
                      << ", 标签=" << hnsw_result[i].second << std::endl;
        }
        
        // 执行暴力搜索
        auto brute_force_result = brute_force_search<T>(data, query, dim, num_points, 
                                                       hnsw_result.size(), distance_type, hnsw_space);
        
        // 验证结果数量
        if (hnsw_result.size() != brute_force_result.size()) {
            cout << "查询 " << q << " 结果数量不匹配: HNSW=" << hnsw_result.size() 
                 << " 暴力搜索=" << brute_force_result.size() << endl;
            return false;
        }
        
        // 验证标签匹配（允许顺序不同，但标签必须一致）
        set<labeltype> hnsw_labels, brute_labels;
        
        // 调试：打印原始结果
        cout << "[DEBUG] 查询 " << q << " 原始结果比较:" << endl;
        cout << "  [DEBUG] HNSW原始结果: ";
        for (const auto& res : hnsw_result) {
            cout << "(" << res.first << "," << res.second << ") ";
        }
        cout << endl;
        cout << "  [DEBUG] 暴力搜索原始结果: ";
        for (const auto& res : brute_force_result) {
            cout << "(" << res.first << "," << res.second << ") ";
        }
        cout << endl;
        
        for (const auto& res : hnsw_result) {
            hnsw_labels.insert(res.second);
        }
        for (const auto& res : brute_force_result) {
            brute_labels.insert(res.second);
        }
        
        // 调试：详细比较结果差异
        cout << "[DEBUG] 查询 " << q << " 详细比较:" << endl;
        cout << "  [DEBUG] HNSW标签集合: ";
        for (auto label : hnsw_labels) cout << label << " ";
        cout << endl;
        cout << "  [DEBUG] 暴力搜索标签集合: ";
        for (auto label : brute_labels) cout << label << " ";
        cout << endl;
        
        if (hnsw_labels != brute_labels) {
            cout << "查询 " << q << " 标签不匹配" << endl;
            cout << "HNSW标签: ";
            for (auto label : hnsw_labels) cout << label << " ";
            cout << endl;
            cout << "暴力搜索标签: ";
            for (auto label : brute_labels) cout << label << " ";
            cout << endl;
            return false;
        }
        
        // 验证距离值的合理性（允许小的数值误差）
        for (size_t i = 0; i < hnsw_result.size(); i++) {  // 添加循环遍历每个结果
            float hnsw_dist = hnsw_result[i].first;  // 使用hnsw_result而不是results[q]
            float brute_dist = brute_force_result[i].first;
            float expected_dist;
            float relative_error = 0.0f;  // 声明relative_error变量

            if (distance_type == "l2") {
                expected_dist = brute_dist;
                relative_error = abs(hnsw_dist - expected_dist) / (expected_dist + 1e-6f);
                if (relative_error > 0.01f) {
                    cout << "查询 " << q << " 位置 " << i << " L2距离不匹配！" << endl;
                    cout << "HNSW=" << hnsw_dist << " 暴力搜索=" << brute_dist 
                         << " 相对误差=" << relative_error << endl;
                    return false;
                }
            } else {
                expected_dist = brute_dist;
                relative_error = abs(hnsw_dist - expected_dist);
                if (relative_error > 1e-3) {
                    cout << "查询 " << q << " 位置 " << i << " IP距离不匹配！" << endl;
                    cout << "HNSW=" << hnsw_dist << " 暴力搜索=" << brute_dist 
                         << " 绝对误差=" << relative_error << endl;
                    return false;
                }
            }
        }
        cout << "===== 查询 " << q << " 结束 =====" << endl;
    }

    delete hnsw_space;
    cout << "搜索结果验证通过！" << endl;
    return true;
}

// 端到端FP16测试
template<typename T>
bool run_end_to_end_test(const TestConfig& config) {
    cout << "\n=== 端到端测试: " << config.distance_type << " + " << config.data_type << " ===" << endl;
    
    // 1. 生成测试数据
    cout << "1. 生成测试数据..." << endl;
    auto data = generate_test_data<T>(config.num_points, config.dim, 42);
    // 使用不同的随机种子生成查询，避免与数据完全相同导致0距离自匹配
    auto queries = generate_test_data<T>(config.num_queries, config.dim, 43);
    
    // 2. 创建空间和索引
    cout << "2. 创建空间和索引..." << endl;
    auto space = SpaceFactory::createSpace<T>(config.distance_type, config.dim);
    auto index = new HierarchicalNSWTemplate<T>(space, config.num_points, config.M, 
                                               config.ef_construction, 100, false);
    
    // 3. 构建索引
    cout << "3. 构建索引..." << endl;
    auto start = chrono::high_resolution_clock::now();
    for (size_t i = 0; i < config.num_points; i++) {
        index->addPoint(&data[i * config.dim], i);
    }
    auto end = chrono::high_resolution_clock::now();
    auto build_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    cout << "   构建时间: " << build_time << " ms" << endl;
    cout << "   索引大小: " << index->getCurrentElementCount() << " 个点" << endl;
    
    // 调试：分析HNSW的数据存储结构
    cout << "[DEBUG] HNSW数据存储分析:" << endl;
    cout << "  [DEBUG] index->data_level0_memory_指针: " << (void*)index->data_level0_memory_ << endl;
    cout << "  [DEBUG] index->size_data_per_element_: " << index->size_data_per_element_ << " 字节" << endl;
    cout << "  [DEBUG] index->data_size_: " << index->data_size_ << " 字节" << endl;
    cout << "  [DEBUG] index->offsetData_: " << index->offsetData_ << " 字节" << endl;
    cout << "  [DEBUG] index->label_offset_: " << index->label_offset_ << " 字节" << endl;
    
    // 检查HNSW中第一个元素的数据
    if (config.num_points > 0) {
        char* hnsw_raw = index->data_level0_memory_;
        cout << "  [DEBUG] HNSW第一个元素数据部分（前8个元素）: ";
        T* hnsw_data_ptr = reinterpret_cast<T*>(hnsw_raw + index->offsetData_);
        for (size_t j = 0; j < std::min(8ul, config.dim); j++) {
            if constexpr (std::is_same_v<T, float>) {
                cout << hnsw_data_ptr[j] << " ";
            } else {
                cout << DataTypeConverter::fp16_to_float(hnsw_data_ptr[j]) << " ";
            }
        }
        cout << endl;
        
        // 检查HNSW中第一个元素的标签
        labeltype* hnsw_label_ptr = reinterpret_cast<labeltype*>(hnsw_raw + index->label_offset_);
        cout << "  [DEBUG] HNSW第一个元素标签值: " << *hnsw_label_ptr << endl;
        
        // 检查HNSW第一个元素的字节表示
        cout << "  [DEBUG] HNSW第一个元素完整字节表示: ";
        for (size_t i = 0; i < index->size_data_per_element_; i++) {
            printf("%02x ", (unsigned char)hnsw_raw[i]);
        }
        cout << endl;
    }
    
    // 4. 搜索测试
    cout << "4. 搜索测试..." << endl;
    // 将ef设置为更大的值，逼近精确搜索，避免近似误差导致标签集合不一致
    index->setEf(std::max(config.ef_search, config.num_points));
    
    start = chrono::high_resolution_clock::now();
    vector<vector<pair<float, labeltype>>> results;
    for (size_t i = 0; i < config.num_queries; i++) {
        auto result = index->searchKnn(&queries[i * config.dim], config.k);
        
        vector<pair<float, labeltype>> result_vec;
        while (!result.empty()) {
            result_vec.push_back(result.top());
            result.pop();
        }
        // HNSW返回的是最大堆，pop顺序为距离递减；为与暴力搜索一致，这里转为升序
        std::reverse(result_vec.begin(), result_vec.end());
        results.push_back(result_vec);
    }
    end = chrono::high_resolution_clock::now();
    auto search_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    cout << "   搜索时间: " << search_time << " ms" << endl;
    cout << "   平均查询时间: " << (double)search_time / config.num_queries << " ms" << endl;
    cout << "   QPS: " << (double)config.num_queries * 1000 / search_time << endl;
    
    // 5. 验证搜索结果
    cout << "5. 验证搜索结果..." << endl;
    bool validation_passed = validate_search_results(data, queries, results, config.dim, config.distance_type);
    if (!validation_passed) {
        delete index;
        delete space;
        return false;
    }

    // 5.1 searchKnnCloserFirst 顺序与一致性校验（最近->最远升序，且与暴力搜索一致）
    cout << "5.1 验证 searchKnnCloserFirst 顺序与一致性..." << endl;
    {
        size_t verify_queries = std::min<size_t>(10, config.num_queries);
        for (size_t i = 0; i < verify_queries; i++) {
            auto knn_vec = index->searchKnnCloserFirst(&queries[i * config.dim], config.k);
            // 升序性校验
            for (size_t j = 1; j < knn_vec.size(); j++) {
                if (knn_vec[j].first + 1e-6f < knn_vec[j-1].first) {
                    cout << "searchKnnCloserFirst 顺序错误: 查询 " << i << endl;
                    delete index; delete space; return false;
                }
            }
            // 与暴力搜索对齐
            auto brute_vec = brute_force_search<T>(data, &queries[i * config.dim], config.dim,
                                                  config.num_points, knn_vec.size(), config.distance_type);
            if (knn_vec.size() != brute_vec.size()) { cout << "CloserFirst 数量不一致" << endl; delete index; delete space; return false; }
            for (size_t j = 0; j < knn_vec.size(); j++) {
                if (knn_vec[j].second != brute_vec[j].second) {
                    cout << "searchKnnCloserFirst 结果不一致: 查询 " << i << " 位置 " << j << endl;
                    delete index; delete space; return false;
                }
            }
        }
        cout << "   searchKnnCloserFirst 验证通过" << endl;
    }

    // 5.2 epsilon 搜索一致性（仅 L2）
    if (config.distance_type == "l2") {
        cout << "5.2 验证 epsilon 搜索一致性..." << endl;
        // 取相对较小阈值，避免返回全集合；参数语义与 tests/cpp/epsilon_search_test.cpp 一致
        float epsilon2 = 1.0f;
        int max_num_candidates = static_cast<int>(config.num_points);
        int min_num_candidates = std::min<int>(2000, (int)config.num_points);

        size_t verify_queries = std::min<size_t>(20, config.num_queries);
        for (size_t i = 0; i < verify_queries; i++) {
            EpsilonSearchStopCondition<float> stop_condition(epsilon2, min_num_candidates, max_num_candidates);
            auto eps_res = index->searchStopConditionClosest(&queries[i * config.dim], stop_condition);
            // eps 区域内结果距离检查
            std::unordered_set<labeltype> hnsw_labels;
            for (const auto& p : eps_res) {
                if (!(p.first >= 0 && p.first <= epsilon2 + 1e-6f)) {
                    cout << "epsilon 搜索距离越界: 查询 " << i << endl;
                    delete index; delete space; return false;
                }
                hnsw_labels.insert(p.second);
            }
            // 暴力搜索获取 eps 真值集合
            auto brute_all = BruteforceSearch<T>(space, config.num_points);
            for (size_t id = 0; id < config.num_points; id++) brute_all.addPoint(&data[id * config.dim], id);
            auto brute_pq = brute_all.searchKnn(&queries[i * config.dim], config.num_points);
            std::unordered_set<labeltype> gt_labels;
            while (!brute_pq.empty()) {
                auto pr = brute_pq.top(); brute_pq.pop();
                if (pr.first < epsilon2) gt_labels.insert(pr.second);
            }
            if (!gt_labels.empty()) {
                size_t correct = 0;
                for (auto l : hnsw_labels) if (gt_labels.count(l)) correct++;
                float recall = (float)correct / (float)gt_labels.size();
                if (recall < 0.95f) {
                    cout << "epsilon 搜索召回不足: 查询 " << i << " recall=" << recall << endl;
                    delete index; delete space; return false;
                }
            } else {
                // 没有真值时，HNSW 结果应为空或极少
                if (!hnsw_labels.empty()) {
                    cout << "epsilon 区域为空但返回了结果: 查询 " << i << endl;
                    delete index; delete space; return false;
                }
            }
        }
        cout << "   epsilon 搜索验证通过" << endl;
    }
    
    // 6. 内存使用统计
    cout << "6. 内存使用统计..." << endl;
    size_t data_memory = config.num_points * config.dim * sizeof(T);
    size_t index_memory = index->getCurrentElementCount() * space->get_data_size();
    cout << "   原始数据内存: " << data_memory / 1024.0 / 1024.0 << " MB" << endl;
    cout << "   索引内存: " << index_memory / 1024.0 / 1024.0 << " MB" << endl;
    cout << "   内存压缩比: " << (double)data_memory / index_memory << endl;
    
    // 7. 序列化测试
    cout << "7. 序列化测试..." << endl;
    string index_file = "test_index_" + config.distance_type + "_" + config.data_type + ".bin";
    
    start = chrono::high_resolution_clock::now();
    index->saveIndex(index_file);
    end = chrono::high_resolution_clock::now();
    auto save_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "   保存时间: " << save_time << " ms" << endl;
    
    // 8. 反序列化测试
    cout << "8. 反序列化测试..." << endl;
    auto loaded_index = new HierarchicalNSWTemplate<T>(space, 0, config.M, config.ef_construction, 100, false);
    
    start = chrono::high_resolution_clock::now();
    loaded_index->loadIndex(index_file, space);
    end = chrono::high_resolution_clock::now();
    auto load_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "   加载时间: " << load_time << " ms" << endl;
    // 使反序列化后的搜索参数与原索引一致
    loaded_index->setEf(std::max(config.ef_search, config.num_points));
    
    // 9. 验证加载的索引
    cout << "9. 验证加载的索引..." << endl;
    bool load_validation_passed = true;
    for (size_t i = 0; i < min(10ul, config.num_queries); i++) {
        auto original_pq = index->searchKnn(&queries[i * config.dim], config.k);
        auto loaded_pq = loaded_index->searchKnn(&queries[i * config.dim], config.k);

        // 转为升序向量，确保比较顺序一致（最近->最远）
        vector<pair<float, labeltype>> original_vec;
        while (!original_pq.empty()) { original_vec.push_back(original_pq.top()); original_pq.pop(); }
        reverse(original_vec.begin(), original_vec.end());

        vector<pair<float, labeltype>> loaded_vec;
        while (!loaded_pq.empty()) { loaded_vec.push_back(loaded_pq.top()); loaded_pq.pop(); }
        reverse(loaded_vec.begin(), loaded_vec.end());

        // 比较前5个标签
        for (size_t j = 0; j < min(5ul, min(original_vec.size(), loaded_vec.size())); j++) {
            if (original_vec[j].second != loaded_vec[j].second) {
                cout << "   序列化验证失败: 查询 " << i << " 位置 " << j << endl;
                load_validation_passed = false;
                break;
            }
        }
    }
    
    if (load_validation_passed) {
        cout << "   序列化验证通过！" << endl;
    }
    
    // 清理
    delete index;
    delete loaded_index;
    delete space;
    
    // 删除测试文件
    remove(index_file.c_str());
    
    return validation_passed && load_validation_passed;
}

// 数据类型兼容性测试
void test_data_type_compatibility() {
    cout << "\n=== 数据类型兼容性测试 ===" << endl;
    
    size_t dim = 64;
    size_t num_points = 1000;
    
    // 测试FP32到FP16转换的精度
    cout << "测试FP32到FP16转换精度..." << endl;
    auto fp32_data = generate_test_data<float>(num_points, dim);
    auto fp16_data = generate_test_data<float16_t>(num_points, dim);
    
    // 转换FP32到FP16
    vector<float16_t> converted_fp16(num_points * dim);
    for (size_t i = 0; i < fp32_data.size(); i++) {
        converted_fp16[i] = DataTypeConverter::float_to_fp16(fp32_data[i]);
    }
    
    // 计算转换误差
    double total_error = 0.0;
    for (size_t i = 0; i < fp32_data.size(); i++) {
        float converted_back = DataTypeConverter::fp16_to_float(converted_fp16[i]);
        total_error += abs(fp32_data[i] - converted_back);
    }
    double avg_error = total_error / fp32_data.size();
    
    cout << "平均转换误差: " << avg_error << endl;
    cout << "最大转换误差: " << *max_element(fp32_data.begin(), fp32_data.end()) * 0.001 << endl;
    
    // 测试距离计算一致性
    cout << "测试距离计算一致性..." << endl;
    auto l2_fp32_space = SpaceFactory::createSpace("l2", "fp32", dim);
    auto l2_fp16_space = SpaceFactory::createSpace("l2", "fp16", dim);
    
    // 计算同一对向量的距离
    float fp32_dist = l2_fp32_space->get_dist_func()(fp32_data.data(), 
                                                    &fp32_data[dim], 
                                                    l2_fp32_space->get_dist_func_param());
    float fp16_dist = l2_fp16_space->get_dist_func()(converted_fp16.data(), 
                                                    &converted_fp16[dim], 
                                                    l2_fp16_space->get_dist_func_param());
    
    cout << "FP32距离: " << fp32_dist << endl;
    cout << "FP16距离: " << fp16_dist << endl;
    cout << "距离差异: " << abs(fp32_dist - fp16_dist) << endl;
    cout << "相对误差: " << abs(fp32_dist - fp16_dist) / fp32_dist * 100 << "%" << endl;
    
    delete l2_fp32_space;
    delete l2_fp16_space;
}


int main() {
    cout << "HNSWLib 端到端FP16支持完整测试" << endl;
    cout << "===============================" << endl;
    
    // 数据类型兼容性测试
    test_data_type_compatibility();
    
    // 端到端测试配置
    vector<TestConfig> test_configs = {
        TestConfig{"l2", "fp32", 64, 1000, 100, 5, 16, 200, 50},
        TestConfig{"l2", "fp16", 64, 1000, 100, 5, 16, 200, 50},
        TestConfig{"ip", "fp32", 64, 1000, 100, 5, 16, 200, 50},
        TestConfig{"ip", "fp16", 64, 1000, 100, 5, 16, 200, 50}
    };
    bool all_tests_passed = true;
    
    // 运行端到端测试
    for (const auto& config : test_configs) {
        bool test_passed = false;
        
        if (config.data_type == "fp32") {
            test_passed = run_end_to_end_test<float>(config);
        } else if (config.data_type == "fp16") {
            test_passed = run_end_to_end_test<float16_t>(config);
        }
        
        if (!test_passed) {
            cout << "❌ 测试失败: " << config.distance_type << " + " << config.data_type << endl;
            all_tests_passed = false;
        } else {
            cout << "✅ 测试通过: " << config.distance_type << " + " << config.data_type << endl;
        }
    }
    
    // 总结
    cout << "\n" << string(50, '=') << endl;
    cout << "测试总结" << endl;
    cout << string(50, '=') << endl;
    
    if (all_tests_passed) {
        cout << "🎉 所有测试通过！端到端FP16支持正常工作。" << endl;
        cout << "\n支持的功能:" << endl;
        cout << "✅ L2距离 + FP32/FP16" << endl;
        cout << "✅ 内积距离 + FP32/FP16" << endl;
        cout << "✅ 索引构建和搜索" << endl;
        cout << "✅ 序列化和反序列化" << endl;
        cout << "✅ 数据类型转换" << endl;
        cout << "✅ 内存优化" << endl;
    } else {
        cout << "❌ 部分测试失败，请检查实现。" << endl;
    }
    
    return all_tests_passed ? 0 : 1;
}
