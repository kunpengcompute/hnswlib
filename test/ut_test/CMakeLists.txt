cmake_minimum_required(VERSION 3.12.0)
project(best LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS 
    "-std=c++17 -fPIC -Ofast -fopenmp -fstack-protector-strong -Wl,-z,relro,-z,now -s -ftrapv -Wall -Wextra -DHNSWLIB_ENABLE_FP16 -DUSE_NEON -DHNSWLIB_USE_ARM"
)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fopenmp")

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../")
message(STATUS "ROOT = ${ROOT}")
#set(SECURE_C_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../huawei_secure_c")

include(FetchContent)
FetchContent_Declare(
    googletest
    #GIT_REPOSITORY https://github.com/google/googletest.git
    #GIT_TAG release-1.11.0
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/googletest/"
)
FetchContent_MakeAvailable(googletest)


include_directories(
    ${ROOT}/hnswlib
    #${SECURE_C_PATH}/include
    ${HDF5_INCLUDE_DIRS}
)

find_package(HDF5 REQUIRED COMPONENTS C HL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod+fp16+sve")


enable_testing()
add_executable(hnswlib_ut_test
    main.cpp
    spaceIpTest.cpp
    spaceL2Test.cpp
)
target_link_libraries(hnswlib_ut_test
    PRIVATE
    gtest
    gtest_main
    ${HDF5_LIBRARIES}
    -lcurl
    -lpthread
    -lz
    -ldl
    -lm
    -lnuma
    -lmockcpp
    -lgcov
)

target_compile_options(hnswlib_ut_test PRIVATE
    -fprofile-arcs      # 启用代码覆盖率
    -ftest-coverage     # 启用代码覆盖率
)

add_test(NAME hnswlib_ut_test COMMAND hnswlib_ut_test)