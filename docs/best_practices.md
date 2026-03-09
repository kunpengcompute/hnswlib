# 最佳实践<a name="ZH-CN_TOPIC_0000002521702931"></a>

本节介绍在鲲鹏平台测试优化后的hnswlib的方法。本节依赖鲲鹏的优化补丁文件0001-hnswlib_0.8.0-optimize-fp32_fp16_dis_idsort_prefetch.patch。

**优化后的hnswlib测试示例<a name="section18521028313"></a>**

下方使用示例以使用fashion-mnist-784-euclidean.hdf5数据集为例。

1.  按照[安装指南](installation_guide.md)安装hnswlib。

2.  获取测试程序。假设路径为"/path/to/sra_test"。

    ```bash
    git clone https://gitcode.com/openeuler/sra_test.git
    ```

3.  创建data文件夹，获取测试数据集。
    ```bash
    cd /path/to/sra_test
    mkdir data && cd data
    wget http://ann-benchmarks.com/fashion-mnist-784-euclidean.hdf5 --no-check-certificate
    ```

4.  编译可执行文件。
    ```bash
    make hnswlib_test          // fp32数据类型，使用neon向量化指令。
    make hnswlib_fp16_test     // fp16数据类型，使用neon向量化指令。
    ```
    完整的目录结构如下所示。
    ```text
    ├── configs/                                                  // 存放对应算法、对应数据集配置文件
    │      └── hnswlib_fp16/
    │            └── hnswlib_fp16_fashion-mnist-784-euclidean.config
    ├── data/                                                     // 存放数据集
    │      └── fashion-mnist-784-euclidean.hdf5
    ├── include/                                                  // 存放测试框架对应的头文件
    ├── indexes/
    │      └── hnswlib_fp16/                                       // 存放构建好的索引，需手动创建。
    │            └── fashion.hnswlib                               // 构建好的索引，运行可执行文件hnswlib_fp16_test后（对应数据集配置文件“save_or_load”为save）时生成
    ├── scripts/
    │      └── build.sh                                            // 脚本文件
    ├── src/                                                      // 存放测试框架对应的源文件
    ├── get_result.py
    ├── LICENSE
    ├── Makefile
    ├── README.md
    ├── test.sh                                                   // 执行性能测试的脚本
    ├── test_multi-numas.sh                                       // 执行多numa性能测试的脚本
    ├── hnswlib_fp16_test                                         // 编译后生成的可执行文件
    └── hnswlib_test
    
    ```
    >![](public_sys-resources/icon-note.gif) **说明：** 
    >-   首次编译时，根据命令行提示交互输入hnswlib路径和需要开启的宏，脚本将在build文件夹下自动保存对应的配置信息。后续再次编译时可以直接修改build下的config_hnswlib.sh或config_hnswlib_fp16.sh文件中的配置，再次运行hnswlib_test或hnswlib_fp16_test即可。
    >-   编译hnswlib_test需要开启宏：-DUSE_NEON
    >-   编译hnswlib_fp16_test需要开启宏：-DUSE_NEON -DUSE_FP16


5.  运行测试脚本。

    ```bash
    sh test.sh hnswlib                // fp32数据类型，使用neon向量化指令。
    sh test.sh hnswlib_fp16           // fp16数据类型，使用neon向量化指令。
    ```

    测试结果存在output文件夹中，关注测试结果的qps指标，并且召回率大于0.99。

    >![](public_sys-resources/icon-note.gif) **说明：** 
    >-   "/configs"文件夹下包含了不同数据集的配置信息，其中，index\_path是索引保存路径，save\_or\_load是图索引构建或加载选项。
    >-   当save\_or\_load选项为save时表示构图模式，此时会将构建的图索引保存到"/index\_path"中。
    >-   当save\_or\_load选项为load时表示加载模式，此时会从"/index\_path"中加载图索引，直接用于后续的搜索。
    >-   首次运行时，在save模式和单NUMA的情况下运行性能测试脚本文件，保存图索引。
    >-   在后续运行中，如果不需要修改k\_f、efc的值，则可以在load模式和整机4numa下运行脚本获取性能数据。
    >-   如果需要修改k\_f、efc的值，则需要在save模式下重新构图。

**性能优化建议**

