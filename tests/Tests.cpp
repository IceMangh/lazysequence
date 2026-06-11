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

bool IsEven(int value) {
    return value % 2 == 0;
}

// Последовательность, созданная из обычного массива.
void TestArrayBackedLazySequence() {
    int sourceItems[] = {1, 2, 3};
    LazySequence<int> sequence(sourceItems, 3);

    assert(sequence.GetLength() == 3);
    assert(sequence.GetLengthOrdinal().FiniteValue() == 3);
    assert(sequence.GetFirst() == 1);
    assert(sequence.GetLast() == 3);
    assert(sequence.Get(1) == 2);
    assert(sequence.GetMaterializedCount() == 3);
}

// Бесконечная рекуррентная последовательность на примере чисел Фибоначчи.
void TestFibonacciRecurrence() {
    int fibonacciSeed[] = {1, 1};
    MutableArraySequence<int> firstFibonacciItems(fibonacciSeed, 2);
    LazySequence<int> fibonacci(FibonacciRule, firstFibonacciItems);

    assert(!fibonacci.GetLengthOrdinal().IsFinite());
    assert(fibonacci.GetMaterializedCount() == 2);
    assert(fibonacci.Get(0) == 1);
    assert(fibonacci.Get(1) == 1);
    assert(fibonacci.Get(9) == 55);
    assert(fibonacci.GetMaterializedCount() == 10);
}

// Вставка, добавление, конкатенация и подпоследовательность для конечных данных.
void TestFiniteEditingOperations() {
    int originalItems[] = {2, 3};
    const LazySequence<int> original(originalItems, 2);
    int rightData[] = {4, 5};
    LazySequence<int> right(rightData, 2);

    LazySequence<int> prepended = original.PrependItem(1);
    LazySequence<int> appended = prepended.AppendItem(4);
    LazySequence<int> inserted = appended.InsertItemAt(99, 2);
    LazySequence<int> middleAndLast = inserted.Subsequence(2, 4);
    LazySequence<int> leftThenRight = original.Concat(right);

    assert(original.GetLength() == 2);
    assert(original.Get(0) == 2);
    assert(inserted.GetLength() == 5);
    assert(inserted.Get(0) == 1);
    assert(inserted.Get(2) == 99);
    assert(inserted.Get(3) == 3);
    assert(inserted.Get(4) == 4);
    assert(middleAndLast.Get(0) == 99);
    assert(middleAndLast.Get(2) == 4);
    assert(leftThenRight.GetLength() == 4);
    assert(leftThenRight.Get(2) == 4);
    assert(leftThenRight.Get(3) == 5);
}

// Конкатенация двух бесконечных последовательностей: omega + omega.
void TestConcatOfTwoOmegaSequences() {
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

    LazySequence<int> naturalsThenTens = naturalNumbers.Concat(tens);

    assert(!naturalsThenTens.GetLengthOrdinal().IsFinite());
    assert(naturalsThenTens.Get(4) == 5);
    assert(naturalsThenTens.GetLengthOrdinal() == Ordinal(2, 0));
    assert(naturalsThenTens.Get(Ordinal::Omega()) == 10);
    assert(naturalsThenTens.Get(Ordinal(1, 3)) == 40);
}

// Основные операции над последовательностью: Map, Where, Zip и Reduce.
void TestMapWhereZipAndReduce() {
    int sourceItems[] = {1, 2, 3, 4};
    LazySequence<int> sequence(sourceItems, 4);

    LazySequence<int> squares = sequence.Map<int>([](int value) {
        return value * value;
    });
    LazySequence<int> evenValues = sequence.Where(IsEven);
    LazySequence<std::pair<int, int>> zipped = sequence.Zip(squares);

    assert(squares.Get(2) == 9);
    assert(evenValues.GetLength() == 2);
    assert(evenValues.Get(0) == 2);
    assert(evenValues.Get(1) == 4);
    assert(zipped.GetLength() == 4);
    assert(zipped.Get(2).first == 3);
    assert(zipped.Get(2).second == 9);
    assert(sequence.Reduce([](int sum, int value) { return sum + value; }, 0) == 10);
}

// Смешивание трех последовательностей по очереди: первая, вторая, третья.
void TestMixWith() {
    int firstData[] = {1, 2, 3};
    int secondData[] = {10, 20, 30};
    int thirdData[] = {100, 200, 300};
    LazySequence<int> first(firstData, 3);
    LazySequence<int> second(secondData, 3);
    LazySequence<int> third(thirdData, 3);

    LazySequence<int> mixed = first.MixWith(second, third);

    assert(mixed.GetLength() == 9);
    assert(mixed.Get(0) == 1);
    assert(mixed.Get(1) == 10);
    assert(mixed.Get(2) == 100);
    assert(mixed.Get(3) == 2);
    assert(mixed.Get(4) == 20);
    assert(mixed.Get(5) == 200);
}

