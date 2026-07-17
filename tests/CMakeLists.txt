# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Qt6 6.7 REQUIRED COMPONENTS Test)

add_executable(
    atlasveil_controller_tests
    fakes/FakeVpnBackend.hpp
    unit/tst_vpncontroller.cpp
    "${PROJECT_SOURCE_DIR}/src/core/IVpnBackend.hpp"
    "${PROJECT_SOURCE_DIR}/src/core/VpnController.cpp"
    "${PROJECT_SOURCE_DIR}/src/core/VpnController.hpp"
    "${PROJECT_SOURCE_DIR}/src/core/VpnProfile.hpp"
    "${PROJECT_SOURCE_DIR}/src/core/VpnState.hpp"
)

target_include_directories(
    atlasveil_controller_tests
    PRIVATE
        "${PROJECT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(
    atlasveil_controller_tests
    PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME controller-unit-tests COMMAND atlasveil_controller_tests)
