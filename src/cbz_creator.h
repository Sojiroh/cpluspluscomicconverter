#pragma once

#include <string>
#include <vector>

class CBZCreator {
public:
    static bool create_cbz_from_images(const std::vector<std::string>& image_paths, 
                                       const std::string& output_cbz_path);
    
    static bool create_cbz_from_directory(const std::string& image_directory, 
                                          const std::string& output_cbz_path);

private:
    static std::vector<std::string> get_image_files_from_directory(const std::string& directory);
    static void sort_image_files_naturally(std::vector<std::string>& files);
};