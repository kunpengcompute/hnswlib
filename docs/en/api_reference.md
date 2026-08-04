# API Reference

## Overview

Based on the open-source hnswlib API, the new distance computation space classes `L2SpacePh` and `IPSpacePh` for the FP16 data type have been introduced to accelerate FP16 vector distance computation on Arm platforms. Other APIs remain unchanged compared with the open-source hnswlib APIs. The implementation difference between FP32 and FP16 distance computation lies in the loaded data type. FP32 distance computation loads FP32 data, while FP16 distance computation loads FP16 data, but both return computation results in the FP32 data type. This document describes the APIs involved in the hnswlib algorithm.

## Differences from Open-Source Code

**Differences from Open-Source APIs<a name="section541141716277"></a>**

Except for the API differences described here, the usage of all other APIs remains consistent with the open-source code APIs.

**Table 1** Distance computation space APIs

<a name="table04611241184112"></a>
<table><thead align="left"><tr id="row1846114117417"><th class="cellrowborder" valign="top" width="11.93%" id="mcps1.2.5.1.1"><p id="p9872141016446"><a name="p9872141016446"></a><a name="p9872141016446"></a>Distance Type</p>
</th>
<th class="cellrowborder" valign="top" width="11.51%" id="mcps1.2.5.1.2"><p id="p16461941194115"><a name="p16461941194115"></a><a name="p16461941194115"></a>Data Type</p>
</th>
<th class="cellrowborder" valign="top" width="31.540000000000003%" id="mcps1.2.5.1.4"><p id="p12461134164117"><a name="p12461134164117"></a><a name="p12461134164117"></a>API</p>
</th>
</tr>
</thead>
<tbody><tr id="row6461134117419"><td class="cellrowborder" valign="top" width="11.93%" headers="mcps1.2.5.1.1 "><p id="p9872510194410"><a name="p9872510194410"></a><a name="p9872510194410"></a>L2</p>
</td>
<td class="cellrowborder" valign="top" width="11.51%" headers="mcps1.2.5.1.2 "><p id="p1046114114411"><a name="p1046114114411"></a><a name="p1046114114411"></a>FP16</p>
</td>
<td class="cellrowborder" valign="top" width="31.540000000000003%" headers="mcps1.2.5.1.4 "><p id="p94547165121"><a name="p94547165121"></a><a name="p94547165121"></a>L2SpacePh</p>
</td>
</tr>
<tr id="row423022213459"><td class="cellrowborder" valign="top" width="11.93%" headers="mcps1.2.5.1.1 "><p id="p04601651065"><a name="p04601651065"></a><a name="p04601651065"></a>IP</p>
</td>
<td class="cellrowborder" valign="top" width="11.51%" headers="mcps1.2.5.1.2 "><p id="p22301822204515"><a name="p22301822204515"></a><a name="p22301822204515"></a>FP16</p>
</td>
<td class="cellrowborder" valign="top" width="31.540000000000003%" headers="mcps1.2.5.1.4 "><p id="p764211191141"><a name="p764211191141"></a><a name="p764211191141"></a>IPSpacePh</p>
</td>
</tr>
</tbody>
</table>

>![note](public_sys-resources/icon-note.gif) **NOTE**
>Except for the differences mentioned above, the usage of all other APIs remains consistent with the open-source code APIs.

**Implementation Differences in Distance Computation<a name="section1839510154"></a>**

The implementation difference between FP32 and FP16 distance computation lies in the loaded data type. FP32 distance computation loads FP32 data, while FP16 distance computation loads FP16 data, but both return computation results in the FP32 data type.

## Core Space Classes

### FP32 Data Type Space

#### L2Space

An implementation of L2 distance space, applicable to the FP32 data type and optimized for Arm NEON.

```cpp
hnswlib::L2Space space(dim); // Construction method 1
space = std::make_unique<hnswlib::L2Space>(dim); // Construction method 2
```

**Parameters**

- `dim`: Vector dimensions

**NOTE**

- Automatically selects the optimal NEON instruction implementation (SIMD16, SIMD4, or version with tail processing) based on the vector dimension.
- Supports arbitrary vector dimensions, with alignment and tail data processed automatically inside.

#### InnerProductSpace

An implementation of Inner Product (IP) distance space, applicable to the FP32 data type and optimized for Arm NEON.

```cpp
hnswlib::InnerProductSpace space(dim); // Construction method 1
space = std::make_unique<hnswlib::InnerProductSpace>(dim); // Construction method 2
```

**Parameters**

- `dim`: Vector dimensions

**NOTE**

- Automatically selects the optimal NEON instruction implementation based on the vector dimension.
- The IP distance is defined as `1.0 - sum(Ai × Bi)`.

### FP16 Data Type Space (NEON Implementation Only)

#### L2SpacePh

An implementation of L2 distance space, applicable to the FP16 data type and accelerated using NEON instructions.

