#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

#include "DynamicArray.h"
#include "DynamicArraySequenceView.h"
#include "Exceptions.h"
#include "Generator.h"
#include "MutableArraySequence.h"
#include "Ordinal.h"
#include "Sequence.h"

template <class T>
class LazySequence : public Sequence<T> {
private:
    friend class Generator<T>;

    using Provider = typename Generator<T>::Provider;
    using Rule = typename Generator<T>::Rule;

    mutable DynamicArray<T> cache_;
    mutable int cacheLength_;
    Ordinal lengthOrdinal_;

    Generator<T>* generator_;
    std::shared_ptr<const LazySequence<T>> concatLeft_;
    std::shared_ptr<const LazySequence<T>> concatRight_;

    class Enumerator : public IEnumerator<T> {
    private:
        const LazySequence<T>* sequence_;
        int index_;

    public:
        explicit Enumerator(const LazySequence<T>* sequence) : sequence_(sequence), index_(-1) {}

        bool MoveNext() override {
            if (index_ == std::numeric_limits<int>::max()) {
                return false;
            }
            if (sequence_->lengthOrdinal_.IsFinite() &&
                static_cast<std::size_t>(index_ + 1) >= sequence_->lengthOrdinal_.FiniteValue()) {
                return false;
            }

            ++index_;
            return true;
        }

        T Current() const override {
            if (index_ < 0) {
                throw std::logic_error("Enumerator is not positioned on an item");
            }
            return sequence_->Get(index_);
        }
    };

    LazySequence(std::shared_ptr<const LazySequence<T>> left,
                 std::shared_ptr<const LazySequence<T>> right)
            : cache_(),
              cacheLength_(0),
              lengthOrdinal_(left->lengthOrdinal_.Add(right->lengthOrdinal_)),
              generator_(nullptr),
              concatLeft_(std::move(left)),
              concatRight_(std::move(right)) {}

    std::shared_ptr<const LazySequence<T>> SharedCopy() const {
        return std::make_shared<LazySequence<T>>(*this);
    }

    static std::shared_ptr<const LazySequence<T>> CopyAsLazy(const Sequence<T>& sequence) {
        if (const auto* lazy = dynamic_cast<const LazySequence<T>*>(&sequence)) {
            return std::make_shared<LazySequence<T>>(*lazy);
        }
        return std::make_shared<LazySequence<T>>(sequence);
    }

    static const Sequence<T>& RequireSequence(const Sequence<T>* sequence) {
        if (sequence == nullptr) {
            throw std::invalid_argument("Sequence pointer is null");
        }
        return *sequence;
    }

    void AppendToCache(const T& item) const {
        if (cacheLength_ == cache_.GetLength()) {
            const int newCapacity = cache_.GetLength() == 0 ? 8 : cache_.GetLength() * 2;
            cache_.Resize(newCapacity);
        }

        cache_.Set(cacheLength_, item);
        ++cacheLength_;
    }

    void CopyFromSequence(const Sequence<T>& sequence) {
        const int length = sequence.GetLength();
        cache_.Resize(length);

        for (int i = 0; i < length; ++i) {
            cache_.Set(i, sequence.Get(i));
        }

        cacheLength_ = length;
        lengthOrdinal_ = Ordinal::Finite(static_cast<std::size_t>(length));
        delete generator_;
        generator_ = nullptr;
        concatLeft_ = nullptr;
        concatRight_ = nullptr;
    }

    DynamicArraySequenceView<T> CreateHistoryView() const {
        return DynamicArraySequenceView<T>(cache_, cacheLength_);
    }

    T GenerateNext() const {
        if (generator_ == nullptr) {
            throw IndexOutOfRange();
        }
        return generator_->GetNext();
    }

    void EnsureMaterialized(int index) const {
        if (index < 0) {
            throw IndexOutOfRange();
        }

        const std::size_t target = static_cast<std::size_t>(index);
        if (lengthOrdinal_.IsFinite() && target >= lengthOrdinal_.FiniteValue()) {
            throw IndexOutOfRange();
        }

        while (cacheLength_ <= index) {
            AppendToCache(GenerateNext());
        }
    }

public:
    LazySequence()
            : cache_(),
              cacheLength_(0),
              lengthOrdinal_(Ordinal::Finite(0)),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {}

    explicit LazySequence(Ordinal lengthOrdinal, Provider provider)
            : cache_(),
              cacheLength_(0),
              lengthOrdinal_(lengthOrdinal),
              generator_(new Generator<T>(this, lengthOrdinal_, std::move(provider))),
              concatLeft_(nullptr),
              concatRight_(nullptr) {}

    LazySequence(const T* items, int count)
            : cache_(items, count),
              cacheLength_(count),
              lengthOrdinal_(Ordinal::Finite(static_cast<std::size_t>(count))),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {}

    explicit LazySequence(const Sequence<T>& sequence)
            : cache_(),
              cacheLength_(0),
              lengthOrdinal_(Ordinal::Finite(0)),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {
        CopyFromSequence(sequence);
    }

