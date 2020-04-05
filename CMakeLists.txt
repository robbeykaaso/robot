cmake_minimum_required(VERSION 3.1)

project(Robot LANGUAGES CXX)

add_definitions( -DRX_USE_EXCEPTIONS=OFF )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-bigobj)
#ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

#set(deepinspectapptemplate_ROOT)
#get_filename_component(deepinspectapptemplate_ROOT ${PROJECT_SOURCE_DIR} DIRECTORY)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

#message("include: ${PROJECT_SOURCE_DIR}")
find_package(Qt5 COMPONENTS Widgets Core Quick REQUIRED)
find_package(OpenCV REQUIRED)
#find_package(AWSSDK REQUIRED COMPONENTS core s3)
set(STMMAN_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)
find_package(STMMAN)
#find_package(PythonLibs REQUIRED)

add_executable(${PROJECT_NAME} "main.cpp" #WIN32: to disable the console window
    "qml.qrc"
    "application/brain.h"
    "application/brain.cpp"
    "application/eye.cpp"
    "application/hand.cpp"
    "application/decimal_infer.h"
    "application/decimal_infer.cpp"

    "application/hearthStone.cpp"
    "resource/logo.rc")
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
#message(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Quick Qt5::Widgets ${OpenCV_LIBS} ${STMMAN_LIBS})# ${AWSSDK_LINK_LIBRARIES} # ${PYTHON_LIBRARIES})#  ${DeepInspectSDK_LIBS} ${gflags_LIBRARIES} glog::glog) # "${deepinspectapptemplate_ROOT}/deepTemplateExe/deepinspectControl.lib")
#include_directories("${deepinspectapptemplate_ROOT}/deepinspectControl")
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${STMMAN_INCLUDE_DIRS})
include_directories("F:/tinnydnn/tiny-dnn")
#include_directories(${RapidJSON_INCLUDE_DIRS})
#include_directories(${DeepInspectSDK_INCLUDE_DIRS})
#include_directories(${RxCpp_INCLUDE_DIRS})
#include_directories (${gflags_INCLUDE_DIR})
#include_directories(${AWSSDK_INCLUDE_DIR})
#include_directories(${PYTHON_INCLUDE_DIRS})
#include_directories("D:/catch/Catch2/single_include/catch2")

#message("include ${RapidJSON_INCLUDE_DIRS}")
#message("include: ${RapidJSON_INCLUDE_DIRS}")
#message("library: ${STMMAN_LIBS}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/resource"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "copying resource.")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/install/${CMAKE_BUILD_TYPE}/bin"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "copying framework.")
