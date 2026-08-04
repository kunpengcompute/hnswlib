# Best Practices

This document describes how to test the optimized hnswlib on the Kunpeng platform. The test depends on the Kunpeng optimization patch file `0001-hnswlib_0.8.0-optimize-fp32_fp16_dis_idsort_prefetch.patch`.

## Testing Example of the Optimized hnswlib<a name="section18521028313"></a>

The following example uses the `fashion-mnist-784-euclidean.hdf5` dataset.

1. Install hnswlib as described in [Installation Guide](./installation_guide.md).

2. Obtain a test program. Assume that the path is `/path/to/sra_test`.

    ```bash
    git clone https://gitcode.com/openeuler/sra_test.git
    ```

3. Create a `data` folder and obtain the test dataset.

    ```bash
    cd /path/to/sra_test
    mkdir data && cd data
    wget http://ann-benchmarks.com/fashion-mnist-784-euclidean.hdf5 --no-check-certificate
    ```

4. Compile the executable file.

    ```bash
    make hnswlib_test          // FP32 data type, using NEON vectorization instructions.
    make hnswlib_fp16_test     // FP16 data type, using NEON vectorization instructions.
    ```

    The complete directory structure is as follows:

    ```text
    ├── configs                                                   // Stores configuration files for the algorithm and dataset.
    │      └── hnswlib_fp16/
    │            └── hnswlib_fp16_fashion-mnist-784-euclidean.config
    ├── data                                                      // Stores the datasets.
    │      └── fashion-mnist-784-euclidean.hdf5
    ├── include                                                   // Stores header files of the test framework.
    ├── indexes/
    │      └── hnswlib_fp16/                                       // Stores the built index, which needs to be manually created.
    │            └── fashion.hnswlib                               // Built index, generated when the executable file hnswlib_fp16_test runs and the save_or_load parameter in the dataset configuration file is set to save.
    ├── scripts/
    │ └── build.sh                                            // Script file
    ├── src                                                       // Stores source files of the test framework.
    ├── get_result.py
    ├── LICENSE
    ├── Makefile
    ├── README.md
    ├── test.sh                                                   // Script for performing performance tests
    ├── test_multi-numas.sh                                       // Script for performing performance tests on multiple NUMA nodes
    ├── hnswlib_fp16_test                                         // Executable file generated after compilation
    └── hnswlib_test
    
    ```

    >![note](public_sys-resources/icon-note.gif) **NOTE**
    >- During the first compilation, as prompted by the command line, enter the `hnswlib` path and the macros to be enabled. The script automatically saves the corresponding configuration in the `build` folder. In subsequent compilations, you can directly modify the configuration in the `config_hnswlib.sh` or `config_hnswlib_fp16.sh` file under the `build` folder and run `hnswlib_test` or `hnswlib_fp16_test` again.
    >- To compile `hnswlib_test`, enable the macro `-DUSE_NEON`.
    >- To compile `hnswlib_fp16_test`, enable the macros `-DUSE_NEON` and `-DUSE_FP16`.

5. Run the test scripts.

    ```bash
    sh test.sh hnswlib                // FP32 data type, using NEON vectorization instructions.
    sh test.sh hnswlib_fp16           // FP16 data type, using NEON vectorization instructions.
    ```

    The test results are stored in the `output` folder. Pay attention to the QPS metric in the test results and ensure that the recall rate is greater than 0.99.

    >![note](public_sys-resources/icon-note.gif) **NOTE**
    >- The `/configs` folder contains the configuration information for different datasets. `index_path` is the path for saving the index, and `save_or_load` is the option for building or loading the graph index.
    >- When `save_or_load` is set to `save`, the graph construction mode is used. In this case, the constructed graph index is saved to `/index_path`.
    >- When `save_or_load` is set to `load`, the loading mode is used. In this case, the graph index is loaded from `/index_path` for subsequent search.
    >- During the first run, execute the performance test script in `save` mode on a single NUMA node and save the graph index.
    >- In subsequent runs, if the values of `k_f` and `efc` do not need to be changed, you can run the script in `load` mode across all 4 NUMA nodes to obtain performance data.
    >- If the values of `k_f` and `efc` need to be changed, you need to reconstruct the graph in `save` mode.

**Performance Optimization Suggestions**

