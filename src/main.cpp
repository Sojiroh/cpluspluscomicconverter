#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <cctype>
#include "pdf_image_extractor.h"
#include "cbz_creator.h"
#include "cbz_to_pdf_converter.h"

std::vector<std::string> find_pdf_files(const std::string& directory) {
    std::vector<std::string> pdf_files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".pdf") {
                    pdf_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    
    // Sort files alphabetically
    std::sort(pdf_files.begin(), pdf_files.end());
    
    return pdf_files;
}

std::vector<std::string> find_cbz_files(const std::string& directory) {
    std::vector<std::string> cbz_files;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension == ".cbz") {
                    cbz_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }

    std::sort(cbz_files.begin(), cbz_files.end());
    return cbz_files;
}

bool process_single_pdf(const std::string& pdf_path, const std::string& base_output_dir,
                       bool create_cbz, bool clean_images, const std::string& format,
                       int quality, double dpi) {
    std::string pdf_name = std::filesystem::path(pdf_path).stem().string();
    std::string output_dir = std::filesystem::path(base_output_dir) / pdf_name;
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Processing: " << pdf_path << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl;
    
    PDFImageExtractor extractor(pdf_path, format, quality, dpi);
    
    if (!extractor.is_valid()) {
        std::cerr << "Error: Could not load PDF file: " << pdf_path << std::endl;
        return false;
    }
    
    std::cout << "PDF loaded successfully! Total pages: " << extractor.get_page_count() << std::endl;
    
    auto extracted_images = extractor.extract_all_images(output_dir);
    
    if (extracted_images.empty()) {
        std::cout << "No images found in the PDF." << std::endl;
        return false;
    }
    
    std::cout << "Extracted " << extracted_images.size() << " images" << std::endl;
    
    if (create_cbz) {
        std::string cbz_filename = pdf_name + ".cbz";
        std::string cbz_path = std::filesystem::path(base_output_dir) / cbz_filename;
        
        std::cout << "Creating CBZ archive..." << std::endl;
        if (CBZCreator::create_cbz_from_directory(output_dir, cbz_path)) {
            std::cout << "CBZ file created: " << cbz_path << std::endl;
            
            if (clean_images) {
                std::cout << "Cleaning up individual image files..." << std::endl;
                try {
                    std::filesystem::remove_all(output_dir);
                    std::cout << "Cleanup complete!" << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to clean up: " << e.what() << std::endl;
                }
            }
        } else {
            std::cerr << "Failed to create CBZ archive for: " << pdf_path << std::endl;
            return false;
        }
    }
    
    return true;
}

bool process_single_cbz(const std::string& cbz_path, const std::string& base_output_dir) {
    std::string cbz_name = std::filesystem::path(cbz_path).stem().string();
    std::filesystem::path output_directory(base_output_dir);
    if (!output_directory.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(output_directory, ec);
        if (ec) {
            std::cerr << "Error creating output directory: " << ec.message() << std::endl;
            return false;
        }
    }

    std::filesystem::path output_pdf = output_directory / (cbz_name + ".pdf");

    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Processing CBZ: " << cbz_path << std::endl;
    std::cout << "Output PDF: " << output_pdf.string() << std::endl;

    if (!CBZToPDFConverter::convert_cbz_to_pdf(cbz_path, output_pdf.string())) {
        return false;
    }

    return true;
}

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
        std::vector<std::string> cbz_files;

        if (std::filesystem::is_directory(input_path)) {
            std::cout << "Input directory: " << input_path << std::endl;
            cbz_files = find_cbz_files(input_path);

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
            cbz_files.push_back(input_path);
        } else {
            std::cerr << "Error: Input path does not exist or is not accessible: " << input_path << std::endl;
            return 1;
        }

        std::cout << "Output directory: " << output_dir << std::endl;
        std::cout << "Mode: PDF output" << std::endl;

        for (const auto& cbz_path : cbz_files) {
            if (process_single_cbz(cbz_path, output_dir)) {
                successful++;
            } else {
                failed++;
            }
        }
    } else {
        std::vector<std::string> pdf_files;

        if (std::filesystem::is_directory(input_path)) {
            std::cout << "Input directory: " << input_path << std::endl;
            pdf_files = find_pdf_files(input_path);

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
            pdf_files.push_back(input_path);
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

        for (const auto& pdf_path : pdf_files) {
            if (process_single_pdf(pdf_path, output_dir, create_cbz, clean_images, format, quality, dpi)) {
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