```cpp
#ifdef USE_NEON
hnswlib::L2SpacePh space(dim); // Construction method 1
space = std::make_unique<hnswlib::L2SpacePh>(dim); // Construction method 2
#endif
```

**Parameters**

- `dim`: Vector dimensions

**NOTE**

- Available only when the `USE_NEON` macro is defined.
- Automatically selects the optimal NEON instruction implementation.
- Memory footprint is significantly reduced compared to FP32.

#### IPSpacePh

An implementation of IP distance space, applicable to the FP16 data type and accelerated using NEON instructions.

```cpp
#ifdef USE_NEON
hnswlib::IPSpacePh space(dim); // Construction method 1
space = std::make_unique<hnswlib::IPSpacePh>(dim); // Construction method 2
#endif
```

**Parameters**

- `dim`: Vector dimensions

**NOTE**

- Available only when the `USE_NEON` macro is defined.
- Automatically selects the optimal NEON instruction implementation.
- Memory footprint is significantly reduced compared to FP32.

## HierarchicalNSW Class

### Constructors

```cpp
// Create an index.
hnswlib::HierarchicalNSW<float>* index = new hnswlib::HierarchicalNSW<float>(
    &space,                        // Space object
    max_elements,                  // Maximum number of elements
    M = 16,                        // Maximum number of connections per node
    ef_construction = 200,         // ef parameter for construction
    random_seed = 100,             // Random seed
    allow_replace_deleted = false  // Whether to allow replacing deleted elements
);

// Load an index from a file.
hnswlib::HierarchicalNSW<float>* index = new hnswlib::HierarchicalNSW<float>(
    &space,                        // Space object
    index_path,                    // Index file path
    nmslib = false,                // Whether the format nmslib is used
    max_elements = 0,              // Optional new maximum number of elements
    allow_replace_deleted = false  // Whether to allow replacing deleted elements
);
```

### Main Methods

#### addPoint

Adds a data point to the index.

```cpp
void addPoint(const void* data_point, labeltype label, bool replace_deleted = false);
```

**Parameters**

- `data_point`: pointer to the data point (FP32 or FP16 type, depending on the space class used)
- `label`: label of the data point
- `replace_deleted`: specifies whether to allow replacing deleted elements (requires setting `allow_replace_deleted` to `true` during construction)

#### searchKnn

Performs the nearest neighbor search (NNS) and returns top k nearest results.

```cpp
std::priority_queue<std::pair<float, labeltype>> searchKnn(
    const void* query_data,                   // Query vector
    size_t k,                                 // Number of nearest neighbors to search
    BaseFilterFunctor* isIdAllowed = nullptr  // Optional filter
) const;
```

**Return Value**

- A priority queue containing the `k` nearest neighbors, sorted from the farthest to closest distance.

#### searchKnnCloserFirst

Performs NNS and returns top k nearest results.

```cpp
std::vector<std::pair<float, labeltype>> searchKnnCloserFirst(
    const void* query_data,                   // Query vector
    size_t k,                                 // Number of nearest neighbors to search
    BaseFilterFunctor* isIdAllowed = nullptr  // Optional filter
) const;
```

**Return Value**

- A vector containing `k` nearest neighbors, sorted from the closest to farthest distance.

#### saveIndex

Saves the index to a file.

```cpp
void saveIndex(const std::string& location);
```

**Parameters**

- `location`: path for saving the file

#### loadIndex

Loads the index from a file.

```cpp
void loadIndex(const std::string& location, SpaceInterface<float>* s, size_t max_elements = 0);
```

**Parameters**

- `location`: path to the index file
- `s`: space object
- `max_elements`: optional new maximum number of elements

#### setEf

Sets the `ef` parameter for query, which controls the trade-off between query precision and speed.

```cpp
void setEf(size_t ef);
```

**Parameters**

- `ef`: `ef` parameter value for query. The value must be greater than the search value `k`.

#### markDelete

Marks an element as deleted.

```cpp
void markDelete(labeltype label);
```

**Parameters**

| Parameter| Parameter Description|
|------|----------|
| label| label of the element to be deleted|

#### unmarkDelete

Unmarks a deleted element.

```cpp
void unmarkDelete(labeltype label);
```

**Parameters**

| Parameter| Parameter Description|
|------|----------|
| label| label of the element to be undeleted|

#### resizeIndex

Resizes the maximum capacity of the index.

```cpp
void resizeIndex(size_t new_max_elements);
```

**Parameters**

| Parameter| Parameter Description|
|------|----------|
| new_max_elements| new maximum number of elements (must be greater than the current number of elements)|

## Comparison between FP32 and FP16 APIs

| Feature| FP32 API| FP16 API|
|------|----------|----------|
| Data type| float | float16_t |
| Memory usage| 4 bytes per dimension| 2 bytes per dimension|
| Precision| High precision| Medium precision|
| Speed| Fast| Faster (on the Arm platform with NEON support)|
| Space| L2Space and InnerProductSpace | L2SpacePh and IPSpacePh |
| Platform| All| Supported on NEON architecture only|
