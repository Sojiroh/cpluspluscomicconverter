#include "ConversionWorker.h"

#include <QStringLiteral>

#include <system_error>
#include <vector>

namespace {
#ifdef _WIN32
std::filesystem::path MakePath(const QString& value) {
    return std::filesystem::path(value.toStdWString());
}
#else
std::filesystem::path MakePath(const QString& value) {
    return std::filesystem::path(value.toStdString());
}
#endif
}

ConversionWorker::ConversionWorker(Settings settings, QObject* parent)
    : QObject(parent), settings_(std::move(settings)) {}

void ConversionWorker::requestCancel() {
    cancelled_.store(true);
}

std::filesystem::path ConversionWorker::ToPath(const QString& value) {
    return MakePath(value);
}

void ConversionWorker::process() {
    auto input_path = ToPath(settings_.inputPath);
    auto output_path = ToPath(settings_.outputPath);

    auto logger = [this](const std::string& message) {
        emit logMessage(QString::fromStdString(message));
    };

    auto ensure_output_dir = [this](const std::filesystem::path& directory) -> bool {
        std::error_code ec;
        if (!directory.empty()) {
            std::filesystem::create_directories(directory, ec);
            if (ec) {
                emit error(QStringLiteral("Failed to create output directory: %1").arg(QString::fromStdString(ec.message())));
                return false;
            }
        }
        return true;
    };

    try {
        if (settings_.convertToPdf) {
            std::vector<std::filesystem::path> cbz_files;

            if (std::filesystem::is_directory(input_path)) {
                cbz_files = ConverterService::FindCbzFiles(input_path);
            } else if (std::filesystem::is_regular_file(input_path)) {
                if (input_path.extension() == ".cbz") {
                    cbz_files.push_back(input_path);
                }
            }

            if (cbz_files.empty()) {
                emit error(QStringLiteral("No CBZ files found to convert."));
                emit finished(0, 0, cancelled_.load());
                return;
            }

            if (!ensure_output_dir(output_path)) {
                emit finished(0, 0, cancelled_.load());
                return;
            }

            emit progressRange(static_cast<int>(cbz_files.size()));
            emit progressValue(0);

            int processed = 0;
            int successful = 0;
            int failed = 0;

            for (const auto& cbz : cbz_files) {
                if (cancelled_.load()) {
                    emit logMessage(QStringLiteral("Conversion cancelled by user."));
                    break;
                }

                bool ok = ConverterService::ConvertSingleCbz(cbz, output_path, logger);
                if (ok) {
                    ++successful;
                } else {
                    ++failed;
                }

                ++processed;
                emit progressValue(processed);
            }

            emit finished(successful, failed, cancelled_.load());
            return;
        }

        std::vector<std::filesystem::path> pdf_files;

        if (std::filesystem::is_directory(input_path)) {
            pdf_files = ConverterService::FindPdfFiles(input_path);
        } else if (std::filesystem::is_regular_file(input_path)) {
            if (input_path.extension() == ".pdf") {
                pdf_files.push_back(input_path);
            }
        }

        if (pdf_files.empty()) {
            emit error(QStringLiteral("No PDF files found to convert."));
            emit finished(0, 0, cancelled_.load());
            return;
        }

        if (!ensure_output_dir(output_path)) {
            emit finished(0, 0, cancelled_.load());
            return;
        }

        emit progressRange(static_cast<int>(pdf_files.size()));
        emit progressValue(0);

        int processed = 0;
        int successful = 0;
        int failed = 0;

        for (const auto& pdf : pdf_files) {
            if (cancelled_.load()) {
                emit logMessage(QStringLiteral("Conversion cancelled by user."));
                break;
            }

            bool ok = ConverterService::ConvertSinglePdf(pdf, output_path, settings_.pdfOptions, logger);
            if (ok) {
                ++successful;
            } else {
                ++failed;
            }

            ++processed;
            emit progressValue(processed);
        }

        emit finished(successful, failed, cancelled_.load());
    } catch (const std::exception& ex) {
        emit error(QStringLiteral("Unexpected error: %1").arg(QString::fromUtf8(ex.what())));
        emit finished(0, 0, cancelled_.load());
    }
}
