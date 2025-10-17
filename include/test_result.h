#ifndef TEST_RESULT_H
#define TEST_RESULT_H

#include <vector>
#include <string>

class TestResult {
public:
    double recall;
    double calculate_quantity;
    double total_time;
    double build_time;
    std::vector<double> search_time;
    TestResult();
    ~TestResult();

    void reorder();
    double get_total(int begin, int end);

private:
    std::vector<double> accumulate_time;
};

#endif // TEST_RESULT_H