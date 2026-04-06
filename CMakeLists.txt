cmake_minimum_required(VERSION 3.20)
project(Metroidvania LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SFML)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install    OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# --- PhysX (pre-built) ---
set(PHYSX_ROOT "${CMAKE_SOURCE_DIR}/third_party/PhysX-src/physx")
set(PHYSX_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/PhysX-install/bin/win.x86_64.vc143.md/debug")

add_executable(Metroidvania
    src/main.cpp
    src/Core/GameObject.cpp
    src/Components/MovementComponent.cpp
    src/Components/RenderComponent.cpp
    src/Components/InputComponent.cpp
    src/Components/AnimationComponent.cpp
    src/Components/PhysicsComponent.cpp
    src/Components/HealthComponent.cpp
    src/Components/EnemyAIComponent.cpp
    src/Map/Map.cpp
    src/Map/MapLoader.cpp
    src/Debug/DebugMenu.cpp
    src/Physics/PhysXWorld.cpp
)

target_include_directories(Metroidvania PRIVATE
    ${PHYSX_ROOT}/include
)

target_link_libraries(Metroidvania
    PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        nlohmann_json::nlohmann_json
        $<$<PLATFORM_ID:Windows>:comdlg32>
        ${PHYSX_LIB_DIR}/PhysX.lib
        ${PHYSX_LIB_DIR}/PhysXFoundation.lib
        ${PHYSX_LIB_DIR}/PhysXCommon.lib
        ${PHYSX_LIB_DIR}/PhysXCooking.lib
        ${PHYSX_LIB_DIR}/PhysXExtensions_static.lib
        ${PHYSX_LIB_DIR}/PhysXPvdSDK_static.lib
)

# Copy SFML and PhysX DLLs next to the executable on Windows
if(WIN32)
    add_custom_command(TARGET Metroidvania POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:Metroidvania>
            $<TARGET_FILE_DIR:Metroidvania>
        COMMAND_EXPAND_LISTS
    )
    add_custom_command(TARGET Metroidvania POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PHYSX_LIB_DIR}/PhysX.dll"
            "${PHYSX_LIB_DIR}/PhysXFoundation.dll"
            "${PHYSX_LIB_DIR}/PhysXCommon.dll"
            "${PHYSX_LIB_DIR}/PhysXCooking.dll"
            $<TARGET_FILE_DIR:Metroidvania>
    )
endif()

# Copy maps folder next to the executable so relative paths work at runtime
add_custom_command(TARGET Metroidvania POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/maps
        $<TARGET_FILE_DIR:Metroidvania>/maps
)

# Copy assets folder next to the executable so sprite sheets are found at runtime
add_custom_command(TARGET Metroidvania POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Metroidvania>/assets
)
