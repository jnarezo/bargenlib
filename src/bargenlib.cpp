#include "bargenlib/bargenlib.h"

#include <fstream>
#include <string>
#include <array>
#include <vector>
#include <stdexcept>

// #include <iostream>

namespace bargenlib
{
namespace
{
    // Bitmap Image Header Format
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t fileType = 0x4D42;
        uint32_t fileSize;
        uint16_t reserveA = 0;
        uint16_t reserveB = 0;
        uint32_t headerOffset;
        BMPFileHeader(int dataSize, bool hasAlpha = false):
            fileSize((14+40+hasAlpha*84) + dataSize),
            headerOffset(14+40+hasAlpha*84-1){}
    };

    struct BMPInfoHeader {
        uint32_t headerSize = 40;
        uint32_t width;
        uint32_t height;
        uint16_t noPlanes = 1;
        uint16_t bitDepth = 24;  // bpp
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
        uint32_t rBitmask = 0x0000FF00;
        uint32_t gBitmask = 0x00FF0000;
        uint32_t bBitmask = 0xFF000000;
        uint32_t aBitmask = 0x000000FF;
        uint32_t sRGB = 0x42475273;
        uint32_t reserved[16]{0};
    };
    #pragma pack(pop)

    std::array<uint8_t, 10> UpcEncodeTable{{
        0b00001101,  // 0
        0b00011001,  // 1
        0b00010011,  // 2
        0b00111101,  // 3
        0b00100011,  // 4
        0b00110001,  // 5
        0b00101111,  // 6
        0b00111011,  // 7
        0b00110111,  // 8
        0b00001011,  // 9
    }};

    std::array<uint8_t, 10> EanLeftPattern{{
        0b00000000,  // 0
        0b00001011,  // 1
        0b00001101,  // 2
        0b00001110,  // 3
        0b00010011,  // 4
        0b00011001,  // 5
        0b00011100,  // 6
        0b00010101,  // 7
        0b00010110,  // 8
        0b00011010,  // 9
    }};

    enum BarRegion {
        S = 0,  // Start
        L = 1,  // Left Digit
        G = 2,  // G Digit
        M = 3,  // Middle
        R = 4,  // Right Digit
        E = 5,  // End
    };

    void writeBMPHead(int dataSizeBytes, int width, int height, std::ofstream& of, bool hasAlpha) {
        BMPFileHeader fileHeader(dataSizeBytes, hasAlpha);
        BMPInfoHeader infoHeader(width, -height, hasAlpha);
        BMPColorHeader colorHeader;

        of.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
        of.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
        if (hasAlpha) {
            of.write(reinterpret_cast<const char*>(&colorHeader), sizeof(colorHeader));
        }
    }

    void writeBar(std::vector<uint8_t>& data, int imgWidth, int& xPos, int barHeight, bool hasAlpha) {
        int channels = (3 + static_cast<int>(hasAlpha));

        // Write a black line in RGB to the bitmap's data
        for (int y = 0; y < barHeight; y++) {
            data[(xPos + y * imgWidth) * channels + 0] = 0; // r
            data[(xPos + y * imgWidth) * channels + 1] = 0; // g
            data[(xPos + y * imgWidth) * channels + 2] = 0; // b
            if (hasAlpha) data[(xPos + y * imgWidth) * channels + 3] = 255;  // a
        }
        xPos += 1;
    }

    void writeGuardUPC(std::vector<uint8_t>& data, int imgWidth,
            int& xPos, int barHeight, BarRegion pos, bool hasAlpha = false) {
        if (pos != BarRegion::S) xPos += 1;
        bargenlib::writeBar(data, imgWidth, xPos, barHeight, hasAlpha);
        xPos += 1;
        bargenlib::writeBar(data, imgWidth, xPos, barHeight, hasAlpha);
        if (pos != BarRegion::E) xPos += 1;
    }

    void writeNumUPC(std::vector<uint8_t>& data, int imgWidth,
            int& xPos, int barHeight, int num, BarRegion pos, bool hasAlpha = false) {
        
        // Add the given UPC number correctly in the given region (L/G/R)
        if (pos == BarRegion::G) {
            for (int i = 6; i >= 0; i--) {
                if ((((~UpcEncodeTable[num]) << i) & 0b01000000)) {
                    bargenlib::writeBar(data, imgWidth, xPos, barHeight, hasAlpha);
                }
                else {
                    xPos += 1;
                }
            }
        }
        else {
            for (int i = 0; i < 7; i++) {
                uint8_t encoding = UpcEncodeTable[num];
                if (pos == BarRegion::R) encoding = ~encoding;
                if (((encoding << i) & 0b01000000)) {
                    bargenlib::writeBar(data, imgWidth, xPos, barHeight, hasAlpha);
                }
                else {
                    xPos += 1;
                }
            }
        }
    }
}

void upc_a(const std::vector<int>& code, const std::string& path, bool hasAlpha) {
    if (code.size() != 11 && code.size() != 12) {
        throw std::invalid_argument("A valid UPC-A code must be 11 or 12 digits.");
    }
    for (int n : code) {
        if (n < 0 || n > 9) throw std::invalid_argument("A UPC-A digit must be 0-9.");
    }

    // UPC-A is equivalent to EAN-13 with international code 0.
    std::vector<int> eanCode(code);
    std::vector<int>::iterator it;
    it = eanCode.begin();
    eanCode.insert(it, 0);
    ean_13(eanCode, path, hasAlpha);
}

