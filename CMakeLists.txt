cmake_minimum_required(VERSION 3.24)

project(
    AtlasVeil
    VERSION 0.1.0
    DESCRIPTION "A minimal WireGuard profile controller for Linux"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH "${ECM_MODULE_PATH}")

include(ECMFindQmlModule)
include(GNUInstallDirs)

find_package(
    Qt6 6.7 REQUIRED
    COMPONENTS
        Core
        DBus
        Gui
        Qml
        Quick
        QuickControls2
)

find_package(
    KF6 6.0 REQUIRED
    COMPONENTS
        Kirigami
        NetworkManagerQt
        QQC2DesktopStyle
)

include(QtVersionOption)
ecm_find_qmlmodule(org.kde.kirigami REQUIRED)
qt_standard_project_setup(REQUIRES 6.7)

qt_add_executable(atlasveil
    src/main.cpp
    src/core/IVpnBackend.hpp
    src/core/VpnController.cpp
    src/core/VpnController.hpp
    src/core/VpnProfile.hpp
    src/core/VpnState.hpp
    src/network/NetworkManagerAdapter.cpp
    src/network/NetworkManagerAdapter.hpp
)

set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_qml_module(atlasveil
    URI AtlasVeil
    VERSION 1.0
    NO_IMPORT_SCAN
    QML_FILES
        qml/Main.qml
)

target_include_directories(
    atlasveil
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(
    atlasveil
    PRIVATE
        Qt6::Core
        Qt6::DBus
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        KF6::NetworkManagerQt
)

install(
    TARGETS atlasveil
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    FILES data/io.github.rotciv004.atlasveil.desktop
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
