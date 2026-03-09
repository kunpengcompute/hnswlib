# hnswlib介绍<a name="ZH-CN_TOPIC_0000002549926519"></a>













## 最新消息<a name="ZH-CN_TOPIC_0000002551970445"></a>

-   \[2026.3.30\]：hnswlib重构版本发布于Gitcode平台，支持fp16数据类型，支持NEON向量化、预取和底库id重排优化，重构后的hnswlib精简了代码结构，提升了易用性。
-   \[2025.12.30\]：hnswlib发布于Gitcode平台，支持fp16数据类型，支持NEON和SVE向量化、预取和底库id重排优化。


## 项目介绍<a name="ZH-CN_TOPIC_0000002551604903"></a>

基于开源hnswlib（Hierarchical Navigable Small World），引入FP16数据类型支持，并针对鲲鹏ARM平台进行深度优化。通过数据类型优化和平台特定调优，实现在保持检索精度的前提下，显著提升计算效率并降低内存占用。改造后的hnswlib将充分利用鲲鹏ARM架构的硬件特性，包括通过编译器优化、内存访问优化、以及ARM向量化改造等手段，实现跨平台性能优化。

hnswlib是一个高效的近似最近邻 （Approximate Nearest Neighbor，ANN）搜索库，特别适用于大规模数据集。它通过构建层次化的图结构，实现了快速的搜索性能。hnswlib支持多种距离度量，如L2（欧氏）距离、IP（内积）距离等，并且可以处理高维数据。

**应用场景<a name="section1268521411013"></a>**

hnswlib适用于推荐系统召回阶段，当数据集的数据类型为FP16时，可使用新增的FP16接口进行图检索，从而优化计算效率与内存占用。


## 目录结构<a name="ZH-CN_TOPIC_0000002551970447"></a>

hnswlib开源仓完整的目录结构如下所示：

```text
├── docs/                                                             // 学习文档
├── public_sys-resources/
├── 0001-hnswlib_0.8.0-optimize-fp32_fp16_dis_idsort_prefetch.patch   // hnswlib源码补丁文件
├── LICENSE                                                           // 项目License文件
└── README.md                                                         // hnswlib开源仓介绍文件
```

使能补丁后的目录结构如下所示：
```text
├── hnswlib/                  // 核心库代码目录
│   ├── bruteforce.h          // 暴力搜索实现
│   ├── hnswalg.h             // HNSW算法核心实现
│   ├── hnswlib.h             // 主头文件，定义核心接口
│   ├── safe_memory.h         // 内存安全操作工具类
│   ├── space_ip.h            // 内积距离空间实现
│   ├── space_l2.h            // L2距离空间实现
│   ├── stop_condition.h      // 停止条件实现
│   └── visited_list_pool.h   // 访问列表池实现
├── examples/                 // 测试代码目录
├── tests/                    // 测试代码目录
├── python_bindings/          // Python接口
├── .gitignore
├── ALGO_PARAMS.md            // HNSW算法参数说明
├── CMakeLists.txt            // CMake构建文件
├── LICENSE
├── Makefile                  // Makefile构建文件
├── MANIFEST.in
├── pyproject.toml            // Python项目配置文件
├── README.md                 // 项目说明文档
├── setup.py                  // Python项目安装脚本
└── TESTING_RECALL.md         // 测试召回率说明
```

## 编译选项

### 启用NEON和FP16支持

在ARM平台上编译时，需要添加以下编译选项以启用NEON和FP16支持：

```bash
-march=armv8.2-a+dotprod+fp16fml -DUSE_NEON
```


## 与原生hnswlib的兼容性

本版本保持了与原生hnswlib API的完全兼容，同时添加了以下新特性：

1. 新增FP16数据类型支持（L2SpacePh、IPSpacePh）
2. ARM平台上的NEON指令优化
3. 底库ID重排优化
4. ARM平台上的预取优化

用户可以无缝迁移现有的hnswlib代码到本版本，并根据需要使用新增的FP16接口和ARM优化特性。

## 版本说明<a name="ZH-CN_TOPIC_0000002521010466"></a>

关于hnswlib算法的版本更新情况请参见[《hnswlib版本说明书》](./docs/release_notes.md)。


## 学习文档<a name="ZH-CN_TOPIC_0000002552050453"></a>

