#pragma once

#include <fstream>
#include <vector>
#include <string>

namespace bargenlib
{
    struct BMPFileHeader;
    struct BMPInfoHeader;
    struct BMPColorHeader;

    // Generates a UPC-A barcode bitmap image at path.
    void upc_a(std::vector<int> code, bool hasAlpha, std::string path);
    // Generates a EAN-8 barcode bitmap image at path.
    void ean_8(std::vector<int> code, bool hasAlpha, std::string path);
    // Write bitmap image headers to ofstream.
    static void writeHeader(int dataSizeBytes, int width, int height, std::ofstream &of, bool hasAlpha = false);
    // Write a black line at (x) spanning entire height given in a bitmap data array.
    static void writeLine(std::vector<unsigned char> &data, int *xPos, int width, int height, bool hasAlpha = false);
}
