cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set( CMAKE_VERBOSE_MAKEFILE ON )
project(Sudoku)

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cinder" ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src" ABSOLUTE )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
        SOURCES     ${APP_PATH}/SudokuApp.cpp
        CINDER_PATH ${CINDER_PATH}
)
