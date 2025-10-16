#include <iostream>
#include <memory>
#include <pthread.h>
#include <unistd.h>
#include "omp.h"
#include "config_parser.h"
#include "datasets.h"
#include "test_result.h"
#include "index_base.h"
#ifdef HNSWLIB_TEST
#include "hnswlib_index.h"
#elif defined(HNSWLIB_FP16_TEST)
#include "hnswlib_index.h"
#endif

using namespace std;

/* 全局同步变量 */
pthread_mutex_t mtx;
pthread_cond_t cond;
int ready = 0;

/* 获取CPU列表，线程绑核使用 */
std::vector<int> getAvailableCPUs() {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(getpid(), sizeof(cpu_set_t), &mask) == -1) {
        perror("sched_getaffinity");
        return {};
    }

    std::vector<int> cpuList;
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, &mask)) {
            cpuList.push_back(i);
        }
    }

    /* 打印可用的 CPU 列表 */
    std::cout << "Available CPUs (numactl -C restricted): ";
    for (int cpu : cpuList) std::cout << cpu << " ";
    std::cout << std::endl;

    return cpuList;
}

/* 统一线程参数 */
struct UnifiedSearchParams {
    IndexBase* index;
    const Datasets* data;
    int n;                    /* 查询数量 */
#ifdef HNSWLIB_ENABLE_FP16
    const float16_t* x; 
#else
    const float* x;           /* 查询数据指针 */
#endif
    int k;                    /* topK值 */
#ifdef HNSWLIB_ENABLE_FP16
    float16_t *D;
#else
    float* D;                 /* 距离结果存储 */
#endif
    int64_t* I;               /* 索引结果存储 */
    int dim;                  /* 向量维度 */
	timespec thread_start;    /* 线程实际开始时间 */
	double thread_time;       /* 线程实际耗时 */
};

/* 统一线程函数 */
void* search_single_thread(void* arg) {
    omp_set_num_threads(1);
    UnifiedSearchParams* params = static_cast<UnifiedSearchParams*>(arg);
    pthread_mutex_lock(&mtx);
    while (ready == 0) pthread_cond_wait(&cond, &mtx);
    pthread_mutex_unlock(&mtx);

	clock_gettime(CLOCK_MONOTONIC, &params->thread_start);

    for (int i = 0; i < params->n; i++) {
        params->index->search_single(
            params->x + i * params->dim,
            params->k,
            params->D + i * params->k,
            params->I + i * params->k
        );
    }
    timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    params->thread_time = (end.tv_sec - params->thread_start.tv_sec) 
                        + 1e-9 * (end.tv_nsec - params->thread_start.tv_nsec);
    return nullptr;
}

