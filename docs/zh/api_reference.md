# API参考

本文档介绍在开源hnswlib API的基础上，新增了FP16数据类型的距离计算空间类L2SpacePh和IPSpacePh，用于在ARM平台上加速FP16向量的距离计算。其余API与开源hnswlib API保持不变。FP32和FP16的距离计算实现差异体现在加载数据类型不同。FP32距离计算加载FP32类型数据，FP16距离计算加载FP16类型数据，但二者均返回FP32数据类型的计算结果。本文档作为hnswlib算法的API参考手册，详细介绍了各接口的功能和使用方法。

## 与开源代码的差异

**与开源接口差异<a name="section541141716277"></a>**

除此处介绍的接口差异外，其余接口与开源代码接口的使用方式保持一致。

**表 1**  距离计算空间接口

| 距离类型 | 数据类型 | 接口名称 |
| --- | --- | --- |
| L2 | FP16 | L2SpacePh |
| IP | FP16 | IPSpacePh |

>![note](public_sys-resources/icon-note.gif) **说明：**
>除上述接口差异外，其余接口与开源代码接口的使用方式保持一致。

**距离计算实现差异<a name="section1839510154"></a>**

FP32和FP16的距离计算实现差异体现在加载数据类型不同。FP32距离计算加载FP32类型数据，FP16距离计算加载FP16类型数据，但二者均返回FP32数据类型的计算结果。

## 核心空间类

### FP32数据类型空间

#### L2Space

L2距离空间实现，适用于FP32数据类型，已针对ARM NEON进行优化。

```cpp
hnswlib::L2Space space(dim); // 构造方法一
space = std::make_unique<hnswlib::L2Space>(dim); // 构造方法二
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| dim | 向量维度。 |

**说明**

- 根据向量维度自动选择最优的NEON指令实现（SIMD16、SIMD4或带残差处理的版本）
- 支持维度为任意大小，内部会自动处理对齐和残差

#### InnerProductSpace

内积距离空间实现，适用于FP32数据类型，已针对ARM NEON进行优化。

```cpp
hnswlib::InnerProductSpace space(dim); // 构造方法一
space = std::make_unique<hnswlib::InnerProductSpace>(dim); // 构造方法二
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| dim | 向量维度。 |

**说明**

- 根据向量维度自动选择最优的NEON指令实现
- 内积距离定义为 `1.0 - sum(Ai*Bi)`

### FP16数据类型空间（仅支持NEON实现）

#### L2SpacePh

L2距离空间实现，适用于FP16数据类型，使用NEON指令加速。

```cpp
#ifdef USE_NEON
hnswlib::L2SpacePh space(dim); // 构造方法一
space = std::make_unique<hnswlib::L2SpacePh>(dim); // 构造方法二
#endif
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| dim | 向量维度。 |

**说明**

- 仅在定义了`USE_NEON`宏时可用
- 自动选择最优的NEON指令实现
- 内存占用对比FP32显著减少

#### IPSpacePh

内积距离空间实现，适用于FP16数据类型，使用NEON指令加速。

```cpp
#ifdef USE_NEON
hnswlib::IPSpacePh space(dim); // 构造方法一
space = std::make_unique<hnswlib::IPSpacePh>(dim); // 构造方法二
#endif
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| dim | 向量维度。 |

**说明**

- 仅在定义了`USE_NEON`宏时可用

- 自动选择最优的NEON指令实现

- 内存占用对比FP32显著减少

## HierarchicalNSW类

### 构造函数

```cpp
// 创建新索引
hnswlib::HierarchicalNSW<float>* index = new hnswlib::HierarchicalNSW<float>(
    &space,                        // 空间对象
    max_elements,                  // 最大元素数量
    M = 16,                        // 每个节点的最大连接数
    ef_construction = 200,         // 构建时的ef参数
    random_seed = 100,             // 随机种子
    allow_replace_deleted = false  // 是否允许替换已删除元素
);

// 从文件加载索引
hnswlib::HierarchicalNSW<float>* index = new hnswlib::HierarchicalNSW<float>(
    &space,                        // 空间对象
    index_path,                    // 索引文件路径
    nmslib = false,                // 是否为nmslib格式
    max_elements = 0,              // 可选的新最大元素数量
    allow_replace_deleted = false  // 是否允许替换已删除元素
);
```

### 主要方法

#### addPoint

添加一个数据点到索引中。

```cpp
void addPoint(const void* data_point, labeltype label, bool replace_deleted = false);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| data_point | 指向数据点的指针（FP32或FP16类型，取决于使用的空间类）。 |
| label | 数据点的标签。 |
| replace_deleted | 是否允许替换已删除的元素（需要构造时设置allow_replace_deleted=true）。 |

#### searchKnn

执行最近邻搜索，返回最近的k个结果。

```cpp
std::priority_queue<std::pair<float, labeltype>> searchKnn(
    const void* query_data,                   // 查询向量
    size_t k,                                 // 搜索最近邻数量
    BaseFilterFunctor* isIdAllowed = nullptr  // 可选的过滤器
) const;
```

**返回值**

- 优先队列，包含k个最近邻，按距离从远到近排序

#### searchKnnCloserFirst

执行最近邻搜索，返回最近的k个结果。

```cpp
std::vector<std::pair<float, labeltype>> searchKnnCloserFirst(
    const void* query_data,                   // 查询向量
    size_t k,                                 // 搜索最近邻数量
    BaseFilterFunctor* isIdAllowed = nullptr  // 可选的过滤器
) const;
```

**返回值**

- 向量，包含k个最近邻，按距离从近到远排序

#### saveIndex

将索引保存到文件。

```cpp
void saveIndex(const std::string& location);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| location | 保存文件的路径。 |

#### loadIndex

从文件加载索引。

```cpp
void loadIndex(const std::string& location, SpaceInterface<float>* s, size_t max_elements = 0);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| location | 索引文件路径。 |
| s | 空间对象。 |
| max_elements | 可选的新最大元素数量。 |

#### setEf

设置查询时的ef参数，控制查询精度和速度的权衡。

```cpp
void setEf(size_t ef);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| ef | 查询时的ef参数值（应大于搜索的k值）。 |

#### markDelete

标记一个元素为已删除。

```cpp
void markDelete(labeltype label);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| label | 要删除的元素标签。 |

#### unmarkDelete

取消标记一个已删除的元素。

```cpp
void unmarkDelete(labeltype label);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| label | 要取消删除的元素标签。 |

#### resizeIndex

调整索引的最大容量。

```cpp
void resizeIndex(size_t new_max_elements);
```

**参数**

| 参数 | 参数说明 |
| --- | --- |
| new_max_elements | 新的最大元素数量（必须大于当前元素数量）。 |

## FP32与FP16接口比较

| 特性 | FP32接口 | FP16接口 |
|------|----------|----------|
| 数据类型 | float | float16_t |
| 内存占用 | 每个维度4字节 | 每个维度2字节 |
| 精度 | 高精度 | 中等精度 |
| 速度 | 快 | 更快（在支持NEON的ARM平台上） |
| 空间类 | L2Space, InnerProductSpace | L2SpacePh, IPSpacePh |
| 平台要求 | 无特殊要求 | 仅支持NEON架构 |

## 修订记录

|文档版本|发布日期|修改说明|
|--|--|--|
|01|2026-03-30|第一次正式发布。|
