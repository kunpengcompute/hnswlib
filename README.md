# 项目介绍<a name="ZH-CN_TOPIC_0000002508579577"></a>

本项目旨在基于开源hnswlib，引入FP16数据类型支持，并针对鲲鹏ARM平台进行深度优化。通过数据类型优化和平台特定调优，实现在保持检索精度的前提下，显著提升计算效率并降低内存占用。改造后的hnswlib将充分利用鲲鹏ARM架构的硬件特性，包括通过编译器优化、内存访问优化、以及ARM向量化改造等手段，实现跨平台性能优化。

# 环境部署<a name="ZH-CN_TOPIC_0000002476580618"></a>



## 已验证环境<a name="ZH-CN_TOPIC_0000002476591056"></a>

<a name="table7770454131613"></a>
<table><thead align="left"><tr id="row277155418163"><th class="cellrowborder" valign="top" width="10.97670698790363%" id="mcps1.1.8.1.1"><p id="p14771195491617"><a name="p14771195491617"></a><a name="p14771195491617"></a>算法</p>
</th>
<th class="cellrowborder" valign="top" width="10.92672198340498%" id="mcps1.1.8.1.2"><p id="p9771454151614"><a name="p9771454151614"></a><a name="p9771454151614"></a>包类型</p>
</th>
<th class="cellrowborder" valign="top" width="16.764970508847345%" id="mcps1.1.8.1.3"><p id="p11771155413168"><a name="p11771155413168"></a><a name="p11771155413168"></a>操作系统</p>
</th>
<th class="cellrowborder" valign="top" width="18.47445766270119%" id="mcps1.1.8.1.4"><p id="p1771105413168"><a name="p1771105413168"></a><a name="p1771105413168"></a>CPU类型</p>
</th>
<th class="cellrowborder" valign="top" width="14.285714285714285%" id="mcps1.1.8.1.5"><p id="p67718545169"><a name="p67718545169"></a><a name="p67718545169"></a>内存</p>
</th>
<th class="cellrowborder" valign="top" width="14.285714285714285%" id="mcps1.1.8.1.6"><p id="p57711054121613"><a name="p57711054121613"></a><a name="p57711054121613"></a>编译器</p>
</th>
<th class="cellrowborder" valign="top" width="14.285714285714285%" id="mcps1.1.8.1.7"><p id="p137711854171610"><a name="p137711854171610"></a><a name="p137711854171610"></a>其他</p>
</th>
</tr>
</thead>
<tbody><tr id="row177115418161"><td class="cellrowborder" rowspan="2" valign="top" width="10.97670698790363%" headers="mcps1.1.8.1.1 "><p id="p225222510179"><a name="p225222510179"></a><a name="p225222510179"></a>hnswlib</p>
</td>
<td class="cellrowborder" rowspan="2" valign="top" width="10.92672198340498%" headers="mcps1.1.8.1.2 "><p id="p1997932151717"><a name="p1997932151717"></a><a name="p1997932151717"></a>源码包</p>
</td>
<td class="cellrowborder" valign="top" width="16.764970508847345%" headers="mcps1.1.8.1.3 "><p id="p577185419166"><a name="p577185419166"></a><a name="p577185419166"></a>openEuler 22.03 LTS SP3</p>
</td>
<td class="cellrowborder" valign="top" width="18.47445766270119%" headers="mcps1.1.8.1.4 "><p id="p1178432114216"><a name="p1178432114216"></a><a name="p1178432114216"></a>鲲鹏920 7282C处理器</p>
</td>
<td class="cellrowborder" valign="top" width="14.285714285714285%" headers="mcps1.1.8.1.5 "><p id="p20325717499"><a name="p20325717499"></a><a name="p20325717499"></a>16 * 32G</p>
</td>
<td class="cellrowborder" rowspan="2" valign="top" width="14.285714285714285%" headers="mcps1.1.8.1.6 "><p id="p13225618153916"><a name="p13225618153916"></a><a name="p13225618153916"></a>GCC 12.3.1</p>
<p id="p1451182531810"><a name="p1451182531810"></a><a name="p1451182531810"></a></p>
</td>
<td class="cellrowborder" rowspan="2" valign="top" width="14.285714285714285%" headers="mcps1.1.8.1.7 "><p id="p1916122941819"><a name="p1916122941819"></a><a name="p1916122941819"></a>CMake&gt;=3.22.0</p>
</td>
</tr>
<tr id="row47712054131620"><td class="cellrowborder" valign="top" headers="mcps1.1.8.1.1 "><p id="p108311659483"><a name="p108311659483"></a><a name="p108311659483"></a>openEuler 24.03 LTS SP3</p>
</td>
<td class="cellrowborder" valign="top" headers="mcps1.1.8.1.2 "><p id="p1577110547162"><a name="p1577110547162"></a><a name="p1577110547162"></a>鲲鹏920 7592C处理器</p>
</td>
<td class="cellrowborder" valign="top" headers="mcps1.1.8.1.3 "><p id="p1483114520483"><a name="p1483114520483"></a><a name="p1483114520483"></a>24 * 64G</p>
</td>
</tr>
</tbody>
</table>

