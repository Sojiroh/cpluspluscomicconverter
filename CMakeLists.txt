cmake_minimum_required(VERSION 3.16)
project(cpluspluscomicconverter)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

add_executable(cpluspluscomicconverter src/main.cpp src/pdf_image_extractor.cpp src/cbz_creator.cpp)

target_link_libraries(cpluspluscomicconverter ${POPPLER_LIBRARIES} ${LIBZIP_LIBRARIES})
target_include_directories(cpluspluscomicconverter PRIVATE ${POPPLER_INCLUDE_DIRS} ${LIBZIP_INCLUDE_DIRS})
target_compile_options(cpluspluscomicconverter PRIVATE ${POPPLER_CFLAGS_OTHER} ${LIBZIP_CFLAGS_OTHER})
