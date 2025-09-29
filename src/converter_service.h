#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

struct PdfConversionOptions {
    bool create_cbz = false;
    bool clean_images = false;
    std::string format = "jpeg";
    int quality = 80;
    double dpi = 150.0;
};

class ConverterService {
public:
    using Logger = std::function<void(const std::string&)>;

    static std::vector<std::filesystem::path> FindPdfFiles(const std::filesystem::path& directory);
    static std::vector<std::filesystem::path> FindCbzFiles(const std::filesystem::path& directory);

    static bool ConvertSinglePdf(const std::filesystem::path& pdf_path,
                                 const std::filesystem::path& base_output_dir,
                                 const PdfConversionOptions& options,
                                 const Logger& logger = {});

    static bool ConvertSingleCbz(const std::filesystem::path& cbz_path,
                                 const std::filesystem::path& base_output_dir,
                                 const Logger& logger = {});
};