/* 统一的测试函数（并发单query） */
void unified_accu_qps(
    IndexBase* index,
    const Datasets* data,
    TestResult* tr,
    const int nloop,
    const int num_threads
) {
    /* 构建索引 */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    index->build(data);
    index->prepare_search();
    clock_gettime(CLOCK_MONOTONIC, &end);
    double timeElapsed = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
    tr->build_time = timeElapsed;
    cout << "Index build time: " << timeElapsed << " s" << endl;

    /* 准备结果存储 - 每个线程有自己的结果缓冲区 */
#ifdef HNSWLIB_ENABLE_FP16
    std::vector<float16_t*> D_vec(num_threads);
    std::vector<int64_t*> I_vec(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        D_vec[i] = new float16_t[data->topk * data->nq];
        I_vec[i] = new int64_t[data->topk * data->nq];
    }
#else
    std::vector<float*> D_vec(num_threads);
    std::vector<int64_t*> I_vec(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        D_vec[i] = new float[data->topk * data->nq];
        I_vec[i] = new int64_t[data->topk * data->nq];
    }
#endif

    /* 多轮测试 */
    for (int iter = 0; iter < nloop; iter++) {
        pthread_mutex_init(&mtx, nullptr);
        pthread_cond_init(&cond, nullptr);
        ready = 0;

        std::vector<pthread_t> threads(num_threads);
        std::vector<UnifiedSearchParams> params(num_threads);
        std::vector<int> cpu_ids = getAvailableCPUs();

        /* 创建线程 */
        for (int i = 0; i < num_threads; ++i) {
            params[i] = {
                index,
                data,
                static_cast<int>(data->nq),  /* 每个线程处理全部查询 */
#ifdef HNSWLIB_ENABLE_FP16
                data->xq_fp16,
#else
                data->xq,                    /* 全部查询数据的起始位置 */
#endif
                static_cast<int>(data->topk),
                D_vec[i],                    /* 线程私有的结果缓冲区 */
                I_vec[i],
                static_cast<int>(data->d),
				{},
				0.0
            };
            pthread_create(&threads[i], nullptr, search_single_thread, &params[i]);

            /* 线程绑核 */
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(cpu_ids[i], &cpuset);
            pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset);
        }

        /* 同步启动所有线程 */
        sleep(1); /* 确保所有线程已启动 */
        pthread_mutex_lock(&mtx);
        ready = 1;
        pthread_cond_broadcast(&cond);
        clock_gettime(CLOCK_MONOTONIC, &start);
        pthread_mutex_unlock(&mtx);

        /* 等待线程完成 */
        for (int i = 0; i < num_threads; i++) pthread_join(threads[i], nullptr);
        clock_gettime(CLOCK_MONOTONIC, &end);

        /* 记录结果 */
        double elapsed_total = (end.tv_sec - start.tv_sec) 
                             + 1e-9 * (end.tv_nsec - start.tv_nsec);
        double avg_thread_time = 0.0;
        double max_thread_time = 0.0;
        double min_thread_time = std::numeric_limits<double>::max();
        
        for (int i = 0; i < num_threads; i++) {
            avg_thread_time += params[i].thread_time;
            if (params[i].thread_time > max_thread_time) 
                max_thread_time = params[i].thread_time;
            if (params[i].thread_time < min_thread_time) 
                min_thread_time = params[i].thread_time;
        }
        avg_thread_time /= num_threads;
        
        // 计算两种QPS
        double qps_total = data->nq / elapsed_total;
        double qps_avg = data->nq / avg_thread_time;

        /* 记录结果 */
        tr->search_time.push_back(elapsed_total);
        tr->total_time += elapsed_total;
        cout << "Loop " << iter + 1 << "/" << nloop << ":\n"
             << "  Total search time = " << elapsed_total << " s\n"
             << "  Thread time stats: min=" << min_thread_time << " s, "
             << "avg=" << avg_thread_time << " s, "
             << "max=" << max_thread_time << " s\n"
             << "  QPS (based on total time) = " << qps_total << "\n"
             << "  QPS (based on thread avg) = " << qps_avg << endl;
        
        /* 清理同步变量 */
        pthread_mutex_destroy(&mtx);
        pthread_cond_destroy(&cond);
    }

    /* 计算召回率 */
    int n_10 = 0;
    for (int iq = 0; iq < data->nq; iq++) {
        for (int i = 0; i < data->topk; i++) {
            for (int j = 0; j < data->topk; j++) {
                if (I_vec[0][iq * data->topk + j] == data->gt[iq * data->topk + i]) {
                    n_10++;
                }
            }
        }
    }
    tr->recall = n_10 / static_cast<float>(data->nq) / data->topk;
    tr->calculate_quantity = data->nq;

    /* 清理资源 */
    for (int i = 0; i < num_threads; ++i) {
        delete[] D_vec[i];
        delete[] I_vec[i];
    }
}

