cmake_minimum_required(VERSION 3.1)

project(RollerCoasters)
set(CMAKE_FIND_ROOT_PATH /c/MinGW/)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAEK_EXE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
set(CMAKE_CREATE_WIN32_EXE  "/subsystem:windowsce -mwindows")

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src/)
add_definitions(-DPROJECT_DIR="${PROJECT_SOURCE_DIR}")

add_executable(RollerCoasters
    ${SRC_DIR}main.cpp
    ${SRC_DIR}CallBacks.h
    ${SRC_DIR}CallBacks.cpp
    ${SRC_DIR}ControlPoint.h
    ${SRC_DIR}ControlPoint.cpp
    ${SRC_DIR}Object.h
    ${SRC_DIR}Track.h
    ${SRC_DIR}Track.cpp
    ${SRC_DIR}TrainView.h
    ${SRC_DIR}TrainView.cpp
    ${SRC_DIR}TrainWindow.h
    ${SRC_DIR}TrainWindow.cpp)

add_library(Utilities 
    ${SRC_DIR}Utilities/3DUtils.h
    ${SRC_DIR}Utilities/3DUtils.cpp
    ${SRC_DIR}Utilities/ArcBallCam.h
    ${SRC_DIR}Utilities/ArcBallCam.cpp
    ${SRC_DIR}Utilities/Pnt3f.h
    ${SRC_DIR}Utilities/Pnt3f.cpp)

target_link_libraries(RollerCoasters Utilities)
target_link_libraries(RollerCoasters fltk fltk_forms fltk_images fltk_jpeg fltk_png fltk_gl crypt32 comctl32)
target_link_libraries(RollerCoasters opengl32 glew32 freeglut glu32)
