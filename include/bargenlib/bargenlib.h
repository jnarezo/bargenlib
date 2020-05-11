#pragma once

#include <cstdint>
#include <fstream>
#include <vector>
#include <string>

namespace bargenlib
{
    /**
     * @brief Generates a UPC-A barcode bitmap image at path.
     * 
     * @param code 
     * @param hasAlpha 
     * @param path 
     */
    void upc_a(const std::vector<int>& code, const std::string& path, bool hasAlpha = true);

    /**
     * @brief Generates a EAN-8 barcode bitmap image at path.
     * 
     * @param code 
     * @param hasAlpha 
     * @param path 
     */
    void ean_8(const std::vector<int>& code, const std::string& path, bool hasAlpha = true);

    /**
     * @brief Generates a EAN-13 barcode bitmap image at path.
     * 
     * @param code 
     * @param hasAlpha 
     * @param path 
     */
    void ean_13(const std::vector<int>& code, const std::string& path, bool hasAlpha = true);
}
