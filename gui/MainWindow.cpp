#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStringLiteral>
#include <QTextCursor>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("C++ Comic Converter"));
    resize(820, 600);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);

    auto* grid = new QGridLayout();

    inputPathEdit_ = new QLineEdit(this);
    auto* inputButtonsWidget = new QWidget(this);
    auto* inputButtonsLayout = new QHBoxLayout(inputButtonsWidget);
    inputButtonsLayout->setContentsMargins(0, 0, 0, 0);
    inputButtonsLayout->setSpacing(4);

    auto* inputFileButton = new QPushButton(tr("File…"), this);
    auto* inputFolderButton = new QPushButton(tr("Folder…"), this);
    inputButtonsLayout->addWidget(inputFileButton);
    inputButtonsLayout->addWidget(inputFolderButton);

    connect(inputFileButton, &QPushButton::clicked, this, &MainWindow::browseInput);
    connect(inputFolderButton, &QPushButton::clicked, this, &MainWindow::browseInputFolder);

    outputPathEdit_ = new QLineEdit(this);
    auto* outputBrowseButton = new QPushButton(tr("Browse…"), this);
    connect(outputBrowseButton, &QPushButton::clicked, this, &MainWindow::browseOutput);

    grid->addWidget(new QLabel(tr("Input"), this), 0, 0);
    grid->addWidget(inputPathEdit_, 0, 1);
    grid->addWidget(inputButtonsWidget, 0, 2);

    grid->addWidget(new QLabel(tr("Output"), this), 1, 0);
    grid->addWidget(outputPathEdit_, 1, 1);
    grid->addWidget(outputBrowseButton, 1, 2);

    formatCombo_ = new QComboBox(this);
    formatCombo_->addItems({QStringLiteral("jpeg"), QStringLiteral("png")});
    connect(formatCombo_, &QComboBox::currentTextChanged, this, &MainWindow::handleFormatChanged);

    qualitySpin_ = new QSpinBox(this);
    qualitySpin_->setRange(1, 100);
    qualitySpin_->setValue(80);

    dpiSpin_ = new QDoubleSpinBox(this);
    dpiSpin_->setRange(50.0, 600.0);
    dpiSpin_->setSingleStep(10.0);
    dpiSpin_->setValue(150.0);

    grid->addWidget(new QLabel(tr("Image format"), this), 2, 0);
    grid->addWidget(formatCombo_, 2, 1);
    grid->addWidget(new QLabel(tr("JPEG quality"), this), 3, 0);
    grid->addWidget(qualitySpin_, 3, 1);
    grid->addWidget(new QLabel(tr("DPI"), this), 4, 0);
    grid->addWidget(dpiSpin_, 4, 1);

    cbzCheck_ = new QCheckBox(tr("Create CBZ archive"), this);
    cleanCheck_ = new QCheckBox(tr("Remove images after CBZ"), this);
    cleanCheck_->setEnabled(false);
    connect(cbzCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        cleanCheck_->setEnabled(checked);
        if (!checked) {
            cleanCheck_->setChecked(false);
        }
    });

    pdfCheck_ = new QCheckBox(tr("Convert CBZ to PDF"), this);
    connect(pdfCheck_, &QCheckBox::toggled, this, &MainWindow::handlePdfToggle);

    grid->addWidget(cbzCheck_, 5, 0, 1, 2);
    grid->addWidget(cleanCheck_, 6, 0, 1, 2);
    grid->addWidget(pdfCheck_, 7, 0, 1, 2);

    mainLayout->addLayout(grid);

    auto* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton(tr("Start"), this);
    cancelButton_ = new QPushButton(tr("Cancel"), this);
    cancelButton_->setEnabled(false);
    buttonLayout->addStretch();
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(cancelButton_);

    connect(startButton_, &QPushButton::clicked, this, &MainWindow::startConversion);
    connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelConversion);

    mainLayout->addLayout(buttonLayout);

    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    mainLayout->addWidget(progressBar_);

    logView_ = new QPlainTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setMinimumHeight(280);
    mainLayout->addWidget(logView_, 1);

    setCentralWidget(centralWidget);
}

MainWindow::~MainWindow() {
    if (workerThread_) {
        workerThread_->quit();
        workerThread_->wait();
    }
}

void MainWindow::browseInput() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setOption(QFileDialog::DontUseNativeDialog, false);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setDirectory(QFileInfo(inputPathEdit_->text()).absolutePath());
    dialog.setWindowTitle(tr("Select PDF/CBZ files or a directory"));
    dialog.setNameFilters({tr("Comic files (*.pdf *.cbz)"), tr("All files (*.*)")});

    if (dialog.exec() == QDialog::Accepted) {
        const auto selected = dialog.selectedFiles();
        if (!selected.isEmpty()) {
            inputPathEdit_->setText(selected.first());
        }
    }
}

void MainWindow::browseInputFolder() {
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("Select Input Directory"),
        inputPathEdit_->text().isEmpty() ? QDir::homePath() : inputPathEdit_->text()
    );

    if (!directory.isEmpty()) {
        inputPathEdit_->setText(directory);
    }
}

void MainWindow::browseOutput() {
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"), outputPathEdit_->text());
    if (!directory.isEmpty()) {
        outputPathEdit_->setText(directory);
    }
}

