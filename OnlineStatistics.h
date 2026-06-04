#pragma once

#include <cstddef>
#include <limits>
#include <stdexcept>

#include "BinaryHeap.h"
#include "OnlineStatisticsSnapshot.h"
#include "ReadOnlyStream.h"

class OnlineStatistics {
private:
    online_statistics_detail::BinaryHeap lower_;
    online_statistics_detail::BinaryHeap upper_;
    std::size_t count_;
    double sum_;
    double min_;
    double max_;

    void Rebalance() {
        if (lower_.GetSize() > upper_.GetSize() + 1) {
            upper_.Push(lower_.Pop());
        } else if (upper_.GetSize() > lower_.GetSize()) {
            lower_.Push(upper_.Pop());
        }
    }

public:
    OnlineStatistics()
        : lower_(false),
          upper_(true),
          count_(0),
          sum_(0.0),
          min_(std::numeric_limits<double>::infinity()),
          max_(-std::numeric_limits<double>::infinity()) {}

    void Add(double value) {
        if (lower_.IsEmpty() || value <= lower_.Top()) {
            lower_.Push(value);
        } else {
            upper_.Push(value);
        }

        Rebalance();
        ++count_;
        sum_ += value;
        if (value < min_) {
            min_ = value;
        }
        if (value > max_) {
            max_ = value;
        }
    }

    bool IsEmpty() const {
        return count_ == 0;
    }

    OnlineStatisticsSnapshot Snapshot() const {
        if (IsEmpty()) {
            throw std::logic_error("Statistics is empty");
        }

        double median = lower_.Top();
        if (lower_.GetSize() == upper_.GetSize()) {
            median = (lower_.Top() + upper_.Top()) / 2.0;
        }

        return OnlineStatisticsSnapshot{
            count_,
            sum_,
            min_,
            max_,
            sum_ / static_cast<double>(count_),
            median
        };
    }
};

inline OnlineStatistics CollectStatistics(ReadOnlyStream<double>& stream, std::size_t limit) {
    OnlineStatistics statistics;

    std::size_t read = 0;
    while (read < limit) {
        try {
            statistics.Add(stream.Read());
            ++read;
        } catch (const EndOfStream&) {
            break;
        }
    }

    return statistics;
}