void ean_8(const std::vector<int>& code, const std::string& path, bool hasAlpha) {
    if (code.size() != 7 && code.size() != 8) {
        throw std::runtime_error("A valid EAN-8 code must be 7 or 8 digits.");
    }
    for (int n : code) {
        if (n < 0 || n > 9) throw std::runtime_error("An EAN-8 digit must be 0-9.");
    }

    // Proceed to use EAN-8 encoding.
    // Establish barcode bitmap data.
    short width = 87;
    short height = 78;
    short channels = 3 + hasAlpha;
    std::vector<unsigned char> data((width * height) * channels, 255);

    // if (hasAlpha) {
    //     // Set every pixel's alpha channel to 0 (transparent)
    //     for (int i = 0; i < (width * height); i++) {
    //         data[i * channels + 3] = 0;
    //     }
    // }

    // Write the barcode's bitmap image to the path.
    std::ofstream of(path, std::ios_base::binary);
    if (of.is_open()) {
        writeBMPHead(data.size(), width, height, of, hasAlpha);
        int linePos = 9; // Space padding

        // Add guard patterns and numbers.
        writeGuardUPC(data, width, linePos, height, S, hasAlpha);
        for (int n = 0; n < 4; n++) {
            writeNumUPC(data, width, linePos, height, code[n], L, hasAlpha);
        }
        writeGuardUPC(data, width, linePos, height, M, hasAlpha);
        for (int n = 4; n < code.size(); n++) {
            writeNumUPC(data, width, linePos, height, code[n], R, hasAlpha);
        }
        
        // Add check digit, if neccessary
        if (code.size() == 7) {
            int remainder = ((3 * (code[0] + code[2] + code[4] + code[6])
                                    + (code[1] + code[3] + code[5])) % 10);
            int checkDigit = (10 - remainder) % 10;

            // Write check digit
            writeNumUPC(data, width, linePos, height, checkDigit, R, hasAlpha);
        }
        
        // Add end guard pattern
        writeGuardUPC(data, width, linePos, height, E, hasAlpha);

        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    of.close();
}

void ean_13(const std::vector<int>& code, const std::string& path, bool hasAlpha) {
    if (code.size() != 12 && code.size() != 13) {
        throw std::runtime_error("A valid EAN-13 code must be 12-13 digits.");
    }
    for (int n : code) {
        if (n < 0 || n > 9) throw std::runtime_error("An EAN-13 digit must be 0-9.");
    }

    // Proceed to use EAN-13 encoding.
    // Establish barcode bitmap data.
    short width = 115;
    short height = 78;
    short channels = 3 + hasAlpha;
    std::vector<unsigned char> data((width * height) * channels, 255);

    // if (hasAlpha) {
    //     // Set every pixel's alpha channel to 0 (transparent)
    //     for (int i = 0; i < (width * height); i++) {
    //         data[i * channels + 3] = 0;
    //     }
    // }

    // Write the barcode's bitmap image to the path.
    std::ofstream of(path, std::ios_base::binary);
    if (of.is_open()) {
        writeBMPHead(data.size(), width, height, of, hasAlpha);

        int linePos = 9; // Space padding

        // Add guard patterns and numbers.
        writeGuardUPC(data, width, linePos, height, S, hasAlpha);
        for (int n = 1; n < 7; n++) {
            // Determine number encoding type, L or G, then write
            BarRegion nType = (EanLeftPattern[code[0]] << (n-1)) & 0b00100000 ? G : L;
            writeNumUPC(data, width, linePos, height, code[n], nType, hasAlpha);
        }
        writeGuardUPC(data, width, linePos, height, M, hasAlpha);
        for (int n = 7; n < code.size(); n++) {
            writeNumUPC(data, width, linePos, height, code[n], R, hasAlpha);
        }
        
        // Add check digit, if neccessary
        if (code.size() == 12) {
            int remainder = ((3 * (code[1] + code[3] + code[5] + code[7] + code[9] + code[11])
                            + (code[0] + code[2] + code[4] + code[6] + code[8] + code[10])) % 10);
            int checkDigit = (10 - remainder) % 10;

            // Write check digit
            writeNumUPC(data, width, linePos, height, checkDigit, R, hasAlpha);
        }
        
        // Add end guard pattern (4 cols)
        writeGuardUPC(data, width, linePos, height, E, hasAlpha);

        of.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    of.close();
}

#ifdef DEBUG
void test(bool hasAlpha, std::string path) {
    int linePos = 0;
    int width = 400;
    int height = 400;
    std::vector<unsigned char> data(width * height * (3 + hasAlpha), (char) 255);

    std::ofstream of(path, std::ios_base::binary);
    if (of.is_open()) {
        writeBMPHead(data.size(), width, height, of, hasAlpha);
        for (int i = 0; i < width; i++) {
            std::cout << "Writing line. ";
            writeBar(data, width, linePos, height, hasAlpha);
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
}