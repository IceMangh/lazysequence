#pragma once

#include <cstddef>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>

#include "LazySequence.h"
#include "MutableArraySequence.h"
#include "StreamExceptions.h"

template <class T>
class WriteOnlyStream {
private:
    enum class Mode {
        Memory,
        File
    };

    Mode mode_;
    MutableArraySequence<T> values_;
    std::string filePath_;
    std::function<std::string(const T&)> serializer_;
    std::ofstream output_;
    std::size_t position_;
    bool opened_;

    explicit WriteOnlyStream(std::string filePath, std::function<std::string(const T&)> serializer)
        : mode_(Mode::File),
          values_(),
          filePath_(filePath),
          serializer_(serializer),
          output_(),
          position_(0),
          opened_(false) {}

public:
    WriteOnlyStream()
        : mode_(Mode::Memory),
          values_(),
          filePath_(),
          serializer_(nullptr),
          output_(),
          position_(0),
          opened_(false) {}

    static WriteOnlyStream<T> ToFile(const std::string& filePath, std::function<std::string(const T&)> serializer) {
        return WriteOnlyStream<T>(filePath, serializer);
    }

    void Open() {
        if (opened_) {
            return;
        }
        if (mode_ == Mode::File) {
            output_.open(filePath_);
            if (!output_) {
                throw std::runtime_error("open failed");
            }
        }
        opened_ = true;
    }

    void Close() {
        if (mode_ == Mode::File && output_.is_open()) {
            output_.close();
        }
        opened_ = false;
    }

    std::size_t Write(const T& value) {
        if (!opened_) {
            throw StreamClosed();
        }

        if (mode_ == Mode::File) {
            output_ << serializer_(value) << '\n';
        } else {
            values_.Append(value);
        }

        ++position_;
        return position_;
    }

    std::size_t GetPosition() const {
        return position_;
    }

    LazySequence<T> ToSequence() const {
        if (mode_ != Mode::Memory) {
            throw std::logic_error("not memory stream");
        }
        return LazySequence<T>(values_);
    }
};
