#include "datasets.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <H5Cpp.h>

Datasets::Datasets(const std::string &_p, const std::string &_name, size_t _k, bool is_ip)
    : path(_p + _name),
      topk(_k),
      dataset_name(_name)
{
    char fname[100];
    strcpy(fname, path.c_str());
    H5::H5File file(fname, H5F_ACC_RDONLY);
    char p[100];

    // 读取训练集
    strcpy(p, "train");
    xb = hdf5_read_floats(&file, p, &nb, &d);
    
    // 读取查询集
    size_t d2;
    strcpy(p, "test");
    xq = hdf5_read_floats(&file, p, &nq, &d2);
    
    // 检查维度一致性
    assert(d == d2 && "Train and test vectors dimension mismatch!");

    // 读取ground truth
    size_t k, nq2;
    strcpy(p, "neighbors");
    int64_t* gt_int = hdf5_read_ints(&file, p, &k, &nq2);
    
    // 检查数据集大小一致性
    assert(nq == nq2 && "Query set size mismatch in ground truth");
    assert(k >= topk && "Ground truth has fewer neighbors than required");
    
    // 复制所需topk个邻居
    gt = new int64_t[topk * nq];
    for (size_t i = 0; i < nq; i++) {
        for (size_t j = 0; j < topk; j++) {
            gt[i * topk + j] = gt_int[i * k + j];
        }
    }

    delete[] gt_int;

    // IP数据集需要归一化
    if (is_ip) {
        normalize(xb, nb, d);
        normalize(xq, nq, d);
    }
#ifdef HNSWLIB_ENABLE_FP16
    convert_to_fp16();
#endif
}

Datasets::~Datasets()
{
    delete[] xb;
    delete[] xq;
    delete[] gt;
#if defined(HNSWLIB_USE_ARM)
    delete[] xb_fp16;
    delete[] xq_fp16;
#endif
}

void Datasets::normalize(float *vectors, size_t n, size_t d)
{
    #pragma omp parallel for
    for (size_t i = 0; i < n; i++) {
        float norm = 0.0f;
        for (size_t j = 0; j < d; j++) {
            norm += vectors[i * d + j] * vectors[i * d + j];
        }
        norm = std::sqrt(norm);
        if (norm > 0) {
            for (size_t j = 0; j < d; j++) {
                vectors[i * d + j] /= norm;
            }
        }
    }
}

float *Datasets::hdf5_read_floats(H5::H5File *file, const char *fname, size_t *n_out, size_t *d_out)
{
    try {
        H5::DataSet dataset = file->openDataSet(fname);
        H5::DataSpace dataspace = dataset.getSpace();
        hsize_t dims[2];
        dataspace.getSimpleExtentDims(dims, nullptr);
        *n_out = dims[0];
        *d_out = dims[1];
        float *data = new float[*n_out * *d_out];
        dataset.read(data, H5::PredType::NATIVE_FLOAT);
        return data;
    } catch (const H5::FileIException &e) {
        std::cerr << "File open error (" << fname << "): " << e.getDetailMsg() << std::endl;
        exit(1);
    } catch (const H5::DataSetIException &e) {
        std::cerr << "Dataset open error (" << fname << "): " << e.getDetailMsg() << std::endl;
        exit(1);
    } catch (const H5::DataSpaceIException &e) {
        std::cerr << "Dataspace error (" << fname << "): " << e.getDetailMsg() << std::endl;
        exit(1);
    }
}

int64_t* Datasets::hdf5_read_ints(H5::H5File *file, const char *fname, size_t *d_out, size_t *n_out)
{
    try {
        H5::DataSet dataset = file->openDataSet(fname);
        H5::DataType dtype = dataset.getDataType();
        H5::DataSpace dataspace = dataset.getSpace();
        hsize_t dims[2];
        dataspace.getSimpleExtentDims(dims, nullptr);
        *n_out = dims[0];
        *d_out = dims[1];
        
        int64_t* data = new int64_t[*n_out * *d_out];
        
        // 处理不同整数类型
        if (dtype == H5::PredType::NATIVE_INT32) {
            int32_t* temp = new int32_t[*n_out * *d_out];
            dataset.read(temp, H5::PredType::NATIVE_INT32);
            for (size_t i = 0; i < *n_out * *d_out; i++) {
                data[i] = static_cast<int64_t>(temp[i]);
            }
            delete[] temp;
        } 
        else if (dtype == H5::PredType::NATIVE_UINT32) {
            uint32_t* temp = new uint32_t[*n_out * *d_out];
            dataset.read(temp, H5::PredType::NATIVE_UINT32);
            for (size_t i = 0; i < *n_out * *d_out; i++) {
                data[i] = static_cast<int64_t>(temp[i]);
            }
            delete[] temp;
        }
        else if (dtype == H5::PredType::NATIVE_INT64) {
            dataset.read(data, H5::PredType::NATIVE_INT64);
        }
        else if (dtype == H5::PredType::NATIVE_UINT64) {
            uint64_t* temp = new uint64_t[*n_out * *d_out];
            dataset.read(temp, H5::PredType::NATIVE_UINT64);
            for (size_t i = 0; i < *n_out * *d_out; i++) {
                data[i] = static_cast<int64_t>(temp[i]);
            }
            delete[] temp;
        }
        else {
            std::cerr << "Unsupported integer type in dataset: " << fname 
                      << " (type class: " << dtype.getClass() << ")" << std::endl;
            exit(1);
        }
        return data;
    } catch (const H5::FileIException &e) {
        std::cerr << "File open error (" << fname << "): " << e.getDetailMsg() << std::endl;
        exit(1);
    } catch (const H5::DataSetIException &e) {
        std::cerr << "Dataset open error (" << fname << "): " << e.getDetailMsg() << std::endl;
        exit(1);
    } catch (const H5::DataSpaceIException &e) {
        std::cerr << "Dataspace error (" << fname << "): " << e.getDetailMsg() << std::endl;
        exit(1);
    }
}

#ifdef HNSWLIB_ENABLE_FP16
void Datasets::convert_to_fp16()
{
    xb_fp16 = new float16_t[nb * d];
    xq_fp16 = new float16_t[nq * d];

    for (size_t i = 0; i < nb * d; i++) {
        xb_fp16[i] = static_cast<float16_t>(xb[i]);
    }

    for (size_t i = 0; i < nq * d; i++) {
        xq_fp16[i] = static_cast<float16_t>(xq[i]);
    }
}
#endif