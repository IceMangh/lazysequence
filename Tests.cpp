#include "Tests.h"

#include <cassert>
#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <string>

#include "LazySequence.h"
#include "MutableArraySequence.h"
#include "OnlineStatistics.h"
#include "Streams.h"

namespace {

int FibonacciRule(Sequence<int>* history) {
    const int length = history->GetLength();
    return history->Get(length - 1) + history->Get(length - 2);
}

int FactorialRule(Sequence<int>* history) {
    const int nextNumber = history->GetLength() + 1;
    return history->GetLast() * nextNumber;
}

int PowersOfTwoRule(Sequence<int>* history) {
    return history->GetLast() * 2;
}

bool IsEven(int value) {
    return value % 2 == 0;
}

double NextNaturalNumber(Sequence<double>* history) {
    return history->GetLast() + 1.0;
}

void TestFiniteLazySequence() {
    int data[] = {1, 2, 3};
    LazySequence<int> sequence(data, 3);

    assert(sequence.GetLength() == 3);
    assert(sequence.GetLengthOrdinal().FiniteValue() == 3);
    assert(sequence.GetFirst() == 1);
    assert(sequence.GetLast() == 3);
    assert(sequence.Get(1) == 2);
    assert(sequence.GetMaterializedCount() == 3);
}

void TestRecurrentLazySequence() {
    int seed[] = {1, 1};
    MutableArraySequence<int> firstItems(seed, 2);
    LazySequence<int> fibonacci(FibonacciRule, firstItems);

    assert(!fibonacci.GetLengthOrdinal().IsFinite());
    assert(fibonacci.GetMaterializedCount() == 2);
    assert(fibonacci.Get(0) == 1);
    assert(fibonacci.Get(1) == 1);
    assert(fibonacci.Get(9) == 55);
    assert(fibonacci.GetMaterializedCount() == 10);
}

void TestAdditionalRecurrentLazySequenceExamples() {
    int factorialSeed[] = {1};
    MutableArraySequence<int> factorialFirstItems(factorialSeed, 1);
    LazySequence<int> factorials(FactorialRule, factorialFirstItems);

    assert(!factorials.GetLengthOrdinal().IsFinite());
    assert(factorials.Get(0) == 1);
    assert(factorials.Get(1) == 2);
    assert(factorials.Get(2) == 6);
    assert(factorials.Get(3) == 24);
    assert(factorials.Get(4) == 120);

    int powersSeed[] = {1};
    MutableArraySequence<int> powersFirstItems(powersSeed, 1);
    LazySequence<int> powersOfTwo(PowersOfTwoRule, powersFirstItems);

    assert(!powersOfTwo.GetLengthOrdinal().IsFinite());
    assert(powersOfTwo.Get(0) == 1);
    assert(powersOfTwo.Get(1) == 2);
    assert(powersOfTwo.Get(2) == 4);
    assert(powersOfTwo.Get(3) == 8);
    assert(powersOfTwo.Get(10) == 1024);
}

void TestLazyEditingOperations() {
    int data[] = {2, 3};
    const LazySequence<int> original(data, 2);

    LazySequence<int> prepended = original.prepend(1);
    LazySequence<int> appended = prepended.append(4);
    LazySequence<int> inserted = appended.insertAt(99, 2);

    assert(original.GetLength() == 2);
    assert(original.Get(0) == 2);
    assert(inserted.GetLength() == 5);
    assert(inserted.Get(0) == 1);
    assert(inserted.Get(2) == 99);
    assert(inserted.Get(3) == 3);
    assert(inserted.Get(4) == 4);
}

void TestSubsequenceAndConcat() {
    int leftData[] = {1, 2, 3};
    int rightData[] = {4, 5};
    LazySequence<int> left(leftData, 3);
    LazySequence<int> right(rightData, 2);

    LazySequence<int> sub = left.subsequence(1, 2);
    LazySequence<int> joined = left.concat(right);
    LazySequence<int> insertedIntoJoined = joined.insertAt(99, 4);

    assert(sub.GetLength() == 2);
    assert(sub.Get(0) == 2);
    assert(sub.Get(1) == 3);
    assert(joined.GetLength() == 5);
    assert(joined.Get(3) == 4);
    assert(joined.Get(4) == 5);
    assert(insertedIntoJoined.GetLength() == 6);
    assert(insertedIntoJoined.Get(3) == 4);
    assert(insertedIntoJoined.Get(4) == 99);
    assert(insertedIntoJoined.Get(5) == 5);
}

void TestInfiniteInsertAndAppend() {
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    const LazySequence<int> naturalNumbers(
        [](Sequence<int>* history) {
            return history->GetLast() + 1;
        },
        firstItems);

    LazySequence<int> inserted = naturalNumbers.insertAt(99, 2);
    LazySequence<int> appended = naturalNumbers.append(500);

    assert(inserted.Get(0) == 1);
    assert(inserted.Get(1) == 2);
    assert(inserted.Get(2) == 99);
    assert(inserted.Get(3) == 3);
    assert(appended.Get(100) == 101);
    assert(!appended.GetLengthOrdinal().IsFinite());
    assert(appended.GetLengthOrdinal() == Ordinal(1, 1));
    assert(appended.Get(Ordinal::Omega()) == 500);

    LazySequence<int> appendedTwice = appended.append(600);
    assert(appendedTwice.GetLengthOrdinal() == Ordinal(1, 2));
    assert(appendedTwice.Get(Ordinal(1, 0)) == 500);
    assert(appendedTwice.Get(Ordinal(1, 1)) == 600);
    assert(appendedTwice.GetLast() == 600);
}

void TestInfiniteConcatJump() {
    int naturalSeed[] = {1};
    int tensSeed[] = {10};
    MutableArraySequence<int> naturalFirstItems(naturalSeed, 1);
    MutableArraySequence<int> tensFirstItems(tensSeed, 1);

    const LazySequence<int> naturalNumbers(
        [](Sequence<int>* history) {
            return history->GetLast() + 1;
        },
        naturalFirstItems);
    const LazySequence<int> tens(
        [](Sequence<int>* history) {
            return history->GetLast() + 10;
        },
        tensFirstItems);

    LazySequence<int> joined = naturalNumbers.concat(tens);

    assert(!joined.GetLengthOrdinal().IsFinite());
    assert(joined.HasConcatParts());
    assert(joined.Get(4) == 5);
    assert(joined.GetConcatPart(0, 4) == 5);
    assert(joined.GetConcatPart(1, 0) == 10);
    assert(joined.GetConcatPart(1, 3) == 40);
    assert(joined.GetLengthOrdinal() == Ordinal(2, 0));
    assert(joined.Get(Ordinal::Omega()) == 10);
    assert(joined.Get(Ordinal(1, 3)) == 40);
}

void TestThreeInfiniteConcatJump() {
    int naturalSeed[] = {1};
    int tensSeed[] = {10};
    int hundredsSeed[] = {100};
    MutableArraySequence<int> naturalFirstItems(naturalSeed, 1);
    MutableArraySequence<int> tensFirstItems(tensSeed, 1);
    MutableArraySequence<int> hundredsFirstItems(hundredsSeed, 1);

    const LazySequence<int> naturalNumbers(
        [](Sequence<int>* history) {
            return history->GetLast() + 1;
        },
        naturalFirstItems);
    const LazySequence<int> tens(
        [](Sequence<int>* history) {
            return history->GetLast() + 10;
        },
        tensFirstItems);
    const LazySequence<int> hundreds(
        [](Sequence<int>* history) {
            return history->GetLast() + 100;
        },
        hundredsFirstItems);

    LazySequence<int> firstJoin = naturalNumbers.concat(tens);
    LazySequence<int> secondJoin = firstJoin.concat(hundreds);

    assert(!secondJoin.GetLengthOrdinal().IsFinite());
    assert(secondJoin.HasConcatParts());
    assert(secondJoin.Get(4) == 5);
    assert(secondJoin.GetConcatPart(1, 2) == 300);
    assert(secondJoin.GetConcatPart(0, 4) == 5);
    assert(secondJoin.GetConcatPart(0, 1) == 2);
    assert(secondJoin.GetLengthOrdinal() == Ordinal(3, 0));
    assert(secondJoin.Get(Ordinal(2, 0)) == 100);
    assert(secondJoin.Get(Ordinal(2, 2)) == 300);
}

void TestMapWhereZipReduce() {
    int data[] = {1, 2, 3, 4};
    LazySequence<int> sequence(data, 4);

    LazySequence<int> squares = sequence.map([](int value) {
        return value * value;
    });
    LazySequence<int> evens = sequence.where(IsEven);
    LazySequence<std::pair<int, int>> zipped = sequence.zip(squares);

    assert(squares.Get(2) == 9);
    assert(evens.GetLength() == 2);
    assert(evens.Get(0) == 2);
    assert(evens.Get(1) == 4);
    assert(zipped.GetLength() == 4);
    assert(zipped.Get(2).first == 3);
    assert(zipped.Get(2).second == 9);
    assert(sequence.Reduce([](int sum, int value) { return sum + value; }, 0) == 10);
}

void TestOrdinalProviderMapZipAndSubsequence() {
    LazySequence<int> ordinalProvider(
        Ordinal(2, 0),
        [](const Ordinal& index) {
            return static_cast<int>(index.OmegaBlocks() * 1000 + index.FiniteOffset());
        });

    assert(ordinalProvider.Get(Ordinal(1, 5)) == 1005);
    assert(ordinalProvider.GetMaterializedCount() == 1);

    LazySequence<int> first(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 1000 + static_cast<int>(index.FiniteValue());
        });
    LazySequence<int> second(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 2000 + static_cast<int>(index.FiniteValue());
        });
    LazySequence<int> third(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 3000 + static_cast<int>(index.FiniteValue());
        });

    LazySequence<int> firstJoin = first.concat(second);
    LazySequence<int> all = firstJoin.concat(third);
    LazySequence<int> mapped = all.map([](int value) {
        return value * 10;
    });
    LazySequence<std::pair<int, int>> zipped = all.zip(mapped);
    LazySequence<int> sliced = all.subsequence(Ordinal(1, 2), Ordinal(2, 1));

    assert(mapped.Get(Ordinal(2, 3)) == 30030);
    assert(zipped.Get(Ordinal(1, 2)).first == 2002);
    assert(zipped.Get(Ordinal(1, 2)).second == 20020);
    assert(sliced.GetLengthOrdinal() == Ordinal(1, 2));
    assert(sliced.Get(0) == 2002);
    assert(sliced.Get(Ordinal::Omega()) == 3000);
    assert(sliced.Get(Ordinal(1, 1)) == 3001);
}

