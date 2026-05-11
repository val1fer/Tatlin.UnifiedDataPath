#include "ITape.h"
#include "FileTape.h"
#include "Config.h"
#include "TapeSorter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <input_file> <output_file> [config_file]\n"
              << "  input_file  - Binary file containing int32_t values (input tape)\n"
              << "  output_file - Binary file to write sorted int32_t values (output tape)\n"
              << "  config_file - Optional configuration file (default: config.txt)\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];
    std::string configPath = (argc >= 4) ? argv[3] : "config.txt";

    Config config(configPath);

    std::cout << "Tape Sort Configuration:\n"
              << "  Memory limit: " << config.memoryLimitBytes_ << " bytes\n"
              << "  Read delay: " << config.readDelay_.count() << " ms\n"
              << "  Write delay: " << config.writeDelay_.count() << " ms\n"
              << "  Rewind delay: " << config.rewindDelay_.count() << " ms\n"
              << "  Move delay: " << config.moveDelay_.count() << " ms\n\n";

    if (!std::filesystem::exists(inputPath)) {
        std::cerr << "Error: Input file does not exist: " << inputPath << "\n";
        return 1;
    }

    size_t inputSize = std::filesystem::file_size(inputPath);
    size_t numElements = inputSize / sizeof(int32_t);
    if (inputSize % sizeof(int32_t) != 0) {
        std::cerr << "Warning: Input file size is not a multiple of sizeof(int32_t). "
                  << "Last " << (inputSize % sizeof(int32_t)) << " bytes will be ignored.\n";
    }
    std::cout << "Input file: " << inputPath << " (" << numElements << " elements)\n";

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Cannot create output file: " << outputPath << "\n";
        return 1;
    }
    for (size_t i = 0; i < numElements; ++i) {
        int32_t zero = 0;
        outFile.write(reinterpret_cast<const char*>(&zero), sizeof(int32_t));
    }

    try {
        FileTape inputTape(inputPath, config);
        FileTape outputTape(outputPath, config);

        TapeSorter sorter(config);
        sorter.sort(inputTape, outputTape);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}