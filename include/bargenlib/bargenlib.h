#pragma once

#include <cstdint>
#include <fstream>
#include <vector>
#include <string>

namespace bargenlib
{
    // Describes your barcode's image format.
    //     BMP > bitmap (8-bpp)
    //     PNG -> png (8-bpp)
    //     PNGa -> png with Alpha (16-bpp)
    enum FileType {
        BMP = 0,
        PNG = 1,
        PNGa = 2,
    };

    // Generates a UPC-A barcode bitmap image at path.
    void upc_a(const std::vector<int>& code, const std::string& path, bargenlib::FileType format);

    // Generates a EAN-8 barcode bitmap image at path.
    void ean_8(const std::vector<int>& code, const std::string& path, bargenlib::FileType format);

    // Generates a EAN-13 barcode bitmap image at path.
    void ean_13(const std::vector<int>& code, const std::string& path, bargenlib::FileType format);
}
