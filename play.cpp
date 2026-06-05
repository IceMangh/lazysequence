#include "LazySequence.h"

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

    int data[] = {1, 2, 3};
    LazySequence<int> first(data, 3);

    PrintFirstItems("Fibonacci", fibonacci, 10);
    PrintFirstItems("Powers of two", powersOfTwo, 10);
    PrintFirstItems("Squares", squares, 10);

    std::cout << "fibonacci.Get(9) = " << fibonacci.Get(9) << '\n';
    std::cout << "powersOfTwo.Get(10) = " << powersOfTwo.Get(10) << '\n';
    std::cout << "squares.Get(12) = " << squares.Get(12) << '\n';
    std::cout << "first.Get(1) = " << first.Get(1) << '\n';

    LazySequence<int> firstJoin = fibonacci.concat(powersOfTwo);
    LazySequence<int> result = firstJoin.concat(first);

    std::cout << result.Get(Ordinal(1, 5)) << std::endl;
    std::cout << result.GetLengthOrdinal().ToString() << std::endl;
    std::cout << result.GetConcatPart(0, 5) << std::endl;
    return 0;
}
