# hnswlib介绍<a name="ZH-CN_TOPIC_0000002549926519"></a>













## 最新消息<a name="ZH-CN_TOPIC_0000002551970445"></a>

-   \[2025.12.30\]：hnswlib发布于Gitcode平台，支持fp16数据类型，支持NEON和SVE向量化、预取和底库id重排优化。
-   \[2026.3.30\]：hnswlib重构版本发布于Gitcode平台，支持fp16数据类型，支持NEON向量化、预取和底库id重排优化，重构后的hnswlib精简了代码结构，提升了易用性。


## 项目介绍<a name="ZH-CN_TOPIC_0000002551604903"></a>

基于开源hnswlib（Hierarchical Navigable Small World），引入FP16数据类型支持，并针对鲲鹏ARM平台进行深度优化。通过数据类型优化和平台特定调优，实现在保持检索精度的前提下，显著提升计算效率并降低内存占用。改造后的hnswlib将充分利用鲲鹏ARM架构的硬件特性，包括通过编译器优化、内存访问优化、以及ARM向量化改造等手段，实现跨平台性能优化。

hnswlib（Hierarchical Navigable Small World）是一个高效的近似最近邻 （Approximate Nearest Neighbor，ANN）搜索库，特别适用于大规模数据集。它通过构建层次化的图结构，实现了快速的搜索性能。hnswlib支持多种距离度量，如L2（欧氏）距离、IP（内积）距离等，并且可以处理高维数据。该库广泛应用于推荐系统、图像检索、自然语言处理等领域。

**应用场景<a name="section1268521411013"></a>**

hnswlib适用于推荐系统召回阶段，当数据集的数据类型为FP16时，可使用新增的FP16接口进行图检索，从而优化计算效率与内存占用。

**算法原理<a name="section975104591010"></a>**

hnswlib的核心原理是基于分层可导航小世界（HNSW）图结构来组织高维空间中的数据点，从而在近似最近邻搜索中实现查询速度与精度之间的高效平衡。

-   该算法通过构建一个多层图结构来工作，其中底层图包含数据集中所有节点，而越高层数则包含越少的节点，这些高层节点间形成的“长连接”充当了快速导航的“高速公路”。
-   当需要插入一个新节点时，算法会通过一个指数衰减的概率分布随机决定该节点出现的最高层数，然后从该层开始向下，逐层寻找该节点的最近邻并建立连接。
-   关键参数M控制了每个节点在各层中的最大连接数，直接影响图的稠密度和搜索质量。在进行最近邻搜索时，查询过程从最高层入口点开始，利用长连接快速定位到目标区域，然后逐层向下细化搜索，通过贪婪遍历逼近最近邻节点。
-   参数ef则用于动态控制搜索过程中维护的候选队列大小，其值越大通常搜索精度越高但耗时越长。这种分层结构有效减少了搜索时需要访问的节点数量，使得hnswlib在面对大规模高维数据时，相比传统精确搜索方法能显著提升效率，同时保持较高的召回率。

FP16数据类型将每个向量分量的存储空间从FP32的4字节减少到2字节。这使得在构建HNSW索引时，所需的内存空间大幅降低，从而允许在有限的硬件资源下处理更大规模的向量数据集。同时，针对鲲鹏ARM架构，对hnswlib的距离计算算法进行优化，提升了图检索过程中的节点距离比较效率。


## 目录结构<a name="ZH-CN_TOPIC_0000002551970447"></a>

hnswlib开源仓完整的目录结构如下所示：

```text
├── docs/                                                             // 学习文档
├── public_sys-resources/
├── 0001-hnswlib_0.8.0-optimize-fp32_fp16_dis_idsort_prefetch.patch   // hnswlib源码补丁文件
├── LICENSE                                                           // 项目License文件
└── README.md                                                         // hnswlib开源仓介绍文件
```

## 版本说明<a name="ZH-CN_TOPIC_0000002521010466"></a>

hnswlib基于开源hnswlib  **V0.8.0**版本进行开发。

## 兼容性信息<a name="ZH-CN_TOPIC_0000002520850478"></a>

