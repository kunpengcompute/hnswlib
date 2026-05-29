# Feature Description

## Overview

The optimized hnswlib is a high-performance vector search library that is deeply optimized for the Arm architecture based on the open-source hnswlib project. This optimized version maintains full compatibility with the original hnswlib APIs while delivering performance gains on Arm processors.

## Algorithm Principles<a name="section975104591010"></a>

The core principle of hnswlib is to organize data points in a high-dimensional space based on a Hierarchical Navigable Small World (HNSW) graph structure, thereby achieving an efficient balance between query speed and accuracy in nearest neighbor searches.

- This algorithm works by constructing a multi-layer graph structure, where the bottom layer contains all nodes in the dataset, and higher layers contain fewer nodes. The "long connections" formed between these higher-layer nodes act as "highways" for fast navigation.
- When a new node is to be inserted, the algorithm randomly determines its maximum layer using an exponentially decaying probability distribution. Then, starting from that layer, the algorithm searches for the nearest neighbors of the node layer by layer and establishes connections.
- The key parameter `M` controls the maximum number of connections per node at each layer, directly affecting the graph density and search quality. During the nearest neighbor search, the query starts from the entry point of the highest layer, utilizing long connections to quickly locate the target region, and then refines the search layer by layer using greedy traversal to approach the nearest neighbor node.
- The parameter `ef` dynamically controls the size of the candidate queue maintained during the search. A larger `ef` value yields higher search precision but increases query latency. This hierarchical structure effectively reduces the number of nodes that need to be accessed during a search. As a result, hnswlib significantly improves efficiency when dealing with large-scale high-dimensional data compared with traditional exact search methods, while maintaining a high recall rate.

The FP16 data type reduces the storage space of each vector component from 4 bytes in FP32 to 2 bytes. This significantly reduces the memory space required for building HNSW indexes, allowing for the processing of larger vector datasets under limited hardware resources. In addition, the distance computation algorithm of hnswlib is optimized for the Kunpeng Arm architecture, improving the efficiency of node distance comparison during graph retrieval.

## Core Optimization Features

The core optimization features include NEON instruction optimization for FP32 distance computation, FP16 data type support, base data ID renumbering optimization, and prefetch optimization. The functions and implementation technologies of each feature are described below.

### NEON Instruction Optimization for FP32 Distance Computation

**Function**

The Arm NEON SIMD instruction set is used to accelerate the L2 distance and inner product (IP) distance computation for FP32 vectors. The optimal computation path is automatically selected based on the vector dimension, significantly speeding up the distance computation.

#### Technical Implementation

- **SIMD vector acceleration**: The 128-bit vector register of Arm NEON is used to process 4 or 16 single-precision floating-point numbers at a time.
- **Multi-version implementation**:
  - `SIMD16`: When the vector dimension is a multiple of 16, a 16-element processing path is used.
  - `SIMD4`: When the vector dimension is a multiple of 4, a 4-element processing path is used.
  - `Residuals`: When the vector dimension is not a multiple of 4 or 16, a hybrid method combining the main path and tail processing is used.
- **Instruction-level parallelism**: fully exploits the parallel execution capability of NEON instructions to reduce loop iterations.

#### Supported Distance Types

- **L2 distance**: `L2SqrSIMD16ExtNEON`, `L2SqrSIMD4ExtNEON`, etc.
- **IP distance**: `InnerProductSIMD16ExtNEON`, `InnerProductSIMD4ExtNEON`, etc.

### FP16 Data Type Support

**Function**

Introduces support for FP16 half-precision floating-point numbers, reducing memory usage and further boosting computing performance while ensuring search quality.

#### Technical Implementation

- **Half-precision numbers**: The `float16_t` data type is supported, which significantly reduces the memory usage compared with FP32.
- **NEON FP16 instructions**: The dedicated FP16 instruction set of Arm NEON is used for acceleration.
- **Automatic type conversion**: The data type is efficiently converted from FP16 to FP32 during distance computation.
- **Complete space implementations**:
  - `L2SpacePh`: FP16 L2 distance space
  - `IPSpacePh`: FP16 IP distance space

#### Core Advantages

- **Memory saving**: The memory usage of vector data is reduced.
- **Performance boost**: The performance of the FP16 version is further improved compared with the FP32 NEON optimized version.

### Base Data ID Renumbering Optimization

**Function**

When using the optimized hnswlib with the `USE_NEON` macro enabled, the `AggressiveRenumberInplace` mechanism is automatically triggered to renumber and optimize node IDs in the HNSW graph. This maximizes the local continuity of neighbor node IDs while keeping the graph topology unchanged, thereby improving cache utilization and access speed.

#### Technical Implementation

- **Memory rearrangement**: Index elements are rearranged to improve the memory access locality.
- **BFS traversal**: The graph structure is traversed using Breadth-First Search (BFS) to ensure that adjacent nodes are also adjacent in the memory.
- **Deleted element management**: Deleted elements are moved to the end of the memory to keep valid elements compactly arranged.
- **Multi-thread support**: OpenMP is used to parallelize both memory copying and linked list updates.

### Prefetch Optimization

**Function**

The hnswlib multi-layered graph search algorithm uses prefetching to reduce memory access latency. It loads node visitation states and the mapping of internal IDs to data pointers into the cache ahead of use, which improves overall search throughput.

#### Technical Implementation

- **Data prefetching**: Data is loaded into the cache before being accessed.
- **Link prefetching**: The link of the next node to be accessed is prefetched.
