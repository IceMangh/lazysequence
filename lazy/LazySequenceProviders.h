#pragma once

#include <memory>
#include <utility>
#include "Exceptions.h"
#include "Ordinal.h"

template <class T>
class LazySequence;

template <class T>
struct LazySequenceConcatProvider {
    std::shared_ptr<const LazySequence<T>> left;
    std::shared_ptr<const LazySequence<T>> right;
    Ordinal leftLength;

    T operator()(const Ordinal& index) const {
        if (index < leftLength) {
            return left->Get(index);
        }
        return right->Get(index.SubtractPrefix(leftLength));
    }
};

template <class T>
struct LazySequenceMixedProvider {
    std::shared_ptr<const LazySequence<T>> first;
    std::shared_ptr<const LazySequence<T>> second;
    std::shared_ptr<const LazySequence<T>> third;

    T operator()(const Ordinal& index) const {
        if (!index.IsFinite()) {
            throw IndexOutOfRange();
        }

        const int indexNumber = static_cast<int>(index.FiniteValue());
        const int itemIndex = indexNumber / 3;
        const int sequenceNumber = indexNumber % 3;

        if (sequenceNumber == 0) {
            return first->Get(itemIndex);
        }
        if (sequenceNumber == 1) {
            return second->Get(itemIndex);
        }
        return third->Get(itemIndex);
    }
};

template <class T>
struct LazySequenceRangeProvider {
    std::shared_ptr<const LazySequence<T>> source;
    Ordinal startIndex;

    T operator()(const Ordinal& index) const {
        return source->Get(startIndex.Add(index));
    }
};

template <class T, class Result, class Mapper>
struct LazySequenceMapProvider {
    std::shared_ptr<const LazySequence<T>> source;
    Mapper mapper;

    Result operator()(const Ordinal& index) const {
        return mapper(source->Get(index));
    }
};

template <class T, class U>
struct LazySequenceZipProvider {
    std::shared_ptr<const LazySequence<T>> left;
    std::shared_ptr<const LazySequence<U>> right;

    std::pair<T, U> operator()(const Ordinal& index) const {
        return std::make_pair(left->Get(index), right->Get(index));
    }
};
