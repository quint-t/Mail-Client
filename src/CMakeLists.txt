cmake_minimum_required(VERSION 3.5)

project(MailClient VERSION 1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS regex date_time system)

cmake_policy(SET CMP0074 NEW)
find_package(Mailio REQUIRED)

set(PROJECT_SOURCES
        tools.h
        imap_settings.cpp
        imap_settings.h
        smtp_settings.cpp
        smtp_settings.h
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        settings_dialog.cpp
        settings_dialog.h
        settings_dialog.ui
        send_message_dialog.cpp
        send_message_dialog.h
        send_message_dialog.ui
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MailClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET MailClient APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(MailClient SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(MailClient
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(MailClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(Boost_FOUND)
    target_include_directories(MailClient PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(MailClient PUBLIC ${Boost_LIBRARIES})
    if(Mailio_FOUND)
        target_link_libraries(MailClient PUBLIC mailio ${CMAKE_THREAD_LIBS_INIT})
    endif()
endif()

find_package(OpenSSL REQUIRED)
target_link_libraries(MailClient PRIVATE OpenSSL::SSL)

set_target_properties(MailClient PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MailClient)
endif()
