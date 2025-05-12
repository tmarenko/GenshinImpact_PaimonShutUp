cmake_minimum_required(VERSION 3.17)
project(GenshinImpact_PaimonShutUp)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /O2 /GL")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
set(MAIN_MANIFEST "main.manifest")
set(CONFIG_FILE "settings.cfg")
if(NOT TESSERACT_LANGUAGE)
    MESSAGE(STATUS "TESSERACT_LANGUAGE is empty. Using 'eng' as default")
    set(TESSERACT_LANGUAGE "eng")
endif()

set(Leptonica_STATIC ON)
set(Tesseract_STATIC ON)

find_package(Leptonica 1.74 REQUIRED)
find_package(Tesseract 3.05 REQUIRED)

MESSAGE(STATUS "Include dirs ${Leptonica_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS}")  
include_directories(${Leptonica_INCLUDE_DIRS})
include_directories(${Tesseract_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} 
    source/main.cpp 
    source/tesseract.cpp 
    source/tesseract.h 
    source/image.cpp 
    source/image.h
    source/config.cpp 
    source/config.h
    appicon.rc)
set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")

MESSAGE(STATUS "LINK LIBRARIES ${Leptonica_LIBRARIES} ${Tesseract_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} ${Leptonica_LIBRARIES} libtesseract)

if (NOT EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tessdata/${TESSERACT_LANGUAGE}.traineddata")
    set(TESSERACT_LANG_DATA "https://github.com/tesseract-ocr/tessdata/raw/3.04.00/${TESSERACT_LANGUAGE}.traineddata")
    MESSAGE(STATUS "Downloading Tesseract-OCR data for '${TESSERACT_LANGUAGE}' language from: ${TESSERACT_LANG_DATA}")
    file(DOWNLOAD
            ${TESSERACT_LANG_DATA}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tessdata/${TESSERACT_LANGUAGE}.traineddata)
endif ()

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/${MAIN_MANIFEST}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MAIN_MANIFEST})

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/${CONFIG_FILE}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG_FILE})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "mt.exe" -manifest "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\${MAIN_MANIFEST}" -outputresource:"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\${PROJECT_NAME}.exe"
        COMMAND ${CMAKE_COMMAND} -E remove -f "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\${MAIN_MANIFEST}"
        COMMENT "Embedding manifest..."
        COMMAND_EXPAND_LISTS)
