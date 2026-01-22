# 项目介绍

本项目旨在基于开源hnswlib，针对鲲鹏ARM平台进行深度优化。在保持检索精度的前提下，显著提升计算效率。


## 补丁文件

| 补丁文件 |  功能 |
| ------------ | ------------ |
| 0001-boostsra-hnswlib-0.8.0-optimize_dis.patch  | 面向AArch64平台的汇编优化，加速L2/IP距离计算。  |


## 使用补丁文件

1.  获取开源代码，假设安装路径为“/path/to“。

    ```
    cd /path/to
    git clone https://github.com/nmslib/hnswlib.git -b v0.8.0
    ```

2.  下载hnswlib的patch文件，假设安装路径为“/path/to“。

    ```
    git clone https://gitcode.com/boostkit/hnswlib.git -b custom
    ```

3.  应用对应功能的补丁文件，注意替换补丁文件名称。

    ```
    cd hnswlib
    patch -p1 < ../0001-boostsra-hnswlib-0.8.0-optimize_xxx.patch
    ```

# 贡献指南

如果使用过程中有任何问题，或者需要反馈特性需求和bug报告，可以提交issues联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。

# 免责声明

此代码仓计划参与hnswlib开源组件，仅作hnswlib性能优化，编码风格遵照原生开源软件，继承原生开源软件安全设计，不破坏原生开源软件设计及编码风格和方式，软件的任何漏洞与安全问题，均由相应的上游社区根据其漏洞和安全响应机制解决。请密切关注上游社区发布的通知和版本更新。鲲鹏计算社区对软件的漏洞及安全问题不承担任何责任。

# 许可证书

hnswlib采用Apache 2.0 License许可证授权，支持修改代码和再开源。
