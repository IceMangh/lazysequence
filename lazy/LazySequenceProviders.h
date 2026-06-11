#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "DynamicArray.h"
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

template <class T>
struct LazySequenceWhereBlock {
    DynamicArray<T> accepted;
    int acceptedLength = 0;
    std::size_t scannedOffset = 0;

    void Append(const T& value) {
        if (acceptedLength == accepted.GetLength()) {
            const int newCapacity = accepted.GetLength() == 0 ? 8 : accepted.GetLength() * 2;
            accepted.Resize(newCapacity);
        }

        accepted.Set(acceptedLength, value);
        ++acceptedLength;
    }
};

template <class T>
struct LazySequenceWhereState {
    std::vector<LazySequenceWhereBlock<T>> blocks;

    LazySequenceWhereBlock<T>& Block(std::size_t blockIndex) {
        if (blocks.size() <= blockIndex) {
            blocks.resize(blockIndex + 1);
        }
        return blocks[blockIndex];
    }
};

template <class T, class Predicate>
struct LazySequenceWhereProvider {
    std::shared_ptr<const LazySequence<T>> source;
    Predicate predicate;
    std::shared_ptr<LazySequenceWhereState<T>> state;

    T operator()(const Ordinal& index) const {
        if (index.FiniteOffset() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw IndexOutOfRange();
        }
        const std::size_t blockIndex = index.OmegaBlocks();
        const int requestedOffset = static_cast<int>(index.FiniteOffset());
        LazySequenceWhereBlock<T>& block = state->Block(blockIndex);

        while (block.acceptedLength <= requestedOffset) {
            T value = source->Get(Ordinal(blockIndex, block.scannedOffset));
            ++block.scannedOffset;

            if (predicate(value)) {
                block.Append(value);
            }
        }

        return block.accepted.Get(requestedOffset);
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
