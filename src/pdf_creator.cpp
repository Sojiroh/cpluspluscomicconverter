#include "pdf_creator.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <cstdio>

namespace {
struct PdfObjectOffset {
    long long value = 0;
};

void write_newline(std::ofstream& stream) {
    stream << '\n';
}
}

bool PDFCreator::create_pdf_from_images(const std::vector<PDFImageInput>& images,
                                        const std::string& output_pdf_path) {
    if (images.empty()) {
        std::cerr << "No images provided for PDF creation" << std::endl;
        return false;
    }

    const auto parent_dir = std::filesystem::path(output_pdf_path).parent_path();
    if (!parent_dir.empty()) {
        std::filesystem::create_directories(parent_dir);
    }
    std::ofstream output(output_pdf_path, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to open output PDF: " << output_pdf_path << std::endl;
        return false;
    }

    const int page_count = static_cast<int>(images.size());
    const int total_objects = 2 + page_count * 3;
    std::vector<PdfObjectOffset> offsets(total_objects + 1);

    auto record_offset = [&](int object_id) {
        offsets[object_id].value = static_cast<long long>(output.tellp());
    };

    output << "%PDF-1.4\n";

    // Object 1: Catalog
    record_offset(1);
    output << "1 0 obj\n";
    output << "<< /Type /Catalog /Pages 2 0 R >>\n";
    output << "endobj\n";

    // Object 2: Pages
    record_offset(2);
    output << "2 0 obj\n";
    output << "<< /Type /Pages /Count " << page_count << " /Kids [";
    for (int i = 0; i < page_count; ++i) {
        const int page_object_id = 3 + i * 3;
        output << ' ' << page_object_id << " 0 R";
    }
    output << " ] >>\n";
    output << "endobj\n";

    for (int page_index = 0; page_index < page_count; ++page_index) {
        const auto& image = images[page_index];
        const int page_object_id = 3 + page_index * 3;
        const int image_object_id = page_object_id + 1;
        const int content_object_id = page_object_id + 2;
        const std::string image_resource_name = "Im" + std::to_string(page_index + 1);

        // Page object
        record_offset(page_object_id);
        output << page_object_id << " 0 obj\n";
        output << "<< /Type /Page /Parent 2 0 R ";
        output << "/MediaBox [0 0 " << image.width << ' ' << image.height << "] ";
        output << "/Resources << /XObject << /" << image_resource_name << ' ' << image_object_id << " 0 R >> >> ";
        output << "/Contents " << content_object_id << " 0 R >>\n";
        output << "endobj\n";

        // Image object
        record_offset(image_object_id);
        output << image_object_id << " 0 obj\n";
        output << "<< /Type /XObject /Subtype /Image ";
        output << "/Width " << image.width << ' ';
        output << "/Height " << image.height << ' ';
        if (image.components == 1) {
            output << "/ColorSpace /DeviceGray ";
        } else if (image.components == 4) {
            output << "/ColorSpace /DeviceCMYK ";
        } else {
            output << "/ColorSpace /DeviceRGB ";
        }
        output << "/BitsPerComponent 8 ";
        output << "/Filter /DCTDecode ";
        output << "/Length " << image.data.size() << " >>\n";
        output << "stream\n";
        output.write(reinterpret_cast<const char*>(image.data.data()), static_cast<std::streamsize>(image.data.size()));
        write_newline(output);
        output << "endstream\n";
        output << "endobj\n";

        // Content stream
        const std::string content_stream = "q " + std::to_string(image.width) + " 0 0 " +
                                           std::to_string(image.height) + " 0 0 cm /" +
                                           image_resource_name + " Do Q\n";
        record_offset(content_object_id);
        output << content_object_id << " 0 obj\n";
        output << "<< /Length " << content_stream.size() << " >>\n";
        output << "stream\n";
        output << content_stream;
        output << "endstream\n";
        output << "endobj\n";
    }

    const long long xref_offset = static_cast<long long>(output.tellp());
    output << "xref\n";
    output << "0 " << (total_objects + 1) << "\n";
    output << "0000000000 65535 f \n";
    for (int i = 1; i <= total_objects; ++i) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%010lld 00000 n \n", offsets[i].value);
        output << buffer;
    }

    output << "trailer\n";
    output << "<< /Size " << (total_objects + 1) << " /Root 1 0 R >>\n";
    output << "startxref\n" << xref_offset << "\n";
    output << "%%EOF";

    if (!output) {
        std::cerr << "Failed while writing PDF: " << output_pdf_path << std::endl;
        return false;
    }

    output.close();
    return true;
}
