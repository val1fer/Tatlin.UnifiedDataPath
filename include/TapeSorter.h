#pragma once

#include "ITape.h"
#include "Config.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class TapeSorter {
public:
    explicit TapeSorter(const Config& config);
    ~TapeSorter();

    void sort(ITape& inputTape, ITape& outputTape);

private:
    void createInitialRuns(ITape& inputTape, std::vector<std::string>& tempFilePaths);
    void mergeRuns(const std::vector<std::string>& inputPaths, std::vector<std::string>& outputPaths);
    void mergeTwoTapes(ITape& tape1, ITape& tape2, ITape& outputTape);

    std::unique_ptr<ITape> createTempTape(const std::string& name);

    Config config_;
    size_t chunkSize_ = 0;
    std::vector<std::string> tempFiles_;
};