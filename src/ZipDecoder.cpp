#include "ZipDecoder.h"

#pragma pack(2)

struct CentralDirectoryEndRecord
{
    uint32_t Signature;
    uint16_t DiskCount;
    uint16_t DiskNumber;
    uint16_t CentralRecordsOnDisk;
    uint16_t CentralRecordsTotal;
    uint32_t CentralDirectorySize;
    uint32_t CentralDirectoryOfset;
    uint16_t ExtentionalFieldLength;
};

struct CentralDirectoryFileHeader
{
    uint32_t Signature;
    uint16_t VersionCreated;
    uint16_t VersionForInflation;
    uint16_t BitFlag;
    uint16_t CompressionMethod;
    uint16_t FileModificationTime;
    uint16_t FileModificationDate;
    uint32_t CRC32;
    uint32_t CompressedSize;
    uint32_t UncompressedSize;
    uint16_t FileNameLength;
    uint16_t ExtraFieldLength;
    uint16_t FileCommentLength;
    uint16_t FileStartingDiskNumber;
    uint16_t InnerFileAttribute;
    uint32_t OuterFileAttribute;
    uint32_t LocalFileHeaderOfset;
};

struct LocalFileHeader
{
    uint32_t Signature;
    uint16_t VersionForInflation;
    uint16_t BitFlag;
    uint16_t CompressionMethod;
    uint16_t FileModificationTime;
    uint16_t FileModificationDate;
    uint32_t CRC32;
    uint32_t CompressedSize;
    uint32_t UncompressedSize;
    uint16_t FileNameLength;
    uint16_t ExtraFieldLength;
};

ZipDecoder::ZipFile::ZipFile() : fileData(nullptr), fileSize(0), endRecordPosision(0) { }
ZipDecoder::ZipFile::~ZipFile() { }

ZipDecoder::FileLoadStatus ZipDecoder::ZipFile::LoadFile(const char* file_path)
{
    // ファイル内容が展開済みならば処理を行わない
    if(fileData != nullptr)
        return ZipDecoder::FileLoadStatus::MEMORY_ALREADY_ALLOCATED;

    // ファイルを開く
    FILE* zip_stream = fopen(file_path, "rb");
    if(zip_stream == nullptr)
        return ZipDecoder::FileLoadStatus::FILE_OPEN_FAILURE;

    // ファイルサイズを取得
    fseek(zip_stream, 0, SEEK_END);
    fileSize = ftell(zip_stream);
    fseek(zip_stream, 0, SEEK_SET);

    // 領域を確保
    fileData = make_unique<uint8_t[]>(fileSize);
    if(fileData == nullptr)
        return ZipDecoder::FileLoadStatus::MEMORY_ALLOCATION_FAILURE;

    // ファイル読み込み
    fread(fileData.get(), 1, fileSize, zip_stream);

    // 探索するシグニチャ
    const uint8_t signature[4] = { 0x50, 0x4B, 0x05, 0x06 };

    // 探索している場所
    endRecordPosision = fileSize - 4;

    // シグニチャの探索
    while(true)
    {
        uint8_t read_data[4];
        memcpy(read_data, fileData.get() + endRecordPosision, 4);
        if(!memcmp(read_data, signature, 4)) return ZipDecoder::FileLoadStatus::SUCCESS;
        if(endRecordPosision == 0) return ZipDecoder::FileLoadStatus::INVALID_FILE_FORMAT;
        --endRecordPosision;
    }
}

vector<uint8_t> ZipDecoder::ZipFile::GetStream(const char* file_path)
{
    // 読み込みを開始する位置
    size_t filePos = endRecordPosision;

    // セントラルディレクトリの終端レコードの読み込み
    CentralDirectoryEndRecord endRecord;
    memcpy(&endRecord, fileData.get() + filePos, sizeof(endRecord));

    // セントラルディレクトリに移動
    filePos = endRecord.CentralDirectoryOfset;
    
    // セントラルディレクトリのファイルヘッダを読み込み
    CentralDirectoryFileHeader fileHeader;
    while(true)
    {
        // サイズが決まっているデータの読み込み
        memcpy(&fileHeader, fileData.get() + filePos, sizeof(fileHeader));

        // ヘッダのシグニチャが終端レコードに達した場合は長さ0のベクタを返す
        if(fileHeader.Signature != 0x02014B50) return vector<uint8_t>(0);

        // ファイル位置を進める
        filePos += sizeof(fileHeader);

        // ファイル名の読み込み
        char* fileName = new char[fileHeader.FileNameLength + 1] { 0 };
        memcpy(fileName, fileData.get() + filePos, fileHeader.FileNameLength);

        // ファイル名を比較して等しかった場合はループを抜ける
        if(!strcmp(fileName, file_path)) break;

        // 一応確保したメモリの開放
        delete[] fileName;
        fileName = nullptr;

        // ファイル位置を進める
        filePos += fileHeader.FileNameLength + fileHeader.ExtraFieldLength + fileHeader.FileCommentLength;
    }

    // ローカルファイルのファイルヘッダにファイル位置を合わせる
    filePos = fileHeader.LocalFileHeaderOfset;

    // ローカルファイルのヘッダの読み込み
    LocalFileHeader localFileHeader;
    memcpy(&localFileHeader, fileData.get() + filePos, sizeof(localFileHeader));

    // ファイル位置を進める
    filePos += sizeof(localFileHeader);

    // ファイル名を読み込んで確認
    char* fileName = new char[localFileHeader.FileNameLength + 1] { 0 };
    memcpy(fileName, fileData.get() + filePos, localFileHeader.FileNameLength);
    assert(!strcmp(fileName, file_path));
    delete[] fileName;
    fileName = nullptr;

    // ファイル位置を進める
    filePos += localFileHeader.FileNameLength + localFileHeader.ExtraFieldLength;

    // 解凍前のデータを用意する
    uint8_t* compressed = new uint8_t[localFileHeader.CompressedSize];
    memcpy(compressed, fileData.get() + filePos, localFileHeader.CompressedSize);

    // 解凍済みデータを格納する領域を確保する
    vector<uint8_t> decompressed(localFileHeader.UncompressedSize);

    // z_stream 構造体の作成
    z_stream zst;
    zst.opaque = Z_NULL;
    zst.zalloc = Z_NULL;
    zst.zfree = Z_NULL;
    zst.next_in = compressed;
    zst.avail_in = localFileHeader.CompressedSize;
    zst.next_out = decompressed.data();
    zst.avail_out = decompressed.size();

    // 解凍を実行
    inflateInit2(&zst, -8);
    inflate(&zst, Z_FINISH);
    inflateEnd(&zst);

    // 解凍したデータを返す
    return decompressed;
}