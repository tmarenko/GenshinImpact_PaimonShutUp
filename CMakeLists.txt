cmake_minimum_required(VERSION 3.17)
project(GenshinImpact_PaimonShutUp)

set(CMAKE_CXX_STANDARD 14)
set(DEBUG_SUFFIX "release")
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(DEBUG_SUFFIX "debug")
endif ()


set(OpenCV_DIR C:\\OpenCV\\opencv\\build)
set(Tesseract_DIR C:\\tesseract-3.05.02\\cmake-build-${DEBUG_SUFFIX})
set(Leptonica_DIR C:\\leptonica-1.74.3\\cmake-build-${DEBUG_SUFFIX})

find_package(OpenCV COMPONENTS core REQUIRED)
find_package(Leptonica 1.74 REQUIRED CONFIG)
find_package(Tesseract 3.05 REQUIRED)

MESSAGE(STATUS "Include dirs ${OpenCV_INCLUDE_DIRS} ${Leptonica_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS}")
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${Leptonica_INCLUDE_DIRS})
include_directories(${Tesseract_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} source/main.cpp source/tesseract.cpp source/tesseract.h)

MESSAGE(STATUS "LINK LIBRARIES ${OpenCV_LIBS} ${Leptonica_LIBRARIES} ${Tesseract_LIBRARIES}")
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${Leptonica_LIBRARIES} libtesseract)

if (WIN32)
    set(DEBUG_SUFFIX)
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    if (EXISTS "${OpenCV_DIR}/x64/vc15/bin/opencv_world420${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${OpenCV_DIR}/x64/vc15/bin/opencv_world420${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
    endif ()
endif ()