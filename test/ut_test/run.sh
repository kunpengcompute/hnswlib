rm -rf build
mkdir build
cd build
cmake ..
make

cd ..
./build/hnswlib_ut_test --gtest_output=xml:./

lcov -d . -o test.info -b . -c --rc lcov_branch_coverage=1
lcov --remove test.info '*10.3.1*' '*gcc*' '/usr/*' '/src/*' '/include/*' '/hnswlib/*' '/opt/*' '*test*' --output-file coverage_final.info --rc lcov_branch_coverage=1
genhtml -o ut_output coverage_final.info --rc lcov_branch_coverage=1