1. **Select a proper data type.** When high precision is not required, preferentially use FP16 to reduce memory footprint and improve performance.
2. **Adjust parameters.** Adjust the `M`, `efc`, and `efs` parameters. For details, see section [Tuning Guide for Performance Test Parameters](#section3966431183112).
3. **Perform batch queries**. Process queries in batches as much as possible to reduce thread creation overhead.

**Tuning Guide for Performance Test Parameters<a id="section3966431183112"></a>**

Assume that the path to the configuration file for parameter tuning is `/path/to/sra_test/configs`. The directory structure of the folder is as follows:

```text
├── configs/
      ├── hnswlib/
      ├── hnswlib_fp16/
            └── hnswlib_fp16_fashion-mnist-784-euclidean.config
```

The content of the `config` file is as follows:

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

[**Table 1** Parameters to be tuned](#parameters-to-be-tuned) describes the parameters to be tuned.

**Table 1** Parameters to be tuned<a id="parameters-to-be-tuned"></a>

<a name="table114596415225"></a>
<table><thead align="left"><tr id="row4459164110224"><th class="cellrowborder" valign="top" width="12.38123812381238%" id="mcps1.2.4.1.1"><p id="p114598418224"><a name="p114598418224"></a><a name="p114598418224"></a>Parameter</p>
</th>
<th class="cellrowborder" valign="top" width="26.512651265126514%" id="mcps1.2.4.1.2"><p id="p44598418228"><a name="p44598418228"></a><a name="p44598418228"></a>Description</p>
</th>
<th class="cellrowborder" valign="top" width="61.106110611061105%" id="mcps1.2.4.1.3"><p id="p24598411224"><a name="p24598411224"></a><a name="p24598411224"></a>Tuning Suggestion</p>
</th>
</tr>
</thead>
<tbody><tr id="row1645974112221"><td class="cellrowborder" valign="top" width="12.38123812381238%" headers="mcps1.2.4.1.1 "><p id="p184591741102210"><a name="p184591741102210"></a><a name="p184591741102210"></a>k_f</p>
</td>
<td class="cellrowborder" valign="top" width="26.512651265126514%" headers="mcps1.2.4.1.2 "><p id="p124591241112214"><a name="p124591241112214"></a><a name="p124591241112214"></a>Maximum number of connections per node in an HNSW graph.</p>
</td>
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p3459341122210"><a name="p3459341122210"></a><a name="p3459341122210"></a>A smaller value reduces the index size but may decrease search precision. A larger value improves search precision but increases the index size and construction time.</p>
</td>
</tr>
<tr id="row1645912412221"><td class="cellrowborder" valign="top" width="12.38123812381238%" headers="mcps1.2.4.1.1 "><p id="p13459134182213"><a name="p13459134182213"></a><a name="p13459134182213"></a>efs</p>
</td>
<td class="cellrowborder" valign="top" width="26.512651265126514%" headers="mcps1.2.4.1.2 "><p id="p7459341192219"><a name="p7459341192219"></a><a name="p7459341192219"></a>Number of candidate neighbors per node during a search.</p>
</td>
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p15459941202217"><a name="p15459941202217"></a><a name="p15459941202217"></a>A smaller value accelerates the search speed but may lower search precision. A larger value improves search precision but increases search latency.</p>
</td>
</tr>
<tr id="row11459741202211"><td class="cellrowborder" valign="top" width="12.38123812381238%" headers="mcps1.2.4.1.1 "><p id="p154591441132214"><a name="p154591441132214"></a><a name="p154591441132214"></a>efc</p>
</td>
<td class="cellrowborder" valign="top" width="26.512651265126514%" headers="mcps1.2.4.1.2 "><p id="p114591741162217"><a name="p114591741162217"></a><a name="p114591741162217"></a>Number of candidate neighbors per node during index construction.</p>
</td>
<td class="cellrowborder" valign="top" width="61.106110611061105%" headers="mcps1.2.4.1.3 "><p id="p245916419226"><a name="p245916419226"></a><a name="p245916419226"></a>A smaller value speeds up index construction but may degrade index quality. A larger value improves index quality but increases construction time. Using a large value (e.g., 200 to 800) during construction achieves better index quality.</p>
</td>
</tr>
</tbody>
</table>

>![note](public_sys-resources/icon-note.gif) **NOTE**
>Tune the above parameters when `save_or_load` is set to `save`. It is recommended that a single NUMA node is used for index construction. The tuning objective is to identify the parameter combination that achieves the highest QPS while maintaining a recall rate greater than 0.99.

**(Optional) UT Test<a name="section136316366272"></a>**

Run the following commands to check whether the compilation and execution environments are normal and whether the algorithm functions correctly.

1. Create a unit test (UT) execution script.

    ```bash
    cd /path/to/hnswlib
    vim run.sh
    ```

    The content of the `run.sh` script is as follows:

    ```bash
    rm -rf build
    mkdir build
    cd build
    cmake ..
    make

    ./distance_calculation_neon_test
    ```

    Paste the preceding content to the script, press `Esc`, enter `:wq`, and press `Enter` to save the file and exit.

2. Run the UT test.

    ```bash
    sh run.sh
    ```