## 源码编译<a name="ZH-CN_TOPIC_0000002476751032"></a>

1.  获取开源代码，假设安装路径为“/path/to“。

    ```
    cd /path/to
    git clone https://github.com/nmslib/hnswlib.git
    ```

2.  下载hnswlib的patch文件，假设安装路径为“/path/to“。

    ```
    git clone https://gitcode.com/boostkit/hnswlib.git k_hnswlib
    ```

3.  检查patch是否可应用于开源代码。如果执行下述命令不产生任何报错则可应用patch。

    ```
    cd hnswlib
    git apply --check ../k_hnswlib/0001-boostsra-hnswlib.patch
    ```

4.  应用patch。

    ```
    git apply  ../k_hnswlib/0001-boostsra-hnswlib.patch
    ```

5.  安装GCC 12.3.1与GCC-C++ 12.3.1。

    ```
    yum install gcc-toolset-12-gcc gcc-toolset-12-gcc-c++
    export PATH=/opt/openEuler/gcc-toolset-12/root/usr/bin/:$PATH
    export LD_LIBRARY_PATH=/opt/openEuler/gcc-toolset-12/root/usr/lib64/:$LD_LIBRARY_PATH
    ```

6.  安装OpenMP、CMake、Make。

    ```
    yum install cmake libgomp make
    ```

7.  安装HDF5库。

    ```
    yum install hdf5-devel
    ```

8.  编译源码。假设源码存放于“/path/to/hnswlib“目录下。

    ```
    cd /path/to/hnswlib
    make hnswlib_neon_test
    make hnswlib_sve_test
    make hnswlib_fp16_neon_test
    make hnswlib_fp16_sve_test
    ```

    >![](public_sys-resources/icon-note.gif) **说明：** 
    >编译选项说明：
    >-   hnswlib\_neon\_test：fp32数据类型使用neon向量化指令。
    >-   hnswlib\_sve\_test：fp32数据类型使用sve向量化指令。
    >-   hnswlib\_fp16\_neon\_test：fp16数据类型使用neon向量化指令。
    >-   hnswlib\_fp16\_sve\_test：fp16数据类型使用sve向量化指令。

# 快速上手<a name="ZH-CN_TOPIC_0000002508580607"></a>




## 测试示例<a name="ZH-CN_TOPIC_0000002508714123"></a>

**目录说明<a name="section20169143153012"></a>**

假设程序运行的目录为“"/path/to/hnswlib"“，完整的目录结构应如下所示：

```
├── configs            // 数据集相关配置文件
├── hnswlib            // hnswlib头文件
├── python_bindings    // python接口文件
├── include            // 性能测试文件
├── src                // 性能测试头文件
├── examples           // 功能测试文件
├── tests              // 功能测试文件
├── test               // 测试文件，包含ut测试和fuzz测试
└── ALGO_PARAMS.md
└── CMakeLists.txt     // 功能测试CMakeLists.txt文件
└── LICENSE
└── Makefile           // 性能测试Makefile文件
└── MANIFEST.in
└── pyproject.toml
└── README.md
└── run.sh             // 编译并执行功能测试的脚本
└── test.sh            // 执行性能测试的脚本
└── setup.py
└── TESTING_RECALL.md
```

**性能测试示例<a name="section18521028313"></a>**

下方使用示例以使用fashion-mnist-784-euclidean.hdf5数据集为例。

1.  创建data文件夹，并将数据集加入data文件夹中。

    ```
    cd /path/to/hnswlib
    mkdir data && cd data
    wget http://ann-benchmarks.com/fashion-mnist-784-euclidean.hdf5 --no-check-certificate
    ```

2.  为脚本添加运行权限。

    ```
    cd /path/to/hnswlib
    chmod +x test.sh
    ```

