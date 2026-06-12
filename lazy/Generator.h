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

    std::function<T(const Ordinal&, Sequence<T>*)> rule_;
    std::size_t nextIndex_;

    bool HasNext() const;
    T GenerateAt(const Ordinal& index) const;
};

template <class T>
Generator<T>::Generator(LazySequence<T>* owner, Ordinal length, std::function<T(const Ordinal&)> provider)
    : owner_(owner),
      lengthOrdinal_(length),
      rule_([provider](const Ordinal& index, Sequence<T>*) {
          return provider(index);
      }),
      nextIndex_(0) {}

template <class T>
Generator<T>::Generator(LazySequence<T>* owner, Ordinal length, std::function<T(Sequence<T>*)> rule)
    : owner_(owner),
      lengthOrdinal_(length),
      rule_([rule](const Ordinal& index, Sequence<T>* history) {
          if (history == nullptr) {
              throw std::logic_error("no owner");
          }
          if (!index.IsFinite() ||
              index.FiniteValue() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
              throw IndexOutOfRange();
          }
          return rule(history);
      }),
      nextIndex_(0) {}

template <class T>
Generator<T>::Generator(const Generator<T>& other, LazySequence<T>* owner, std::size_t nextIndex)
    : owner_(owner),
      lengthOrdinal_(other.lengthOrdinal_),
      rule_(other.rule_),
      nextIndex_(nextIndex) {}

template <class T>
bool Generator<T>::HasNext() const {
    if (!rule_ && owner_ == nullptr) {
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
    if (rule_) {
        if (owner_ == nullptr) {
            return rule_(index, nullptr);
        }

        auto history = owner_->CreateHistoryView();
        return rule_(index, &history);
    }
    if (owner_ != nullptr) {
        return owner_->Get(index);
    }

    throw IndexOutOfRange();
}
