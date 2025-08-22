# C++ Comic Converter

A fast C++ command-line tool that converts PDF files into comic book formats. Extract pages as high-quality images or create CBZ archives for comic book readers.

## Features

- 🔄 **Single & Batch Processing**: Convert individual PDFs or entire directories
- 🖼️ **High-Quality Extraction**: 150 DPI PNG images with antialiasing
- 📚 **CBZ Archive Support**: Create comic book archives compatible with all readers
- 🧹 **Clean Mode**: Automatically remove temporary files after CBZ creation
- ⚡ **Fast Processing**: Built with Poppler for efficient PDF rendering
- 📋 **Progress Tracking**: Clear feedback with success/failure statistics

## Installation

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt-get install libpoppler-cpp-dev libzip-dev build-essential cmake
```

**Fedora/RHEL:**
```bash
sudo dnf install poppler-cpp-devel libzip-devel gcc-c++ cmake
```

**macOS:**
```bash
brew install poppler libzip cmake
```

### Building

```bash
git clone <repository-url>
cd cpluspluscomicconverter
cmake -B build -S .
cmake --build build
```

## Usage

### Basic Examples

```bash
# Convert single PDF to images
./build/c__omicconverter document.pdf ./output

# Convert to CBZ archive
./build/c__omicconverter document.pdf ./output --cbz

# Convert and clean up temporary files
./build/c__omicconverter document.pdf ./output --cbz --clean

# Batch process entire directory
./build/c__omicconverter /path/to/pdfs/ ./converted_comics --cbz --clean
```

### Command Line Options

```
Usage: c__omicconverter <pdf_file_or_directory> [output_directory] [--cbz] [--clean]

Options:
  --cbz     Create a CBZ (Comic Book Archive) file instead of separate images
  --clean   Remove individual image files after creating CBZ (requires --cbz)

Examples:
  c__omicconverter document.pdf ./extracted_images
  c__omicconverter /path/to/pdfs/ ./converted_comics --cbz --clean
  c__omicconverter document.pdf ./extracted_images --cbz --clean
```

## Output Formats

### Individual Images
- **Format**: PNG with transparency support
- **Resolution**: 150 DPI (high quality)
- **Naming**: `{filename}_page{N}_img1.png`
- **Organization**: Each PDF gets its own subdirectory

### CBZ Archives
- **Format**: ZIP archive with `.cbz` extension
- **Compatibility**: Works with all major comic readers
- **Page Order**: Natural sorting (page1, page2, ..., page10, page11)
- **Compression**: Optimized for file size and loading speed


## Performance

Typical performance on modern hardware:
- **Single Page**: ~100-200ms extraction time
- **20-page Comic**: ~3-5 seconds total processing
- **Batch Processing**: Scales linearly with number of files
- **Memory Usage**: Minimal (processes one page at a time)

## Architecture

The tool consists of three main components:

- **PDFImageExtractor**: Handles PDF loading and page rendering using Poppler
- **CBZCreator**: Creates ZIP archives with proper comic book formatting
- **Main Application**: Command-line interface with batch processing support

## Troubleshooting

### Common Issues

**"Failed to load PDF"**
- Check if PDF is password-protected or corrupted
- Verify file permissions

**"No PDF files found"**
- Ensure directory contains `.pdf` files (case-insensitive)
- Check directory permissions

**Build errors**
- Install missing dependencies (poppler-cpp-dev, libzip-dev)
- Update CMake to version 3.16+

### Debug Mode

For verbose output, you can examine the console logs which show:
- PDF loading status and page count
- Individual page extraction progress
- CBZ creation details
- File cleanup operations

## Future Features

- [ ] Convert from CBZ to PDF (reverse operation)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0) - see the [LICENSE](LICENSE) file for details.

## Dependencies

- **Poppler**: PDF rendering library (GPL-2.0/GPL-3.0)
- **libzip**: ZIP file creation library (BSD-3-Clause)
- **C++20**: Modern C++ standard library

## Acknowledgments

- [Poppler Project](https://poppler.freedesktop.org/) for excellent PDF rendering
- [libzip](https://libzip.org/) for reliable ZIP archive creation