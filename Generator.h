#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

#include "Ordinal.h"
#include "Exceptions.h"
#include "Sequence.h"

template <class T>
class LazySequence;

template <class T>
class Generator {
public:
    using Provider = std::function<T(int)>;
    using Rule = std::function<T(Sequence<T>*)>;

    Generator(LazySequence<T>* owner, Ordinal length, Provider provider);
    Generator(LazySequence<T>* owner, Ordinal length, Rule rule);
    Generator(const Generator<T>& other, LazySequence<T>* owner, std::size_t nextIndex);

    Ordinal GetLengthOrdinal() const;
    std::size_t GetPosition() const;
    void Reset(std::size_t nextIndex = 0);

    bool HasNext() const;
    T GetNext();
    std::optional<T> TryGetNext();

    Generator<T>* Append(T item) const;
    Generator<T>* Append(Sequence<T>* items) const;
    Generator<T>* Insert(T item) const;
    Generator<T>* Insert(Sequence<T>* items) const;
    Generator<T>* Remove(T item) const;
    Generator<T>* Remove(Sequence<T>* items) const;

private:
    LazySequence<T>* owner_;
    Ordinal lengthOrdinal_;

    Provider provider_;
    Rule recurrentRule_;
    std::size_t nextIndex_;

    static const Sequence<T>& RequireSequence(const Sequence<T>* items);
    static Ordinal AddLength(const Ordinal& left, const Ordinal& right);
    Ordinal RemoveLength(Ordinal length) const;

    std::shared_ptr<const LazySequence<T>> CopyOwner() const;
    std::shared_ptr<const LazySequence<T>> MakeLazyCopy(const Sequence<T>& items) const;
    Generator<T>* Create(Ordinal length, Provider provider) const;
    T GenerateAt(std::size_t index) const;
};

template <class T>
Generator<T>::Generator(LazySequence<T>* owner, Ordinal length, Provider provider)
    : owner_(owner),
      lengthOrdinal_(length),
      provider_(std::move(provider)),
      recurrentRule_(nullptr),
      nextIndex_(0) {}

template <class T>
Generator<T>::Generator(LazySequence<T>* owner, Ordinal length, Rule rule)
    : owner_(owner),
      lengthOrdinal_(length),
      provider_(nullptr),
      recurrentRule_(std::move(rule)),
      nextIndex_(0) {}

template <class T>
Generator<T>::Generator(const Generator<T>& other, LazySequence<T>* owner, std::size_t nextIndex)
    : owner_(owner),
      lengthOrdinal_(other.lengthOrdinal_),
      provider_(other.provider_),
      recurrentRule_(other.recurrentRule_),
      nextIndex_(nextIndex) {}

template <class T>
Ordinal Generator<T>::GetLengthOrdinal() const {
    return lengthOrdinal_;
}

template <class T>
std::size_t Generator<T>::GetPosition() const {
    return nextIndex_;
}

template <class T>
void Generator<T>::Reset(std::size_t nextIndex) {
    nextIndex_ = nextIndex;
}

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

    T value = GenerateAt(nextIndex_);
    ++nextIndex_;
    return value;
}

template <class T>
std::optional<T> Generator<T>::TryGetNext() {
    if (!HasNext()) {
        return std::nullopt;
    }
    return GetNext();
}

template <class T>
Generator<T>* Generator<T>::Append(T item) const {
    auto source = CopyOwner();
    const Ordinal newLength = AddLength(lengthOrdinal_, Ordinal::Finite(1));

    if (lengthOrdinal_.IsFinite() == false) {
        return Create(
            newLength,
            [source](int index) {
                return source->Get(index);
            });
    }

    const std::size_t oldLength = lengthOrdinal_.FiniteValue();
    return Create(
        newLength,
        [source, item = std::move(item), oldLength](int index) {
            if (static_cast<std::size_t>(index) == oldLength) {
                return item;
            }
            return source->Get(index);
        });
}

template <class T>
Generator<T>* Generator<T>::Append(Sequence<T>* items) const {
    const Sequence<T>& sequence = RequireSequence(items);
    auto source = CopyOwner();
    auto suffix = MakeLazyCopy(sequence);
    const Ordinal suffixLength = suffix->GetLengthOrdinal();
    const Ordinal newLength = AddLength(lengthOrdinal_, suffixLength);

    if (lengthOrdinal_.IsFinite() == false) {
        return Create(
            newLength,
            [source](int index) {
                return source->Get(index);
            });
    }

    const std::size_t oldLength = lengthOrdinal_.FiniteValue();
    return Create(
        newLength,
        [source, suffix, oldLength](int index) {
            const std::size_t current = static_cast<std::size_t>(index);
            if (current < oldLength) {
                return source->Get(index);
            }
            return suffix->Get(static_cast<int>(current - oldLength));
        });
}

template <class T>
Generator<T>* Generator<T>::Insert(T item) const {
    if (lengthOrdinal_.IsFinite() && nextIndex_ > lengthOrdinal_.FiniteValue()) {
        throw IndexOutOfRange();
    }

    auto source = CopyOwner();
    const std::size_t insertIndex = nextIndex_;
    const Ordinal newLength = lengthOrdinal_.IsFinite()
                                  ? AddLength(lengthOrdinal_, Ordinal::Finite(1))
                                  : lengthOrdinal_;

    return Create(
        newLength,
        [source, item = std::move(item), insertIndex](int index) {
            const std::size_t current = static_cast<std::size_t>(index);

            if (current < insertIndex) {
                return source->Get(index);
            }
            if (current == insertIndex) {
                return item;
            }
            return source->Get(index - 1);
        });
}

