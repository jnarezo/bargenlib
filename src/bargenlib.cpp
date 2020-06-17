#include "bargenlib/bargenlib.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <array>
#include <vector>

#include "lodepng.h"

namespace bargenlib
{
    using std::uint16_t;
    using std::uint32_t;
    namespace
    {
    // BMP File Formatting Structs
    #pragma pack(push, 1)
    struct BMPFileHeader {
        uint16_t fileType = 0x4D42;
        uint32_t fileSize;
        uint16_t reserveA = 0;
        uint16_t reserveB = 0;
        uint32_t imageOffset;
        BMPFileHeader(int dataSize):
            fileSize((14+40+8) + dataSize),
            imageOffset(14+40+8){}
    };

    struct BMPInfoHeader {
        uint32_t headerSize = 40;
        uint32_t width;
        uint32_t height;
        uint16_t noPlanes = 1;
        uint16_t bitDepth = 8;  // bpp
        uint32_t compression = 0;
        uint32_t imageSize = 0;
        uint32_t xPPM = 0;
        uint32_t yPPM = 0;
        uint32_t noColorIndices = 2;  // Colors in palette
        uint32_t colorPriority = 0;
        BMPInfoHeader(int width, int height):
            width(width),
            height(height){}
    };

    struct BMPColorTable {
        uint32_t white = 0x00FFFFFF;
        uint32_t black = 0x00000000;
    };
    #pragma pack(pop)

    std::array<uint8_t, 10> UpcEncodeTable{{
        0b0001101,  // 0
        0b0011001,  // 1
        0b0010011,  // 2
        0b0111101,  // 3
        0b0100011,  // 4
        0b0110001,  // 5
        0b0101111,  // 6
        0b0111011,  // 7
        0b0110111,  // 8
        0b0001011,  // 9
    }};

    std::array<uint8_t, 10> EanParityPattern{{
        0b0000000,  // 0
        0b0001011,  // 1
        0b0001101,  // 2
        0b0001110,  // 3
        0b0010011,  // 4
        0b0011001,  // 5
        0b0011100,  // 6
        0b0010101,  // 7
        0b0010110,  // 8
        0b0011010,  // 9
    }};

    enum BarRegion {
        S = 0,  // Start
        L = 1,  // Left Digit
        G = 2,  // G Digit
        M = 3,  // Middle
        R = 4,  // Right Digit
        E = 5,  // End
    };

    struct ImageInfo {
        FileType fileType;
        int width;
        int height;
        int bytesWidth;
        bool hasAlpha;
        int bitDepth;
        int channels;
        ImageInfo(FileType fileType, int width, int height, int bytesWidth, bool hasAlpha,
                int bitDepth, int channels):
            fileType(fileType),
            width(width),
            height(height),
            bytesWidth(bytesWidth),
            hasAlpha(hasAlpha),
            bitDepth(bitDepth),
            channels(channels) {}
    };

    void initImage(const ImageInfo &info, std::vector<uint8_t> &data) {
        uint8_t defaultVal = 0;
        switch (info.fileType) {
            case PNG:
                defaultVal = 255;
                break;
            case PNG_A:
            case BMP:
            default:
                defaultVal = 0;
                break;
        }
        data.resize(info.bytesWidth * info.height, defaultVal);
    }

    void writeBar(const ImageInfo &info, std::vector<uint8_t> &data, const int &xPos) {
        // Write a black opaque line in to the bitmap data.
        for (int y = 0; y < info.height; y++) {
            switch (info.fileType) {
                case PNG_A:
                    data[(xPos * info.channels) + (y * info.bytesWidth) + 1] = 255;
                    break;
                case PNG:
                    data[(xPos * info.channels) + (y * info.bytesWidth)] = 0;
                case BMP:
                default:
                    data[(xPos * info.channels) + (y * info.bytesWidth)] = 1;
                    break;
            }
        }
    }

    void writeGuardUPC(const ImageInfo &info, std::vector<uint8_t> &data,
            int &xPos, const BarRegion &region) {
        if (region != BarRegion::S) xPos++;
        bargenlib::writeBar(info, data, xPos);
        xPos += 2;
        bargenlib::writeBar(info, data, xPos);
        xPos++;
        if (region != BarRegion::E) xPos++;
    }

    void writeNumUPC(const ImageInfo &info, std::vector<uint8_t> &data, int &xPos,
            const int &num, const BarRegion &region) {
        // Add the given UPC number correctly in the given region (L/G/R)
        for (int i = 0; i < 7; i++) {
            int lineNum = (region == BarRegion::G) ? 6-i : i;
            uint8_t encoding = (region == BarRegion::L)?UpcEncodeTable[num]:~UpcEncodeTable[num];
            if (((encoding << lineNum) & 0b1000000)) {
                bargenlib::writeBar(info, data, xPos);
            }
            xPos++;
        }
    }
    
    void writePNG(const ImageInfo &info, const std::vector<uint8_t> &data, const std::string &path) {
        std::vector<uint8_t> pngImage;
        unsigned int error = lodepng::encode(
                pngImage,
                data,
                info.width,
                info.height,
                (info.hasAlpha) ? LodePNGColorType::LCT_GREY_ALPHA : LodePNGColorType::LCT_GREY,
                info.bitDepth);
        if (!error) lodepng::save_file(pngImage, path);
    }

