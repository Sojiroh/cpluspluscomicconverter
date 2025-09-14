#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace poppler {
    class document;
    class page_renderer;
}

class PDFImageExtractor {
public:
    explicit PDFImageExtractor(const std::string& pdf_path, const std::string& format = "jpeg", int quality = 80, double dpi = 150.0);
    ~PDFImageExtractor();

    bool is_valid() const;
    int get_page_count() const;
    
    struct ImageInfo {
        std::string name;
        int width;
        int height;
        std::string format;
    };
    
    std::vector<ImageInfo> extract_images_from_page(int page_index, const std::string& output_dir = ".");
    std::vector<ImageInfo> extract_all_images(const std::string& output_dir = ".");

private:
    std::unique_ptr<poppler::document> document_;
    std::unique_ptr<poppler::page_renderer> renderer_;
    std::string pdf_path_;
    bool valid_;
    mutable std::mutex renderer_mutex_;
    std::string format_;
    int quality_;
    double dpi_;
    
    std::string generate_image_filename(int page_index, int image_index, const std::string& format) const;
};