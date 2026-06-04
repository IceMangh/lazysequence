#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <string>

#include "LazySequence.h"
#include "MutableArraySequence.h"
#include "OnlineStatistics.h"
#include "Tests.h"

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

double NextNaturalNumber(Sequence<double>* history) {
    return history->GetLast() + 1.0;
}

void PrintSequencePrefix(const LazySequence<int>& sequence, int count) {
    for (int i = 0; i < count; ++i) {
        std::cout << sequence.Get(i) << (i == count - 1 ? '\n' : ' ');
    }
}

void PrintSnapshot(const OnlineStatisticsSnapshot& snapshot) {
    std::cout << "Count: " << snapshot.count << "\n";
    std::cout << "Sum: " << snapshot.sum << "\n";
    std::cout << "Min: " << snapshot.min << "\n";
    std::cout << "Max: " << snapshot.max << "\n";
    std::cout << "Average: " << snapshot.average << "\n";
    std::cout << "Median: " << snapshot.median << "\n";
}

void PrintFibonacciDemo() {
    int seed[] = {1, 1};
    MutableArraySequence<int> firstItems(seed, 2);
    const LazySequence<int> fibonacci(FibonacciRule, firstItems);

    PrintSequencePrefix(fibonacci, 22);
    std::cout << "Materialized items: " << fibonacci.GetMaterializedCount() << "\n";

    const LazySequence<int>* edited = fibonacci.InsertAt(1000, 5);
    PrintSequencePrefix(*edited, 10);
    delete edited;
}

void PrintFactorialDemo() {
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    const LazySequence<int> factorials(FactorialRule, firstItems);

    std::cout << "\nFirst 8 factorials:\n";
    PrintSequencePrefix(factorials, 8);
    std::cout << "Materialized items: " << factorials.GetMaterializedCount() << "\n";
}

void PrintPowersOfTwoDemo() {
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    const LazySequence<int> powersOfTwo(PowersOfTwoRule, firstItems);

    std::cout << "\nFirst 12 powers of two:\n";
    PrintSequencePrefix(powersOfTwo, 12);
    std::cout << "Materialized items: " << powersOfTwo.GetMaterializedCount() << "\n";
}

void PrintLazySequenceDemos() {
    PrintFibonacciDemo();
    PrintFactorialDemo();
    PrintPowersOfTwoDemo();
}

OnlineStatisticsSnapshot CollectStatisticsFromText(const std::string& text) {
    ReadOnlyStream<double> stream(text, [](const std::string& token) {
        return std::stod(token);
    });

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, std::numeric_limits<std::size_t>::max());
    stream.Close();
    return statistics.Snapshot();
}

void RunPreparedStatisticsSamples() {
    std::cout << "Prepared dataset: shuffled odd count\n";
    PrintSnapshot(CollectStatisticsFromText("5 1 3 2 4"));

    std::cout << "\nPrepared dataset: even count with negatives\n";
    PrintSnapshot(CollectStatisticsFromText("-10 0 10 20"));

    std::cout << "\nPrepared dataset: repeated values\n";
    PrintSnapshot(CollectStatisticsFromText("7 7 7 7 7 7"));
}

void RunGeneratedStressStatistics() {
    std::cout << "Enter generated stream length, for example 1000000:\n";
    std::size_t count = 0;
    std::cin >> count;
    if (count == 0) {
        std::cout << "Length must be positive.\n";
        return;
    }

    double seed[] = {1.0};
    MutableArraySequence<double> firstItems(seed, 1);
    LazySequence<double> naturalNumbers(NextNaturalNumber, firstItems);
    ReadOnlyStream<double> stream(naturalNumbers);

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, count);
    stream.Close();

    PrintSnapshot(statistics.Snapshot());
}

void RunFileStatistics() {
    std::cout << "Enter file path with space-separated numbers:\n";
    std::string path;
    std::cin >> path;

    ReadOnlyStream<double> stream = ReadOnlyStream<double>::FromFile(path, [](const std::string& token) {
        return std::stod(token);
    });

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, std::numeric_limits<std::size_t>::max());
    stream.Close();

    if (statistics.IsEmpty()) {
        std::cout << "No numbers were read.\n";
        return;
    }

    PrintSnapshot(statistics.Snapshot());
}

void RunManualStatistics() {
    std::cout << "Enter numbers separated by spaces:\n";
    std::string line;
    std::getline(std::cin >> std::ws, line);

    ReadOnlyStream<double> stream(line, [](const std::string& token) {
        return std::stod(token);
    });

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, std::numeric_limits<std::size_t>::max());
    stream.Close();

    if (statistics.IsEmpty()) {
        std::cout << "No numbers were read.\n";
        return;
    }

    PrintSnapshot(statistics.Snapshot());
}

void PrintMenu() {
    std::cout << "1. Run automatic tests\n";
    std::cout << "2. Show LazySequence demos\n";
    std::cout << "3. Collect stream statistics manually\n";
    std::cout << "4. Run prepared statistics datasets\n";
    std::cout << "5. Run generated stress stream\n";
    std::cout << "6. Collect stream statistics from file\n";
    std::cout << "0. Exit\n";
    std::cout << "> ";
}

}  //namespace

int main() {
    while (true) {
        PrintMenu();

        int command = -1;
        if (!(std::cin >> command)) {
            return 0;
        }

        try {
            if (command == 0) {
                return 0;
            }
            if (command == 1) {
                RunAllTests();
                std::cout << "All tests passed.\n";
            } else if (command == 2) {
                PrintLazySequenceDemos();
            } else if (command == 3) {
                RunManualStatistics();
            } else if (command == 4) {
                RunPreparedStatisticsSamples();
            } else if (command == 5) {
                RunGeneratedStressStatistics();
            } else if (command == 6) {
                RunFileStatistics();
            } else {
                std::cout << "Unknown command.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << "\n";
        }
    }
}
