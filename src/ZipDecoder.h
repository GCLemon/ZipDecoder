#ifndef ZIP_DECODER_HPP
#define ZIP_DECODER_HPP

#include <zlib.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

namespace ZipDecoder
{

enum class FileLoadStatus
{
    SUCCESS = 0,
    MEMORY_ALREADY_ALLOCATED = 1,
    FILE_OPEN_FAILURE = 2,
    MEMORY_ALLOCATION_FAILURE = 3,
    INVALID_FILE_FORMAT = 4
};

class ZipFile
{
private:
    unique_ptr<uint8_t[]> fileData;
    size_t fileSize;
    size_t endRecordPosision;

public:
    ZipFile();
    ~ZipFile();

    FileLoadStatus LoadFile(const char* file_path);

    vector<uint8_t> GetStream(const char* file_path);
};

} // namespace ZipDecoder

#endif