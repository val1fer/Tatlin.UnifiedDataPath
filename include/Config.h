#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#define TEMP_DIR "tmp"

class Duration {
public:
    using Milliseconds = std::chrono::milliseconds;
    
    constexpr Duration() : duration_(0) {}
    constexpr explicit Duration(int32_t ms) : duration_(ms) {}
    
    constexpr int32_t count() const noexcept { return duration_.count(); }
    
private:
    Milliseconds duration_;
};


class Config {
public:
    Config();
    explicit Config(const std::string& filename);
    void loadFromFile(const std::string& filename);
    size_t memoryLimitBytes_;
    Duration readDelay_;
    Duration writeDelay_;
    Duration rewindDelay_;
    Duration moveDelay_;
    
private:
    static constexpr Duration parseDuration(int32_t ms) noexcept;
    static std::string trim(const std::string& str) noexcept;
    
    static constexpr size_t DEFAULT_MEMORY_LIMIT = 64 * 1024 * 1024;
};