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
    std::cout << "Количество: " << snapshot.count << "\n";
    std::cout << "Сумма: " << snapshot.sum << "\n";
    std::cout << "Минимум: " << snapshot.min << "\n";
    std::cout << "Максимум: " << snapshot.max << "\n";
    std::cout << "Среднее: " << snapshot.average << "\n";
    std::cout << "Медиана: " << snapshot.median << "\n";
}

void PrintFibonacciDemo() {
    int seed[] = {1, 1};
    MutableArraySequence<int> firstItems(seed, 2);
    const LazySequence<int> fibonacci(FibonacciRule, firstItems);

    std::cout << "Фибоначчи:\n";
    PrintSequencePrefix(fibonacci, 22);
    std::cout << "В кэше: " << fibonacci.GetMaterializedCount() << "\n";

    LazySequence<int> edited = fibonacci.InsertItemAt(1000, 5);
    std::cout << "После вставки:\n";
    PrintSequencePrefix(edited, 10);
}

void PrintFactorialDemo() {
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    const LazySequence<int> factorials(FactorialRule, firstItems);

    std::cout << "\nФакториалы:\n";
    PrintSequencePrefix(factorials, 8);
    std::cout << "В кэше: " << factorials.GetMaterializedCount() << "\n";
}

void PrintPowersOfTwoDemo() {
    int seed[] = {1};
    MutableArraySequence<int> firstItems(seed, 1);
    const LazySequence<int> powersOfTwo(PowersOfTwoRule, firstItems);

    std::cout << "\nСтепени двойки:\n";
    PrintSequencePrefix(powersOfTwo, 12);
    std::cout << "В кэше: " << powersOfTwo.GetMaterializedCount() << "\n";
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
    std::cout << "Набор 1:\n";
    PrintSnapshot(CollectStatisticsFromText("5 1 3 2 4"));

    std::cout << "\nНабор 2:\n";
    PrintSnapshot(CollectStatisticsFromText("-10 0 10 20"));

    std::cout << "\nНабор 3:\n";
    PrintSnapshot(CollectStatisticsFromText("7 7 7 7 7 7"));
}

void RunGeneratedStressStatistics() {
    std::cout << "Сколько чисел сгенерировать?\n";
    std::size_t count = 0;
    std::cin >> count;
    if (count == 0) {
        std::cout << "Число должно быть больше нуля.\n";
        return;
    }

    double seed[] = {1.0};
    MutableArraySequence<double> firstItems(seed, 1);
    LazySequence<double> naturalNumbers(NextNaturalNumber, firstItems);

    const std::size_t outputCount = count > 100 ? 100 : count;
    std::cout << "Сгенерированные числа:\n";
    for (std::size_t i = 0; i < outputCount; ++i) {
        std::cout << naturalNumbers.Get(static_cast<int>(i));
        std::cout << (i + 1 == outputCount ? '\n' : ' ');
    }
    if (outputCount < count) {
        std::cout << "Показаны первые " << outputCount << " из " << count << ".\n";
    }

    ReadOnlyStream<double> stream(naturalNumbers);

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, count);
    stream.Close();

    PrintSnapshot(statistics.Snapshot());
}

void RunFileStatistics() {
    std::cout << "Путь к файлу с числами:\n";
    std::string path;
    std::cin >> path;

    ReadOnlyStream<double> stream = ReadOnlyStream<double>::FromFile(path, [](const std::string& token) {
        return std::stod(token);
    });

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, std::numeric_limits<std::size_t>::max());
    stream.Close();

    if (statistics.IsEmpty()) {
        std::cout << "Числа не найдены.\n";
        return;
    }

    PrintSnapshot(statistics.Snapshot());
}

void RunManualStatistics() {
    std::cout << "Введите числа через пробел:\n";
    std::string line;
    std::getline(std::cin >> std::ws, line);

    ReadOnlyStream<double> stream(line, [](const std::string& token) {
        return std::stod(token);
    });

    stream.Open();
    OnlineStatistics statistics = CollectStatistics(stream, std::numeric_limits<std::size_t>::max());
    stream.Close();

    if (statistics.IsEmpty()) {
        std::cout << "Числа не найдены.\n";
        return;
    }

    PrintSnapshot(statistics.Snapshot());
}

void PrintMenu() {
    std::cout << "\nМеню\n";
    std::cout << "1. Тесты\n";
    std::cout << "2. Примеры LazySequence\n";
    std::cout << "3. Статистика по введенным числам\n";
    std::cout << "4. Готовые наборы чисел\n";
    std::cout << "5. Статистика по генератору\n";
    std::cout << "6. Статистика из файла\n";
    std::cout << "0. Выход\n";
    std::cout << "> ";
}

}

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
                std::cout << "Тесты прошли.\n";
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
                std::cout << "Нет такого пункта.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "Ошибка: " << error.what() << "\n";
        }
    }
}
