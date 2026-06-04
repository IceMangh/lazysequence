#pragma once

#include <cstddef>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "LazySequence.h"
#include "MutableArraySequence.h"
#include "StreamExceptions.h"

template <class T>
class ReadOnlyStream {
public:
    using Deserializer = std::function<T(const std::string&)>;

private:
    struct FileTag {};

    enum class Mode {
        Indexed,
        File
    };

    Mode mode_;
    std::shared_ptr<Sequence<T>> sequence_;
    MutableArraySequence<T> values_;
    std::string filePath_;
    Deserializer deserializer_;
    std::ifstream input_;
    std::size_t position_;
    bool opened_;
    bool canSeek_;
    bool hasLength_;
    std::size_t length_;

    static MutableArraySequence<T> ParseTokens(const std::string& text, const Deserializer& deserializer) {
        MutableArraySequence<T> values;
        std::istringstream input(text);
        std::string token;
        while (input >> token) {
            values.Append(deserializer(token));
        }
        return values;
    }

    static const Sequence<T>& RequireSequence(const Sequence<T>* sequence) {
        if (sequence == nullptr) {
            throw std::invalid_argument("Sequence pointer is null");
        }
        return *sequence;
    }

    ReadOnlyStream(FileTag, std::string filePath, Deserializer deserializer)
        : mode_(Mode::File),
          sequence_(nullptr),
          values_(),
          filePath_(std::move(filePath)),
          deserializer_(std::move(deserializer)),
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
          values_(),
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

    explicit ReadOnlyStream(const Sequence<T>* sequence) : ReadOnlyStream(RequireSequence(sequence)) {}

    ReadOnlyStream(const std::string& text, Deserializer deserializer)
        : mode_(Mode::Indexed),
          sequence_(nullptr),
          values_(ParseTokens(text, deserializer)),
          filePath_(),
          deserializer_(std::move(deserializer)),
          input_(),
          position_(0),
          opened_(false),
          canSeek_(true),
          hasLength_(true),
          length_(static_cast<std::size_t>(values_.GetLength())) {}

    static ReadOnlyStream<T> FromFile(const std::string& filePath, Deserializer deserializer) {
        return ReadOnlyStream<T>(FileTag{}, filePath, std::move(deserializer));
    }

    void Open() {
        if (opened_) {
            return;
        }

        if (mode_ == Mode::File) {
            input_.open(filePath_);
            if (!input_) {
                throw std::runtime_error("Cannot open stream file");
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

    bool IsEndOfStream() const {
        if (mode_ == Mode::File) {
            return opened_ && input_.eof();
        }
        return hasLength_ && position_ >= length_;
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
            T value = sequence_ ? sequence_->Get(static_cast<int>(position_)) : values_.Get(static_cast<int>(position_));
            ++position_;
            return value;
        } catch (const std::out_of_range&) {
            throw EndOfStream();
        }
    }

    std::size_t GetPosition() const {
        return position_;
    }

    bool IsCanSeek() const {
        return canSeek_;
    }

    bool IsCanGoBack() const {
        return canSeek_;
    }

    std::size_t Seek(std::size_t index) {
        if (!canSeek_) {
            throw std::logic_error("Stream cannot seek");
        }
        if (hasLength_ && index > length_) {
            throw EndOfStream();
        }
        position_ = index;
        return position_;
    }
};