// Ordinal-провайдеры, Map/Zip и срез через границу omega-блоков.
void TestOrdinalProviderMapZipAndCrossBlockSubsequence() {
    LazySequence<int> ordinalProvider(
        Ordinal(2, 0),
        [](const Ordinal& index) {
            return static_cast<int>(index.OmegaBlocks() * 1000 + index.FiniteOffset());
        });

    assert(ordinalProvider.Get(Ordinal(1, 5)) == 1005);
    assert(ordinalProvider.GetMaterializedCount() == 1);

    LazySequence<int> firstOmegaBlock(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 1000 + static_cast<int>(index.FiniteValue());
        });
    LazySequence<int> secondOmegaBlock(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 2000 + static_cast<int>(index.FiniteValue());
        });
    LazySequence<int> thirdOmegaBlock(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            return 3000 + static_cast<int>(index.FiniteValue());
        });

    LazySequence<int> firstTwoBlocks = firstOmegaBlock.Concat(secondOmegaBlock);
    LazySequence<int> threeBlocks = firstTwoBlocks.Concat(thirdOmegaBlock);
    LazySequence<int> mapped = threeBlocks.Map<int>([](int value) {
        return value * 10;
    });
    LazySequence<std::pair<int, int>> zipped = threeBlocks.Zip(mapped);
    LazySequence<int> crossBlockSlice = threeBlocks.Subsequence(Ordinal(1, 2), Ordinal(2, 1));

    assert(mapped.Get(Ordinal(2, 3)) == 30030);
    assert(zipped.Get(Ordinal(1, 2)).first == 2002);
    assert(zipped.Get(Ordinal(1, 2)).second == 20020);
    assert(crossBlockSlice.GetLengthOrdinal() == Ordinal(1, 2));
    assert(crossBlockSlice.Get(0) == 2002);
    assert(crossBlockSlice.Get(Ordinal::Omega()) == 3000);
    assert(crossBlockSlice.Get(Ordinal(1, 1)) == 3001);
}

// Insert и Where для последовательностей с финит и инфинит.
void TestOrdinalInsertAndWhereOverConcatenatedOmegaSequences() {
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

    LazySequence<int> insertedAtFiniteIndex = naturals.InsertItemAt(77, 2);
    LazySequence<int> insertedAtOmega = naturals.InsertItemAt(88, Ordinal::Omega());
    LazySequence<int> naturalsThenTens = naturals.Concat(tens);
    LazySequence<int> evenValues = naturalsThenTens.Where([](int value) {
        return value % 2 == 0;
    });

    assert(insertedAtFiniteIndex.GetLengthOrdinal() == Ordinal::Omega());
    assert(insertedAtFiniteIndex.Get(0) == 0);
    assert(insertedAtFiniteIndex.Get(2) == 77);
    assert(insertedAtFiniteIndex.Get(3) == 2);
    assert(insertedAtOmega.GetLengthOrdinal() == Ordinal(1, 1));
    assert(insertedAtOmega.Get(Ordinal::Omega()) == 88);

    assert(evenValues.GetLengthOrdinal() == Ordinal(2, 0));
    assert(evenValues.Get(0) == 0);
    assert(evenValues.Get(1) == 2);
    assert(evenValues.Get(Ordinal::Omega()) == 100);
    assert(evenValues.Get(Ordinal(1, 2)) == 104);
}

// Чтение/запись потоков и вычисление статистики по потоку.
void TestStreamsAndStatistics() {
    int streamItems[] = {10, 20, 30};
    LazySequence<int> sequence(streamItems, 3);
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

    WriteOnlyStream<int> output;
    output.Open();
    output.Write(2);
    output.Write(4);
    output.Write(6);
    LazySequence<int> result = output.ToSequence();
    assert(output.GetPosition() == 3);
    assert(result.GetLength() == 3);
    assert(result.Get(0) == 2);
    assert(result.Get(2) == 6);
    output.Close();

    ReadOnlyStream<double> statisticsStream("5 1 3 2 4", [](const std::string& token) {
        return std::stod(token);
    });

    statisticsStream.Open();
    OnlineStatistics statistics = CollectStatistics(statisticsStream, 100);
    OnlineStatisticsSnapshot snapshot = statistics.Snapshot();

    assert(snapshot.count == 5);
    assert(std::fabs(snapshot.sum - 15.0) < 0.000001);
    assert(std::fabs(snapshot.min - 1.0) < 0.000001);
    assert(std::fabs(snapshot.max - 5.0) < 0.000001);
    assert(std::fabs(snapshot.average - 3.0) < 0.000001);
    assert(std::fabs(snapshot.median - 3.0) < 0.000001);
    statisticsStream.Close();
}

}

void RunAllTests() {
    TestArrayBackedLazySequence();
    TestFibonacciRecurrence();
    TestFiniteEditingOperations();
    TestConcatOfTwoOmegaSequences();
    TestMapWhereZipAndReduce();
    TestMixWith();
    TestOrdinalProviderMapZipAndCrossBlockSubsequence();
    TestOrdinalInsertAndWhereOverConcatenatedOmegaSequences();
    TestStreamsAndStatistics();
}
