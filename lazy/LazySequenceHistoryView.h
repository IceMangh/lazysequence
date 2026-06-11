#pragma once

#include <stdexcept>

#include "Exceptions.h"
#include "MutableArraySequence.h"
#include "Sequence.h"

template <class T>
class LazySequence;

template <class T>
class LazySequenceHistoryView : public Sequence<T> {
private:
    const LazySequence<T>* source_;
    int length_;

    class Enumerator : public IEnumerator<T> {
    private:
        const LazySequenceHistoryView<T>* view_;
        int index_;

    public:
        explicit Enumerator(const LazySequenceHistoryView<T>* view) : view_(view), index_(-1) {}

        bool MoveNext() override {
            if (index_ < view_->length_) {
                ++index_;
            }
            return index_ < view_->length_;
        }

        T Current() const override {
            return view_->Get(index_);
        }
    };

public:
    LazySequenceHistoryView(const LazySequence<T>* source, int length)
            : source_(source), length_(length) {}

    const T& GetFirst() const override {
        if (length_ == 0) {
            throw EmptyStructure("empty history");
        }
        return Get(0);
    }

    const T& GetLast() const override {
        if (length_ == 0) {
            throw EmptyStructure("empty history");
        }
        return Get(length_ - 1);
    }

    const T& Get(int index) const override {
        if (index < 0 || index >= length_) {
            throw IndexOutOfRange();
        }
        return source_->Get(index);
    }

    int GetLength() const override {
        return length_;
    }

    Sequence<T>* GetSubsequence(int startIndex, int endIndex) const override {
        if (startIndex < 0 || endIndex < 0 || startIndex > endIndex || endIndex >= length_) {
            throw IndexOutOfRange();
        }

        auto* result = new MutableArraySequence<T>();
        try {
            for (int i = startIndex; i <= endIndex; ++i) {
                result->Append(Get(i));
            }
            return result;
        } catch (...) {
            delete result;
            throw;
        }
    }

    Sequence<T>* Append(const T&) override {
        throw std::logic_error("read only");
    }

    Sequence<T>* Prepend(const T&) override {
        throw std::logic_error("read only");
    }

    Sequence<T>* InsertAt(const T&, int) override {
        throw std::logic_error("read only");
    }

    Sequence<T>* Concat(const Sequence<T>& other) const override {
        auto* result = new MutableArraySequence<T>();
        IEnumerator<T>* enumerator = other.GetEnumerator();
        try {
            for (int i = 0; i < length_; ++i) {
                result->Append(Get(i));
            }
            while (enumerator->MoveNext()) {
                result->Append(enumerator->Current());
            }
            delete enumerator;
            return result;
        } catch (...) {
            delete enumerator;
            delete result;
            throw;
        }
    }

    Sequence<T>* Clone() const override {
        auto* result = new MutableArraySequence<T>();
        try {
            for (int i = 0; i < length_; ++i) {
                result->Append(Get(i));
            }
            return result;
        } catch (...) {
            delete result;
            throw;
        }
    }

    Sequence<T>* CreateEmpty() const override {
        return new MutableArraySequence<T>();
    }

    IEnumerator<T>* GetEnumerator() const override {
        return new Enumerator(this);
    }
};
