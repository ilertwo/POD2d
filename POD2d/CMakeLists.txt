cmake_minimum_required(VERSION 3.16)

project(POD2d VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(CORE_FILES
    core/actionmanager.h
    core/actionmanager.cpp
    core/filecontroller.h
    core/filecontroller.cpp
    core/settingsmanager.h
    core/settingsmanager.cpp
    core/thememanager.h
    core/thememanager.cpp
    core/logger.h
    core/logger.cpp
)

set(MODELS_FILES
    models/projectmodel.h
    models/projectmodel.cpp
    models/codegenerator.h
    models/codegenerator.cpp
    models/painttools.h
    models/painttools.cpp
    models/gif.h
)

set(WIDGETS_FILES
    widgets/pixelcanvas.h
    widgets/pixelcanvas.cpp
    widgets/palettewidget.h
    widgets/palettewidget.cpp
    widgets/layerlistwidget.h
    widgets/layerlistwidget.cpp
    widgets/framelistwidget.h
    widgets/framelistwidget.cpp
    widgets/flowlayout.h
    widgets/flowlayout.cpp
)

set(DIALOGS_FILES
    dialogs/createprojectdialog.h
    dialogs/createprojectdialog.cpp
    dialogs/createprojectdialog.ui
    dialogs/exportdialog.h
    dialogs/exportdialog.cpp
    dialogs/exportdialog.ui
    dialogs/settingsdialog.h
    dialogs/settingsdialog.cpp
    dialogs/settingsdialog.ui
    dialogs/palettedialog.h
    dialogs/palettedialog.cpp
    dialogs/palettedialog.ui
    dialogs/textdialog.h
    dialogs/textdialog.cpp
    dialogs/textdialog.ui
)

set(MAIN_FILES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
)

set(PROJECT_SOURCES
    ${MAIN_FILES}
    ${CORE_FILES}
    ${MODELS_FILES}
    ${WIDGETS_FILES}
    ${DIALOGS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(POD2d
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(POD2d SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(POD2d
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(POD2d PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(POD2d PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.POD2d)
endif()

set_target_properties(POD2d PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}
)

include(GNUInstallDirs)
install(TARGETS POD2d
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(POD2d)
endif()
