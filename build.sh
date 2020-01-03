g++ -std=c++17 -O -o ./test/build/ZipDecoderTest `find . -name "*.cpp"` -lz
cd test/build
./ZipDecoderTest
cd ../..