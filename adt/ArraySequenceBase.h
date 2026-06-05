#pragma once

#include "DynamicArray.h"
#include "Exceptions.h"
#include "Sequence.h"

template <class T>
class ArraySequenceBase : public Sequence<T> {
protected:
    DynamicArray<T> data_;

    virtual ArraySequenceBase<T>* Instance() = 0;

    Sequence<T>* AppendInternal(const T& item) {
        const int oldLength = data_.GetLength();
        data_.Resize(oldLength + 1);
        data_.Set(oldLength, item);
        return this;
    }

    Sequence<T>* PrependInternal(const T& item) {
        const int oldLength = data_.GetLength();
        data_.Resize(oldLength + 1);

        for (int i = oldLength; i > 0; --i) {
            data_.Set(i, data_.Get(i - 1));
        }
        data_.Set(0, item);
        return this;
    }

    Sequence<T>* InsertAtInternal(const T& item, int index) {
        const int length = data_.GetLength();
        if (index < 0 || index > length) {
            throw IndexOutOfRange();
        }

        if (index == length) {
            return AppendInternal(item);
        }

        data_.Resize(length + 1);
        for (int i = length; i > index; --i) {
            data_.Set(i, data_.Get(i - 1));
        }
        data_.Set(index, item);
        return this;
    }

public:
    ArraySequenceBase() : data_(0) {}

    ArraySequenceBase(const T* items, int count) : data_(items, count) {}

    ArraySequenceBase(const DynamicArray<T>& dynamicArray) : data_(dynamicArray) {}

    ArraySequenceBase(const ArraySequenceBase<T>& other) : data_(other.data_) {}

    const T& GetFirst() const override {
        if (data_.GetLength() == 0) {
            throw EmptyStructure();
        }
        return data_.Get(0);
    }

    const T& GetLast() const override {
        if (data_.GetLength() == 0) {
            throw EmptyStructure();
        }
        return data_.Get(data_.GetLength() - 1);
    }

    const T& Get(int index) const override {
        return data_.Get(index);
    }

    Sequence<T>* GetSubsequence(int startIndex, int endIndex) const override {
        if (startIndex < 0 || endIndex < 0 || startIndex > endIndex || endIndex >= data_.GetLength()) {
            throw IndexOutOfRange();
        }

        Sequence<T>* result = this->CreateEmpty();

        try {
            for (int i = startIndex; i <= endIndex; ++i) {
                this->AppendToResult(result, data_.Get(i));
            }
            return result;
        } catch (...) {
            delete result;
            throw;
        }
    }

    int GetLength() const override {
        return data_.GetLength();
    }

    IEnumerator<T>* GetEnumerator() const override {
        return data_.GetEnumerator();
    }

    Sequence<T>* Append(const T& item) override {
        ArraySequenceBase<T>* target = Instance();

        try {
            return target->AppendInternal(item);
        } catch (...) {
            if (target != this) {
                delete target;
            }
            throw;
        }
    }

    Sequence<T>* Prepend(const T& item) override {
        ArraySequenceBase<T>* target = Instance();

        try {
            return target->PrependInternal(item);
        } catch (...) {
            if (target != this) {
                delete target;
            }
            throw;
        }
    }

    Sequence<T>* InsertAt(const T& item, int index) override {
        ArraySequenceBase<T>* target = Instance();

        try {
            return target->InsertAtInternal(item, index);
        } catch (...) {
            if (target != this) {
                delete target;
            }
            throw;
        }
    }

    Sequence<T>* Concat(const Sequence<T>& list) const override {
        Sequence<T>* result = this->Clone();
        IEnumerator<T>* enumerator = list.GetEnumerator();

        try {
            while (enumerator->MoveNext()) {
                this->AppendToResult(result, enumerator->Current());
            }
            delete enumerator;
            return result;
        } catch (...) {
            delete enumerator;
            delete result;
            throw;
        }
    }
};
