#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>

#include "Ordinal.h"
#include "Exceptions.h"
#include "Sequence.h"

template <class T>
class LazySequence;

template <class T>
class Generator {
public:
    Generator(LazySequence<T>* owner, Ordinal length, std::function<T(const Ordinal&)> provider);
    Generator(LazySequence<T>* owner, Ordinal length, std::function<T(Sequence<T>*)> rule);
    Generator(const Generator<T>& other, LazySequence<T>* owner, std::size_t nextIndex);

    T GetNext();
    T Get(const Ordinal& index) const;

private:
    LazySequence<T>* owner_;
    Ordinal lengthOrdinal_;

    std::function<T(const Ordinal&)> provider_;
    std::function<T(Sequence<T>*)> recurrentRule_;
    std::size_t nextIndex_;

    bool HasNext() const;
    T GenerateAt(const Ordinal& index) const;
};

template <class T>
Generator<T>::Generator(LazySequence<T>* owner, Ordinal length, std::function<T(const Ordinal&)> provider)
    : owner_(owner),
      lengthOrdinal_(length),
      provider_(provider),
      recurrentRule_(nullptr),
      nextIndex_(0) {}

template <class T>
Generator<T>::Generator(LazySequence<T>* owner, Ordinal length, std::function<T(Sequence<T>*)> rule)
    : owner_(owner),
      lengthOrdinal_(length),
      provider_(nullptr),
      recurrentRule_(rule),
      nextIndex_(0) {}

template <class T>
Generator<T>::Generator(const Generator<T>& other, LazySequence<T>* owner, std::size_t nextIndex)
    : owner_(owner),
      lengthOrdinal_(other.lengthOrdinal_),
      provider_(other.provider_),
      recurrentRule_(other.recurrentRule_),
      nextIndex_(nextIndex) {}

template <class T>
bool Generator<T>::HasNext() const {
    if (!provider_ && !recurrentRule_ && owner_ == nullptr) {
        return false;
    }
    return lengthOrdinal_.IsFinite() == false || nextIndex_ < lengthOrdinal_.FiniteValue();
}

template <class T>
T Generator<T>::GetNext() {
    if (!HasNext()) {
        throw IndexOutOfRange();
    }

    T value = GenerateAt(Ordinal::Finite(nextIndex_));
    ++nextIndex_;
    return value;
}

template <class T>
T Generator<T>::Get(const Ordinal& index) const {
    return GenerateAt(index);
}

template <class T>
T Generator<T>::GenerateAt(const Ordinal& index) const {
    if (provider_) {
        return provider_(index);
    }
    if (recurrentRule_) {
        if (owner_ == nullptr) {
            throw std::logic_error("no owner");
        }
        if (!index.IsFinite() ||
            index.FiniteValue() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw IndexOutOfRange();
        }

        auto history = owner_->CreateHistoryView();
        return recurrentRule_(&history);
    }
    if (owner_ != nullptr) {
        return owner_->Get(index);
    }

    throw IndexOutOfRange();
}
