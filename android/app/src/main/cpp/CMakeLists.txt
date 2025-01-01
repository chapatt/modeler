cmake_minimum_required(VERSION 3.22.1)

project("modeler")

find_package(game-activity REQUIRED CONFIG)
get_target_property(
        game-activity-include
        game-activity::game-activity
        INTERFACE_INCLUDE_DIRECTORIES)

find_package(Vulkan REQUIRED)

add_library(modeler_android STATIC IMPORTED)
set_target_properties(
        modeler_android
        PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../modeler_android.a)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src/)

add_library(imgui STATIC IMPORTED)
set_target_properties(
        imgui
        PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../imgui.a)

add_library(${PROJECT_NAME} SHARED
        main.c
        ${game-activity-include}/game-activity/native_app_glue/android_native_app_glue.c
        ${game-activity-include}/game-activity/GameActivity.cpp
        ${game-activity-include}/game-text-input/gametextinput.cpp)

target_link_libraries(${PROJECT_NAME}
        modeler_android
        imgui
        Vulkan::Vulkan
        game-activity::game-activity
        EGL
        GLESv3
        jnigraphics
        android
        log)


set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")