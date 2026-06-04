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

    const LazySequence<int>* prepended = original.Prepend(1);
    const LazySequence<int>* appended = prepended->Append(4);
    const LazySequence<int>* inserted = appended->InsertAt(99, 2);

    assert(original.GetLength() == 2);
    assert(original.Get(0) == 2);
    assert(inserted->GetLength() == 5);
    assert(inserted->Get(0) == 1);
    assert(inserted->Get(2) == 99);
    assert(inserted->Get(3) == 3);
    assert(inserted->Get(4) == 4);

    delete inserted;
    delete appended;
    delete prepended;
}

void TestSubsequenceAndConcat() {
    int leftData[] = {1, 2, 3};
    int rightData[] = {4, 5};
    LazySequence<int> left(leftData, 3);
    LazySequence<int> right(rightData, 2);

    const LazySequence<int>* sub = left.GetSubsequence(1, 2);
    const LazySequence<int>* joined = left.Concat(right);
    const LazySequence<int>* insertedIntoJoined = joined->InsertAt(99, 4);

    assert(sub->GetLength() == 2);
    assert(sub->Get(0) == 2);
    assert(sub->Get(1) == 3);
    assert(joined->GetLength() == 5);
    assert(joined->Get(3) == 4);
    assert(joined->Get(4) == 5);
    assert(insertedIntoJoined->GetLength() == 6);
    assert(insertedIntoJoined->Get(3) == 4);
    assert(insertedIntoJoined->Get(4) == 99);
    assert(insertedIntoJoined->Get(5) == 5);

    delete insertedIntoJoined;
    delete joined;
    delete sub;
}

void TestInfiniteInsertAndAppend() {
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    const LazySequence<int> naturalNumbers(
        [](Sequence<int>* history) {
            return history->GetLast() + 1;
        },
        firstItems);

    const LazySequence<int>* inserted = naturalNumbers.InsertAt(99, 2);
    const LazySequence<int>* appended = naturalNumbers.Append(500);

    assert(inserted->Get(0) == 1);
    assert(inserted->Get(1) == 2);
    assert(inserted->Get(2) == 99);
    assert(inserted->Get(3) == 3);
    assert(appended->Get(100) == 101);
    assert(!appended->GetLengthOrdinal().IsFinite());
    assert(appended->GetLengthOrdinal() == Ordinal(1, 1));
    assert(appended->Get(Ordinal::Omega()) == 500);

    const LazySequence<int>* appendedTwice = appended->Append(600);
    assert(appendedTwice->GetLengthOrdinal() == Ordinal(1, 2));
    assert(appendedTwice->Get(Ordinal(1, 0)) == 500);
    assert(appendedTwice->Get(Ordinal(1, 1)) == 600);
    assert(appendedTwice->GetLast() == 600);

    delete appendedTwice;
    delete appended;
    delete inserted;
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

    const LazySequence<int>* joined = naturalNumbers.Concat(tens);

    assert(!joined->GetLengthOrdinal().IsFinite());
    assert(joined->HasConcatParts());
    assert(joined->Get(4) == 5);
    assert(joined->GetConcatPart(0, 4) == 5);
    assert(joined->GetConcatPart(1, 0) == 10);
    assert(joined->GetConcatPart(1, 3) == 40);
    assert(joined->GetLengthOrdinal() == Ordinal(2, 0));
    assert(joined->Get(Ordinal::Omega()) == 10);
    assert(joined->Get(Ordinal(1, 3)) == 40);

    delete joined;
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

    const LazySequence<int>* firstJoin = naturalNumbers.Concat(tens);
    const LazySequence<int>* secondJoin = firstJoin->Concat(hundreds);

    assert(!secondJoin->GetLengthOrdinal().IsFinite());
    assert(secondJoin->HasConcatParts());
    assert(secondJoin->Get(4) == 5);
    assert(secondJoin->GetConcatPart(1, 2) == 300);
    assert(secondJoin->GetConcatPart(0, 4) == 5);
    assert(secondJoin->GetConcatPart(0, 1) == 2);
    assert(secondJoin->GetLengthOrdinal() == Ordinal(3, 0));
    assert(secondJoin->Get(Ordinal(2, 0)) == 100);
    assert(secondJoin->Get(Ordinal(2, 2)) == 300);

    delete secondJoin;
    delete firstJoin;
}

