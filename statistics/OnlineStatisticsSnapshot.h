#pragma once

#include <cstddef>

struct OnlineStatisticsSnapshot {
    std::size_t count;
    double sum;
    double min;
    double max;
    double average;
    double median;
};
