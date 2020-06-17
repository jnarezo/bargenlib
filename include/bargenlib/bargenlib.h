#pragma once

#include <vector>
#include <string>

namespace bargenlib
{
    /*
     * Specifies your barcode's type and encoding:
     *     UPC_A    - UPC-A encoding [default]
     *     EAN_13   - EAN-13 encoding (a superset of UPC-A)
     *     EAN-8    - EAN-8 encoding
     */
    enum Encoding {
        EAN_13 = 0,
        UPC_A = 1,
        EAN_8 = 2,
    };

    /* 
     * Describes your barcode's image format.
     *     BMP      - bitmap (8-bpp) [default]
     *     PNG      - png (smallest disk-size)
     *     PNG_A    - png, with alpha channel
     */
    enum FileType {
        BMP = 0,
        PNG = 1,
        PNG_A = 2,
    };

    /*
     * Exports a barcode image to the disk at the specified file path with
     * the specified file type. The barcode's encoding must be specified with
     * through the CodeType enumerator so the appropriate encoding is used.
     */
    void save(const std::vector<int> &code, const std::string &path, Encoding codeType, FileType fileType);
}