void TestMapWhereZipReduce() {
    int data[] = {1, 2, 3, 4};
    LazySequence<int> sequence(data, 4);

    LazySequence<int>* squares = sequence.Map([](int value) {
        return value * value;
    });
    LazySequence<int>* evens = sequence.Where(IsEven);
    LazySequence<std::pair<int, int>>* zipped = sequence.Zip(*squares);

    assert(squares->Get(2) == 9);
    assert(evens->GetLength() == 2);
    assert(evens->Get(0) == 2);
    assert(evens->Get(1) == 4);
    assert(zipped->GetLength() == 4);
    assert(zipped->Get(2).first == 3);
    assert(zipped->Get(2).second == 9);
    assert(sequence.Reduce([](int sum, int value) { return sum + value; }, 0) == 10);

    delete zipped;
    delete evens;
    delete squares;
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
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });

    Generator<int>* appended = generator.Append(4);
    int expected[] = {1, 2, 3, 4};
    AssertFiniteGeneratorValues(*appended, expected, 4);
    delete appended;
}

void TestGeneratorAppendSequence() {
    int data[] = {1, 2};
    int suffixData[] = {3, 4, 5};
    LazySequence<int> owner(data, 2);
    MutableArraySequence<int> suffix(suffixData, 3);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });

    Generator<int>* appended = generator.Append(&suffix);
    int expected[] = {1, 2, 3, 4, 5};
    AssertFiniteGeneratorValues(*appended, expected, 5);
    delete appended;
}

void TestGeneratorInsertItem() {
    int data[] = {1, 3};
    LazySequence<int> owner(data, 2);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });

    generator.Reset(1);
    Generator<int>* inserted = generator.Insert(2);
    int expected[] = {1, 2, 3};
    AssertFiniteGeneratorValues(*inserted, expected, 3);
    delete inserted;
}

void TestGeneratorInsertSequence() {
    int data[] = {1, 4, 5};
    int middleData[] = {2, 3};
    LazySequence<int> owner(data, 3);
    MutableArraySequence<int> middle(middleData, 2);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });

    generator.Reset(1);
    Generator<int>* inserted = generator.Insert(&middle);
    int expected[] = {1, 2, 3, 4, 5};
    AssertFiniteGeneratorValues(*inserted, expected, 5);
    delete inserted;
}

void TestGeneratorRemoveItem() {
    int data[] = {1, 2, 3, 4};
    LazySequence<int> owner(data, 4);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });

    generator.Reset(1);
    Generator<int>* removed = generator.Remove(2);
    int expected[] = {1, 3, 4};
    AssertFiniteGeneratorValues(*removed, expected, 3);
    delete removed;
}

void TestGeneratorRemoveSequence() {
    int data[] = {1, 2, 3, 4, 5};
    int removedData[] = {2, 3};
    LazySequence<int> owner(data, 5);
    MutableArraySequence<int> removedItems(removedData, 2);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });

    generator.Reset(1);
    Generator<int>* removed = generator.Remove(&removedItems);
    int expected[] = {1, 4, 5};
    AssertFiniteGeneratorValues(*removed, expected, 3);
    delete removed;
}

void TestGeneratorOperationExceptions() {
    int data[] = {1};
    LazySequence<int> owner(data, 1);
    Generator<int> generator(&owner, owner.GetLengthOrdinal(), [](int) {
        return 0;
    });
    Sequence<int>* nullSequence = nullptr;

    bool appendNullThrown = false;
    try {
        Generator<int>* appended = generator.Append(nullSequence);
        delete appended;
    } catch (const std::invalid_argument&) {
        appendNullThrown = true;
    }
    assert(appendNullThrown);

    bool insertOutOfRangeThrown = false;
    generator.Reset(2);
    try {
        Generator<int>* inserted = generator.Insert(9);
        delete inserted;
    } catch (const IndexOutOfRange&) {
        insertOutOfRangeThrown = true;
    }
    assert(insertOutOfRangeThrown);

    bool removeOutOfRangeThrown = false;
    generator.Reset(1);
    try {
        Generator<int>* removed = generator.Remove(1);
        delete removed;
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
