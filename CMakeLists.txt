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

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

# --- GLFW ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glfw)

# --- FreeType ---
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(freetype)

# --- PhysX (pre-built) ---
set(PHYSX_ROOT "${CMAKE_SOURCE_DIR}/third_party/PhysX-src/physx")
set(PHYSX_LIB_DIR "${CMAKE_SOURCE_DIR}/third_party/PhysX-install/bin/win.x86_64.vc143.md/debug")

add_executable(Metroidvania
    src/main.cpp
    src/Core/GameObject.cpp
    src/Core/SaveSystem.cpp
    src/Core/InputBindings.cpp
    src/Components/MovementComponent.cpp
    src/Components/RenderComponent.cpp
    src/Components/InputComponent.cpp
    src/Components/AnimationComponent.cpp
    src/Components/PhysicsComponent.cpp
    src/Components/HealthComponent.cpp
    src/Components/EnemyAIComponent.cpp
    src/Components/CombatComponent.cpp
    src/Components/SlimeAttackComponent.cpp
    src/Map/Map.cpp
    src/Map/MapLoader.cpp
    src/Map/TransitionManager.cpp
    src/Debug/DebugMenu.cpp
    src/Physics/PhysXWorld.cpp
    src/UI/SaveSlotScreen.cpp
    src/UI/PauseMenu.cpp
    src/UI/ControlsMenu.cpp
    src/Rendering/SFMLRenderer.cpp
    src/Input/SFMLInput.cpp
    third_party/glad/src/gl.c
    third_party/stb/stb_image_impl.cpp
)

target_include_directories(Metroidvania PRIVATE
    ${PHYSX_ROOT}/include
    third_party/glad/include
    third_party/stb
)

target_link_libraries(Metroidvania
    PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        nlohmann_json::nlohmann_json
        glm::glm
        glfw
        freetype
        $<$<PLATFORM_ID:Windows>:opengl32>
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
