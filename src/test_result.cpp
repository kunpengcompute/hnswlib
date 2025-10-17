#include "test_result.h"
#include <algorithm>

TestResult::TestResult()
    : total_time(0), recall(0), calculate_quantity(0), build_time(0) {
    search_time.clear();
    accumulate_time.clear();
}

TestResult::~TestResult() {
    search_time.clear();
    accumulate_time.clear();
}

void TestResult::reorder() {
    if (search_time.size() > 0) {
        std::sort(search_time.begin(), search_time.end());
        accumulate_time.clear();
        accumulate_time.push_back(0);
        for (size_t i = 0; i < search_time.size(); ++i) {
            accumulate_time.push_back(accumulate_time.back() + search_time[i]);
        }
    }
}

double TestResult::get_total(int begin, int end) {
    if (begin >= 0 && end < accumulate_time.size() && begin < end) {
        return accumulate_time[end] - accumulate_time[begin];
    }
    return 0;
}