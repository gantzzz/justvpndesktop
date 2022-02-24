cmake_minimum_required(VERSION 3.5)

project(justvpndesktop LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Threads REQUIRED)

add_executable( justvpndesktop
                main.cpp
                mainwindow.cpp
                mainwindow.h
                mainwindow.ui
                server.h
                server.cpp
                serverdatamodel.h
                serverslistitemdelegate.cpp
                serverslistitemdelegate.h
                tundev.h
                resources.qrc
                )

target_link_libraries(justvpndesktop PRIVATE Qt5::Widgets Qt5::Network Threads::Threads)
