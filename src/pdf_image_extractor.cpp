#include "pdf_image_extractor.h"
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-image.h>
#include <poppler-page-renderer.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <future>
#include <algorithm>

PDFImageExtractor::PDFImageExtractor(const std::string& pdf_path, const std::string& format, int quality, double dpi)
    : pdf_path_(pdf_path), valid_(false), format_(format), quality_(quality), dpi_(dpi) {

    try {
        document_ = std::unique_ptr<poppler::document>(
            poppler::document::load_from_file(pdf_path_)
        );

        if (document_ && !document_->is_locked()) {
            renderer_ = std::make_unique<poppler::page_renderer>();
            renderer_->set_render_hint(poppler::page_renderer::antialiasing, true);
            renderer_->set_render_hint(poppler::page_renderer::text_antialiasing, true);
            valid_ = true;
        } else {
            std::cerr << "Failed to load PDF or PDF is locked: " << pdf_path_ << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading PDF : " << e.what() << std::endl;
    }
}

PDFImageExtractor::~PDFImageExtractor() = default;

bool PDFImageExtractor::is_valid() const {
    return valid_;
}

int PDFImageExtractor::get_page_count() const {
    if (!valid_) return 0;
    return document_->pages();
}

std::string PDFImageExtractor::generate_image_filename(int page_index, int image_index, const std::string& format) const {
    std::string base_name = std::filesystem::path(pdf_path_).stem().string();
    return base_name + "_page" + std::to_string(page_index + 1) + "_img" + std::to_string(image_index + 1) + "." + format;
}

std::vector<PDFImageExtractor::ImageInfo> PDFImageExtractor::extract_images_from_page(int page_index, const std::string& output_dir) {
    std::vector<ImageInfo> extracted_images;

    if (!valid_ || page_index < 0 || page_index >= document_->pages()) {
        std::cerr << "Invalid page index : " << page_index << std::endl;
        return extracted_images;
    }
    
    try {
        std::filesystem::create_directories(output_dir);
        
        auto page = std::unique_ptr<poppler::page>(document_->create_page(page_index));
        if (!page) {
            std::cerr << "Failed to create page: " << page_index << std::endl;
            return extracted_images;
        }
        
        // Use shared page renderer to convert page to image
        poppler::image page_image;
        {
            std::lock_guard<std::mutex> lock(renderer_mutex_);
            page_image = renderer_->render_page(page.get(), dpi_, dpi_);
        }
        
        if (page_image.is_valid()) {
            std::string filename = generate_image_filename(page_index, 0, format_);
            std::string full_path = std::filesystem::path(output_dir) / filename;

            bool save_success;
            if (format_ == "jpeg") {
                save_success = page_image.save(full_path, format_, quality_);
            } else {
                save_success = page_image.save(full_path, format_);
            }

            if (save_success) {
                ImageInfo info;
                info.name = filename;
                info.width = page_image.width();
                info.height = page_image.height();
                info.format = format_;
                extracted_images.push_back(info);
                
                std::cout << "Extracted page as image: " << filename 
                          << " (" << info.width << "x" << info.height << ")" << std::endl;
            } else {
                std::cerr << "Failed to save page image: " << filename << std::endl;
            }
        } else {
            std::cerr << "Failed to render page " << (page_index + 1) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error extracting images from page " << page_index << ": " << e.what() << std::endl;
    }
    
    return extracted_images;
}

std::vector<PDFImageExtractor::ImageInfo> PDFImageExtractor::extract_all_images(const std::string& output_dir) {
    std::vector<ImageInfo> all_images;
    
    if (!valid_) {
        std::cerr << "PDF document is not valid" << std::endl;
        return all_images;
    }
    
    int total_pages = get_page_count();
    std::cout << "Extracting images from " << total_pages << " pages..." << std::endl;
    
    // Use parallel processing for page extraction
    const unsigned int num_threads = std::min(static_cast<unsigned int>(total_pages), 
                                               std::thread::hardware_concurrency());
    
    std::vector<std::future<std::vector<ImageInfo>>> futures;
    std::vector<int> page_indices;
    
    // Create page index list
    for (int i = 0; i < total_pages; ++i) {
        page_indices.push_back(i);
    }
    
    // Launch async tasks
    for (unsigned int t = 0; t < num_threads; ++t) {
        int start = (total_pages * t) / num_threads;
        int end = (total_pages * (t + 1)) / num_threads;
        
        futures.emplace_back(std::async(std::launch::async, [this, &output_dir, start, end]() {
            std::vector<ImageInfo> thread_images;
            for (int i = start; i < end; ++i) {
                auto page_images = extract_images_from_page(i, output_dir);
                thread_images.insert(thread_images.end(), page_images.begin(), page_images.end());
            }
            return thread_images;
        }));
    }
    
    // Collect results
    for (auto& future : futures) {
        auto thread_images = future.get();
        all_images.insert(all_images.end(), thread_images.begin(), thread_images.end());
    }
    
    std::cout << "Total images extracted: " << all_images.size() << std::endl;
    return all_images;
}