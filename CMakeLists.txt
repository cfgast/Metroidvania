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

add_executable(Metroidvania
    src/main.cpp
    src/Core/GameObject.cpp
    src/Components/MovementComponent.cpp
    src/Components/RenderComponent.cpp
    src/Components/PhysicsComponent.cpp
    src/Map/Map.cpp
)

target_link_libraries(Metroidvania
    PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
)

# Copy SFML DLLs next to the executable on Windows
if(WIN32)
    add_custom_command(TARGET Metroidvania POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:Metroidvania>
            $<TARGET_FILE_DIR:Metroidvania>
        COMMAND_EXPAND_LISTS
    )
endif()