template <class T>
Generator<T>* Generator<T>::Insert(Sequence<T>* items) const {
    if (lengthOrdinal_.IsFinite() && nextIndex_ > lengthOrdinal_.FiniteValue()) {
        throw IndexOutOfRange();
    }

    const Sequence<T>& sequence = RequireSequence(items);
    auto source = CopyOwner();
    auto inserted = MakeLazyCopy(sequence);
    const Ordinal insertedLength = inserted->GetLengthOrdinal();
    const Ordinal newLength = lengthOrdinal_.IsFinite()
                                  ? AddLength(lengthOrdinal_, insertedLength)
                                  : lengthOrdinal_;
    const std::size_t insertIndex = nextIndex_;

    return Create(
        newLength,
        [source, inserted, insertedLength, insertIndex](int index) {
            const std::size_t current = static_cast<std::size_t>(index);

            if (current < insertIndex) {
                return source->Get(index);
            }
            if (insertedLength.IsFinite() == false) {
                return inserted->Get(static_cast<int>(current - insertIndex));
            }

            const std::size_t length = insertedLength.FiniteValue();
            if (current - insertIndex < length) {
                return inserted->Get(static_cast<int>(current - insertIndex));
            }
            return source->Get(static_cast<int>(current - length));
        });
}

template <class T>
Generator<T>* Generator<T>::Remove(T) const {
    if (lengthOrdinal_.IsFinite() && nextIndex_ >= lengthOrdinal_.FiniteValue()) {
        throw IndexOutOfRange();
    }

    auto source = CopyOwner();
    const std::size_t removeIndex = nextIndex_;
    const Ordinal newLength = RemoveLength(Ordinal::Finite(1));

    return Create(
        newLength,
        [source, removeIndex](int index) {
            const std::size_t current = static_cast<std::size_t>(index);
            if (current < removeIndex) {
                return source->Get(index);
            }
            return source->Get(index + 1);
        });
}

template <class T>
Generator<T>* Generator<T>::Remove(Sequence<T>* items) const {
    if (lengthOrdinal_.IsFinite() && nextIndex_ >= lengthOrdinal_.FiniteValue()) {
        throw IndexOutOfRange();
    }

    const Sequence<T>& sequence = RequireSequence(items);
    auto source = CopyOwner();
    auto removed = MakeLazyCopy(sequence);
    const Ordinal removedLength = removed->GetLengthOrdinal();
    const Ordinal newLength = RemoveLength(removedLength);
    const std::size_t removeIndex = nextIndex_;

    return Create(
        newLength,
        [source, removedLength, removeIndex](int index) {
            const std::size_t current = static_cast<std::size_t>(index);
            if (current < removeIndex || removedLength.IsFinite() == false) {
                return source->Get(index);
            }

            return source->Get(static_cast<int>(current + removedLength.FiniteValue()));
        });
}

template <class T>
const Sequence<T>& Generator<T>::RequireSequence(const Sequence<T>* items) {
    if (items == nullptr) {
        throw std::invalid_argument("Sequence pointer is null");
    }
    return *items;
}

template <class T>
Ordinal Generator<T>::AddLength(const Ordinal& left, const Ordinal& right) {
    return left.Add(right);
}

template <class T>
Ordinal Generator<T>::RemoveLength(Ordinal length) const {
    if (length.IsFinite() && length.FiniteValue() == 0) {
        return lengthOrdinal_;
    }

    if (lengthOrdinal_.IsFinite() == false) {
        if (length.IsFinite() == false) {
            return Ordinal::Finite(nextIndex_);
        }
        return lengthOrdinal_;
    }

    const std::size_t currentLength = lengthOrdinal_.FiniteValue();
    if (length.IsFinite() == false || nextIndex_ > currentLength || length.FiniteValue() > currentLength - nextIndex_) {
        throw IndexOutOfRange();
    }

    return Ordinal::Finite(currentLength - length.FiniteValue());
}

template <class T>
std::shared_ptr<const LazySequence<T>> Generator<T>::CopyOwner() const {
    if (owner_ == nullptr) {
        throw std::logic_error("Generator has no owner");
    }
    return std::make_shared<LazySequence<T>>(*owner_);
}

template <class T>
std::shared_ptr<const LazySequence<T>> Generator<T>::MakeLazyCopy(const Sequence<T>& items) const {
    if (const auto* lazy = dynamic_cast<const LazySequence<T>*>(&items)) {
        return std::make_shared<LazySequence<T>>(*lazy);
    }
    return std::make_shared<LazySequence<T>>(items);
}

template <class T>
Generator<T>* Generator<T>::Create(Ordinal length, Provider provider) const {
    auto* result = new Generator<T>(*this, owner_, nextIndex_);
    result->lengthOrdinal_ = length;
    result->provider_ = std::move(provider);
    result->recurrentRule_ = nullptr;
    return result;
}

template <class T>
T Generator<T>::GenerateAt(std::size_t index) const {
    if (index > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw IndexOutOfRange();
    }

    const int intIndex = static_cast<int>(index);
    if (provider_) {
        return provider_(intIndex);
    }
    if (recurrentRule_) {
        if (owner_ == nullptr) {
            throw std::logic_error("Generator has no owner");
        }

        auto history = owner_->CreateHistoryView();
        return recurrentRule_(&history);
    }
    if (owner_ != nullptr) {
        return owner_->Get(intIndex);
    }

    throw IndexOutOfRange();
}
