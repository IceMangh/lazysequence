#pragma once
#include <stdexcept>

class IndexOutOfRange : public std::out_of_range {
public:
    IndexOutOfRange() : std::out_of_range("Index out of range") {}
};

class EmptyStructure : public std::out_of_range {
public:
    EmptyStructure() : std::out_of_range("Structure is empty") {}

    explicit EmptyStructure(const char* message) : std::out_of_range(message) {}
};
