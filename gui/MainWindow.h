#pragma once

#include <QMainWindow>

#include "ConversionWorker.h"

class QLineEdit;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;
class QPlainTextEdit;
class QProgressBar;
class QThread;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void browseInput();
    void browseInputFolder();
    void browseOutput();
    void startConversion();
    void cancelConversion();
    void handleLogMessage(const QString& message);
    void handleProgressRange(int maximum);
    void handleProgressValue(int value);
    void handleFinished(int successful, int failed, bool cancelled);
    void handleError(const QString& message);
    void handlePdfToggle(bool enabled);
    void handleFormatChanged(const QString& format);

private:
    void setRunningState(bool running);
    ConversionWorker::Settings gatherSettings() const;
    void appendLog(const QString& message);

    QLineEdit* inputPathEdit_ = nullptr;
    QLineEdit* outputPathEdit_ = nullptr;
    QComboBox* formatCombo_ = nullptr;
    QSpinBox* qualitySpin_ = nullptr;
    QDoubleSpinBox* dpiSpin_ = nullptr;
    QCheckBox* cbzCheck_ = nullptr;
    QCheckBox* cleanCheck_ = nullptr;
    QCheckBox* pdfCheck_ = nullptr;
    QPushButton* startButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QPlainTextEdit* logView_ = nullptr;
    QProgressBar* progressBar_ = nullptr;

    QThread* workerThread_ = nullptr;
    ConversionWorker* worker_ = nullptr;
};
