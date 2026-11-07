
# Find SFML 3
find_package(SFML 3 REQUIRED COMPONENTS Graphics)

# Add source files to executable
add_executable(Vault_Heist
    Vault_Heist.cpp
    Vault_Heist.h
)

# Require C++20
target_compile_features(Vault_Heist PRIVATE cxx_std_20)

# Link SFML Graphics
target_link_libraries(Vault_Heist PRIVATE SFML::Graphics)

if(MSVC)
    target_compile_options(Vault_Heist PRIVATE /EHsc)
endif()