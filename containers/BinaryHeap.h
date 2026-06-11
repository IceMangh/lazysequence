#pragma once

#include <stdexcept>

#include "DynamicArray.h"

namespace online_statistics_detail {

class BinaryHeap {
private:
    DynamicArray<double> data_;
    int size_;
    bool minHeap_;

    bool HasHigherPriority(double left, double right) const {
        return minHeap_ ? left < right : left > right;
    }

    void EnsureCapacity() {
        if (size_ < data_.GetLength()) {
            return;
        }

        const int newCapacity = data_.GetLength() == 0 ? 8 : data_.GetLength() * 2;
        data_.Resize(newCapacity);
    }

    void Swap(int left, int right) {
        const double temp = data_.Get(left);
        data_.Set(left, data_.Get(right));
        data_.Set(right, temp);
    }

public:
    explicit BinaryHeap(bool minHeap) : data_(), size_(0), minHeap_(minHeap) {}

    int GetSize() const {
        return size_;
    }

    bool IsEmpty() const {
        return size_ == 0;
    }

    double Top() const {
        if (IsEmpty()) {
            throw std::logic_error("empty heap");
        }
        return data_.Get(0);
    }

    void Push(double value) {
        EnsureCapacity();
        data_.Set(size_, value);

        int index = size_;
        ++size_;
        while (index > 0) {
            const int parent = (index - 1) / 2;
            if (!HasHigherPriority(data_.Get(index), data_.Get(parent))) {
                break;
            }
            Swap(index, parent);
            index = parent;
        }
    }

    double Pop() {
        const double result = Top();
        --size_;

        if (size_ > 0) {
            data_.Set(0, data_.Get(size_));

            int index = 0;
            while (true) {
                const int left = index * 2 + 1;
                const int right = left + 1;
                int bestIndex = index;

                if (left < size_ && HasHigherPriority(data_.Get(left), data_.Get(bestIndex))) {
                    bestIndex = left;
                }
                if (right < size_ && HasHigherPriority(data_.Get(right), data_.Get(bestIndex))) {
                    bestIndex = right;
                }
                if (bestIndex == index) {
                    break;
                }
                Swap(index, bestIndex);
                index = bestIndex;
            }
        }

        return result;
    }
};

}
