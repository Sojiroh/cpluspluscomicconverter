#pragma once

#include <string>

class CBZToPDFConverter {
public:
    static bool convert_cbz_to_pdf(const std::string& cbz_path,
                                   const std::string& output_pdf_path);
};
