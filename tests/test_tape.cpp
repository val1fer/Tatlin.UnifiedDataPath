#include "ITape.h"
#include "FileTape.h"
#include "Config.h"
#include "TapeSorter.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <vector>

std::string testInputFile = "test_input.bin";
std::string testOutputFile = "test_output.bin";

void writeBinaryFile(const std::string& path, const std::vector<int32_t>& data) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(int32_t));
}

std::vector<int32_t> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    size_t size = static_cast<size_t>(file.tellg()) / sizeof(int32_t);
    file.seekg(0);
    std::vector<int32_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size * sizeof(int32_t));
    return data;
}

void cleanupFiles(const std::vector<std::string>& files) {
    for (const auto& f : files) {
        try { std::filesystem::remove(f); } catch (...) {}
    }
    try { std::filesystem::remove(TEMP_DIR); } catch (...) {}
}

class TapeSortTest : public ::testing::Test {
protected:
    void TearDown() override {
        cleanupFiles({testInputFile, testOutputFile});
        if (std::filesystem::exists(TEMP_DIR)) {
            for (const auto& entry : std::filesystem::directory_iterator(TEMP_DIR)) {
                std::filesystem::remove(entry.path());
            }
        }
    }
};


TEST_F(TapeSortTest, EmptyFile) {
    writeBinaryFile(testInputFile, {});

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    EXPECT_TRUE(result.empty());
}

TEST_F(TapeSortTest, SingleElement) {
    writeBinaryFile(testInputFile, {42});

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], 42);
}

TEST_F(TapeSortTest, AlreadySorted) {
    std::vector<int32_t> data = {1, 2, 3, 4, 5};
    writeBinaryFile(testInputFile, data);

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    EXPECT_EQ(result, (std::vector<int32_t>{1, 2, 3, 4, 5}));
}

TEST_F(TapeSortTest, ReverseSorted) {
    std::vector<int32_t> data = {5, 4, 3, 2, 1};
    writeBinaryFile(testInputFile, data);

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    EXPECT_EQ(result, (std::vector<int32_t>{1, 2, 3, 4, 5}));
}

TEST_F(TapeSortTest, RandomSmall) {
    std::vector<int32_t> data = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    writeBinaryFile(testInputFile, data);

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    std::vector<int32_t> expected = data;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(result, expected);
}

TEST_F(TapeSortTest, Duplicates) {
    std::vector<int32_t> data = {5, 3, 5, 3, 5, 3, 1, 1, 1};
    writeBinaryFile(testInputFile, data);

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    std::vector<int32_t> expected = {1, 1, 1, 3, 3, 3, 5, 5, 5};
    EXPECT_EQ(result, expected);
}

TEST_F(TapeSortTest, NegativeNumbers) {
    std::vector<int32_t> data = {-5, 3, -1, 0, -10, 7, 2};
    writeBinaryFile(testInputFile, data);

    Config cfg;
    FileTape inputTape(testInputFile, cfg);
    FileTape outputTape(testOutputFile, cfg);

    TapeSorter sorter(cfg);
    sorter.sort(inputTape, outputTape);

    auto result = readBinaryFile(testOutputFile);
    std::vector<int32_t> expected = {-10, -5, -1, 0, 2, 3, 7};
    EXPECT_EQ(result, expected);
}

TEST_F(TapeSortTest, FileTapeBasics) {
    std::string testFile = "ft_test.bin";
    {
        Config cfg;
        FileTape tape(testFile, cfg, true);

        tape.write(10);
        tape.moveForward();
        tape.write(20);
        tape.moveForward();
        tape.write(30);

        EXPECT_EQ(tape.size(), 3u);

        tape.rewind();
        EXPECT_EQ(tape.getPosition(), 0u);
        EXPECT_EQ(tape.read(), 10);
        EXPECT_TRUE(tape.hasNext());
        EXPECT_FALSE(tape.hasPrev());

        tape.moveForward();
        EXPECT_EQ(tape.getPosition(), 1u);
        EXPECT_EQ(tape.read(), 20);
        EXPECT_TRUE(tape.hasNext());
        EXPECT_TRUE(tape.hasPrev());

        tape.moveForward();
        EXPECT_EQ(tape.getPosition(), 2u);
        EXPECT_EQ(tape.read(), 30);
        EXPECT_FALSE(tape.hasNext());
        EXPECT_TRUE(tape.hasPrev());

        tape.rewind();
        EXPECT_TRUE(tape.isAtStart());
        EXPECT_FALSE(tape.isAtEnd());
    }
    cleanupFiles({testFile});
}

TEST_F(TapeSortTest, FileTapeSeek) {
    std::string testFile = "ft_seek_test.bin";
    {
        Config cfg;
        FileTape tape(testFile, cfg, true);
        tape.write(100);
        tape.moveForward();
        tape.write(200);
        tape.moveForward();
        tape.write(300);

        tape.seek(1);
        EXPECT_EQ(tape.getPosition(), 1u);
        EXPECT_EQ(tape.read(), 200);

        tape.seek(0);
        EXPECT_EQ(tape.read(), 100);

        tape.seek(2);
        EXPECT_EQ(tape.read(), 300);
    }
    cleanupFiles({testFile});
}