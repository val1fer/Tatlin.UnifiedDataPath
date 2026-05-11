#include "TapeSorter.h"
#include "FileTape.h"

#include <string>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>


std::string makeTempFilePath(const std::string& name, const std::filesystem::path& tmpDir) {
    return (tmpDir / name).string();
}

void ensureFileSize(const std::string& path, size_t count) {
    size_t requiredSize = count * sizeof(int32_t);
    
    if (std::filesystem::exists(path)) {
        size_t currentSize = std::filesystem::file_size(path);
        if (currentSize < requiredSize) {
            std::filesystem::resize_file(path, requiredSize);
        }
    } else {
        std::ofstream file(path, std::ios::binary);
        file.seekp(requiredSize - 1);
        file.write("", 1);
        file.close();
    }
}


bool writeElement(ITape& outputTape, int32_t value, size_t& written, size_t totalElements) {
    outputTape.write(value);
    ++written;
    if (written < totalElements) {
        outputTape.moveForward();
    }
    return written < totalElements;
}

void drainRemaining(ITape& source, ITape& outputTape, size_t& written, size_t totalElements) {
    while (source.hasNext()) {
        writeElement(outputTape, source.read(), written, totalElements);
        source.moveForward();
    }
    if (!source.isAtEnd()) {
        writeElement(outputTape, source.read(), written, totalElements);
    }
}


TapeSorter::TapeSorter(const Config& config)
    : config_(config)
{
    if (config_.memoryLimitBytes_ < sizeof(int32_t)) {
        throw std::invalid_argument("Memory limit must be at least sizeof(int32_t) = 4 bytes");
    }
    chunkSize_ = config_.memoryLimitBytes_ / sizeof(int32_t);

    std::filesystem::path tmpDir(TEMP_DIR);
    if (!std::filesystem::exists(tmpDir)) {
        std::filesystem::create_directory(tmpDir);
    }
}

TapeSorter::~TapeSorter() {
    for (const auto& path : tempFiles_) {
        try {
            std::filesystem::remove(path);
        } catch (...) {}
    }
}

std::unique_ptr<ITape> TapeSorter::createTempTape(const std::string& name) {
    std::filesystem::path tmpDir(TEMP_DIR);
    std::string fullPath = makeTempFilePath(name, tmpDir);
    tempFiles_.push_back(fullPath);
    return std::make_unique<FileTape>(fullPath, config_, true);
}

void TapeSorter::sort(ITape& inputTape, ITape& outputTape) {
    if (inputTape.size() == 0) {
        return;
    }

    std::vector<std::string> runFiles;
    createInitialRuns(inputTape, runFiles);

    if (runFiles.empty()) {
        return;
    }

    while (runFiles.size() > 1) {
        std::vector<std::string> mergedFiles;
        size_t mergeIndex = 0;
        for (size_t i = 0; i < runFiles.size(); i += 2) {
            if (i + 1 < runFiles.size()) {
                std::string mergedName = "merge_" + std::to_string(mergeIndex++) + ".bin";
                auto mergedPath = (std::filesystem::path(TEMP_DIR) / mergedName).string();

                FileTape tape1(runFiles[i], config_);
                FileTape tape2(runFiles[i + 1], config_);

                size_t totalSize = tape1.size() + tape2.size();

                std::ofstream prealloc(mergedPath, std::ios::binary);
                std::vector<int32_t> zeros(totalSize, 0);
                prealloc.write(reinterpret_cast<const char*>(zeros.data()), totalSize * sizeof(int32_t));
                prealloc.close();

                FileTape mergedTape(mergedPath, config_);
                mergeTwoTapes(tape1, tape2, mergedTape);

                mergedFiles.push_back(mergedPath);
            } else {
                mergedFiles.push_back(runFiles[i]);
            }
        }

        for (const auto& f : runFiles) {
            bool stillInUse = false;
            for (const auto& mf : mergedFiles) {
                if (mf == f) { stillInUse = true; break; }
            }
            if (!stillInUse) {
                try { std::filesystem::remove(f); } catch (...) {}
            }
        }

        runFiles = std::move(mergedFiles);
    }

    FileTape finalTape(runFiles[0], config_);
    finalTape.rewind();
    outputTape.rewind();

    if (!finalTape.isAtEnd()) {
        outputTape.write(finalTape.read());
        while (finalTape.hasNext()) {
            finalTape.moveForward();
            outputTape.moveForward();
            outputTape.write(finalTape.read());
        }
    }

    std::string lastFile = runFiles[0];
    tempFiles_.erase(
        std::remove(tempFiles_.begin(), tempFiles_.end(), lastFile),
        tempFiles_.end()
    );
    try { std::filesystem::remove(lastFile); } catch (...) {}
}

