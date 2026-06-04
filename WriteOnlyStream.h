#pragma once

#include <cstddef>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

#include "LazySequence.h"
#include "MutableArraySequence.h"
#include "StreamExceptions.h"

template <class T>
class WriteOnlyStream {
public:
    using Serializer = std::function<std::string(const T&)>;

private:
    enum class Mode {
        Memory,
        File
    };

    Mode mode_;
    MutableArraySequence<T> values_;
    std::string filePath_;
    Serializer serializer_;
    std::ofstream output_;
    std::size_t position_;
    bool opened_;

    explicit WriteOnlyStream(std::string filePath, Serializer serializer)
        : mode_(Mode::File),
          values_(),
          filePath_(std::move(filePath)),
          serializer_(std::move(serializer)),
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

    static WriteOnlyStream<T> ToFile(const std::string& filePath, Serializer serializer) {
        return WriteOnlyStream<T>(filePath, std::move(serializer));
    }

    void Open() {
        if (opened_) {
            return;
        }
        if (mode_ == Mode::File) {
            output_.open(filePath_);
            if (!output_) {
                throw std::runtime_error("Cannot open output stream file");
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
            throw std::logic_error("Only memory stream can be converted to sequence");
        }
        return LazySequence<T>(values_);
    }
};