    explicit LazySequence(const Sequence<T>* sequence) : LazySequence(RequireSequence(sequence)) {}

    LazySequence(Rule rule, const Sequence<T>& firstItems)
            : cache_(),
              cacheLength_(0),
              lengthOrdinal_(Ordinal::Omega()),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {
        const int length = firstItems.GetLength();
        cache_.Resize(length);

        for (int i = 0; i < length; ++i) {
            cache_.Set(i, firstItems.Get(i));
        }
        cacheLength_ = length;
        generator_ = new Generator<T>(this, lengthOrdinal_, std::move(rule));
    }

    LazySequence(Rule rule, const Sequence<T>* firstItems) : LazySequence(std::move(rule), RequireSequence(firstItems)) {}

    LazySequence(const LazySequence<T>& other)
            : cache_(other.cache_),
              cacheLength_(other.cacheLength_),
              lengthOrdinal_(other.lengthOrdinal_),
              generator_(other.generator_ ? new Generator<T>(*other.generator_, this, static_cast<std::size_t>(other.cacheLength_)) : nullptr),
              concatLeft_(other.concatLeft_),
              concatRight_(other.concatRight_) {}

    ~LazySequence() override {
        delete generator_;
    }

    LazySequence<T>& operator=(const LazySequence<T>& other) {
        if (this == &other) {
            return *this;
        }

        Generator<T>* newGenerator = other.generator_
                                     ? new Generator<T>(*other.generator_, this, static_cast<std::size_t>(other.cacheLength_))
                                     : nullptr;
        delete generator_;

        cache_ = other.cache_;
        cacheLength_ = other.cacheLength_;
        lengthOrdinal_ = other.lengthOrdinal_;
        generator_ = newGenerator;
        concatLeft_ = other.concatLeft_;
        concatRight_ = other.concatRight_;
        return *this;
    }

    Ordinal GetLengthOrdinal() const {
        return lengthOrdinal_;
    }

    std::size_t GetMaterializedCount() const {
        return static_cast<std::size_t>(cacheLength_);
    }

    bool HasConcatParts() const {
        return concatLeft_ != nullptr && concatRight_ != nullptr;
    }

    const T& GetConcatPart(int partIndex, int index) const {
        if (!HasConcatParts()) {
            if (partIndex == 0) {
                return Get(index);
            }
            throw IndexOutOfRange();
        }

        if (partIndex == 0) {
            return concatLeft_->Get(index);
        }
        if (partIndex == 1) {
            return concatRight_->Get(index);
        }
        throw IndexOutOfRange();
    }

    const T& GetFirst() const override {
        return Get(0);
    }

    const T& GetLast() const override {
        if (lengthOrdinal_.IsZero()) {
            throw IndexOutOfRange();
        }

        const std::optional<Ordinal> lastIndex = lengthOrdinal_.Predecessor();
        if (!lastIndex.has_value()) {
            throw std::logic_error("Lazy sequence length has no last item");
        }
        return Get(*lastIndex);
    }

    const T& Get(int index) const override {
        if (index < 0) {
            throw IndexOutOfRange();
        }
        return Get(Ordinal::Finite(static_cast<std::size_t>(index)));
    }

    const T& Get(const Ordinal& index) const {
        if (!(index < lengthOrdinal_)) {
            throw IndexOutOfRange();
        }

        if (HasConcatParts()) {
            if (index < concatLeft_->lengthOrdinal_) {
                return concatLeft_->Get(index);
            }
            return concatRight_->Get(index.SubtractPrefix(concatLeft_->lengthOrdinal_));
        }

        if (!index.IsFinite() ||
            index.FiniteValue() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw IndexOutOfRange();
        }

        const int finiteIndex = static_cast<int>(index.FiniteValue());
        EnsureMaterialized(finiteIndex);
        return cache_.Get(finiteIndex);
    }

    int GetLength() const override {
        if (!lengthOrdinal_.IsFinite()) {
            throw std::overflow_error("Lazy sequence length is transfinite");
        }
        if (lengthOrdinal_.FiniteValue() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::overflow_error("Lazy sequence is too long");
        }
        return static_cast<int>(lengthOrdinal_.FiniteValue());
    }

    LazySequence<T>* GetSubsequence(int startIndex, int endIndex) const override {
        if (startIndex < 0 || endIndex < 0 || startIndex > endIndex) {
            throw IndexOutOfRange();
        }
        if (lengthOrdinal_.IsFinite() && static_cast<std::size_t>(endIndex) >= lengthOrdinal_.FiniteValue()) {
            throw IndexOutOfRange();
        }

        auto source = SharedCopy();
        const int length = endIndex - startIndex + 1;
        return new LazySequence<T>(
                Ordinal::Finite(static_cast<std::size_t>(length)),
                [source, startIndex](int index) {
                    return source->Get(startIndex + index);
                });
    }

    IEnumerator<T>* GetEnumerator() const override {
        return new Enumerator(this);
    }

    LazySequence<T>* Append(const T& item) const {
        const LazySequence<T> suffix(&item, 1);
        return Concat(suffix);
    }

