#pragma once

#include <fstream>
#include <vector>
#include <string>

namespace bargenlib
{
    enum CodeType {
        UPC_A = 0,
        EAN_13 = 1,
        EAN_8 = 2,
    };

    // Describes your barcode's image format.
    //     BMP > bitmap (8bpp)
    //     PNG -> png (8bpp)
    //     PNGa -> png with Alpha (16bpp)
    enum FileType {
        BMP = 0,
        PNG = 1,
        PNG_A = 2,
    };

    // Generates a UPC-A barcode bitmap image at path.
    void save(const std::vector<int> &code, const std::string &path, CodeType codeType, FileType fileType);
}
