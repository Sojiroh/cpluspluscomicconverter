#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <filesystem>

#include "converter_service.h"

class ConversionWorker : public QObject {
    Q_OBJECT

public:
    struct Settings {
        QString inputPath;
        QString outputPath;
        PdfConversionOptions pdfOptions;
        bool convertToPdf = false;
    };

    explicit ConversionWorker(Settings settings, QObject* parent = nullptr);

    void requestCancel();

public slots:
    void process();

signals:
    void logMessage(const QString& message);
    void progressRange(int maximum);
    void progressValue(int value);
    void finished(int successful, int failed, bool cancelled);
    void error(const QString& message);

private:
    Settings settings_;
    std::atomic_bool cancelled_{false};

    static std::filesystem::path ToPath(const QString& value);
};
