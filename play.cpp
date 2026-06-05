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
    int fibonacciData[] = {1, 1};
    MutableArraySequence<int> fibonacciSeed(fibonacciData, 2);
    LazySequence<int> A(FibonacciRule, fibonacciSeed);

    int powersData[] = {1};
    MutableArraySequence<int> powersSeed(powersData, 1);
    LazySequence<int> B(PowersOfTwoRule, powersSeed);

    LazySequence<int> C(
        Ordinal::Omega(),
        [](const Ordinal& index) {
            const int value = static_cast<int>(index.FiniteValue());
            return value * value;
        });

    LazySequence<int> interleaved(
        Ordinal::Omega(),
        [&A, &B, &C](const Ordinal& index) -> int {
            const int indexNumber = static_cast<int>(index.FiniteValue());
            const int itemIndex = indexNumber / 3;
            const int sequenceNumber = indexNumber % 3;

            if (sequenceNumber == 0) {
                return A.Get(itemIndex);
            }
            if (sequenceNumber == 1) {
                return B.Get(itemIndex);
            }
            return C.Get(itemIndex);
        });

    PrintFirstItems("Fibonacci", A, 10);
    PrintFirstItems("Two^", B, 10);
    PrintFirstItems("Squares", C, 10);
    PrintFirstItems("Interleaved", interleaved, 28);
    return 0;
}
