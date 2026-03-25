cmake_minimum_required(VERSION 3.19)

project(LocalGen-new VERSION 6.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    unset(WrapOpenGL_AGL CACHE)
    set(WrapOpenGL_AGL "-framework OpenGL")
endif()

find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets Svg Network)

set(LOCALGEN_BOT_SOURCES
    src/bots/smartRandomBot.cpp
    src/bots/xiaruizeBot.cpp
    src/bots/szlyBot.cpp
    src/bots/zlyBot.cpp
    src/bots/zlyBot_v2.cpp
    src/bots/zlyBot_v2.1.cpp
    src/bots/gcBot.cpp
    src/bots/oimBot.cpp
    src/bots/KutuBot.cpp
    src/bots/LyBot.cpp
)

set(LOCALGEN_WINDOWS_RESOURCES)

if(WIN32)
    list(APPEND LOCALGEN_WINDOWS_RESOURCES
        res/localgen.rc
    )
endif()

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE)

target_sources(${PROJECT_NAME} PRIVATE
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
    ${LOCALGEN_BOT_SOURCES}
    src/core/board.hpp
    src/core/bot.h
    src/core/game.hpp
    src/core/map.hpp
    src/core/message.hpp
    src/core/move.hpp
    src/core/player.hpp
    src/core/tile.hpp
    src/core/utils.hpp
    res/res.qrc
    ${LOCALGEN_WINDOWS_RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Svg Qt6::Network)

add_executable(LocalGen-bot-simulator
    simulator/botSimulator.cpp
    ${LOCALGEN_BOT_SOURCES}
    src/core/board.hpp
    src/core/bot.h
    src/core/game.hpp
    src/core/message.hpp
    src/core/move.hpp
    src/core/player.hpp
    src/core/tile.hpp
    src/core/utils.hpp
)

target_include_directories(LocalGen-bot-simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/maps
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/maps
    COMMENT "Copying maps to output directory"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.${PROJECT_NAME}"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
