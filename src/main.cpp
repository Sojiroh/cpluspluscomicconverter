#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "converter_service.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file_or_directory> [output_directory] [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --cbz                Create a CBZ (Comic Book Archive) file instead of separate images" << std::endl;
        std::cout << "  --clean              Remove individual image files after creating CBZ (requires --cbz)" << std::endl;
        std::cout << "  --format <format>    Output format: png or jpeg (default: jpeg)" << std::endl;
        std::cout << "  --quality <1-100>    JPEG quality (default: 80, ignored for PNG)" << std::endl;
        std::cout << "  --dpi <value>        DPI for image extraction (default: 150)" << std::endl;
        std::cout << "  --pdf                Convert CBZ archives to PDF documents (JPEG pages only)" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " document.pdf ./extracted_images" << std::endl;
        std::cout << "  " << argv[0] << " /path/to/pdfs/ ./converted_comics --cbz --clean" << std::endl;
        std::cout << "  " << argv[0] << " document.pdf ./output --format png --dpi 300" << std::endl;
        std::cout << "  " << argv[0] << " document.pdf ./output --format jpeg --quality 90 --dpi 150" << std::endl;
        std::cout << "  " << argv[0] << " comic.cbz ./output --pdf" << std::endl;
        return 1;
    }
    
    std::string input_path = argv[1];
    std::string output_dir = "./converted_comics";
    bool create_cbz = false;
    bool clean_images = false;
    bool output_pdf = false;
    std::string format = "jpeg";
    int quality = 80;
    double dpi = 150.0;
    
    // Parse arguments
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--cbz") {
            create_cbz = true;
        } else if (arg == "--clean") {
            clean_images = true;
        } else if (arg == "--pdf") {
            output_pdf = true;
        } else if (arg == "--format" && i + 1 < argc) {
            format = argv[++i];
            if (format != "png" && format != "jpeg") {
                std::cerr << "Error: Format must be 'png' or 'jpeg'" << std::endl;
                return 1;
            }
        } else if (arg == "--quality" && i + 1 < argc) {
            quality = std::stoi(argv[++i]);
            if (quality < 1 || quality > 100) {
                std::cerr << "Error: Quality must be between 1 and 100" << std::endl;
                return 1;
            }
        } else if (arg == "--dpi" && i + 1 < argc) {
            dpi = std::stod(argv[++i]);
            if (dpi <= 0) {
                std::cerr << "Error: DPI must be greater than 0" << std::endl;
                return 1;
            }
        } else if (arg[0] != '-') {
            output_dir = arg;
        }
    }
    
    // Validate arguments
    if (output_pdf) {
        if (create_cbz || clean_images) {
            std::cerr << "Error: --cbz and --clean are not supported with --pdf" << std::endl;
            return 1;
        }
    } else {
        if (clean_images && !create_cbz) {
            std::cerr << "Error: --clean option requires --cbz option" << std::endl;
            return 1;
        }
    }

    std::cout << "Comic Converter" << std::endl;
    std::cout << "================" << std::endl;
    
    int successful = 0;
    int failed = 0;
    
    if (output_pdf) {
        std::vector<std::filesystem::path> cbz_files;

        if (std::filesystem::is_directory(input_path)) {
            std::cout << "Input directory: " << input_path << std::endl;
            cbz_files = ConverterService::FindCbzFiles(input_path);

            if (cbz_files.empty()) {
                std::cerr << "No CBZ files found in directory: " << input_path << std::endl;
                return 1;
            }

            std::cout << "Found " << cbz_files.size() << " CBZ files" << std::endl;
        } else if (std::filesystem::is_regular_file(input_path)) {
            std::string extension = std::filesystem::path(input_path).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension != ".cbz") {
                std::cerr << "Error: Input file is not a CBZ archive: " << input_path << std::endl;
                return 1;
            }
            std::cout << "Input file: " << input_path << std::endl;
            cbz_files.emplace_back(input_path);
        } else {
            std::cerr << "Error: Input path does not exist or is not accessible: " << input_path << std::endl;
            return 1;
        }

        std::cout << "Output directory: " << output_dir << std::endl;
        std::cout << "Mode: PDF output" << std::endl;

        for (const auto& cbz_path : cbz_files) {
            if (ConverterService::ConvertSingleCbz(cbz_path, output_dir)) {
                successful++;
            } else {
                failed++;
            }
        }
    } else {
        std::vector<std::filesystem::path> pdf_files;

        if (std::filesystem::is_directory(input_path)) {
            std::cout << "Input directory: " << input_path << std::endl;
            pdf_files = ConverterService::FindPdfFiles(input_path);

            if (pdf_files.empty()) {
                std::cerr << "No PDF files found in directory: " << input_path << std::endl;
                return 1;
            }

            std::cout << "Found " << pdf_files.size() << " PDF files" << std::endl;
        } else if (std::filesystem::is_regular_file(input_path)) {
            std::string extension = std::filesystem::path(input_path).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension != ".pdf") {
                std::cerr << "Error: Input file is not a PDF: " << input_path << std::endl;
                return 1;
            }
            std::cout << "Input PDF: " << input_path << std::endl;
            pdf_files.emplace_back(input_path);
        } else {
            std::cerr << "Error: Input path does not exist or is not accessible: " << input_path << std::endl;
            return 1;
        }

        std::cout << "Output directory: " << output_dir << std::endl;
        std::cout << "Mode: PDF to images" << std::endl;
        std::cout << "Image format: " << format << std::endl;
        if (format == "jpeg") {
            std::cout << "JPEG quality: " << quality << std::endl;
        }
        std::cout << "DPI: " << dpi << std::endl;
        if (create_cbz) {
            std::cout << "Output format: CBZ (Comic Book Archive)" << std::endl;
            if (clean_images) {
                std::cout << "Clean mode: Individual images will be removed after CBZ creation" << std::endl;
            }
        } else {
            std::cout << "Output format: Individual " << format << " images" << std::endl;
        }

        PdfConversionOptions pdf_options;
        pdf_options.create_cbz = create_cbz;
        pdf_options.clean_images = clean_images;
        pdf_options.format = format;
        pdf_options.quality = quality;
        pdf_options.dpi = dpi;

        for (const auto& pdf_path : pdf_files) {
            if (ConverterService::ConvertSinglePdf(pdf_path, output_dir, pdf_options)) {
                successful++;
            } else {
                failed++;
            }
        }
    }
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Processing complete!" << std::endl;
    std::cout << "Successful: " << successful << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    
    if (failed > 0) {
        return 1;
    }
    
    return 0;
}
