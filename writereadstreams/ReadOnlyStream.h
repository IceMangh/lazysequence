#pragma once

#include <cstddef>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "LazySequence.h"
#include "MutableArraySequence.h"
#include "StreamExceptions.h"

template <class T>
class ReadOnlyStream {
private:
    struct FileTag {};

    enum class Mode {
        Indexed,
        File
    };

    Mode mode_;
    std::shared_ptr<Sequence<T>> sequence_;
    std::string filePath_;
    std::function<T(const std::string&)> deserializer_;
    std::ifstream input_;
    std::size_t position_;
    bool opened_;
    bool canSeek_;
    bool hasLength_;
    std::size_t length_;

    ReadOnlyStream(FileTag, std::string filePath, std::function<T(const std::string&)> deserializer)
        : mode_(Mode::File),
          sequence_(nullptr),
          filePath_(filePath),
          deserializer_(deserializer),
          input_(),
          position_(0),
          opened_(false),
          canSeek_(false),
          hasLength_(false),
          length_(0) {}

public:
    explicit ReadOnlyStream(const Sequence<T>& sequence)
        : mode_(Mode::Indexed),
          sequence_(sequence.Clone()),
          filePath_(),
          deserializer_(nullptr),
          input_(),
          position_(0),
          opened_(false),
          canSeek_(true),
          hasLength_(true),
          length_(0) {
            if (const auto* lazy = dynamic_cast<const LazySequence<T>*>(&sequence)) {
                if (!lazy->GetLengthOrdinal().IsFinite()) {
                    hasLength_ = false;
                } else {
                    length_ = lazy->GetLengthOrdinal().FiniteValue();
                }
            } else {
                length_ = static_cast<std::size_t>(sequence.GetLength());
            }
        }

    static ReadOnlyStream<T> FromFile(const std::string& filePath, std::function<T(const std::string&)> deserializer) {
        return ReadOnlyStream<T>(FileTag{}, filePath, deserializer);
    }

    void Open() {
        if (opened_) {
            return;
        }

        if (mode_ == Mode::File) {
            input_.open(filePath_);
            if (!input_) {
                throw std::runtime_error("open failed");
            }
        }
        position_ = 0;
        opened_ = true;
    }

    void Close() {
        if (mode_ == Mode::File && input_.is_open()) {
            input_.close();
        }
        opened_ = false;
    }

    T Read() {
        if (!opened_) {
            throw StreamClosed();
        }

        if (mode_ == Mode::File) {
            std::string token;
            if (!(input_ >> token)) {
                throw EndOfStream();
            }
            ++position_;
            return deserializer_(token);
        }

        if (hasLength_ && position_ >= length_) {
            throw EndOfStream();
        }

        try {
            T value = sequence_->Get(static_cast<int>(position_));
            ++position_;
            return value;
        } catch (const std::out_of_range&) {
            throw EndOfStream();
        }
    }

    std::size_t GetPosition() const {
        return position_;
    }

    std::size_t Seek(std::size_t index) {
        if (!canSeek_) {
            throw std::logic_error("seek failed");
        }
        if (hasLength_ && index > length_) {
            throw EndOfStream();
        }
        position_ = index;
        return position_;
    }
};
