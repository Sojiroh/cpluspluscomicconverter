cmake_minimum_required(VERSION 3.16)
project(cpluspluscomicconverter)

set(CMAKE_CXX_STANDARD 20)

option(ENABLE_GUI "Build the Qt-based desktop application" ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED IMPORTED_TARGET poppler-cpp)
pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)

if (ENABLE_GUI)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if (Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
        set(QT_PACKAGE Qt6)
    else()
        find_package(Qt5 COMPONENTS Widgets QUIET)
        if (Qt5_FOUND)
            set(QT_VERSION_MAJOR 5)
            set(QT_PACKAGE Qt5)
        else()
            message(WARNING "Qt5/Qt6 not found. Set Qt6_DIR or Qt5_DIR to enable the GUI target.")
            set(ENABLE_GUI OFF)
        endif()
    endif()
endif()

add_library(converter_core
    src/pdf_image_extractor.cpp
    src/cbz_creator.cpp
    src/pdf_creator.cpp
    src/cbz_to_pdf_converter.cpp
    src/converter_service.cpp
)

target_include_directories(converter_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(converter_core PUBLIC PkgConfig::POPPLER PkgConfig::LIBZIP)

add_executable(cpluspluscomicconverter src/main.cpp)
target_link_libraries(cpluspluscomicconverter PRIVATE converter_core)

if (ENABLE_GUI)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    add_executable(cpluspluscomicconverter_gui
        gui/main.cpp
        gui/MainWindow.cpp
        gui/ConversionWorker.cpp
    )

    target_link_libraries(cpluspluscomicconverter_gui
        PRIVATE
            converter_core
            ${QT_PACKAGE}::Widgets
    )
endif()
