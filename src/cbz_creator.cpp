#include "cbz_creator.h"
#include <zip.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <regex>

bool CBZCreator::create_cbz_from_images(const std::vector<std::string>& image_paths, 
                                        const std::string& output_cbz_path) {
    if (image_paths.empty()) {
        std::cerr << "No images provided for CBZ creation" << std::endl;
        return false;
    }
    
    int error = 0;
    zip_t* archive = zip_open(output_cbz_path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    
    if (!archive) {
        zip_error_t zip_error;
        zip_error_init_with_code(&zip_error, error);
        std::cerr << "Failed to create CBZ archive: " << zip_error_strerror(&zip_error) << std::endl;
        zip_error_fini(&zip_error);
        return false;
    }
    
    std::cout << "Creating CBZ archive: " << output_cbz_path << std::endl;
    
    for (size_t i = 0; i < image_paths.size(); ++i) {
        const auto& image_path = image_paths[i];
        
        if (!std::filesystem::exists(image_path)) {
            std::cerr << "Warning: Image file not found: " << image_path << std::endl;
            continue;
        }
        
        // Get just the filename for the archive
        std::string filename = std::filesystem::path(image_path).filename().string();
        
        // Create zip source directly from file - more reliable than buffer
        zip_source_t* source = zip_source_file(archive, image_path.c_str(), 0, 0);
        if (!source) {
            std::cerr << "Warning: Failed to create zip source for: " << filename << std::endl;
            continue;
        }
        
        // Get file size for reporting
        auto file_size = std::filesystem::file_size(image_path);
        
        // Add file to archive
        zip_int64_t index = zip_file_add(archive, filename.c_str(), source, ZIP_FL_OVERWRITE);
        if (index < 0) {
            std::cerr << "Warning: Failed to add file to archive: " << filename << std::endl;
            zip_source_free(source);
            continue;
        }
        
        std::cout << "Added to CBZ: " << filename << " (" << file_size << " bytes)" << std::endl;
    }
    
    if (zip_close(archive) != 0) {
        std::cerr << "Failed to close CBZ archive" << std::endl;
        return false;
    }
    
    std::cout << "CBZ archive created successfully: " << output_cbz_path << std::endl;
    return true;
}

bool CBZCreator::create_cbz_from_directory(const std::string& image_directory, 
                                           const std::string& output_cbz_path) {
    if (!std::filesystem::exists(image_directory)) {
        std::cerr << "Image directory does not exist: " << image_directory << std::endl;
        return false;
    }
    
    auto image_files = get_image_files_from_directory(image_directory);
    if (image_files.empty()) {
        std::cerr << "No image files found in directory: " << image_directory << std::endl;
        return false;
    }
    
    sort_image_files_naturally(image_files);
    
    std::cout << "Found " << image_files.size() << " image files in directory" << std::endl;
    
    return create_cbz_from_images(image_files, output_cbz_path);
}

std::vector<std::string> CBZCreator::get_image_files_from_directory(const std::string& directory) {
    std::vector<std::string> image_files;
    
    const std::vector<std::string> image_extensions = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp"};
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (std::find(image_extensions.begin(), image_extensions.end(), extension) != image_extensions.end()) {
                    image_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }
    
    return image_files;
}

void CBZCreator::sort_image_files_naturally(std::vector<std::string>& files) {
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        std::filesystem::path path_a(a);
        std::filesystem::path path_b(b);
        
        std::string name_a = path_a.stem().string();
        std::string name_b = path_b.stem().string();
        
        // Try to extract page numbers for natural sorting
        std::regex page_regex(R"(page(\d+))");
        std::smatch match_a, match_b;
        
        if (std::regex_search(name_a, match_a, page_regex) && 
            std::regex_search(name_b, match_b, page_regex)) {
            int page_a = std::stoi(match_a[1].str());
            int page_b = std::stoi(match_b[1].str());
            return page_a < page_b;
        }
        
        // Fallback to lexicographic sorting
        return name_a < name_b;
    });
}