hnswlib已验证环境如下：

<a name="table7770454131613"></a>
<table><thead align="left"><tr id="row277155418163"><th class="cellrowborder" valign="top" width="21.469999999999995%" id="mcps1.1.6.1.1"><p id="p11771155413168"><a name="p11771155413168"></a><a name="p11771155413168"></a>操作系统</p>
</th>
<th class="cellrowborder" valign="top" width="23.659999999999997%" id="mcps1.1.6.1.2"><p id="p1771105413168"><a name="p1771105413168"></a><a name="p1771105413168"></a>CPU类型</p>
</th>
<th class="cellrowborder" valign="top" width="18.29%" id="mcps1.1.6.1.3"><p id="p67718545169"><a name="p67718545169"></a><a name="p67718545169"></a>内存</p>
</th>
<th class="cellrowborder" valign="top" width="18.29%" id="mcps1.1.6.1.4"><p id="p57711054121613"><a name="p57711054121613"></a><a name="p57711054121613"></a>编译器</p>
</th>
<th class="cellrowborder" valign="top" width="18.29%" id="mcps1.1.6.1.5"><p id="p137711854171610"><a name="p137711854171610"></a><a name="p137711854171610"></a>其他</p>
</th>
</tr>
</thead>
<tbody><tr id="row177115418161"><td class="cellrowborder" valign="top" width="21.469999999999995%" headers="mcps1.1.6.1.1 "><p id="p577185419166"><a name="p577185419166"></a><a name="p577185419166"></a>openEuler 22.03 LTS SP3</p>
</td>
<td class="cellrowborder" valign="top" width="23.659999999999997%" headers="mcps1.1.6.1.2 "><p id="p1178432114216"><a name="p1178432114216"></a><a name="p1178432114216"></a>鲲鹏920 7282C处理器</p>
</td>
<td class="cellrowborder" valign="top" width="18.29%" headers="mcps1.1.6.1.3 "><p id="p20325717499"><a name="p20325717499"></a><a name="p20325717499"></a>16*32G</p>
</td>
<td class="cellrowborder" valign="top" width="18.29%" headers="mcps1.1.6.1.4 "><p id="p13225618153916"><a name="p13225618153916"></a><a name="p13225618153916"></a>GCC 12.3.1</p>
</td>
<td class="cellrowborder" valign="top" width="18.29%" headers="mcps1.1.6.1.5 "><p id="p1916122941819"><a name="p1916122941819"></a><a name="p1916122941819"></a>CMake&gt;=3.22.0</p>
</td>
</tr>
<tr id="row2373153174019"><td class="cellrowborder" valign="top" width="21.469999999999995%" headers="mcps1.1.6.1.1 "><p id="p108311659483"><a name="p108311659483"></a><a name="p108311659483"></a>openEuler 24.03 LTS SP3</p>
</td>
<td class="cellrowborder" valign="top" width="23.659999999999997%" headers="mcps1.1.6.1.2 "><p id="p1577110547162"><a name="p1577110547162"></a><a name="p1577110547162"></a>鲲鹏950 7592C处理器</p>
</td>
<td class="cellrowborder" valign="top" width="18.29%" headers="mcps1.1.6.1.3 "><p id="p1483114520483"><a name="p1483114520483"></a><a name="p1483114520483"></a>24*64G</p>
</td>
<td class="cellrowborder" valign="top" width="18.29%" headers="mcps1.1.6.1.4 "><p id="p122401813238"><a name="p122401813238"></a><a name="p122401813238"></a>GCC 12.3.1</p>
</td>
<td class="cellrowborder" valign="top" width="18.29%" headers="mcps1.1.6.1.5 "><p id="p12409819232"><a name="p12409819232"></a><a name="p12409819232"></a>CMake&gt;=3.22.0</p>
</td>
</tr>
</tbody>
</table>

## 环境部署<a name="ZH-CN_TOPIC_0000002520604912"></a>

请先获取hnswlib源码、源码Patch补丁，并安装相关依赖工具，应用补丁文件，然后重新编译hnswlib。详细操作请参见《hnswlib安装指南》