    LazySequence<T>* Prepend(const T& item) const {
        const LazySequence<T> prefix(&item, 1);
        return prefix.Concat(*this);
    }

    LazySequence<T>* InsertAt(const T& item, int index) const {
        if (index < 0) {
            throw IndexOutOfRange();
        }
        if (lengthOrdinal_.IsFinite() && static_cast<std::size_t>(index) > lengthOrdinal_.FiniteValue()) {
            throw IndexOutOfRange();
        }

        auto source = SharedCopy();
        const std::size_t insertIndex = static_cast<std::size_t>(index);
        const Ordinal newLength = lengthOrdinal_.IsFinite()
                                  ? lengthOrdinal_.Add(Ordinal::Finite(1))
                                  : lengthOrdinal_;

        return new LazySequence<T>(
                newLength,
                [source, item, insertIndex](int currentIndex) {
                    const std::size_t current = static_cast<std::size_t>(currentIndex);

                    if (current < insertIndex) {
                        return source->Get(currentIndex);
                    }
                    if (current == insertIndex) {
                        return item;
                    }
                    return source->Get(currentIndex - 1);
                });
    }

    LazySequence<T>* Concat(const Sequence<T>& other) const override {
        return new LazySequence<T>(SharedCopy(), CopyAsLazy(other));
    }

    LazySequence<T>* Concat(const LazySequence<T>* other) const {
        return Concat(RequireSequence(other));
    }

    LazySequence<T>* Clone() const override {
        return new LazySequence<T>(*this);
    }

    LazySequence<T>* CreateEmpty() const override {
        return new LazySequence<T>();
    }

    template <class Mapper>
    auto Map(Mapper mapper) const -> LazySequence<decltype(mapper(std::declval<T>()))>* {
        using Result = decltype(mapper(std::declval<T>()));

        auto source = SharedCopy();
        return new LazySequence<Result>(
                lengthOrdinal_,
                [source, mapper](int index) {
                    return mapper(source->Get(index));
                });
    }

    template <class Reducer, class Accumulator>
    Accumulator Reduce(Reducer reducer, Accumulator start) const {
        if (!lengthOrdinal_.IsFinite()) {
            throw std::logic_error("Reduce over an infinite lazy sequence is not finite");
        }

        Accumulator result = start;
        const int length = GetLength();
        for (int i = 0; i < length; ++i) {
            result = reducer(result, Get(i));
        }
        return result;
    }

    template <class Predicate>
    LazySequence<T>* Where(Predicate predicate) const {
        auto source = SharedCopy();

        if (lengthOrdinal_.IsFinite()) {
            MutableArraySequence<T> filtered;
            const int length = GetLength();

            for (int i = 0; i < length; ++i) {
                const T& value = Get(i);
                if (predicate(value)) {
                    filtered.Append(value);
                }
            }

            return new LazySequence<T>(filtered);
        }

        auto accepted = std::make_shared<DynamicArray<T>>();
        auto acceptedLength = std::make_shared<int>(0);
        auto scanned = std::make_shared<int>(0);

        return new LazySequence<T>(
                Ordinal::Omega(),
                [source, predicate, accepted, acceptedLength, scanned](int index) {
                    while (*acceptedLength <= index) {
                        T value = source->Get((*scanned)++);

                        if (predicate(value)) {
                            if (*acceptedLength == accepted->GetLength()) {
                                const int newCapacity = accepted->GetLength() == 0 ? 8 : accepted->GetLength() * 2;
                                accepted->Resize(newCapacity);
                            }

                            accepted->Set(*acceptedLength, value);
                            ++(*acceptedLength);
                        }
                    }

                    return accepted->Get(index);
                });
    }

    template <class U>
    LazySequence<std::pair<T, U>>* Zip(const Sequence<U>& other) const {
        auto left = SharedCopy();

        std::shared_ptr<Sequence<U>> rightSequence;
        Ordinal rightLength;
        if (const auto* lazy = dynamic_cast<const LazySequence<U>*>(&other)) {
            rightSequence.reset(lazy->Clone());
            rightLength = lazy->GetLengthOrdinal();
        } else {
            rightSequence.reset(other.Clone());
            rightLength = Ordinal::Finite(static_cast<std::size_t>(other.GetLength()));
        }

        const Ordinal newLength = MinOrdinal(lengthOrdinal_, rightLength);
        return new LazySequence<std::pair<T, U>>(
                newLength,
                [left, rightSequence](int index) {
                    return std::make_pair(left->Get(index), rightSequence->Get(index));
                });
    }

private:
    LazySequence<T>* Append(const T& item) override {
        return static_cast<const LazySequence<T>&>(*this).Append(item);
    }

    LazySequence<T>* Prepend(const T& item) override {
        return static_cast<const LazySequence<T>&>(*this).Prepend(item);
    }

    LazySequence<T>* InsertAt(const T& item, int index) override {
        return static_cast<const LazySequence<T>&>(*this).InsertAt(item, index);
    }
};
