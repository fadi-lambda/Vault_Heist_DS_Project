
cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>"
    )
endif()

project(Vault_Heist LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(SFML CONFIG REQUIRED COMPONENTS Graphics)
find_package(GTest CONFIG REQUIRED)

# Main application
add_executable(Vault_Heist
    src/main.cpp
)

target_include_directories(Vault_Heist
    PRIVATE include
)

target_link_libraries(Vault_Heist
    PRIVATE SFML::Graphics
)

if(MSVC)
    target_compile_options(Vault_Heist PRIVATE /EHsc)
endif()

# Tests
add_subdirectory(tests)