void TestOrdinalInsertAndWhereOverConcat() {
    const LazySequence<int> naturals(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return static_cast<int>(index.FiniteValue());
        });
    const LazySequence<int> tens(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 100 + static_cast<int>(index.FiniteValue());
        });

    LazySequence<int> insertedInside = naturals.insertAt(77, 2);
    LazySequence<int> insertedAtOmega = naturals.insertAt(88, Ordinal::Omega());
    LazySequence<int> joined = naturals.concat(tens);
    LazySequence<int> evens = joined.where([](int value) {
        return value % 2 == 0;
    });

    assert(insertedInside.GetLengthOrdinal() == Ordinal::Omega());
    assert(insertedInside.Get(0) == 0);
    assert(insertedInside.Get(2) == 77);
    assert(insertedInside.Get(3) == 2);
    assert(insertedAtOmega.GetLengthOrdinal() == Ordinal(1, 1));
    assert(insertedAtOmega.Get(Ordinal::Omega()) == 88);

    assert(evens.GetLengthOrdinal() == Ordinal(2, 0));
    assert(evens.Get(0) == 0);
    assert(evens.Get(1) == 2);
    assert(evens.Get(Ordinal::Omega()) == 100);
    assert(evens.Get(Ordinal(1, 2)) == 104);
}

