cmake_minimum_required(VERSION 3.16)
project(GenshinImpact_PaimonShutUp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

find_package(OpenCV 4.2.0 COMPONENTS core imgproc REQUIRED)
find_package(Leptonica 1.74 REQUIRED)
find_package(Tesseract 3.05 REQUIRED)

MESSAGE(STATUS "Include dirs ${OpenCV_INCLUDE_DIRS} ${Leptonica_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS}")
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${Leptonica_INCLUDE_DIRS})
include_directories(${Tesseract_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} source/main.cpp source/tesseract.cpp source/tesseract.h)

MESSAGE(STATUS "LINK LIBRARIES ${OpenCV_LIBS} ${Leptonica_LIBRARIES} ${Tesseract_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${Leptonica_LIBRARIES} libtesseract)

if (NOT EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tessdata/eng.traineddata")
    MESSAGE(STATUS "Downloading Tesseract-OCR data for ENG language")
    file(DOWNLOAD
            https://github.com/tesseract-ocr/tessdata/raw/3.04.00/eng.traineddata
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tessdata/eng.traineddata)
endif ()