## 快速入门<a name="ZH-CN_TOPIC_0000002551604905"></a>

鲲鹏优化版hnswlib核心逻辑与原生一致，但强化了ARM架构下的性能，入门流程可复用核心逻辑，重点关注鲲鹏专属调优。详细操作请参见《hnswlib快速入门》

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
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p2091893722011"><a name="p2091893722011"></a><a name="p2091893722011"></a>版本说明书</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p491893752010"><a name="p491893752010"></a><a name="p491893752010"></a>提供hnswlib每个发布版本的基础信息和特性更新信息。</p>
</td>
</tr>
<tr id="row2918153732017"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p598512211214"><a name="p598512211214"></a><a name="p598512211214"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p17918337172020"><a name="p17918337172020"></a><a name="p17918337172020"></a>快速入门</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p15918183742018"><a name="p15918183742018"></a><a name="p15918183742018"></a>提供hnswlib部署方法和验证性能与功能的快速入门指导。</p>
</td>
</tr>
<tr id="row1491853716207"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p3615192392120"><a name="p3615192392120"></a><a name="p3615192392120"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p191833772015"><a name="p191833772015"></a><a name="p191833772015"></a>安装指南</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p091873712013"><a name="p091873712013"></a><a name="p091873712013"></a>提供hnswlib源码编译安装方法的详细指导。</p>
</td>
</tr>
<tr id="row109181737112010"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p6213152432115"><a name="p6213152432115"></a><a name="p6213152432115"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p5918537142018"><a name="p5918537142018"></a><a name="p5918537142018"></a>最佳实践</p>
</td>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772013"><a name="p991893772013"></a><a name="p991893772013"></a>提供hnswlib的使用实践案例。</p>
</td>
</tr>
<tr id="row169181237152012"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p169181237152012"><a name="p169181237152012"></a><a name="p169181237152012"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p19181237162012"><a name="p19181237162012"></a><a name="p19181237162012"></a>API参考</p>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772014"><a name="p991893772014"></a><a name="p991893772014"></a>提供hnswlib的API接口定义和接口说明。</p>
</td>
</tr>
<tr id="row169181237152013"><td class="cellrowborder" valign="top" width="12.04120412041204%" headers="mcps1.1.4.1.1 "><p id="p169181237152013"><a name="p169181237152013"></a><a name="p169181237152013"></a>文档</p>
</td>
<td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p19181237162013"><a name="p19181237162013"></a><a name="p19181237162013"></a>特性介绍</p>
<td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772015"><a name="p991893772015"></a><a name="p991893772015"></a>提供hnswlib的优化特性介绍。</p>
</td>
</tr>
</tbody>
</table>

## 免责声明<a name="ZH-CN_TOPIC_0000002520444912"></a>

此代码仓计划参与hnswlib开源组件，仅作hnswlib适配ARM平台以及提供FP16接口实现，编码风格遵照原生开源软件，继承原生开源软件安全设计，不破坏原生开源软件设计及编码风格和方式，软件的任何漏洞与安全问题，均由相应的上游社区根据其漏洞和安全响应机制解决。请密切关注上游社区发布的通知和版本更新。鲲鹏计算社区对软件的漏洞及安全问题不承担任何责任。

## License<a name="ZH-CN_TOPIC_0000002551484917"></a>

hnswlib采用Apache 2.0 License许可证授权，支持修改代码和再开源。

## 贡献声明<a name="ZH-CN_TOPIC_0000002520604914"></a>

欢迎大家为社区做贡献，如果使用过程中有任何问题/建议，或者需要反馈特性需求和bug报告，可以提交[Issues](https://gitcode.com/boostkit/community/blob/master/docs/contributor/issue-submit.md)联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。同时也欢迎大家在[讨论专区](https://gitcode.com/boostkit/community/discussions)展开讨论交流。感谢您的支持。

## 致谢<a name="ZH-CN_TOPIC_0000002553554263"></a>

hnswlib由华为公司的下列部门联合贡献：

-   鲲鹏计算Boostkit开发部

感谢来自社区的每一个PR，欢迎贡献hnswlib！