void AssertFiniteGeneratorValues(Generator<int>& generator, const int* expected, int count) {
    generator.Reset(0);
    assert(generator.GetLengthOrdinal().FiniteValue() == static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        assert(generator.HasNext());
        assert(generator.GetNext() == expected[i]);
    }
    assert(!generator.HasNext());
    assert(!generator.TryGetNext());
}

void TestGeneratorAppendItem() {
    int data[] = {1, 2, 3};
    LazySequence<int> owner(data, 3);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });

    Generator<int> appended = generator.Append(4);
    int expected[] = {1, 2, 3, 4};
    AssertFiniteGeneratorValues(appended, expected, 4);
}

void TestGeneratorAppendSequence() {
    int data[] = {1, 2};
    int suffixData[] = {3, 4, 5};
    LazySequence<int> owner(data, 2);
    MutableArraySequence<int> suffix(suffixData, 3);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });

    Generator<int> appended = generator.Append(&suffix);
    int expected[] = {1, 2, 3, 4, 5};
    AssertFiniteGeneratorValues(appended, expected, 5);
}

void TestGeneratorInsertItem() {
    int data[] = {1, 3};
    LazySequence<int> owner(data, 2);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });

    generator.Reset(1);
    Generator<int> inserted = generator.Insert(2);
    int expected[] = {1, 2, 3};
    AssertFiniteGeneratorValues(inserted, expected, 3);
}

