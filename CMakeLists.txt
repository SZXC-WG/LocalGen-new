cmake_minimum_required(VERSION 3.10)

project(LocalGen-new VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg Network)

set(PROJECT_SOURCES
        src/main.cpp
        src/ui/mainWindow/mainWindow.cpp
        src/ui/mainWindow/mainWindow.h
        src/ui/mainWindow/mainWindow.ui
        src/ui/localGameDialog/localGameDialog.cpp
        src/ui/localGameDialog/localGameDialog.h
        src/ui/localGameDialog/localGameDialog.ui
        src/ui/mapWidget/mapWidget.cpp
        src/ui/mapWidget/mapWidget.h
        src/ui/localGameWindow/localGameWindow.cpp
        src/ui/localGameWindow/localGameWindow.h
        src/ui/mapCreatorWindow/mapCreatorWindow.cpp
        src/ui/mapCreatorWindow/mapCreatorWindow.h
        src/bots/smartRandomBot.cpp
        src/bots/xrzBot.cpp
        src/bots/szlyBot.cpp
        src/bots/zlyBot.cpp
        src/bots/zlyBot_v2.cpp
        src/bots/zlyBot_v2.1.cpp
        src/bots/gcBot.cpp
        src/core/board.hpp
        src/core/bot.h
        src/core/game.hpp
        src/core/message.hpp
        src/core/move.hpp
        src/core/player.hpp
        src/core/tile.hpp
        src/core/utils.hpp
        res/res.qrc
)

qt_add_executable(LocalGen-new
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(LocalGen-new PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(LocalGen-new PRIVATE Qt6::Widgets Qt6::Svg Qt6::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.LocalGen-new)
endif()
set_target_properties(LocalGen-new PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LocalGen-new
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LocalGen-new)
endif()
