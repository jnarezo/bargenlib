#include "bargenlib.h"

#include <fstream>
#include <string>
#include <vector>

#ifdef DEBUG
#include <iostream>
#endif

namespace bargenlib
{
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t fileType = 0x4D42;
        uint32_t fileSize;
        uint16_t reserveA = 0;
        uint16_t reserveB = 0;
        uint32_t headerOffset;
        BMPFileHeader(int dataSize, bool hasAlpha = false):
            fileSize((uint32_t) ((14+40+hasAlpha*84) + dataSize)),
            headerOffset((uint32_t) (14+40+hasAlpha*84-1)){}
    };

    struct BMPInfoHeader {
        uint32_t headerSize = 40;
        uint32_t width;
        uint32_t height;
        uint16_t noPlanes = 1;
        uint16_t bitDepth = 24;
        uint32_t compression = 0;
        uint32_t imageSize = 0;
        uint32_t xPPM = 0;
        uint32_t yPPM = 0;
        uint32_t noColorIndices = 0;
        uint32_t colorPriority = 0;
        BMPInfoHeader(int width, int height, bool hasAlpha = false):
            width(width),
            height(height),
            bitDepth(24 + hasAlpha*8),
            compression(0 + hasAlpha*3){}
    };

    struct BMPColorHeader {
        uint32_t rBitmask = 0x0000ff00;
        uint32_t gBitmask = 0x00ff0000;
        uint32_t bBitmask = 0xff000000;
        uint32_t aBitmask = 0x000000ff;
        uint32_t sRGB = 0x42475273;
        uint32_t reserved[16]{0};
    };
    #pragma pack(pop)

    void upc_a(std::vector<int> code, bool hasAlpha, std::string path) {
        if (code.size() != 11) throw std::runtime_error("A UPC-A valid number must be 11 or 12 digits.");
        for (int n : code) {
            if (n < 0 || n > 9) throw std::runtime_error("A single UPC-A digit must be 0-9.");
        }

        unsigned short width = 115;
        unsigned short height = 78;
        unsigned short channels = 3 + static_cast<int>(hasAlpha);
        std::vector<unsigned char> data((width * height) * channels, 255);
        if (hasAlpha) {
            // Initialize a transparent canvas
            for (int i = 0; i < width*height; i++) {
                data[i * channels + 3] = 0;  // a
            }
        }

        std::vector<std::vector<bool>> UpcAEncodeTable = {
                            {0, 0, 0, 1, 1, 0, 1},      // 0
                            {0, 0, 1, 1, 0, 0, 1},      // 1
                            {0, 0, 1, 0, 0, 1, 1},      // 2
                            {0, 1, 1, 1, 1, 0, 1},      // 3
                            {0, 1, 0, 0, 0, 1, 1},      // 4
                            {0, 1, 1, 0, 0, 0, 1},      // 5
                            {0, 1, 0, 1, 1, 1, 1},      // 6
                            {0, 1, 1, 1, 0, 1, 1},      // 7
                            {0, 1, 1, 0, 1, 1, 1},      // 8
                            {0, 0, 0, 1, 0, 1, 1}};     // 9

        std::ofstream of(path, std::ios_base::binary);
        if (of.is_open()) {
            // Add BMP header information
            bargenlib::writeHeader(data.size(), width, -height, of, hasAlpha);

            int linePos = 9; // Padding for readability

            // Add start guard pattern
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;

            // Add left-side digits
            for (int i = 0; i < 6; i++) {
                for (int x = 0; x < 7; x++) {
                    if (UpcAEncodeTable[code[i]][x]) {
                        if (i == 0) bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
                        else bargenlib::writeLine(data, &linePos, width, height-5, hasAlpha);
                    } else {
                        linePos += 1;
                    }
                }
            }

            // Add middle guard pattern
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;


            // Add right-side digits and check digit, if given.
            for (int i = 6; i < code.size(); i++) {
                for (int x = 0; x < 7; x++) {
                    if (!UpcAEncodeTable[code[i]][x]) bargenlib::writeLine(data, &linePos, width, height-5, hasAlpha);
                    else linePos += 1;
                }
            }
            
            // Calculate and add check digit, if not given.
            if (code.size() == 11) {
                int remainder = ((3 * (code[0] + code[2] + code[4] + code[6] + code[8] + code[10])
                                        + (code[1] + code[3] + code[5] + code[7] + code[9])) % 10);
                int checkDigit = (10 - remainder) % 10;

                for (int x = 0; x < 7; x++) {
                    if (!UpcAEncodeTable[checkDigit][x]) bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
                    else linePos += 1;
                }
            }
            
            // Add end guard pattern
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);

            // Write data to file
            of.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
        else {
            throw std::runtime_error("Unable to open output.");
        }
    }

    void ean_8(std::vector<int> code, bool hasAlpha, std::string path) {
        if (code.size() != 7) throw std::runtime_error("A EAN-8 valid number must be 7 or 8 digits.");
        for (int n : code) {
            if (n < 0 || n > 9) throw std::runtime_error("A single EAN-8 digit must be 0-9 (inclusive).");
        }

        unsigned short width = 87;
        unsigned short height = 78;
        unsigned short channels = 3 + hasAlpha;
        std::vector<unsigned char> data((width * height) * channels, 255);
        if (hasAlpha) {
            // Initialize a transparent canvas
            for (int i = 0; i < width*height; i++) {
                data[i * channels + 3] = 0;  // a
            }
        }

        std::vector<std::vector<bool>> UpcAEncodeTable = {
                            {0, 0, 0, 1, 1, 0, 1},      // 0
                            {0, 0, 1, 1, 0, 0, 1},      // 1
                            {0, 0, 1, 0, 0, 1, 1},      // 2
                            {0, 1, 1, 1, 1, 0, 1},      // 3
                            {0, 1, 0, 0, 0, 1, 1},      // 4
                            {0, 1, 1, 0, 0, 0, 1},      // 5
                            {0, 1, 0, 1, 1, 1, 1},      // 6
                            {0, 1, 1, 1, 0, 1, 1},      // 7
                            {0, 1, 1, 0, 1, 1, 1},      // 8
                            {0, 0, 0, 1, 0, 1, 1}};     // 9

        std::ofstream of(path, std::ios_base::binary);
        if (of.is_open()) {
            bargenlib::writeHeader(data.size(), width, -height, of, hasAlpha);

            int linePos = 9; // Padding for readability

            // Add start guard pattern (4 cols)
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;

            // Add left-side digits (30 cols - 7 cols per digit)
            for (int i = 0; i < 4; i++) {
                for (int x = 0; x < 7; x++) {
                    if (UpcAEncodeTable[code[i]][x]) bargenlib::writeLine(data, &linePos, width, height-5, hasAlpha);
                    else linePos += 1;
                }
            }

            // Add middle guard pattern (5 cols)
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;


            // Add right-side digits (7 cols per digit)
            for (int i = 4; i < code.size(); i++) {
                for (int x = 0; x < 7; x++) {
                    if (!UpcAEncodeTable[code[i]][x]) bargenlib::writeLine(data, &linePos, width, height-5, hasAlpha);
                    else linePos += 1;
                }
            }
            
            // Calculate and add check digit (7 cols)
            if (code.size() == 7) {
                int remainder = ((3 * (code[0] + code[2] + code[4] + code[6])
                                        + (code[1] + code[3] + code[5])) % 10);
                int checkDigit = (10 - remainder) % 10;

                for (int x = 0; x < 7; x++) {
                    if (!UpcAEncodeTable[checkDigit][x]) bargenlib::writeLine(data, &linePos, width, height-5, hasAlpha);
                    else linePos += 1;
                }
            }
            
            // Add end guard pattern (6 cols)
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
            linePos += 1;
            bargenlib::writeLine(data, &linePos, width, height, hasAlpha);

            // Write data to file
            of.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
        else {
            throw std::runtime_error("Unable to open output.");
        }
    }

    #ifdef DEBUG
    void test(bool hasAlpha, std::string path) {
        int linePos = 0;
        int width = 400;
        int height = 400;
        std::vector<unsigned char> data(width * height * (3 + hasAlpha), (char) 255);
        std::ofstream of(path, std::ios_base::binary);

        if (of.is_open()) {
            bargenlib::writeHeader(data.size(), width, -height, of, hasAlpha);
            for (int i = 0; i < width; i++) {
                std::cout << "Writing line. ";
                bargenlib::writeLine(data, &linePos, width, height, hasAlpha);
                std::cout << "Wrote line.\n";
            }
            of.write(reinterpret_cast<const char*>(data.data()), data.size());
            of.close();
            std::cout << "Finished writing.\n";
        } else {
            throw std::runtime_error("Unable to open output.");
        }
        std::cout << "Test finished.\n";
    }
    #endif

    void writeHeader(int dataSizeBytes, int width, int height, std::ofstream &of, bool hasAlpha) {
        bargenlib::BMPFileHeader fileHeader(dataSizeBytes, hasAlpha);
        bargenlib::BMPInfoHeader infoHeader(width, height, hasAlpha);
        bargenlib::BMPColorHeader colorHeader;

        #ifdef DEBUG
        std::cout << "Writing BMP header. ";
        #endif

        of.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        of.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
        if (hasAlpha) of.write(reinterpret_cast<const char*>(&colorHeader), sizeof(colorHeader));

        #ifdef DEBUG
        std::cout << "Done.\n";
        #endif
    }

    void writeLine(std::vector<unsigned char> &data, int *xPos, int width, int height, bool hasAlpha) {
        int channels = (3 + static_cast<int>(hasAlpha));
        // Write a black line in RGB to a BMP data vector
        for (int y = 0; y < height; y++) {
            data[(*xPos + y * width) * channels + 0] = 0; // r
            data[(*xPos + y * width) * channels + 1] = 0; // g
            data[(*xPos + y * width) * channels + 2] = 0; // b
            if (hasAlpha) data[(*xPos + y * width) * channels + 3] = 255;  // a
        }
        *xPos += 1;
    }
}