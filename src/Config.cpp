#include "Config.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

Config::Config()
    : memoryLimitBytes_(DEFAULT_MEMORY_LIMIT)
    , readDelay_(Duration(0))
    , writeDelay_(Duration(0))
    , rewindDelay_(Duration(0))
    , moveDelay_(Duration(0))
{}

Config::Config(const std::string& filename) {
    loadFromFile(filename);
}

Duration Config::parseDuration(int32_t ms) {
    return Duration(ms);
}

std::string Config::trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return str.substr(start, end - start);
}

void Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if (key == "memory_limit_bytes") {
            memoryLimitBytes_ = std::stoul(value);
        } else if (key == "read_delay_ms") {
            readDelay_ = parseDuration(std::stoi(value));
        } else if (key == "write_delay_ms") {
            writeDelay_ = parseDuration(std::stoi(value));
        } else if (key == "rewind_delay_ms") {
            rewindDelay_ = parseDuration(std::stoi(value));
        } else if (key == "move_delay_ms") {
            moveDelay_ = parseDuration(std::stoi(value));
        }
    }
}
