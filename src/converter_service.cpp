#include "converter_service.h"

#include "pdf_image_extractor.h"
#include "cbz_creator.h"
#include "cbz_to_pdf_converter.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>

namespace {
void Emit(const ConverterService::Logger& logger, const std::string& message) {
    if (logger) {
        logger(message);
    } else {
        std::cout << message << std::endl;
    }
}

void EmitSeparator(const ConverterService::Logger& logger) {
    Emit(logger, std::string(50, '='));
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
}

std::vector<std::filesystem::path> ConverterService::FindPdfFiles(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> pdf_files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto extension = ToLower(entry.path().extension().string());
                if (extension == ".pdf") {
                    pdf_files.push_back(entry.path());
                }
            }
        }
    } catch (const std::exception& e) {
        Emit({}, std::string("Error reading directory: ") + e.what());
    }

    std::sort(pdf_files.begin(), pdf_files.end());
    return pdf_files;
}

std::vector<std::filesystem::path> ConverterService::FindCbzFiles(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> cbz_files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                auto extension = ToLower(entry.path().extension().string());
                if (extension == ".cbz") {
                    cbz_files.push_back(entry.path());
                }
            }
        }
    } catch (const std::exception& e) {
        Emit({}, std::string("Error reading directory: ") + e.what());
    }

    std::sort(cbz_files.begin(), cbz_files.end());
    return cbz_files;
}

bool ConverterService::ConvertSinglePdf(const std::filesystem::path& pdf_path,
                                        const std::filesystem::path& base_output_dir,
                                        const PdfConversionOptions& options,
                                        const Logger& logger) {
    const std::string pdf_name = pdf_path.stem().string();
    const std::filesystem::path output_dir = base_output_dir / pdf_name;

    Emit(logger, "");
    EmitSeparator(logger);
    Emit(logger, "Processing: " + pdf_path.string());
    Emit(logger, "Output directory: " + output_dir.string());

    PDFImageExtractor extractor(pdf_path.string(), options.format, options.quality, options.dpi);
    if (!extractor.is_valid()) {
        Emit(logger, "Error: Could not load PDF file: " + pdf_path.string());
        return false;
    }

    Emit(logger, "PDF loaded successfully! Total pages: " + std::to_string(extractor.get_page_count()));

    auto extracted_images = extractor.extract_all_images(output_dir.string());
    if (extracted_images.empty()) {
        Emit(logger, "No images found in the PDF.");
        return false;
    }

    Emit(logger, "Extracted " + std::to_string(extracted_images.size()) + " images");

    if (options.create_cbz) {
        const std::string cbz_filename = pdf_name + ".cbz";
        const std::filesystem::path cbz_path = base_output_dir / cbz_filename;

        Emit(logger, "Creating CBZ archive...");
        if (CBZCreator::create_cbz_from_directory(output_dir.string(), cbz_path.string())) {
            Emit(logger, "CBZ file created: " + cbz_path.string());

            if (options.clean_images) {
                Emit(logger, "Cleaning up individual image files...");
                try {
                    std::filesystem::remove_all(output_dir);
                    Emit(logger, "Cleanup complete!");
                } catch (const std::exception& e) {
                    Emit(logger, std::string("Warning: Failed to clean up: ") + e.what());
                }
            }
        } else {
            Emit(logger, "Failed to create CBZ archive for: " + pdf_path.string());
            return false;
        }
    }

    return true;
}

bool ConverterService::ConvertSingleCbz(const std::filesystem::path& cbz_path,
                                        const std::filesystem::path& base_output_dir,
                                        const Logger& logger) {
    const std::string cbz_name = cbz_path.stem().string();
    std::error_code ec;
    std::filesystem::create_directories(base_output_dir, ec);
    if (ec) {
        Emit(logger, std::string("Error creating output directory: ") + ec.message());
        return false;
    }

    const std::filesystem::path output_pdf = base_output_dir / (cbz_name + ".pdf");

    Emit(logger, "");
    EmitSeparator(logger);
    Emit(logger, "Processing CBZ: " + cbz_path.string());
    Emit(logger, "Output PDF: " + output_pdf.string());

    if (!CBZToPDFConverter::convert_cbz_to_pdf(cbz_path.string(), output_pdf.string())) {
        Emit(logger, "Failed to convert CBZ to PDF: " + cbz_path.string());
        return false;
    }

    return true;
}
