#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct PDFImageInput {
    std::string name;
    int width;
    int height;
    int components;
    std::vector<std::uint8_t> data;
};

class PDFCreator {
public:
    static bool create_pdf_from_images(const std::vector<PDFImageInput>& images,
                                       const std::string& output_pdf_path);
};