void MainWindow::startConversion() {
    if (inputPathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Missing input"), tr("Select an input file or directory."));
        return;
    }

    if (outputPathEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Missing output"), tr("Select an output directory."));
        return;
    }

    if (!pdfCheck_->isChecked() && cleanCheck_->isChecked() && !cbzCheck_->isChecked()) {
        QMessageBox::warning(this, tr("Invalid options"), tr("Clean-up requires the CBZ option."));
        return;
    }

    logView_->clear();
    appendLog(tr("Starting conversion..."));
    progressBar_->setRange(0, 0);
    progressBar_->setValue(0);

    setRunningState(true);

    auto settings = gatherSettings();
    worker_ = new ConversionWorker(settings);
    workerThread_ = new QThread(this);

    worker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::started, worker_, &ConversionWorker::process);
    connect(worker_, &ConversionWorker::logMessage, this, &MainWindow::handleLogMessage);
    connect(worker_, &ConversionWorker::progressRange, this, &MainWindow::handleProgressRange);
    connect(worker_, &ConversionWorker::progressValue, this, &MainWindow::handleProgressValue);
    connect(worker_, &ConversionWorker::finished, this, &MainWindow::handleFinished);
    connect(worker_, &ConversionWorker::error, this, &MainWindow::handleError);

    connect(worker_, &ConversionWorker::finished, workerThread_, &QThread::quit);
    connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(workerThread_, &QThread::finished, workerThread_, &QObject::deleteLater);
    connect(workerThread_, &QThread::finished, this, [this]() {
        workerThread_ = nullptr;
        worker_ = nullptr;
        setRunningState(false);
    });

    workerThread_->start();
}

void MainWindow::cancelConversion() {
    if (worker_) {
        worker_->requestCancel();
    }
}

void MainWindow::handleLogMessage(const QString& message) {
    appendLog(message);
}

void MainWindow::handleProgressRange(int maximum) {
    if (maximum <= 0) {
        progressBar_->setRange(0, 0);
    } else {
        progressBar_->setRange(0, maximum);
        progressBar_->setValue(0);
    }
}

void MainWindow::handleProgressValue(int value) {
    if (progressBar_->maximum() == 0) {
        return;
    }
    progressBar_->setValue(value);
}

void MainWindow::handleFinished(int successful, int failed, bool cancelled) {
    QString summary = tr("Completed. Successful: %1, Failed: %2").arg(successful).arg(failed);
    if (cancelled) {
        summary.append(tr(" (cancelled)"));
    }
    appendLog(summary);
    progressBar_->setRange(0, 1);
    progressBar_->setValue(0);
}

void MainWindow::handleError(const QString& message) {
    appendLog(message);
    QMessageBox::warning(this, tr("Conversion error"), message);
}

void MainWindow::handlePdfToggle(bool enabled) {
    if (enabled) {
        cleanCheck_->setChecked(false);
        cbzCheck_->setChecked(false);
    }

    setRunningState(workerThread_ != nullptr);
}

void MainWindow::handleFormatChanged(const QString& format) {
    const bool running = workerThread_ != nullptr;
    const bool enableQuality = (format == QStringLiteral("jpeg")) && !pdfCheck_->isChecked() && !running;
    qualitySpin_->setEnabled(enableQuality);
}

void MainWindow::setRunningState(bool running) {
    const bool pdfMode = pdfCheck_->isChecked();
    const bool cbzMode = cbzCheck_->isChecked() && !pdfMode;

    startButton_->setEnabled(!running);
    cancelButton_->setEnabled(running);

    inputPathEdit_->setEnabled(!running);
    outputPathEdit_->setEnabled(!running);

    const bool formatEnabled = !running && !pdfMode;
    const bool qualityEnabled = !running && !pdfMode && formatCombo_->currentText() == QStringLiteral("jpeg");
    const bool dpiEnabled = !running && !pdfMode;
    const bool cleanEnabled = !running && cbzMode;

    formatCombo_->setEnabled(formatEnabled);
    qualitySpin_->setEnabled(qualityEnabled);
    dpiSpin_->setEnabled(dpiEnabled);
    cbzCheck_->setEnabled(!running && !pdfMode);
    cleanCheck_->setEnabled(cleanEnabled);
    pdfCheck_->setEnabled(!running);
}

ConversionWorker::Settings MainWindow::gatherSettings() const {
    ConversionWorker::Settings settings;
    settings.inputPath = inputPathEdit_->text();
    settings.outputPath = outputPathEdit_->text();
    settings.convertToPdf = pdfCheck_->isChecked();

    PdfConversionOptions options;
    options.format = formatCombo_->currentText().toStdString();
    options.quality = qualitySpin_->value();
    options.dpi = dpiSpin_->value();
    options.create_cbz = cbzCheck_->isChecked();
    options.clean_images = cleanCheck_->isChecked();
    settings.pdfOptions = options;

    return settings;
}

void MainWindow::appendLog(const QString& message) {
    logView_->appendPlainText(message);
    QTextCursor cursor = logView_->textCursor();
    cursor.movePosition(QTextCursor::End);
    logView_->setTextCursor(cursor);
}
