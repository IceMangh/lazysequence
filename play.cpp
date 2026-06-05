#include "LazySequence.h"

#include <cstddef>
#include <iostream>

int FibonacciRule(Sequence<int>* history) {
    const int length = history->GetLength();
    return history->Get(length - 1) + history->Get(length - 2);
}

int PowersOfTwoRule(Sequence<int>* history) {
    return history->GetLast() * 2;
}

void PrintFirstItems(const char* name, const LazySequence<int>& sequence, int count) {
    std::cout << name << ": ";
    for (int i = 0; i < count; ++i) {
        std::cout << sequence.Get(i);
        if (i + 1 < count) {
            std::cout << ' ';
        }
    }
    std::cout << '\n';
}

int main() {
    int fibonacciSeedData[] = {1, 1};
    MutableArraySequence<int> fibonacciSeed(fibonacciSeedData, 2);
    LazySequence<int> fibonacci(FibonacciRule, fibonacciSeed);

    int powersSeedData[] = {1};
    MutableArraySequence<int> powersSeed(powersSeedData, 1);
    LazySequence<int> powersOfTwo(PowersOfTwoRule, powersSeed);

    LazySequence<int> squares(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            const int value = static_cast<int>(index.FiniteValue());
            return value * value;
        });

    LazySequence<int> interleaved(
        Ordinal::Omega(),
        [&fibonacci, &powersOfTwo, &squares](const Ordinal& index) -> int {
            if (!index.IsFinite()) {
                throw IndexOutOfRange();
            }
            const std::size_t number = index.FiniteValue();
            const Ordinal sourceIndex = Ordinal::Finite(number / 3);
            const std::size_t sourceNumber = number % 3;

            if (sourceNumber == 0) {
                return fibonacci.Get(sourceIndex);
            }
            if (sourceNumber == 1) {
                return powersOfTwo.Get(sourceIndex);
            }
            return squares.Get(sourceIndex);
        });

    PrintFirstItems("Fibonacci", fibonacci, 10);
    PrintFirstItems("Powers of two", powersOfTwo, 10);
    PrintFirstItems("Squares", squares, 10);
    PrintFirstItems("Interleaved", interleaved, 28);
    return 0;
}
