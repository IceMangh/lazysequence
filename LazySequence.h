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

    struct OrdinalCacheEntry {
        Ordinal index;
        T value;

        OrdinalCacheEntry() : index(), value() {}

        OrdinalCacheEntry(const Ordinal& indexValue, const T& valueValue)
            : index(indexValue), value(valueValue) {}
    };

    mutable DynamicArray<T> cache_;
    mutable int cacheLength_;
    mutable DynamicArray<OrdinalCacheEntry> ordinalCache_;
    mutable int ordinalCacheLength_;
    Ordinal lengthOrdinal_;

    std::unique_ptr<Generator<T>> generator_;
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
                throw std::logic_error("bad enumerator");
            }
            return sequence_->Get(index_);
        }
    };

    LazySequence(std::shared_ptr<const LazySequence<T>> left,
                 std::shared_ptr<const LazySequence<T>> right)
            : cache_(),
              cacheLength_(0),
              ordinalCache_(),
              ordinalCacheLength_(0),
              lengthOrdinal_(left->lengthOrdinal_.Add(right->lengthOrdinal_)),
              generator_(nullptr),
              concatLeft_(left),
              concatRight_(right) {}

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
            throw std::invalid_argument("null sequence");
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

    const T* FindOrdinalCache(const Ordinal& index) const {
        for (int i = 0; i < ordinalCacheLength_; ++i) {
            const OrdinalCacheEntry& entry = ordinalCache_.Get(i);
            if (entry.index == index) {
                return &entry.value;
            }
        }
        return nullptr;
    }

    const T& StoreOrdinalCache(const Ordinal& index, const T& value) const {
        if (ordinalCacheLength_ == ordinalCache_.GetLength()) {
            const int newCapacity = ordinalCache_.GetLength() == 0 ? 8 : ordinalCache_.GetLength() * 2;
            ordinalCache_.Resize(newCapacity);
        }

        ordinalCache_.Set(ordinalCacheLength_, OrdinalCacheEntry(index, value));
        ++ordinalCacheLength_;
        return ordinalCache_.Get(ordinalCacheLength_ - 1).value;
    }

    void CopyFromSequence(const Sequence<T>& sequence) {
        const int length = sequence.GetLength();
        cache_.Resize(length);

        for (int i = 0; i < length; ++i) {
            cache_.Set(i, sequence.Get(i));
        }

        cacheLength_ = length;
        ordinalCache_.Resize(0);
        ordinalCacheLength_ = 0;
        lengthOrdinal_ = Ordinal::Finite(static_cast<std::size_t>(length));
        generator_.reset(nullptr);
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

    T GenerateAt(const Ordinal& index) const {
        if (generator_ == nullptr) {
            throw IndexOutOfRange();
        }
        return generator_->Get(index);
    }

    const T& GetGeneratedOrdinal(const Ordinal& index) const {
        const T* cached = FindOrdinalCache(index);
        if (cached != nullptr) {
            return *cached;
        }

        return StoreOrdinalCache(index, GenerateAt(index));
    }

    LazySequence<T> Range(const Ordinal& startIndex, const Ordinal& rangeLength) const {
        const Ordinal end = startIndex.Add(rangeLength);
        if (startIndex > lengthOrdinal_ || end > lengthOrdinal_) {
            throw IndexOutOfRange();
        }

        auto source = SharedCopy();
        return LazySequence<T>(
                rangeLength,
                [source, startIndex](const Ordinal& index) {
                    return source->Get(startIndex.Add(index));
                });
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
              ordinalCache_(),
              ordinalCacheLength_(0),
              lengthOrdinal_(Ordinal::Finite(0)),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {}

    explicit LazySequence(Ordinal lengthOrdinal, std::function<T(const Ordinal&)> provider)
            : cache_(),
              cacheLength_(0),
              ordinalCache_(),
              ordinalCacheLength_(0),
              lengthOrdinal_(lengthOrdinal),
              generator_(new Generator<T>(this, lengthOrdinal_, provider)),
              concatLeft_(nullptr),
              concatRight_(nullptr) {}

    LazySequence(const T* items, int count)
            : cache_(items, count),
              cacheLength_(count),
              ordinalCache_(),
              ordinalCacheLength_(0),
              lengthOrdinal_(Ordinal::Finite(static_cast<std::size_t>(count))),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {}

    explicit LazySequence(const Sequence<T>& sequence)
            : cache_(),
              cacheLength_(0),
              ordinalCache_(),
              ordinalCacheLength_(0),
              lengthOrdinal_(Ordinal::Finite(0)),
              generator_(nullptr),
              concatLeft_(nullptr),
              concatRight_(nullptr) {
        CopyFromSequence(sequence);
    }

    explicit LazySequence(const Sequence<T>* sequence) : LazySequence(RequireSequence(sequence)) {}

    LazySequence(std::function<T(Sequence<T>*)> rule, const Sequence<T>& firstItems)
            : cache_(),
              cacheLength_(0),
              ordinalCache_(),
              ordinalCacheLength_(0),
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
        generator_.reset(new Generator<T>(this, lengthOrdinal_, rule));
    }

    LazySequence(std::function<T(Sequence<T>*)> rule, const Sequence<T>* firstItems) : LazySequence(rule, RequireSequence(firstItems)) {}

    LazySequence(const LazySequence<T>& other)
            : cache_(other.cache_),
              cacheLength_(other.cacheLength_),
              ordinalCache_(other.ordinalCache_),
              ordinalCacheLength_(other.ordinalCacheLength_),
              lengthOrdinal_(other.lengthOrdinal_),
              generator_(other.generator_ ? new Generator<T>(*other.generator_, this, static_cast<std::size_t>(other.cacheLength_)) : nullptr),
              concatLeft_(other.concatLeft_),
              concatRight_(other.concatRight_) {}

    ~LazySequence() override = default;

    LazySequence<T>& operator=(const LazySequence<T>& other) {
        if (this == &other) {
            return *this;
        }

        Generator<T>* newGenerator = other.generator_
                                     ? new Generator<T>(*other.generator_, this, static_cast<std::size_t>(other.cacheLength_))
                                     : nullptr;

        cache_ = other.cache_;
        cacheLength_ = other.cacheLength_;
        ordinalCache_ = other.ordinalCache_;
        ordinalCacheLength_ = other.ordinalCacheLength_;
        lengthOrdinal_ = other.lengthOrdinal_;
        generator_.reset(newGenerator);
        concatLeft_ = other.concatLeft_;
        concatRight_ = other.concatRight_;
        return *this;
    }

    Ordinal GetLengthOrdinal() const {
        return lengthOrdinal_;
    }

    std::size_t GetMaterializedCount() const {
        std::size_t count = static_cast<std::size_t>(cacheLength_ + ordinalCacheLength_);
        if (HasConcatParts()) {
            count += concatLeft_->GetMaterializedCount();
            count += concatRight_->GetMaterializedCount();
        }
        return count;
    }

    bool HasConcatParts() const {
        return concatLeft_ != nullptr && concatRight_ != nullptr;
    }

    const T& GetConcatPart(int partIndex, const Ordinal& index) const {
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

    const T& GetConcatPart(int partIndex, int index) const {
        if (index < 0) {
            throw IndexOutOfRange();
        }
        return GetConcatPart(partIndex, Ordinal::Finite(static_cast<std::size_t>(index)));
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
            throw std::logic_error("no last item");
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
        if (index >= lengthOrdinal_) {
            throw IndexOutOfRange();
        }

        if (HasConcatParts()) {
            if (index < concatLeft_->lengthOrdinal_) {
                return concatLeft_->Get(index);
            }
            return concatRight_->Get(index.SubtractPrefix(concatLeft_->lengthOrdinal_));
        }

        if (index.IsFinite() && index.FiniteValue() <= static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            const int finiteIndex = static_cast<int>(index.FiniteValue());
            EnsureMaterialized(finiteIndex);
            return cache_.Get(finiteIndex);
        }

        return GetGeneratedOrdinal(index);
    }

    int GetLength() const override {
        if (!lengthOrdinal_.IsFinite()) {
            throw std::overflow_error("non-finite length");
        }
        if (lengthOrdinal_.FiniteValue() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::overflow_error("length overflow");
        }
        return static_cast<int>(lengthOrdinal_.FiniteValue());
    }

    LazySequence<T> Subsequence(int startIndex, int endIndex) const {
        if (startIndex < 0 || endIndex < 0 || startIndex > endIndex) {
            throw IndexOutOfRange();
        }

        return Subsequence(
                Ordinal::Finite(static_cast<std::size_t>(startIndex)),
                Ordinal::Finite(static_cast<std::size_t>(endIndex)));
    }

    LazySequence<T> Subsequence(const Ordinal& startIndex, const Ordinal& endIndex) const {
        if (endIndex < startIndex || !(endIndex < lengthOrdinal_)) {
            throw IndexOutOfRange();
        }

        return Range(startIndex, endIndex.SubtractPrefix(startIndex).Successor());
    }

    IEnumerator<T>* GetEnumerator() const override {
        return new Enumerator(this);
    }

    LazySequence<T> AppendItem(const T& item) const {
        const LazySequence<T> suffix(&item, 1);
        return Concat(suffix);
    }

    LazySequence<T> PrependItem(const T& item) const {
        const LazySequence<T> prefix(&item, 1);
        return prefix.Concat(*this);
    }

    LazySequence<T> InsertItemAt(const T& item, int index) const {
        if (index < 0) {
            throw IndexOutOfRange();
        }
        return InsertItemAt(item, Ordinal::Finite(static_cast<std::size_t>(index)));
    }

    LazySequence<T> InsertItemAt(const T& item, const Ordinal& index) const {
        if (index > lengthOrdinal_) {
            throw IndexOutOfRange();
        }

        LazySequence<T> prefix = Range(Ordinal::Finite(0), index);
        LazySequence<T> inserted(&item, 1);
        LazySequence<T> suffix = Range(index, lengthOrdinal_.SubtractPrefix(index));
        return prefix.Concat(inserted).Concat(suffix);
    }

    LazySequence<T> Concat(const LazySequence<T>& other) const {
        return LazySequence<T>(SharedCopy(), other.SharedCopy());
    }

    LazySequence<T> Concat(const LazySequence<T>* other) const {
        if (other == nullptr) {
            throw std::invalid_argument("null sequence");
        }
        return Concat(*other);
    }

    LazySequence<T> ConcatWith(const Sequence<T>& other) const {
        return LazySequence<T>(SharedCopy(), CopyAsLazy(other));
    }

    LazySequence<T> ConcatWith(const Sequence<T>* other) const {
        return ConcatWith(RequireSequence(other));
    }

    template <class Result, class Mapper>
    LazySequence<Result> Map(Mapper mapper) const {
        if (HasConcatParts()) {
            LazySequence<Result> left = concatLeft_->template Map<Result>(mapper);
            LazySequence<Result> right = concatRight_->template Map<Result>(mapper);
            return left.Concat(right);
        }

        auto source = SharedCopy();
        return LazySequence<Result>(
                lengthOrdinal_,
                [source, mapper](const Ordinal& index) {
                    return mapper(source->Get(index));
                });
    }

    template <class Reducer, class Accumulator>
    Accumulator Reduce(Reducer reducer, Accumulator start) const {
        if (!lengthOrdinal_.IsFinite()) {
            throw std::logic_error("non-finite reduce");
        }

        Accumulator result = start;
        const int length = GetLength();
        for (int i = 0; i < length; ++i) {
            result = reducer(result, Get(i));
        }
        return result;
    }

    template <class Predicate>
    LazySequence<T> Where(Predicate predicate) const {
        if (HasConcatParts()) {
            LazySequence<T> left = concatLeft_->Where(predicate);
            LazySequence<T> right = concatRight_->Where(predicate);
            return left.Concat(right);
        }

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

            return LazySequence<T>(filtered);
        }

        auto accepted = std::make_shared<DynamicArray<T>>();
        auto acceptedLength = std::make_shared<int>(0);
        auto scanned = std::make_shared<int>(0);

        return LazySequence<T>(
                Ordinal::Omega(),
                [source, predicate, accepted, acceptedLength, scanned](const Ordinal& index) {
                    if (!index.IsFinite() ||
                        index.FiniteValue() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                        throw IndexOutOfRange();
                    }
                    const int finiteIndex = static_cast<int>(index.FiniteValue());
                    while (*acceptedLength <= finiteIndex) {
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

                    return accepted->Get(finiteIndex);
                });
    }

    template <class U>
    LazySequence<std::pair<T, U>> Zip(const Sequence<U>& other) const {
        auto left = SharedCopy();

        std::shared_ptr<const LazySequence<U>> rightSequence;
        Ordinal rightLength;
        if (const auto* lazy = dynamic_cast<const LazySequence<U>*>(&other)) {
            rightSequence = std::make_shared<LazySequence<U>>(*lazy);
            rightLength = lazy->GetLengthOrdinal();
        } else {
            rightSequence = std::make_shared<LazySequence<U>>(other);
            rightLength = Ordinal::Finite(static_cast<std::size_t>(other.GetLength()));
        }

        const Ordinal newLength = MinOrdinal(lengthOrdinal_, rightLength);
        return LazySequence<std::pair<T, U>>(
                newLength,
                [left, rightSequence](const Ordinal& index) {
                    return std::make_pair(left->Get(index), rightSequence->Get(index));
                });
    }

private:
    Sequence<T>* Clone() const override {
        return new LazySequence<T>(*this);
    }

    Sequence<T>* CreateEmpty() const override {
        return new LazySequence<T>();
    }

    Sequence<T>* GetSubsequence(int startIndex, int endIndex) const override {
        return new LazySequence<T>(Subsequence(startIndex, endIndex));
    }

    Sequence<T>* Concat(const Sequence<T>& other) const override {
        return new LazySequence<T>(ConcatWith(other));
    }

    Sequence<T>* Append(const T& item) override {
        return new LazySequence<T>(static_cast<const LazySequence<T>&>(*this).AppendItem(item));
    }

    Sequence<T>* Prepend(const T& item) override {
        return new LazySequence<T>(static_cast<const LazySequence<T>&>(*this).PrependItem(item));
    }

    Sequence<T>* InsertAt(const T& item, int index) override {
        return new LazySequence<T>(static_cast<const LazySequence<T>&>(*this).InsertItemAt(item, index));
    }
};
