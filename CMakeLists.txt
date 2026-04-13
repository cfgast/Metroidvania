cmake_minimum_required(VERSION 3.20)
project(Metroidvania LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- Vulkan SDK ---
find_package(Vulkan REQUIRED COMPONENTS glslc)

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

# --- VMA (Vulkan Memory Allocator) ---
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.1.0
    GIT_SHALLOW    TRUE
)
set(VMA_STATIC_VULKAN_FUNCTIONS  OFF CACHE BOOL "" FORCE)
set(VMA_DYNAMIC_VULKAN_FUNCTIONS ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

# --- vk-bootstrap ---
FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG        v1.3.295
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(vk-bootstrap)

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

# --- Compile GLSL shaders to SPIR-V ---
# Usage: compile_shaders(TARGET <target> SOURCES shader1.glsl ...
#                        [INCLUDE_DIRS dir1 ...] [DEPENDS header.glsl ...])
# Compiled .spv files are placed in ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders/.
# INCLUDE_DIRS are passed as -I flags to glslc (for #include support).
# DEPENDS lists header files that all shaders depend on (e.g. lighting.glsl).
function(compile_shaders)
    cmake_parse_arguments(SHADER "" "TARGET" "SOURCES;INCLUDE_DIRS;DEPENDS" ${ARGN})

    # Build -I flags from include directories
    set(INCLUDE_FLAGS)
    foreach(INC_DIR ${SHADER_INCLUDE_DIRS})
        list(APPEND INCLUDE_FLAGS "-I${INC_DIR}")
    endforeach()

    foreach(GLSL_FILE ${SHADER_SOURCES})
        get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
        set(SPV_FILE "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT  ${SPV_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders"
            COMMAND Vulkan::glslc
                    --target-env=vulkan1.3
                    ${INCLUDE_FLAGS}
                    ${GLSL_FILE} -o ${SPV_FILE}
            DEPENDS ${GLSL_FILE} ${SHADER_DEPENDS}
            COMMENT "Compiling SPIR-V: ${FILE_NAME} -> ${FILE_NAME}.spv"
            VERBATIM
        )
        list(APPEND SPV_FILES ${SPV_FILE})
    endforeach()
    add_custom_target(${SHADER_TARGET}_shaders DEPENDS ${SPV_FILES})
    add_dependencies(${SHADER_TARGET} ${SHADER_TARGET}_shaders)
endfunction()

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
    src/Components/LightComponent.cpp
    src/Map/Map.cpp
    src/Map/MapLoader.cpp
    src/Map/TransitionManager.cpp
    src/Debug/DebugMenu.cpp
    src/Physics/PhysXWorld.cpp
    src/UI/SaveSlotScreen.cpp
    src/UI/PauseMenu.cpp
    src/UI/ControlsMenu.cpp
    src/Rendering/VulkanRenderer.cpp
    src/Rendering/VmaImpl.cpp
    src/Input/GLFWInput.cpp
    third_party/stb/stb_image_impl.cpp
)

target_include_directories(Metroidvania PRIVATE
    ${PHYSX_ROOT}/include
    third_party/stb
)

target_link_libraries(Metroidvania
    PRIVATE
        nlohmann_json::nlohmann_json
        glm::glm
        glfw
        freetype
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        vk-bootstrap::vk-bootstrap
        $<$<PLATFORM_ID:Windows>:comdlg32>
        ${PHYSX_LIB_DIR}/PhysX.lib
        ${PHYSX_LIB_DIR}/PhysXFoundation.lib
        ${PHYSX_LIB_DIR}/PhysXCommon.lib
        ${PHYSX_LIB_DIR}/PhysXCooking.lib
        ${PHYSX_LIB_DIR}/PhysXExtensions_static.lib
        ${PHYSX_LIB_DIR}/PhysXPvdSDK_static.lib
)

# --- Compile Vulkan GLSL shaders to SPIR-V ---
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
compile_shaders(
    TARGET       Metroidvania
    SOURCES
        ${SHADER_DIR}/flat.vert
        ${SHADER_DIR}/flat.frag
        ${SHADER_DIR}/textured.vert
        ${SHADER_DIR}/textured.frag
        ${SHADER_DIR}/text.vert
        ${SHADER_DIR}/text.frag
        ${SHADER_DIR}/vertcolor.vert
        ${SHADER_DIR}/vertcolor.frag
        ${SHADER_DIR}/blit.vert
        ${SHADER_DIR}/blit.frag
    INCLUDE_DIRS ${SHADER_DIR}
    DEPENDS      ${SHADER_DIR}/lighting.glsl
)

# GLM Vulkan-compatible settings
target_compile_definitions(Metroidvania PRIVATE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_FORCE_RADIANS
)

# Copy PhysX DLLs next to the executable on Windows
if(WIN32)
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

# --- Generate armor sprite sheets from the base player sprite sheet ---
find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
    set(ARMOR_SPRITE_SCRIPT "${CMAKE_SOURCE_DIR}/tools/generate_armor_sprites.py")
    set(BASE_SPRITESHEET    "${CMAKE_SOURCE_DIR}/assets/player_spritesheet.png")
    set(ARMOR_OUTPUTS
        "${CMAKE_SOURCE_DIR}/assets/player_armor_2.png"
        "${CMAKE_SOURCE_DIR}/assets/player_armor_3.png"
        "${CMAKE_SOURCE_DIR}/assets/player_armor_4.png"
        "${CMAKE_SOURCE_DIR}/assets/player_armor_5.png"
    )
    add_custom_command(
        OUTPUT  ${ARMOR_OUTPUTS}
        COMMAND ${Python3_EXECUTABLE} ${ARMOR_SPRITE_SCRIPT}
                --base "${BASE_SPRITESHEET}"
        DEPENDS ${BASE_SPRITESHEET} ${ARMOR_SPRITE_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating armored player sprite sheets (tiers 2-5)"
        VERBATIM
    )
    add_custom_target(armor_sprites DEPENDS ${ARMOR_OUTPUTS})
    add_dependencies(Metroidvania armor_sprites)
endif()

# Copy assets folder next to the executable so sprite sheets are found at runtime
add_custom_command(TARGET Metroidvania POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Metroidvania>/assets
)

# Copy compiled SPIR-V shaders next to the executable
add_custom_command(TARGET Metroidvania POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_BINARY_DIR}/assets/shaders
        $<TARGET_FILE_DIR:Metroidvania>/assets/shaders
)