3.  运行测试脚本。

    ```
    ./test.sh hnswlib hnswlib_neon               // fp32数据类型，使用neon指令
    ./test.sh hnswlib hnswlib_sve                // fp32数据类型，使用sve指令
    ./test.sh hnswlib_fp16 hnswlib_fp16_neon     // fp16数据类型，使用neon指令
    ./test.sh hnswlib_fp16 hnswlib_fp16_sve      // fp16数据类型，使用sve指令
    ```

    测试结果存在output文件夹中，关注测试结果的qps指标，并且召回率大于0.99。

    >![](public_sys-resources/icon-note.gif) **说明：** 
    >-   “/path/to/hnswlib/configs“文件夹下包含了不同数据集的配置信息，其中，index\_path是索引保存路径，save\_or\_load是图索引构建或加载选项。
    >-   当save\_or\_load选项为save时表示构图模式，此时会将构建的图索引保存到“/path/to/hnswlib/index\_path“中。
    >-   当save\_or\_load选项为load时表示加载模式，此时会从“/path/to/hnswlib/index\_path“中加载图索引，直接用于后续的搜索。
    >-   首次运行时，在save模式和单NUMA的情况下运行性能测试脚本文件，保存图索引。
    >-   在后续运行中，如果不需要修改k\_f、efs、efc的值，则可以在load模式和整机4numa下运行脚本获取性能数据。
    >-   如果需要修改k\_f、efs、efc的值，则需要在save模式下重新构图。

**性能测试参数调优指导<a name="section3966431183112"></a>**

假设参数调优的配置文件路径为“/path/to/hnswlib/configs“，该文件夹的目录结构如下:

```
├── configs
      ├── hnswlib
      ├── hnswlib_fp16
            └── hnswlib_fp16_fashion-mnist-784-euclidean.config
```

config文件的内容如下所示：

```
# HNSWLIB
k_f = 16
efs = 30
efc = 200
metric = L2
nloop = 3
num_threads = 64
top_k = 10
batch_mode = false
batch_size = 100
save_or_load = save
index_path = indexes/hnsw/fashion.faiss
```

调优的参数如[表1 调优参数表](#table114596415225)所示。

**表 1**  调优参数表

<a name="table114596415225"></a>
<table><thead align="left"><tr id="row4459164110224"><th class="cellrowborder" valign="top" width="12.38123812381238%" id="mcps1.2.4.1.1"><p id="p114598418224"><a name="p114598418224"></a><a name="p114598418224"></a>参数</p>
</th>
<th class="cellrowborder" valign="top" width="26.512651265126514%" id="mcps1.2.4.1.2"><p id="p44598418228"><a name="p44598418228"></a><a name="p44598418228"></a>含义</p>
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
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p245916419226"><a name="p245916419226"></a><a name="p245916419226"></a>较小的值可以加快索引构建速度，但可能会降低索引质量；较大的值可以提高索引的质量，但会增加索引构建时间。</p>
</td>
</tr>
</tbody>
</table>

>![](public_sys-resources/icon-note.gif) **说明：** 
>在**save\_or\_load = save**模式下进行上述参数调优，构建索引时建议使用单NUMA，调优目标为保证召回率大于0.99的情况下，寻找qps最高的参数组合。

## UT测试（可选）<a name="ZH-CN_TOPIC_0000002508594139"></a>

执行下方指令，用于看护编译及运行环境是否正常，算法功能是否正确。

```
cd /path/to/hnswlib/test/ut_test
chmod +x run.sh
sh run.sh
```

## 功能测试（可选）<a name="ZH-CN_TOPIC_0000002476594158"></a>

执行下方指令，用于看护算法功能是否正确。

```
cd /path/to/hnswlib
chmod +x run.sh
sh run.sh
```

# 贡献指南<a name="ZH-CN_TOPIC_0000002508700579"></a>

如果使用过程中有任何问题，或者需要反馈特性需求和bug报告，可以提交issues联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。

# 免责声明<a name="ZH-CN_TOPIC_0000002476740608"></a>

此代码仓计划参与hnswlib开源组件，仅作hnswlib适配ARM平台以及提供FP16接口实现，编码风格遵照原生开源软件，继承原生开源软件安全设计，不破坏原生开源软件设计及编码风格和方式，软件的任何漏洞与安全问题，均由相应的上游社区根据其漏洞和安全响应机制解决。请密切关注上游社区发布的通知和版本更新。鲲鹏计算社区对软件的漏洞及安全问题不承担任何责任。

# 许可证书<a name="ZH-CN_TOPIC_0000002476580620"></a>

hnswlib采用Apache 2.0 License许可证授权，支持修改代码和再开源。