<a name="table1191773710200"></a>
<table><thead align="left"><tr id="row1291816372202"><th class="cellrowborder" valign="top" width="12.04120412041204%" id="mcps1.1.4.1.1"><p id="p291823714205"><a name="p291823714205"></a><a name="p291823714205"></a>学习资源类别</p>
</th>
<th class="cellrowborder" valign="top" width="16.67166716671667%" id="mcps1.1.4.1.2"><p id="p13918183762016"><a name="p13918183762016"></a><a name="p13918183762016"></a>学习资源名称</p>
</th>
<th class="cellrowborder" valign="top" width="71.28712871287128%" id="mcps1.1.4.1.3"><p id="p89181437152019"><a name="p89181437152019"></a><a name="p89181437152019"></a>学习资源简介</p>
</th>
</tr>
</thead>
<tbody><tr id="row179181137112015"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p1918123710208"><a name="p1918123710208"></a><a name="p1918123710208"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p2091893722011"><a name="p2091893722011"></a><a name="p2091893722011"></a><a href="./docs/release_notes.md">版本说明书</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p491893752010"><a name="p491893752010"></a><a name="p491893752010"></a>提供hnswlib每个发布版本的基础信息和特性更新信息。</p>
</td>
</tr>
<tr id="row2918153732017"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p598512211214"><a name="p598512211214"></a><a name="p598512211214"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p17918337172020"><a name="p17918337172020"></a><a name="p17918337172020"></a><a href="./docs/quick_start.md">快速入门</p>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p15918183742018"><a name="p15918183742018"></a><a name="p15918183742018"></a>提供hnswlib部署方法和验证性能与功能的快速入门指导。</p>
</td>
</tr>
<tr id="row1491853716207"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p3615192392120"><a name="p3615192392120"></a><a name="p3615192392120"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p191833772015"><a name="p191833772015"></a><a name="p191833772015"></a><a href="./docs/installation_guide.md">安装指南</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p091873712013"><a name="p091873712013"></a><a name="p091873712013"></a>提供hnswlib源码编译安装方法的详细指导。</p>
</td>
</tr>
<tr id="row109181737112010"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p6213152432115"><a name="p6213152432115"></a><a name="p6213152432115"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p5918537142018"><a name="p5918537142018"></a><a name="p5918537142018"></a><a href="./docs/best_practices.md">最佳实践</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772013"><a name="p991893772013"></a><a name="p991893772013"></a>提供hnswlib的测试方法和性能优化建议。</p>
</td>
</tr>
<tr id="row169181237152012"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p169181237152012"><a name="p169181237152012"></a><a name="p169181237152012"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p19181237162012"><a name="p19181237162012"></a><a name="p19181237162012"></a><a href="./docs/api_reference.md">API参考</p>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772014"><a name="p991893772014"></a><a name="p991893772014"></a>提供hnswlib的API说明和使用示例。</p>
</td>
</tr>
<tr id="row169181237152013"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p169181237152013"><a name="p169181237152013"></a><a name="p169181237152013"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p19181237162013"><a name="p19181237162013"></a><a name="p19181237162013"></a><a href="./docs/feature_introduction.md">特性介绍</p>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772015"><a name="p991893772015"></a><a name="p991893772015"></a>提供hnswlib的优化特性介绍。</p>
</td>
</tr>
</tbody>
</table>

## 免责声明<a name="ZH-CN_TOPIC_0000002520444912"></a>

**致hnswlib使用者**

-   本项目仅供调试和开发之用，使用者需自行承担使用风险，并理解以下内容：
    -   数据处理及删除：用户在使用本工具过程中产生的数据属于用户责任范畴。建议用户在使用完毕后及时删除相关数据，以防信息泄露。
    -   数据保密与传播：使用者了解并同意不得将通过本工具产生的数据随意外发或传播。对于由此产生的信息泄露、数据泄露或其他不良后果，本工具及其开发者概不负责。
    -   用户输入安全性：用户需自行保证输入的命令行的安全性，并承担因输入不当而导致的任何安全风险或损失。对于输入命令行不当所导致的问题，本工具及其开发者概不负责。

-   免责声明范围：本免责声明适用于所有使用本工具的个人或实体。使用本工具即表示您同意并接受本声明的内容，并愿意承担因使用该功能而产生的风险和责任，如有异议请停止使用本工具。
-   在使用本工具之前，请**谨慎阅读并理解以上免责声明的内容**。对于使用本工具所产生的任何问题或疑问，请及时联系开发者。

**致数据所有者**

如果您不希望您的模型或数据集等信息在本项目中被提及，或希望更新本项目中有关的描述，请在GitCode提交issue，我们将根据您的issue要求删除或更新您相关描述。衷心感谢您对本项目的理解和贡献。

## License<a name="ZH-CN_TOPIC_0000002551484917"></a>

hnswlib采用Apache 2.0 License许可证授权，支持修改代码和再开源。详见[LICENSE](LICENSE)文件。

本项目的文档适用CC-BY 4.0许可证，具体请参见[LICENSE](docs/LICENSE)文件。

## 贡献声明<a name="ZH-CN_TOPIC_0000002520604914"></a>

欢迎大家为社区做贡献，如果使用过程中有任何问题/建议，或者需要反馈特性需求和bug报告，可以提交[Issues](https://gitcode.com/boostkit/community/blob/master/docs/contributor/issue-submit.md)联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。同时也欢迎大家在[讨论专区](https://gitcode.com/boostkit/community/discussions)展开讨论交流。感谢您的支持。

## 致谢<a name="ZH-CN_TOPIC_0000002553554263"></a>

hnswlib由华为公司的下列部门联合贡献：

-   鲲鹏计算Boostkit开发部

感谢来自社区的每一个PR，欢迎贡献hnswlib！