void TapeSorter::createInitialRuns(ITape& inputTape, std::vector<std::string>& tempFilePaths) {
    inputTape.rewind();
    size_t runIndex = 0;

    while (!inputTape.isAtEnd()) {
        std::vector<int32_t> chunk;
        chunk.reserve(chunkSize_);

        while (chunk.size() < chunkSize_) {
            if (inputTape.isAtEnd()) break;
            chunk.push_back(inputTape.read());
            if (!inputTape.hasNext()) {
                break;
            }
            inputTape.moveForward();
        }

        if (chunk.empty()) break;

        std::sort(chunk.begin(), chunk.end());

        std::string runName = "run_" + std::to_string(runIndex++) + ".bin";
        auto runPath = (std::filesystem::path(TEMP_DIR) / runName).string();

        std::ofstream outFile(runPath, std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("Failed to create run file: " + runPath);
        }
        outFile.write(reinterpret_cast<const char*>(chunk.data()), chunk.size() * sizeof(int32_t));
        outFile.close();

        tempFilePaths.push_back(runPath);
        tempFiles_.push_back(runPath);

        if (!inputTape.hasNext()) {
            break;
        }
        inputTape.moveForward();
    }
}

void TapeSorter::mergeTwoTapes(ITape& tape1, ITape& tape2, ITape& outputTape) {
    tape1.rewind();
    tape2.rewind();
    outputTape.rewind();

    size_t totalElements = tape1.size() + tape2.size();
    size_t written = 0;

    bool t1HasVal = !tape1.isAtEnd();
    bool t2HasVal = !tape2.isAtEnd();
    int32_t v1 = 0, v2 = 0;

    if (t1HasVal) v1 = tape1.read();
    if (t2HasVal) v2 = tape2.read();

    while (t1HasVal && t2HasVal) {
        if (v1 <= v2) {
            if (!writeElement(outputTape, v1, written, totalElements)) return;
            if (tape1.hasNext()) {
                tape1.moveForward();
                v1 = tape1.read();
            } else {
                t1HasVal = false;
            }
        } else {
            if (!writeElement(outputTape, v2, written, totalElements)) return;
            if (tape2.hasNext()) {
                tape2.moveForward();
                v2 = tape2.read();
            } else {
                t2HasVal = false;
            }
        }
    }

    if (t1HasVal) {
        if (!writeElement(outputTape, v1, written, totalElements)) return;
        if (tape1.hasNext()) {
            tape1.moveForward();
            drainRemaining(tape1, outputTape, written, totalElements);
        }
    } else if (t2HasVal) {
        if (!writeElement(outputTape, v2, written, totalElements)) return;
        if (tape2.hasNext()) {
            tape2.moveForward();
            drainRemaining(tape2, outputTape, written, totalElements);
        }
    }
}

void TapeSorter::mergeRuns(const std::vector<std::string>& inputPaths, std::vector<std::string>& outputPaths) {
    for (size_t i = 0; i < inputPaths.size(); i += 2) {
        if (i + 1 < inputPaths.size()) {
            auto tape1 = std::make_unique<FileTape>(inputPaths[i], config_);
            auto tape2 = std::make_unique<FileTape>(inputPaths[i + 1], config_);

            std::string mergedName = "merge_" + std::to_string(i / 2) + ".bin";
            auto mergedPath = (std::filesystem::path(TEMP_DIR) / mergedName).string();

            size_t totalSize = tape1->size() + tape2->size();

            std::ofstream prealloc(mergedPath, std::ios::binary);
            std::vector<int32_t> zeros(totalSize, 0);
            prealloc.write(reinterpret_cast<const char*>(zeros.data()), totalSize * sizeof(int32_t));
            prealloc.close();

            FileTape mergedTape(mergedPath, config_);
            mergeTwoTapes(*tape1, *tape2, mergedTape);

            outputPaths.push_back(mergedPath);
        } else {
            outputPaths.push_back(inputPaths[i]);
        }
    }
}