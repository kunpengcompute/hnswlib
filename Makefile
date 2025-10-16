ARCH := $(shell uname -m)

CXX = g++
LDFLAGS = -lhdf5_cpp -lhdf5
SRCS = src/config_parser.cpp src/datasets.cpp src/test_result.cpp src/main.cpp
ROOT = /home/lzy/hnswlib/kp/hnswlib
HNSWLIBROOT = /home/lzy/hnswlib/kp/hnswlib/hnswlib/
CXXFLAGS = -std=c++17 -fPIC -Ofast -Iinclude -I${ROOT}/include -fopenmp

ifeq ($(ARCH),aarch64)
	CXXFLAGS += -march=armv8.2-a+dotprod+fp16+sve
else
	ONEAPIROOT = /opt/intel/oneapi
	CXXFLAGS += -m64 -DMKL_ILP64  -I${ONEAPIROOT}/mkl/2025.0/include
	LDFLAGS += -L${ONEAPIROOT}/mkl/2025.0/lib/ -L${ONEAPIROOT}/compiler/2025.0/lib/ -Wl,--no-as-needed -lgomp -lpthread -lm -ldl -DHNSWERR=std::cerr
endif

ifeq ($(ARCH),aarch64)
ifeq ($(MAKECMDGOALS), hnswlib_sve_test)
	TARGET = hnswlib_sve_test
	DEFINES = -DHNSWLIB_TEST
	CXXFLAGS += -I${HNSWLIBROOT} -DUSE_SVE -DHNSWLIB_USE_ARM
	SRCS += src/hnswlib_index_fp16.cpp
else ifeq ($(MAKECMDGOALS), hnswlib_neon_test)
	TARGET = hnswlib_neon_test
	DEFINES = -DHNSWLIB_TEST
	CXXFLAGS += -I${HNSWLIBROOT} -DUSE_NEON -DHNSWLIB_USE_ARM
	SRCS += src/hnswlib_index_fp16.cpp
else ifeq ($(MAKECMDGOALS), hnswlib_fp16_sve_test)
	TARGET = hnswlib_fp16_sve_test
	DEFINES = -DHNSWLIB_FP16_TEST
	CXXFLAGS += -I${HNSWLIBROOT} -DHNSWLIB_ENABLE_FP16 -DUSE_SVE -DHNSWLIB_USE_ARM
	SRCS += src/hnswlib_index_fp16.cpp
else ifeq ($(MAKECMDGOALS), hnswlib_fp16_neon_test)
	TARGET = hnswlib_fp16_neon_test
	DEFINES = -DHNSWLIB_FP16_TEST
	CXXFLAGS += -I${HNSWLIBROOT} -DHNSWLIB_ENABLE_FP16 -DUSE_NEON -DHNSWLIB_USE_ARM
	SRCS += src/hnswlib_index_fp16.cpp
endif
else
ifeq ($(MAKECMDGOALS), hnswlib_test)
	TARGET = hnswlib_test
	DEFINES = -DHNSWLIB_TEST
	CXXFLAGS += -I${HNSWLIBROOT}
	SRCS += src/hnswlib_index.cpp
endif
endif

$(TARGET):
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) $(DEFINES) -o $@
