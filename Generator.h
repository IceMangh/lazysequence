#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
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

    Ordinal GetLengthOrdinal() const;
    std::size_t GetPosition() const;
    void Reset(std::size_t nextIndex = 0);

    bool HasNext() const;
    T GetNext();
    T Get(const Ordinal& index) const;
    std::optional<T> TryGetNext();

    Generator<T> Append(T item) const;
    Generator<T> Append(Sequence<T>* items) const;
    Generator<T> Insert(T item) const;
    Generator<T> Insert(Sequence<T>* items) const;
    Generator<T> Remove(T item) const;
    Generator<T> Remove(Sequence<T>* items) const;

private:
    LazySequence<T>* owner_;
    Ordinal lengthOrdinal_;

    std::function<T(const Ordinal&)> provider_;
    std::function<T(Sequence<T>*)> recurrentRule_;
    std::size_t nextIndex_;

    static const Sequence<T>& RequireSequence(const Sequence<T>* items);
    static Ordinal AddLength(const Ordinal& left, const Ordinal& right);
    Ordinal RemoveLength(Ordinal length) const;

    std::shared_ptr<const LazySequence<T>> CopyOwner() const;
    std::shared_ptr<const LazySequence<T>> MakeLazyCopy(const Sequence<T>& items) const;
    Generator<T> Create(Ordinal length, std::function<T(const Ordinal&)> provider) const;
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

    T value = GenerateAt(Ordinal::Finite(nextIndex_));
    ++nextIndex_;
    return value;
}

template <class T>
T Generator<T>::Get(const Ordinal& index) const {
    return GenerateAt(index);
}

template <class T>
std::optional<T> Generator<T>::TryGetNext() {
    if (!HasNext()) {
        return std::nullopt;
    }
    return GetNext();
}

template <class T>
Generator<T> Generator<T>::Append(T item) const {
    auto source = CopyOwner();
    const Ordinal newLength = AddLength(lengthOrdinal_, Ordinal::Finite(1));

    if (lengthOrdinal_.IsFinite() == false) {
        return Create(
            newLength,
            [source](const Ordinal& index) {
                return source->Get(index);
            });
    }

    const std::size_t oldLength = lengthOrdinal_.FiniteValue();
    return Create(
        newLength,
        [source, item, oldLength](const Ordinal& index) {
            if (index == Ordinal::Finite(oldLength)) {
                return item;
            }
            return source->Get(index);
        });
}

template <class T>
Generator<T> Generator<T>::Append(Sequence<T>* items) const {
    const Sequence<T>& sequence = RequireSequence(items);
    auto source = CopyOwner();
    auto suffix = MakeLazyCopy(sequence);
    const Ordinal suffixLength = suffix->GetLengthOrdinal();
    const Ordinal newLength = AddLength(lengthOrdinal_, suffixLength);

    if (lengthOrdinal_.IsFinite() == false) {
        return Create(
            newLength,
            [source](const Ordinal& index) {
                return source->Get(index);
            });
    }

    const std::size_t oldLength = lengthOrdinal_.FiniteValue();
    return Create(
        newLength,
        [source, suffix, oldLength](const Ordinal& index) {
            const Ordinal oldLengthOrdinal = Ordinal::Finite(oldLength);
            if (index < oldLengthOrdinal) {
                return source->Get(index);
            }
            return suffix->Get(index.SubtractPrefix(oldLengthOrdinal));
        });
}

template <class T>
Generator<T> Generator<T>::Insert(T item) const {
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
        [source, item, insertIndex](const Ordinal& index) {
            const Ordinal insertOrdinal = Ordinal::Finite(insertIndex);

            if (index < insertOrdinal) {
                return source->Get(index);
            }
            if (index == insertOrdinal) {
                return item;
            }
            const std::optional<Ordinal> previous = index.Predecessor();
            if (!previous.has_value()) {
                throw IndexOutOfRange();
            }
            return source->Get(*previous);
        });
}

template <class T>
Generator<T> Generator<T>::Insert(Sequence<T>* items) const {
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
        [source, inserted, insertedLength, insertIndex](const Ordinal& index) {
            const Ordinal insertOrdinal = Ordinal::Finite(insertIndex);

            if (index < insertOrdinal) {
                return source->Get(index);
            }
            const Ordinal insertedIndex = index.SubtractPrefix(insertOrdinal);
            if (insertedLength.IsFinite() == false) {
                return inserted->Get(insertedIndex);
            }

            if (insertedIndex < insertedLength) {
                return inserted->Get(insertedIndex);
            }
            return source->Get(index.SubtractPrefix(Ordinal::Finite(insertedLength.FiniteValue())));
        });
}

template <class T>
Generator<T> Generator<T>::Remove(T) const {
    if (lengthOrdinal_.IsFinite() && nextIndex_ >= lengthOrdinal_.FiniteValue()) {
        throw IndexOutOfRange();
    }

    auto source = CopyOwner();
    const std::size_t removeIndex = nextIndex_;
    const Ordinal newLength = RemoveLength(Ordinal::Finite(1));

    return Create(
        newLength,
        [source, removeIndex](const Ordinal& index) {
            const Ordinal removeOrdinal = Ordinal::Finite(removeIndex);
            if (index < removeOrdinal) {
                return source->Get(index);
            }
            return source->Get(index.Successor());
        });
}

template <class T>
Generator<T> Generator<T>::Remove(Sequence<T>* items) const {
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
        [source, removedLength, removeIndex](const Ordinal& index) {
            const Ordinal removeOrdinal = Ordinal::Finite(removeIndex);
            if (index < removeOrdinal || removedLength.IsFinite() == false) {
                return source->Get(index);
            }

            return source->Get(index.Add(removedLength));
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
Generator<T> Generator<T>::Create(Ordinal length, std::function<T(const Ordinal&)> provider) const {
    Generator<T> result(*this, owner_, nextIndex_);
    result.lengthOrdinal_ = length;
    result.provider_ = provider;
    result.recurrentRule_ = nullptr;
    return result;
}

template <class T>
T Generator<T>::GenerateAt(const Ordinal& index) const {
    if (provider_) {
        return provider_(index);
    }
    if (recurrentRule_) {
        if (owner_ == nullptr) {
            throw std::logic_error("Generator has no owner");
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
