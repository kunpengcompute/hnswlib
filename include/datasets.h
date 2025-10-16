#ifndef DATASETS_H
#define DATASETS_H

#include <string>
#include <H5Cpp.h>
#include <cstdint>  // 用于int64_t
#include "../hnswlib/hnswlib.h"

class Datasets {
public:
    // 数据集特征
    std::string path;           // 数据集路径
    std::string dataset_name;   // 数据集名称
    size_t d;                   // 向量维度
    size_t nb;                  // 训练集基数
    size_t nq;                  // 查询集数量
    size_t topk;                // 检索的topK值
    
    // 数据指针
    float* xb = nullptr;        // 训练集向量 [nb × d]
    float* xq = nullptr;        // 查询集向量 [nq × d]
    int64_t* gt = nullptr;      // Ground truth索引 [nq × topk]
#if defined(HNSWLIB_USE_ARM)
    float16_t *xb_fp16 = nullptr;  // 新增FP16训练数据
    float16_t *xq_fp16 = nullptr;  // 新增FP16测试数据
#endif

    /**
     * @brief 构造函数
     * @param _p 数据集基础路径
     * @param _name 数据集名称
     * @param _k 需要检索的topK值
     * @param is_ip 是否为内积数据集（需要归一化）
     */
    Datasets(const std::string& _p, const std::string& _name, size_t _k, bool is_ip = false);
    
    ~Datasets();

    /**
     * @brief 对向量进行L2归一化
     * @param vectors 向量数组
     * @param n 向量数量
     * @param d 向量维度
     */
    void normalize(float* vectors, size_t n, size_t d);

private:
    /**
     * @brief 从HDF5读取浮点数组
     * @param file HDF5文件指针
     * @param fname 数据集名称
     * @param n_out 输出向量数量
     * @param d_out 输出向量维度
     * @return 浮点数组指针
     */
    float* hdf5_read_floats(H5::H5File* file, const char* fname, size_t* n_out, size_t* d_out);
    
    /**
     * @brief 从HDF5读取整数数组（支持自动类型转换）
     * @param file HDF5文件指针
     * @param fname 数据集名称
     * @param d_out 输出每行维度
     * @param n_out 输出行数
     * @return int64_t数组指针
     */
    int64_t* hdf5_read_ints(H5::H5File* file, const char* fname, size_t* d_out, size_t* n_out);
#ifdef HNSWLIB_ENABLE_FP16
    void convert_to_fp16();
#endif
};

#endif // DATASETS_H