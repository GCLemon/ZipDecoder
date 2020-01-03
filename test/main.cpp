#include "../src/ZipDecoder.h"

int main()
{
    // ファイルのインスタンスを作成
    ZipDecoder::ZipFile zip_file;

    // Zip ファイルをロード
    auto status = zip_file.LoadFile("leibnitz.zip");
    auto buf = zip_file.GetStream("leibnitz.py");
}