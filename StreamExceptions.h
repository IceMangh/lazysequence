#pragma once

#include <stdexcept>

class EndOfStream : public std::out_of_range {
public:
    EndOfStream() : std::out_of_range("End of stream") {}
};

class StreamClosed : public std::logic_error {
public:
    StreamClosed() : std::logic_error("Stream is closed") {}
};
