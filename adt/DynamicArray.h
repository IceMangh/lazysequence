#pragma once
#include <stdexcept>
#include "Exceptions.h"
#include "IEnumerator.h"

template <class T>
class DynamicArray {
private:
    T* data_;
    int size_;

    void CheckIndex(int index) const {
        if (index < 0 || index >= size_) {
            throw IndexOutOfRange();
        }
    }

public:
    class Enumerator : public IEnumerator<T> {
    private:
        const T* items_;
        int length_;
        int index_;

    public:
        Enumerator(const T* items, int length) : items_(items), length_(length), index_(-1) {}

        bool MoveNext() override {
            if (index_ < length_) {
                ++index_;
            }
            return index_ < length_;
        }

        T Current() const override {
            return items_[index_];
        }
    };

    DynamicArray() : data_(nullptr), size_(0) {}

    explicit DynamicArray(int size) : data_(nullptr), size_(size) {
        if (size < 0) {
            throw std::invalid_argument("Negative size");
        }
        if (size_ > 0) {
            data_ = new T[size_]{};
        }
    }

    DynamicArray(const T* items, int count) : data_(nullptr), size_(count) {
        if (count < 0) {
            throw std::invalid_argument("Negative size");
        }
        if (size_ > 0) {
            data_ = new T[size_];
            for (int i = 0; i < size_; ++i) {
                data_[i] = items[i];
            }
        }
    }

    DynamicArray(const DynamicArray<T>& other) : data_(nullptr), size_(other.size_) {
        if (size_ > 0) {
            data_ = new T[size_];
            for (int i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
        }
    }

    DynamicArray<T>& operator=(const DynamicArray<T>& other) {
        if (this != &other) {
            T* newData = nullptr;
            if (other.size_ > 0) {
                newData = new T[other.size_];
                for (int i = 0; i < other.size_; ++i) {
                    newData[i] = other.data_[i];
                }
            }
            delete[] data_;
            data_ = newData;
            size_ = other.size_;
        }
        return *this;
    }

    ~DynamicArray() {
        delete[] data_;
    }

    const T& Get(int index) const {
        CheckIndex(index);
        return data_[index];
    }


    int GetLength() const {
        return size_;
    }

    int GetSize() const {
        return GetLength();
    }

    IEnumerator<T>* GetEnumerator() const {
        return new Enumerator(data_, size_);
    }

    void Set(int index, const T& value) {
        CheckIndex(index);
        data_[index] = value;
    }

    void Resize(int newSize) {
        if (newSize < 0) {
            throw std::invalid_argument("Negative size");
        }

        T* newData = nullptr;
        if (newSize > 0) {
            newData = new T[newSize]{};
            int copySize = (newSize < size_) ? newSize : size_;
            for (int i = 0; i < copySize; ++i) {
                newData[i] = data_[i];
            }
        }

        delete[] data_;
        data_ = newData;
        size_ = newSize;
    }

    T& operator[](int index) {
        CheckIndex(index);
        return data_[index];
    }

};