    void writeBMP(const ImageInfo &info, const std::vector<uint8_t> &data, const std::string &path) {
        std::ofstream of(path, std::ios_base::binary);
        if (of.is_open()) {
            BMPFileHeader fileHeader(data.size());
            BMPInfoHeader infoHeader(info.width, -info.height);
            BMPColorTable colorTable = BMPColorTable();
            of.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
            of.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
            of.write(reinterpret_cast<const char*>(&colorTable), sizeof(colorTable));
            of.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
        of.close();
    }

    void writeImage(const ImageInfo &info, const std::vector<uint8_t> &data,
            const std::string &path, bargenlib::FileType format) {
        switch (format) {
            case FileType::PNG:
            case FileType::PNG_A:
                writePNG(info, data, path);
                break;
            case FileType::BMP:
            default:
                writeBMP(info, data, path);
                break;
        }
    }

    void encodeEAN8(ImageInfo &info, std::vector<uint8_t> &data, const std::vector<int> &code) {
        if (code.size() != 7 && code.size() != 8) {
            throw std::runtime_error("A valid EAN-8 code must be 7 or 8 digits.");
        }
        for (int n : code) {
            if (n < 0 || n > 9) throw std::runtime_error("An EAN-8 digit must be 0-9.");
        }

        info.width = 88;  // padding, divisible by 4
        info.height = 78;
        info.bytesWidth = info.width * info.channels * (info.bitDepth / 8);
        initImage(info, data);
        int linePos = 9; // Space padding

        // Add guard patterns and numbers.
        writeGuardUPC(info, data, linePos, S);
        for (int n = 0; n < 4; n++) {
            writeNumUPC(info, data, linePos, code[n], L);
        }
        writeGuardUPC(info, data, linePos, M);
        for (int n = 4; n < code.size(); n++) {
            writeNumUPC(info, data, linePos, code[n], R);
        }

        // Add check digit, if neccessary
        if (code.size() == 7) {
            int remainder = ((3 * (code[0] + code[2] + code[4] + code[6])
                                    + (code[1] + code[3] + code[5])) % 10);
            int checkDigit = (10 - remainder) % 10;
            writeNumUPC(info, data, linePos, checkDigit, R);
        }

        // Add end guard pattern
        writeGuardUPC(info, data, linePos, E);
    }

    void encodeEAN13(ImageInfo &info, std::vector<uint8_t> &data, const std::vector<int> &code) {
        if (code.size() != 12 && code.size() != 13) {
            throw std::runtime_error("A valid EAN-13 code must be 12-13 digits.");
        }
        for (int n : code) {
            if (n < 0 || n > 9) throw std::runtime_error("An EAN-13 digit must be 0-9.");
        }

        info.width = 116;  // padding, divisible by 4
        info.height = 78;
        info.bytesWidth = info.width * info.channels * (info.bitDepth / 8);
        initImage(info, data);
        int linePos = 9; // Space padding

        // Add guard patterns and numbers.
        writeGuardUPC(info, data, linePos, S);
        for (int n = 1; n < 7; n++) {
            // Determine parity type, G (even) or L (odd), then write
            BarRegion region = (EanParityPattern[code[0]] << (n-1)) & (0b100000) ? G : L;
            writeNumUPC(info, data, linePos, code[n], region);
        }
        writeGuardUPC(info, data, linePos, M);
        for (int n = 7; n < code.size(); n++) {
            writeNumUPC(info, data, linePos, code[n], R);
        }

        // Add check digit, if neccessary
        if (code.size() == 12) {
            int remainder = ((3 * (code[1] + code[3] + code[5] + code[7] + code[9] + code[11])
                            + (code[0] + code[2] + code[4] + code[6] + code[8] + code[10])) % 10);
            int checkDigit = (10 - remainder) % 10;
            writeNumUPC(info, data, linePos, checkDigit, R);
        }

        // Add end guard pattern (4 cols)
        writeGuardUPC(info, data, linePos, E);
    }

    void encodeUPCA(ImageInfo &info, std::vector<uint8_t> &data, const std::vector<int> &code) {
        if (code.size() != 11 && code.size() != 12) {
            throw std::invalid_argument("A valid UPC-A code must be 11 or 12 digits.");
        }
        for (int n : code) {
            if (n < 0 || n > 9) throw std::invalid_argument("A UPC-A digit must be 0-9.");
        }
        // UPC-A is equivalent to EAN-13 with international code 0.
        std::vector<int> eanCode(code);
        std::vector<int>::iterator iterator;
        iterator = eanCode.begin();
        eanCode.insert(iterator, 0);
        encodeEAN13(info, data, eanCode);
    }
    }

void save(const std::vector<int> &code, const std::string &path,
        Encoding codeType, FileType fileType) {
    ImageInfo *info;
    switch (fileType) {
        case PNG_A:
            info = &ImageInfo(fileType, 0, 0, 0, true, 8, 2);
            break;
        case PNG:
        case BMP:
        default:
            info = &ImageInfo(fileType, 0, 0, 0, false, 8, 1);
            break;
    }
    std::vector<uint8_t> data;
    switch (codeType) {
        case EAN_8:
            encodeEAN8(*info, data, code);
            break;
        case EAN_13:
            encodeEAN13(*info, data, code);
            break;
        case UPC_A:
        default:
            encodeUPCA(*info, data, code);
            break;
    }
    writeImage(*info, data, path, fileType);
}
}