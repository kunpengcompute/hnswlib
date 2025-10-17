rm -rf build
mkdir build
cd build
cmake ..
make

cd ../tests/cpp
python update_gen_data.py

cd ../../build/
./epsilon_search_test
./example_epsilon_search
./example_filter
./example_mt_filter
./example_mt_replace_deleted
./example_mt_search
./example_multivector_search
./example_replace_deleted
./example_search
./multiThreadLoad_test
./multiThread_replace_test
./multivector_search_test
./searchKnnCloserFirst_test
./searchKnnWithFilter_test
./test_updates
./test_updates update
./fp16_test

lcov -d . -o test.info -b . -c --rc lcov_branch_coverage=1
lcov --remove test.info '*10.3.1*' '*gcc*' '/usr/*' '/src/*' '/include/*' '/opt/*' '*/test/*' '!*/tests/*' --output-file coverage_final.info --rc lcov_branch_coverage=1
genhtml -o test_output coverage_final.info --rc lcov_branch_coverage=1