// 统一的测试函数（batch）
void unified_accu_qps_batch(
    IndexBase* index,
    const Datasets* data,
    TestResult* tr,
    const int nloop,
    const int num_threads,
    int batch_size
) {
    // 构建索引
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    index->build(data);
    index->prepare_search();
    clock_gettime(CLOCK_MONOTONIC, &end);
    double timeElapsed = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
    tr->build_time = timeElapsed;
    cout << "Index build time: " << timeElapsed << " s" << endl;

    // 准备结果存储 - 每个线程有自己的结果缓冲区
#ifdef HNSWLIB_ENABLE_FP16
    float16_t *D = new float16_t[data->topk * data->nq];
#else
    float *D = new float[data->topk * data->nq];
#endif
    int64_t *I = new int64_t[data->topk * data->nq];

    int nq = data->nq;
    int batch_iters = (nq + batch_size - 1) / batch_size;

    // 多轮测试
    for (int iter = 0; iter < nloop; iter++) {
        double single_loop_time = 0;
        for (int j = 0, st = 0; j < batch_iters; ++j, st += batch_size) {
            int en = min(st + batch_size, nq);
            int batch_nq = en - st;
            clock_gettime(CLOCK_MONOTONIC, &start);
#ifdef HNSWLIB_ENABLE_FP16
            index->search_batch(batch_nq, data->xq_fp16 + st * data->d, data->topk, D + st * data->topk, I + st * data->topk);
#else
            index->search_batch(batch_nq, data->xq + st * data->d, data->topk, D + st * data->topk, I + st * data->topk);
#endif
            clock_gettime(CLOCK_MONOTONIC, &end);
            timeElapsed = (end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
            single_loop_time += timeElapsed;
        }
        tr->search_time.push_back(single_loop_time);
        tr->total_time += single_loop_time;
        cout << "Loop " << iter + 1 << "/" << nloop << ": "
             << "Search time = " << single_loop_time << " s, "
             << "QPS = " << data->nq / single_loop_time << endl;
    }
    tr->calculate_quantity = data->nq;

    // 计算召回率
    int n_10 = 0;
    for (int iq = 0; iq < data->nq; iq++) {
        for (int i = 0; i < data->topk; i++) {
            for (int j = 0; j < data->topk; j++) {
                if (I[iq * data->topk + j] == data->gt[iq * data->topk + i]) {
                    n_10++;
                }
            }
        }
    }
    tr->recall = n_10 / static_cast<float>(data->nq) / data->topk;
    tr->calculate_quantity = data->nq;

    // 清理资源
    delete[] D;
    delete[] I;
}

// 测试函数
std::pair<double, double> test(
    const std::string &dataset_path, 
    const std::string &dataset_name, 
    IndexBase* index,
    const ConfigParser &config
) {
    TestResult tr;
    std::string metric_str = config.get_string("metric", "L2");
    bool is_euclidean = (metric_str != "L2");
    int nloop = config.get_int("nloop", 5);
    int num_threads = config.get_int("num_threads", 80);
    int top_k = config.get_int("top_k", 10);
    bool batch_mode = config.get_bool("batch_mode", false);

    Datasets data(dataset_path, dataset_name, top_k, is_euclidean);
    printf("base vector size: %zu\n", data.nb);
    printf("query vector size: %zu\n", data.nq);

    if (!batch_mode) { 
        unified_accu_qps(index, &data, &tr, nloop, num_threads);
    } else { 
        int batch_size = config.get_int("batch_size", 100);
        unified_accu_qps_batch(index, &data, &tr, nloop, num_threads, batch_size);
    }

    cout << "==================================================" << endl;
    int begin_id = nloop / 4;      // 跳过前1/4的循环
    int end_id = nloop * 3 / 4;    // 取中间50%的循环（从1/4到3/4）
    tr.reorder(); // 对搜索时间进行排序并计算累积时间
    double qps = tr.calculate_quantity / tr.get_total(begin_id, end_id) * (end_id - begin_id);
    cout << "Final Results: qps " << qps << " recall " << tr.recall << endl;
    return std::make_pair(qps, tr.recall);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <algorithm> <dataset> [config_dir=config]\n";
        cerr << "Example: " << argv[0] << " kbest sift-128-euclidean \n";
        return 1;
    }
    
    std::string algorithm = argv[1];
    std::string dataset = argv[2];
    std::string config_dir = (argc > 3) ? argv[3] : "configs";
    
    // 数据集路径和完整名称
    std::string dataset_path = "data/";
    std::string full_dataset_name = dataset + ".hdf5";
    
    // 加载配置文件
    std::string config_file = config_dir + "/" + algorithm + "/" + algorithm + "_" + dataset + ".config";
    ConfigParser config(config_file);
    config.print_config();
    
    // 创建索引并测试
    std::unique_ptr<IndexBase> index;
#ifdef HNSWLIB_TEST
    if (algorithm == "hnswlib") {
        index = std::make_unique<HNSWLIBIndex>(config);
    } else {
        cerr << "Unknown algorithm: " << algorithm << endl;
        return 1;
    }
#elif defined(HNSWLIB_FP16_TEST)
    if (algorithm == "hnswlib_fp16") {
        index = std::make_unique<HNSWLIBIndex>(config);
    } else {
        cerr << "Unknown algorithm: " << algorithm << endl;
        return 1;
    }
#endif
    // 运行测试
    test(dataset_path, full_dataset_name, index.get(), config);
    return 0;
}