void TestGeneratorInsertSequence() {
    int data[] = {1, 4, 5};
    int middleData[] = {2, 3};
    LazySequence<int> owner(data, 3);
    MutableArraySequence<int> middle(middleData, 2);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });

    generator.Reset(1);
    Generator<int> inserted = generator.Insert(&middle);
    int expected[] = {1, 2, 3, 4, 5};
    AssertFiniteGeneratorValues(inserted, expected, 5);
}

void TestGeneratorRemoveItem() {
    int data[] = {1, 2, 3, 4};
    LazySequence<int> owner(data, 4);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });

    generator.Reset(1);
    Generator<int> removed = generator.Remove(2);
    int expected[] = {1, 3, 4};
    AssertFiniteGeneratorValues(removed, expected, 3);
}

void TestGeneratorRemoveSequence() {
    int data[] = {1, 2, 3, 4, 5};
    int removedData[] = {2, 3};
    LazySequence<int> owner(data, 5);
    MutableArraySequence<int> removedItems(removedData, 2);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });

    generator.Reset(1);
    Generator<int> removed = generator.Remove(&removedItems);
    int expected[] = {1, 4, 5};
    AssertFiniteGeneratorValues(removed, expected, 3);
}

void TestGeneratorOperationExceptions() {
    int data[] = {1};
    LazySequence<int> owner(data, 1);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](const Ordinal&) {
        return 0;
    });
    Sequence<int>* nullSequence = nullptr;

    bool appendNullThrown = false;
    try {
        Generator<int> appended = generator.Append(nullSequence);
        (void)appended;
    } catch (const std::invalid_argument&) {
        appendNullThrown = true;
    }
    assert(appendNullThrown);

    bool insertOutOfRangeThrown = false;
    generator.Reset(2);
    try {
        Generator<int> inserted = generator.Insert(9);
        (void)inserted;
    } catch (const IndexOutOfRange&) {
        insertOutOfRangeThrown = true;
    }
    assert(insertOutOfRangeThrown);

    bool removeOutOfRangeThrown = false;
    generator.Reset(1);
    try {
        Generator<int> removed = generator.Remove(1);
        (void)removed;
    } catch (const IndexOutOfRange&) {
        removeOutOfRangeThrown = true;
    }
    assert(removeOutOfRangeThrown);
}

