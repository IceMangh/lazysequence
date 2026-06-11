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

int NaturalNumbersRule(const Ordinal& index) {
    return static_cast<int>(index.FiniteValue()) + 1;
}

int EvenNaturalNumbersRule(const Ordinal& index) {
    return (static_cast<int>(index.FiniteValue()) + 1) * 2;
}

int OddNaturalNumbersRule(const Ordinal& index) {
    return static_cast<int>(index.FiniteValue()) * 2 + 1;
}

int NegativeNumbersRule(const Ordinal& index) {
    return -(static_cast<int>(index.FiniteValue()) + 1);
}

int SquaresRule(const Ordinal& index) {
    const int value = static_cast<int>(index.FiniteValue());
    return value * value;
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
    int fibonacciData[] = {1, 1};
    MutableArraySequence<int> fibonacciSeed(fibonacciData, 2);
    LazySequence<int> fibonacci(FibonacciRule, fibonacciSeed);

    int powersData[] = {1};
    MutableArraySequence<int> powersSeed(powersData, 1);
    LazySequence<int> powersOfTwo(PowersOfTwoRule, powersSeed);

    LazySequence<int> squares(Ordinal::Omega(), SquaresRule);

    LazySequence<int> mixed = fibonacci.MixWith(powersOfTwo, squares);

    PrintFirstItems("mixed", mixed, 10);

    LazySequence<int> naturals(Ordinal::Omega(), NaturalNumbersRule);
    LazySequence<int> evens(Ordinal::Omega(), EvenNaturalNumbersRule);
    LazySequence<int> odds(Ordinal::Omega(), OddNaturalNumbersRule);
    LazySequence<int> negatives(Ordinal::Omega(), NegativeNumbersRule);

    LazySequence<int> A = naturals.Concat(negatives);
    PrintFirstItems("A", A, 10);

    LazySequence<int> B = A.AppendItem(42);
    B = B.AppendItem(32);
    std::cout << B.Get(Ordinal(1, 5)) << std::endl;
    std::cout << B.Get(Ordinal(2, 1)) << std::endl;
    return 0;
}