1. **选择合适的数据类型**：在精度要求不高的场景下，优先使用FP16以减少内存占用和提升性能。
2. **调整参数**：调整M、efc、efs参数，详见[性能测试参数调优指导](#section3966431183112)。
3. **批处理查询**：尽量批量处理查询，减少线程创建开销。

**性能测试参数调优指导<a name="section3966431183112"></a>**

假设参数调优的配置文件路径为“/path/to/sra_test/configs“，该文件夹的目录结构如下:

```text
├── configs/
      ├── hnswlib/
      ├── hnswlib_fp16/
            └── hnswlib_fp16_fashion-mnist-784-euclidean.config
```

config文件的内容如下所示：

```text
# HNSWLIB
k_f = 16
efs = 30
efc = 200
metric = L2
nloop = 3
num_threads = 80
top_k = 10
batch_mode = false
batch_size = 100
save_or_load = save
index_path = indexes/hnswlib_fp16/fashion.hnswlib
```

调优的参数如[**表 1** 调优参数表](#调优参数表)所示。

**表 1** 调优参数表<a id="调优参数表"></a>

<a name="table114596415225"></a>
<table><thead align="left"><tr id="row4459164110224"><th class="cellrowborder" valign="top" width="12.38123812381238%" id="mcps1.2.4.1.1"><p id="p114598418224"><a name="p114598418224"></a><a name="p114598418224"></a>参数名称</p>
</th>
<th class="cellrowborder" valign="top" width="26.512651265126514%" id="mcps1.2.4.1.2"><p id="p44598418228"><a name="p44598418228"></a><a name="p44598418228"></a>参数含义</p>
</th>
<th class="cellrowborder" valign="top" width="61.106110611061105%" id="mcps1.2.4.1.3"><p id="p24598411224"><a name="p24598411224"></a><a name="p24598411224"></a>调优建议</p>
</th>
</tr>
</thead>
<tbody><tr id="row1645974112221"><td class="cellrowborder" valign="top" width="12.38123812381238%" headers="mcps1.2.4.1.1 "><p id="p184591741102210"><a name="p184591741102210"></a><a name="p184591741102210"></a>k_f</p>
</td>
<td class="cellrowborder" valign="top" width="26.512651265126514%" headers="mcps1.2.4.1.2 "><p id="p124591241112214"><a name="p124591241112214"></a><a name="p124591241112214"></a>每个节点在HNSW图中的最大连接数。</p>
</td>
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p3459341122210"><a name="p3459341122210"></a><a name="p3459341122210"></a>较小的值可以减少索引的大小，但可能会降低搜索精度；较大的值可以提高搜索精度，但会增加索引的大小和构建时间。</p>
</td>
</tr>
<tr id="row1645912412221"><td class="cellrowborder" valign="top" width="12.38123812381238%" headers="mcps1.2.4.1.1 "><p id="p13459134182213"><a name="p13459134182213"></a><a name="p13459134182213"></a>efs</p>
</td>
<td class="cellrowborder" valign="top" width="26.512651265126514%" headers="mcps1.2.4.1.2 "><p id="p7459341192219"><a name="p7459341192219"></a><a name="p7459341192219"></a>在搜索时，每个节点的候选邻居数量。</p>
</td>
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p15459941202217"><a name="p15459941202217"></a><a name="p15459941202217"></a>较小的值可以加快搜索速度，但可能会降低搜索精度；较大的值可以提高搜索精度，但会增加搜索时间。</p>
</td>
</tr>
<tr id="row11459741202211"><td class="cellrowborder" valign="top" width="12.38123812381238%" headers="mcps1.2.4.1.1 "><p id="p154591441132214"><a name="p154591441132214"></a><a name="p154591441132214"></a>efc</p>
</td>
<td class="cellrowborder" valign="top" width="26.512651265126514%" headers="mcps1.2.4.1.2 "><p id="p114591741162217"><a name="p114591741162217"></a><a name="p114591741162217"></a>在构建索引时，每个节点的候选邻居数量。</p>
</td>
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p245916419226"><a name="p245916419226"></a><a name="p245916419226"></a>较小的值可以加快索引构建速度，但可能会降低索引质量；较大的值可以提高索引的质量，但会增加索引构建时间。构建时使用较大值（如200-800）以获得更好的索引质量。</p>
</td>
</tr>
</tbody>
</table>

>![](public_sys-resources/icon-note.gif) **说明：** 
>在**save\_or\_load = save**模式下进行上述参数调优，构建索引时建议使用单NUMA，调优目标为保证召回率大于0.99的情况下，寻找qps最高的参数组合。

**（可选）UT测试<a name="section136316366272"></a>**

执行下方指令，用于看护编译及运行环境是否正常，算法功能是否正确。

1.  创建UT测试运行脚本。
    ```bash
    cd /path/to/hnswlib
    vim run.sh
    ```
    run.sh脚本内容如下：
    ```bash
    rm -rf build
    mkdir build
    cd build
    cmake ..
    make

    ./distance_calculation_neon_test
    ```
    粘贴上述内容至脚本后，按ESC，输入":wq"后回车，保存文件并退出。

2.  运行UT测试。
    ```bash
    sh run.sh
    ```
