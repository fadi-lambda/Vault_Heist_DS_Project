
find_package(SFML 3 REQUIRED COMPONENTS Graphics)

add_executable(Vault_Heist
    Vault_Heist.cpp
)

target_compile_features(Vault_Heist PRIVATE cxx_std_20)

target_link_libraries(Vault_Heist PRIVATE SFML::Graphics)

if(MSVC)
    target_compile_options(Vault_Heist PRIVATE /EHsc)
endif()