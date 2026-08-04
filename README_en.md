# Introduction to hnswlib

## Latest Updates

- [2026.03.30]: The reconstructed hnswlib version is released on the GitCode platform. It supports the FP 16 data type, NEON vectorization, prefetching, and database ID renumbering optimization. The reconstructed hnswlib simplifies the code structure and improves usability.
- [2025.12.30]: hnswlib is released on the GitCode platform. It supports the FP16 data type, NEON and SVE vectorization, prefetching, and database ID renumbering optimization.

## Overview

Based on the open-source hnswlib (Hierarchical Navigable Small World), this library introduces FP16 data type support and is deeply optimized for the Kunpeng Arm platform. Through data type optimization and platform-specific tuning, computing efficiency is significantly improved and memory usage is reduced while maintaining retrieval precision. The reconstructed hnswlib fully utilizes the hardware features of the Kunpeng Arm architecture, leveraging compiler optimization, memory access optimization, and Arm vectorization to achieve robust cross-platform performance.

hnswlib is an efficient approximate nearest neighbor (ANN) search library, especially suitable for large-scale datasets. It achieves fast search performance by constructing a hierarchical graph structure. hnswlib supports multiple distance metrics, such as L2 (Euclidean) distance and inner product (IP) distance, and can process high-dimensional data.

**Application Scenarios<a name="section1268521411013"></a>**

hnswlib is applicable to the recall phase of recommendation systems. When the data type of the dataset is FP16, the newly introduced FP16 APIs can be used for graph retrieval to optimize computing efficiency and minimize memory footprint.

## Directory Structure

The complete directory structure of the open-source hnswlib is as follows:

```text
├── docs/                                                             // Resource documents
    ├── en                                                          
       ├── api_reference.md                                            // API reference
       ├── best_practices.md                                           // Best practices
       ├── feature_introduction.md                                     // Feature introduction
       ├── installation_guide.md                                       // Installation guide
       ├── quick_start.md                                              // Quick start
       └── release_notes.md                                            // Release notes
├── public_sys-resources/
├── 0001-hnswlib_0.8.0-optimize-fp32_fp16_dis_idsort_prefetch.patch   // hnswlib source code patch file
├── LICENSE                                                           // Project license file
└── README_en.md                                                      // Introduction to the open-source hnswlib
```

## Compilation Options

### Enabling NEON and FP16 Support

When compiling on the Arm platform, you need to add the following compilation options to enable NEON and FP16 support:

```bash
-march=armv8.2-a+dotprod+fp16fml -DUSE_NEON
```

## Compatibility with the Native hnswlib

This version maintains full compatibility with native HNSWLIB APIs while introducing the following new features:

1. Support for the FP16 data type (`L2SpacePh` and `IPSpacePh`)
2. NEON instruction optimization on the Arm platform
3. Base data ID renumbering optimization
4. Prefetch optimization on the Arm platform

You can seamlessly port the existing hnswlib code to this version and use the new FP16 APIs and Arm optimizations as required.

## Release Notes

For details about the version updates of the hnswlib algorithm, see [Release Notes](./docs/en/release_notes.md).

## Documents

<a name="table1191773710200"></a>
<table>
  <thead align="left">
    <tr id="row1291816372202">
      <th class="cellrowborder" valign="top" width="16.67166716671667%" id="mcps1.1.4.1.2"><p id="p13918183762016">Resource Name</p></th>
      <th class="cellrowborder" valign="top" width="71.28712871287128%" id="mcps1.1.4.1.3"><p id="p89181437152019">Resource Description</p></th>
    </tr>
  </thead>
  <tbody>
    <tr id="row179181137112015">
      <td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p2091893722011"><a href="./docs/en/release_notes.md">Release Notes</a></p></td>
      <td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p491893752010">Provides the basic information and feature updates of each released version of hnswlib.</p></td>
    </tr>
    <tr id="row2918153732017">
      <td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p17918337172020"><a href="./docs/en/quick_start.md">Quick Start</a></p></td>
      <td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p15918183742018">Provides guidance on how to quickly deploy hnswlib and verify its performance and functions.</p></td>
    </tr>
    <tr id="row1491853716207">
      <td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p191833772015"><a href="./docs/en/installation_guide.md">Installation Guide</a></p></td>
      <td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p091873712013">Provides detailed guidance on how to compile and install the hnswlib source code.</p></td>
    </tr>
    <tr id="row109181737112010">
      <td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p5918537142018"><a href="./docs/en/best_practices.md">Best Practices</a></p></td>
      <td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772013">Describes how to test hnswlib and provides tuning suggestions.</p></td>
    </tr>
    <tr id="row169181237152012">
      <td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p19181237162012"><a href="./docs/en/api_reference.md">API Reference</a></p></td>
      <td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772014">Provides definitions of hnswlib APIs and describes how to use them.</p></td>
    </tr>
    <tr id="row169181237152013">
      <td class="cellrowborder" valign="top" width="16.67166716671667%" headers="mcps1.1.4.1.2 "><p id="p19181237162013"><a href="./docs/en/feature_introduction.md">Feature Introduction</a></p></td>
      <td class="cellrowborder" valign="top" width="71.28712871287128%" headers="mcps1.1.4.1.3 "><p id="p991893772015">Describes the optimization features of hnswlib.</p></td>
    </tr>
  </tbody>
</table>

## Disclaimer

**To the hnswlib users**

- This project is intended solely for debugging and development. You are responsible for any risks and should carefully review the following information:
    - Data processing and deletion: Users are responsible for managing and deleting any data generated while using this tool. Users are advised to delete such data promptly after use to prevent information leakage.
    - Data confidentiality and transmission: Users understand and agree not to share or transmit any data generated by this tool. Neither the tool nor its developers are responsible for any information leaks, data breaches, or other negative consequences.
    - User input security: Users are responsible for the security of any commands they enter and for any risks or losses resulting from improper input. The tool and its developers are not liable for issues caused by incorrect command usage.

- Disclaimer scope: This disclaimer applies to all individuals and entities using this tool. By using the tool, you acknowledge and accept this statement and assume all risks and responsibilities arising from its use. If you do not agree, please stop using the tool immediately.
- Before using this tool, please **read and understand the preceding disclaimer**. If you have any questions, contact the developer.

**To data owners**

If you do not want your model or dataset to be mentioned in this project, or if you wish to update its description, please submit an issue on GitCode. We will delete or update your description according to your request. Thank you for your understanding and contribution to this project.

## License

hnswlib is licensed under the Apache 2.0 License, which allows modification and redistribution of derivative works as open source. For details, see [LICENSE](docs/LICENSE).

The documents of this project are licensed under CC-BY 4.0. For details, see [LICENSE](docs/LICENSE).

## Contribution Statement

We welcome your contributions to the community. If you have any questions/suggestions or want to provide feedback on feature requirements and bug reports, you can submit [issues](https://gitcode.com/boostkit/community/blob/master/docs/contributor/issue-submit.md). For details, see [Contribution Guideline](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md). You are also welcome to share insights in the [Discussions](https://gitcode.com/boostkit/community/discussions). Thank you for your support.

## Acknowledgments

hnswlib is jointly developed by the following Huawei department:

- Kunpeng Computing BoostKit Development Dept

Thank you to everyone in the community for your PRs. We warmly welcome contributions to hnswlib!