void TestStreams() {
    int data[] = {10, 20, 30};
    LazySequence<int> sequence(data, 3);
    ReadOnlyStream<int> stream(sequence);

    stream.Open();
    assert(stream.Read() == 10);
    assert(stream.GetPosition() == 1);
    assert(stream.Seek(2) == 2);
    assert(stream.Read() == 30);

    bool thrown = false;
    try {
        stream.Read();
    } catch (const EndOfStream&) {
        thrown = true;
    }
    assert(thrown);
    stream.Close();
}

void TestStringStreamAndWriteStream() {
    ReadOnlyStream<int> input("1 2 3", [](const std::string& token) {
        return std::stoi(token);
    });

    input.Open();
    WriteOnlyStream<int> output;
    output.Open();
    while (true) {
        try {
            output.Write(input.Read() * 2);
        } catch (const EndOfStream&) {
            break;
        }
    }

    LazySequence<int> result = output.ToSequence();
    assert(output.GetPosition() == 3);
    assert(result.GetLength() == 3);
    assert(result.Get(0) == 2);
    assert(result.Get(2) == 6);
    input.Close();
    output.Close();
}

void TestOnlineStatistics() {
    ReadOnlyStream<double> stream("5 1 3 2 4", [](const std::string& token) {
        return std::stod(token);
    });

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, 100);
    OnlineStatisticsSnapshot snapshot = statistics.Snapshot();

    assert(snapshot.count == 5);
    assert(std::fabs(snapshot.sum - 15.0) < 0.000001);
    assert(std::fabs(snapshot.min - 1.0) < 0.000001);
    assert(std::fabs(snapshot.max - 5.0) < 0.000001);
    assert(std::fabs(snapshot.average - 3.0) < 0.000001);
    assert(std::fabs(snapshot.median - 3.0) < 0.000001);
    stream.Close();
}

void TestGeneratedStressStream() {
    const std::size_t count = 100000;
    double seed[] = {1.0};
    MutableArraySequence<double> firstItems(seed, 1);
    LazySequence<double> naturalNumbers(NextNaturalNumber, firstItems);
    ReadOnlyStream<double> stream(naturalNumbers);

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, count);
    stream.Close();

    OnlineStatisticsSnapshot snapshot = statistics.Snapshot();
    assert(snapshot.count == count);
    assert(std::fabs(snapshot.sum - 5000050000.0) < 0.000001);
    assert(std::fabs(snapshot.min - 1.0) < 0.000001);
    assert(std::fabs(snapshot.max - 100000.0) < 0.000001);
    assert(std::fabs(snapshot.average - 50000.5) < 0.000001);
    assert(std::fabs(snapshot.median - 50000.5) < 0.000001);
}

void TestExceptions() {
    LazySequence<int> empty;

    bool getThrown = false;
    try {
        empty.GetFirst();
    } catch (const IndexOutOfRange&) {
        getThrown = true;
    }
    assert(getThrown);

    bool lengthThrown = false;
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    LazySequence<int> infinite(
        [](Sequence<int>* history) {
            return history->GetLast() + 1;
        },
        firstItems);

    try {
        infinite.GetLength();
    } catch (const std::overflow_error&) {
        lengthThrown = true;
    }
    assert(lengthThrown);
}

}

void RunAllTests() {
    TestFiniteLazySequence();
    TestRecurrentLazySequence();
    TestAdditionalRecurrentLazySequenceExamples();
    TestLazyEditingOperations();
    TestSubsequenceAndConcat();
    TestInfiniteInsertAndAppend();
    TestInfiniteConcatJump();
    TestThreeInfiniteConcatJump();
    TestMapWhereZipReduce();
    TestOrdinalProviderMapZipAndSubsequence();
    TestOrdinalInsertAndWhereOverConcat();
    TestGeneratorAppendItem();
    TestGeneratorAppendSequence();
    TestGeneratorInsertItem();
    TestGeneratorInsertSequence();
    TestGeneratorRemoveItem();
    TestGeneratorRemoveSequence();
    TestGeneratorOperationExceptions();
    TestStreams();
    TestStringStreamAndWriteStream();
    TestOnlineStatistics();
    TestGeneratedStressStream();
    TestExceptions();
}
