#pragma once

#include "ITape.h"
#include "Config.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

class FileTape : public ITape {
public:
    FileTape(const std::filesystem::path& filePath, const Config& cfg, bool createIfNotExists = true);
    ~FileTape() override;

    int32_t read() const override;
    void write(int32_t value) override;
    void moveForward() override;
    void moveBackward() override;
    void rewind() override;
    bool hasNext() const override;
    bool hasPrev() const override;
    size_t getPosition() const override;
    void seek(size_t pos) override;
    size_t size() const override;
    bool isAtStart() const override;
    bool isAtEnd() const override;

    void openForWrite();
    void closeForWrite();

private:
    void applyDelay(int delayMs) const;
    void ensureOpen() const;
    size_t fileSizeInElements() const;

    std::filesystem::path filePath_;
    Config config_;
    mutable std::fstream stream_;
    size_t position_ = 0;
    bool openForWriteFlag_ = false;
};