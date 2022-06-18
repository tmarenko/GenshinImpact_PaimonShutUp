cmake_minimum_required(VERSION 3.21)
project(GenshinImpact_PaimonShutUp)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(NOT TESSERACT_ENG_DATA)
    set(TESSERACT_ENG_DATA "https://github.com/tesseract-ocr/tessdata/raw/3.04.00/eng.traineddata")
endif()

find_package(OpenCV 4.2.0 COMPONENTS core imgproc REQUIRED)
find_package(Leptonica 1.74 REQUIRED)
find_package(Tesseract 3.05 REQUIRED)

MESSAGE(STATUS "Include dirs ${OpenCV_INCLUDE_DIRS} ${Leptonica_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS}")
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${Leptonica_INCLUDE_DIRS})
include_directories(${Tesseract_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} source/main.cpp source/tesseract.cpp source/tesseract.h)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'")

MESSAGE(STATUS "LINK LIBRARIES ${OpenCV_LIBS} ${Leptonica_LIBRARIES} ${Tesseract_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${Leptonica_LIBRARIES} libtesseract)

if (NOT EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tessdata/eng.traineddata")
    MESSAGE(STATUS "Downloading Tesseract-OCR data for ENG language from: ${TESSERACT_ENG_DATA}")
    file(DOWNLOAD
            ${TESSERACT_ENG_DATA}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tessdata/eng.traineddata)
endif ()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS)
