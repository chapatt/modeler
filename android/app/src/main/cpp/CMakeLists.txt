cmake_minimum_required(VERSION 3.22.1)

project("modeler")

find_package(game-activity REQUIRED CONFIG)

get_target_property(game-activity-include
        game-activity::game-activity
        INTERFACE_INCLUDE_DIRECTORIES)

add_library(${PROJECT_NAME} SHARED
        main.c
        ${game-activity-include}/game-activity/native_app_glue/android_native_app_glue.c
        ${game-activity-include}/game-activity/GameActivity.cpp
        ${game-activity-include}/game-text-input/gametextinput.cpp)

target_link_libraries(modeler
        game-activity::game-activity
        EGL
        GLESv3
        jnigraphics
        android
        log)


set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")