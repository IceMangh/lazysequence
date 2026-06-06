#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

class Ordinal {
private:
    std::size_t omegaBlocks_;
    std::size_t finiteOffset_;

public:
    Ordinal() : omegaBlocks_(0), finiteOffset_(0) {}

    Ordinal(std::size_t omegaBlocks, std::size_t finiteOffset)
        : omegaBlocks_(omegaBlocks), finiteOffset_(finiteOffset) {}

    static Ordinal Finite(std::size_t value) {
        return Ordinal(0, value);
    }

    static Ordinal Omega() {
        return Ordinal(1, 0);
    }

    bool IsFinite() const {
        return omegaBlocks_ == 0;
    }

    bool IsZero() const {
        return omegaBlocks_ == 0 && finiteOffset_ == 0;
    }

    std::size_t FiniteValue() const {
        if (!IsFinite()) {
            throw std::overflow_error("non-finite ordinal");
        }
        return finiteOffset_;
    }

    std::size_t OmegaBlocks() const {
        return omegaBlocks_;
    }

    std::size_t FiniteOffset() const {
        return finiteOffset_;
    }

    Ordinal Add(const Ordinal& other) const {
        if (other.omegaBlocks_ == 0) {
            if (finiteOffset_ > std::numeric_limits<std::size_t>::max() - other.finiteOffset_) {
                throw std::overflow_error("ordinal overflow");
            }
            return Ordinal(omegaBlocks_, finiteOffset_ + other.finiteOffset_);
        }

        if (omegaBlocks_ > std::numeric_limits<std::size_t>::max() - other.omegaBlocks_) {
            throw std::overflow_error("ordinal overflow");
        }
        return Ordinal(omegaBlocks_ + other.omegaBlocks_, other.finiteOffset_);
    }

    Ordinal Successor() const {
        return Add(Ordinal::Finite(1));
    }

    Ordinal SubtractPrefix(const Ordinal& prefix) const {
        if (*this < prefix) {
            throw std::out_of_range("bad prefix");
        }
        if (omegaBlocks_ == prefix.omegaBlocks_) {
            return Ordinal::Finite(finiteOffset_ - prefix.finiteOffset_);
        }
        return Ordinal(omegaBlocks_ - prefix.omegaBlocks_, finiteOffset_);
    }

    std::optional<Ordinal> Predecessor() const {
        if (finiteOffset_ == 0) {
            return std::nullopt;
        }
        return Ordinal(omegaBlocks_, finiteOffset_ - 1);
    }

    std::string ToString() const {
        if (omegaBlocks_ == 0) {
            return std::to_string(finiteOffset_);
        }

        std::string result = "omega";
        if (omegaBlocks_ > 1) {
            result += "*" + std::to_string(omegaBlocks_);
        }
        if (finiteOffset_ > 0) {
            result += "+" + std::to_string(finiteOffset_);
        }
        return result;
    }

    bool operator==(const Ordinal& other) const {
        return omegaBlocks_ == other.omegaBlocks_ && finiteOffset_ == other.finiteOffset_;
    }

    bool operator!=(const Ordinal& other) const {
        return !(*this == other);
    }

    bool operator<(const Ordinal& other) const {
        if (omegaBlocks_ != other.omegaBlocks_) {
            return omegaBlocks_ < other.omegaBlocks_;
        }
        return finiteOffset_ < other.finiteOffset_;
    }

    bool operator<=(const Ordinal& other) const {
        return *this < other || *this == other;
    }

    bool operator>(const Ordinal& other) const {
        return other < *this;
    }

    bool operator>=(const Ordinal& other) const {
        return other <= *this;
    }
};

inline Ordinal MinOrdinal(const Ordinal& left, const Ordinal& right) {
    return left < right ? left : right;
}
