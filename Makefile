# ================= Arm 构建（四种测试形式）=================

CXX ?= g++
INCLUDES := -I./ -I./hnswlib -I./python_bindings
# 通用优化与并行
COMMON_FLAGS ?= -std=c++17 -fPIC -Ofast -fopenmp
# 允许从外部覆盖，例如 make ARCH_FLAGS="-march=armv9-a+fp16+dotprod"
ARCH_FLAGS ?= -march=armv8.2-a+dotprod
ARCH_FLAGS_SVE ?= -march=armv8.2-a+dotprod+sve
ARCH_FLAGS_FP16 ?= -march=armv8.2-a+fp16+dotprod
ARCH_FLAGS_SVE_FP16 ?= -march=armv8.2-a+fp16+dotprod+sve

# 依赖库（可通过 make HDF5_LIBS="..." 覆盖）
HDF5_LIBS ?= -lhdf5_cpp -lhdf5
NUMA_LIBS ?= -lnuma
LIBS := $(HDF5_LIBS) $(NUMA_LIBS)

# 源文件
SRC_FP32 := hnswlib_test.cpp
SRC_FP16 := hnswlib_test_fp16.cpp

# 目标名
BIN_FP32_NEON := test_fp32_neon
BIN_FP32_SVE  := test_fp32_sve
BIN_FP16_NEON := test_fp16_neon
BIN_FP16_SVE  := test_fp16_sve

# 1) FP32 + NEON（不启用 SVE）
$(BIN_FP32_NEON): $(SRC_FP32)
	$(CXX) $(COMMON_FLAGS) $(ARCH_FLAGS) $(INCLUDES) -DUSE_NEON -o $@ $< $(LIBS)

# 2) FP32 + SVE
$(BIN_FP32_SVE): $(SRC_FP32)
	$(CXX) $(COMMON_FLAGS) $(ARCH_FLAGS_SVE) $(INCLUDES) -DUSE_SVE -o $@ $< $(LIBS)

# 3) FP16 + NEON（端到端 FP16，不启用 SVE）
$(BIN_FP16_NEON): $(SRC_FP16)
	$(CXX) $(COMMON_FLAGS) $(ARCH_FLAGS_FP16) $(INCLUDES) -DUSE_NEON -DHNSWLIB_ENABLE_FP16 -o $@ $< $(LIBS)

# 4) FP16 + SVE（端到端 FP16 + SVE）
$(BIN_FP16_SVE): $(SRC_FP16)
	$(CXX) $(COMMON_FLAGS) $(ARCH_FLAGS_SVE_FP16) $(INCLUDES) -DUSE_SVE -DHNSWLIB_ENABLE_FP16 -o $@ $< $(LIBS)

# 便捷聚合目标
arm_all: $(BIN_FP32_NEON) $(BIN_FP32_SVE) $(BIN_FP16_NEON) $(BIN_FP16_SVE)

.PHONY: arm_all fp32_neon fp32_sve fp16_neon fp16_sve
fp32_neon: $(BIN_FP32_NEON)
fp32_sve:  $(BIN_FP32_SVE)
fp16_neon: $(BIN_FP16_NEON)
fp16_sve:  $(BIN_FP16_SVE)

# 清理本地二进制
clean_arm:
	rm -f $(BIN_FP32_NEON) $(BIN_FP32_SVE) $(BIN_FP16_NEON) $(BIN_FP16_SVE)
