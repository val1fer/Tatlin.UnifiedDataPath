#include "FileTape.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

FileTape::FileTape(const std::filesystem::path& filePath, const Config& cfg, bool createIfNotExists)
    : filePath_(filePath)
    , config_(cfg)
{
    if (createIfNotExists && !std::filesystem::exists(filePath)) {
        std::ofstream createFile(filePath, std::ios::binary);
        if (!createFile) {
            throw std::runtime_error("Failed to create tape file: " + filePath.string());
        }
    }

    stream_.open(filePath_, std::ios::binary | std::ios::in | std::ios::out);
    if (!stream_.is_open()) {
        throw std::runtime_error("Failed to open tape file: " + filePath.string());
    }
}

FileTape::~FileTape() {
    if (stream_.is_open()) {
        stream_.close();
    }
}

void FileTape::openForWrite() {
    if (stream_.is_open()) {
        stream_.close();
    }
    stream_.open(filePath_, std::ios::binary | std::ios::out | std::ios::app);
    openForWriteFlag_ = true;
    stream_.seekp(0, std::ios::end);
    std::streampos pos = stream_.tellp();
    position_ = static_cast<size_t>(pos) / sizeof(int32_t);
}

void FileTape::closeForWrite() {
    openForWriteFlag_ = false;
    if (stream_.is_open()) {
        stream_.close();
    }
    stream_.open(filePath_, std::ios::binary | std::ios::in | std::ios::out);
}

void FileTape::applyDelay(int32_t delayMs) const {
    if (delayMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
}

void FileTape::ensureOpen() const {
    if (!stream_.is_open()) {
        throw std::runtime_error("Tape file is not open: " + filePath_.string());
    }
}

size_t FileTape::fileSizeInElements() const {
    ensureOpen();
    stream_.clear();
    std::streampos cur = stream_.tellg();
    stream_.seekg(0, std::ios::end);
    std::streampos end = stream_.tellg();
    stream_.seekg(cur);
    return static_cast<size_t>(end) / sizeof(int32_t);
}

int32_t FileTape::read() const {
    applyDelay(config_.readDelay_.count());
    ensureOpen();
    stream_.clear();

    int32_t value = 0;
    std::streampos pos = static_cast<std::streampos>(position_) * static_cast<std::streampos>(sizeof(int32_t));
    stream_.seekg(pos);
    stream_.read(reinterpret_cast<char*>(&value), sizeof(int32_t));
    if (!stream_.good()) {
        throw std::runtime_error("Failed to read from tape at position " + std::to_string(position_));
    }
    return value;
}

void FileTape::write(int32_t value) {
    applyDelay(config_.writeDelay_.count());
    ensureOpen();

    std::streampos pos = static_cast<std::streampos>(position_) * static_cast<std::streampos>(sizeof(int32_t));
    stream_.seekp(pos);
    stream_.write(reinterpret_cast<const char*>(&value), sizeof(int32_t));
    if (!stream_.good()) {
        throw std::runtime_error("Failed to write to tape at position " + std::to_string(position_));
    }
    stream_.flush();
}

void FileTape::moveForward() {
    applyDelay(config_.moveDelay_.count());
    size_t sz = fileSizeInElements();
    if (position_ >= sz) {
        throw std::out_of_range("Cannot move forward: already at end of tape (pos=" + std::to_string(position_) + ", size=" + std::to_string(sz) + ")");
    }
    ++position_;
}

void FileTape::moveBackward() {
    applyDelay(config_.moveDelay_.count());
    if (position_ == 0) {
        throw std::out_of_range("Cannot move backward: already at beginning of tape");
    }
    --position_;
}

void FileTape::rewind() {
    applyDelay(config_.rewindDelay_.count());
    position_ = 0;
}

bool FileTape::hasNext() const {
    if (!stream_.is_open()) return false;
    return position_ + 1 < fileSizeInElements();
}

bool FileTape::hasPrev() const {
    return position_ > 0;
}

size_t FileTape::getPosition() const {
    return position_;
}

void FileTape::seek(size_t pos) {
    size_t sz = fileSizeInElements();
    if (pos >= sz) {
        throw std::out_of_range("Seek position " + std::to_string(pos) + " is beyond tape size " + std::to_string(sz));
    }
    position_ = pos;
}

size_t FileTape::size() const {
    return fileSizeInElements();
}

bool FileTape::isAtStart() const {
    return position_ == 0;
}

bool FileTape::isAtEnd() const {
    if (!stream_.is_open()) return true;
    return position_ >= fileSizeInElements();
}