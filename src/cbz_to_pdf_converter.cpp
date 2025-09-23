#include "cbz_to_pdf_converter.h"
#include "pdf_creator.h"

#include <zip.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <regex>
#include <utility>
#include <vector>

namespace {
struct ImageEntry {
    std::string name;
    std::vector<std::uint8_t> data;
    int width = 0;
    int height = 0;
    int components = 0;
};

bool has_jpeg_extension(const std::string& file_name) {
    const auto extension = std::filesystem::path(file_name).extension().string();
    std::string lower;
    lower.resize(extension.size());
    std::transform(extension.begin(), extension.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower == ".jpg" || lower == ".jpeg";
}

bool parse_jpeg_dimensions(const std::vector<std::uint8_t>& data, int& width, int& height, int& components) {
    if (data.size() < 4) {
        return false;
    }

    if (!(data[0] == 0xFF && data[1] == 0xD8)) {
        return false;
    }

    std::size_t index = 2;
    while (index + 1 < data.size()) {
        if (data[index] != 0xFF) {
            ++index;
            continue;
        }

        const std::uint8_t marker = data[index + 1];
        index += 2;

        if (marker == 0xD8 || marker == 0xD9) {
            continue;
        }

        if (marker == 0xDA) {
            break;
        }

        if (index + 1 >= data.size()) {
            return false;
        }

        const std::uint16_t segment_length = static_cast<std::uint16_t>(data[index] << 8 | data[index + 1]);
        if (segment_length < 2 || index + segment_length > data.size()) {
            return false;
        }

        const bool is_sof = (marker >= 0xC0 && marker <= 0xC3) ||
                            (marker >= 0xC5 && marker <= 0xC7) ||
                            (marker >= 0xC9 && marker <= 0xCB) ||
                            (marker >= 0xCD && marker <= 0xCF);
        if (is_sof) {
            if (segment_length < 8) {
                return false;
            }
            const std::size_t precision_index = index + 2;
            const std::size_t height_index = precision_index + 1;
            const std::size_t width_index = height_index + 2;
            height = static_cast<int>(data[height_index] << 8 | data[height_index + 1]);
            width = static_cast<int>(data[width_index] << 8 | data[width_index + 1]);
            components = static_cast<int>(data[width_index + 2]);
            return width > 0 && height > 0;
        }

        index += segment_length;
    }

    return false;
}

int extract_page_hint(const std::string& name) {
    static const std::regex number_regex(R"((\d+))");
    std::smatch match;
    if (std::regex_search(name, match, number_regex)) {
        try {
            return std::stoi(match[1].str());
        } catch (const std::exception&) {
            return std::numeric_limits<int>::max();
        }
    }
    return std::numeric_limits<int>::max();
}

void sort_images(std::vector<ImageEntry>& entries) {
    std::sort(entries.begin(), entries.end(), [](const ImageEntry& lhs, const ImageEntry& rhs) {
        const auto lhs_name = std::filesystem::path(lhs.name).stem().string();
        const auto rhs_name = std::filesystem::path(rhs.name).stem().string();
        const int lhs_number = extract_page_hint(lhs_name);
        const int rhs_number = extract_page_hint(rhs_name);
        if (lhs_number != rhs_number) {
            return lhs_number < rhs_number;
        }
        return lhs_name < rhs_name;
    });
}
}

bool CBZToPDFConverter::convert_cbz_to_pdf(const std::string& cbz_path,
                                           const std::string& output_pdf_path) {
    int zip_error = 0;
    zip_t* archive = zip_open(cbz_path.c_str(), ZIP_RDONLY, &zip_error);
    if (!archive) {
        zip_error_t error;
        zip_error_init_with_code(&error, zip_error);
        std::cerr << "Failed to open CBZ: " << cbz_path << ". Reason: " << zip_error_strerror(&error) << std::endl;
        zip_error_fini(&error);
        return false;
    }

    std::vector<ImageEntry> images;
    const zip_int64_t entry_count = zip_get_num_entries(archive, ZIP_FL_UNCHANGED);
    for (zip_int64_t i = 0; i < entry_count; ++i) {
        zip_stat_t stat;
        if (zip_stat_index(archive, i, ZIP_FL_ENC_GUESS, &stat) != 0) {
            std::cerr << "Warning: Failed to stat entry index " << i << std::endl;
            continue;
        }

        if (stat.name == nullptr) {
            continue;
        }

        const std::string entry_name(stat.name);
        if (!entry_name.empty() && entry_name.back() == '/') {
            continue; // skip directories
        }

        if (!has_jpeg_extension(entry_name)) {
            continue; // unsupported format for now
        }

        zip_file_t* file = zip_fopen_index(archive, i, ZIP_FL_UNCHANGED);
        if (!file) {
            std::cerr << "Warning: Failed to open entry: " << entry_name << std::endl;
            continue;
        }

        std::vector<std::uint8_t> buffer;
        buffer.resize(static_cast<std::size_t>(stat.size));
        const zip_int64_t bytes_read = zip_fread(file, buffer.data(), buffer.size());
        zip_fclose(file);

        if (bytes_read != static_cast<zip_int64_t>(buffer.size())) {
            std::cerr << "Warning: Failed to read entire entry: " << entry_name << std::endl;
            continue;
        }

        int width = 0;
        int height = 0;
        int components = 0;
        if (!parse_jpeg_dimensions(buffer, width, height, components)) {
            std::cerr << "Warning: Unable to read JPEG dimensions for: " << entry_name << std::endl;
            continue;
        }

        ImageEntry entry;
        entry.name = entry_name;
        entry.width = width;
        entry.height = height;
        entry.components = components;
        entry.data = std::move(buffer);
        images.push_back(std::move(entry));
    }

    zip_close(archive);

    if (images.empty()) {
        std::cerr << "No supported images found inside CBZ: " << cbz_path << std::endl;
        return false;
    }

    sort_images(images);

    std::vector<PDFImageInput> pdf_images;
    pdf_images.reserve(images.size());
    for (auto& entry : images) {
        pdf_images.push_back(PDFImageInput{
            entry.name,
            entry.width,
            entry.height,
            entry.components,
            std::move(entry.data)
        });
    }

    if (!PDFCreator::create_pdf_from_images(pdf_images, output_pdf_path)) {
        std::cerr << "Failed to create PDF for CBZ: " << cbz_path << std::endl;
        return false;
    }

    std::cout << "Created PDF: " << output_pdf_path << std::endl;
    return true;
}
