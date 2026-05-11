#pragma once

#include <cstdint>

class ITape {
public:
    virtual ~ITape() = default;
    virtual int32_t read() const = 0;
    virtual void write(int32_t value) = 0;

    virtual void moveForward() = 0;
    virtual void moveBackward() = 0;
    virtual void rewind() = 0;

    virtual bool hasNext() const = 0;
    virtual bool hasPrev() const = 0;

    virtual size_t getPosition() const = 0;
    virtual void seek(size_t pos) = 0;

    virtual size_t size() const = 0;
    virtual bool isAtStart() const = 0;
    virtual bool isAtEnd() const = 0;
};