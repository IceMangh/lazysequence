#pragma once

#include "ArraySequenceBase.h"

template <class T>
class MutableArraySequence : public ArraySequenceBase<T> {
protected:
    ArraySequenceBase<T>* Instance() override {
        return this;
    }

public:
    MutableArraySequence() : ArraySequenceBase<T>() {}

    MutableArraySequence(const T* items, int count) : ArraySequenceBase<T>(items, count) {}

    MutableArraySequence(const DynamicArray<T>& dynamicArray) : ArraySequenceBase<T>(dynamicArray) {}

    MutableArraySequence(const MutableArraySequence<T>& other) : ArraySequenceBase<T>(other) {}

    Sequence<T>* Clone() const override {
        return new MutableArraySequence<T>(*this);
    }

    Sequence<T>* CreateEmpty() const override {
        return new MutableArraySequence<T>();
    }

    static MutableArraySequence<T> From(const T* items, int count) {
        return MutableArraySequence<T>(items, count);
    }
};
