cmake_minimum_required(VERSION 3.22)

project(w)

set(CMAKE_C_STANDARD 99)

set(SOURCES
  src/glad.c
  src/window.c)

add_library(w
  STATIC
  ${SOURCES})

if(WANDROID)
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate -u ANativeWindow_setFrameRateWithChangeStrategy")

  find_library(log-lib log)

  add_library (native_app_glue
    STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

  target_include_directories (native_app_glue
    PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

  target_link_libraries(w
    PRIVATE
    android
    ${log-lib}
    native_app_glue
    GLESv3)

  target_compile_definitions(w
    PRIVATE
    WANDROID)
endif()

target_compile_options(w
  PRIVATE
  -Wall
  -Werror
  -Wextra
  -pedantic
  -pedantic-errors)

target_include_directories(w
  PUBLIC
  include/)

target_link_libraries(w
  PRIVATE
  EGL)