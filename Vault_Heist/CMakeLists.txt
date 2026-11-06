# CMakeList.txt : CMake project for Vault_Heist, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(Vault_Heist
    Vault_Heist.cpp
    Vault_Heist.h
)

target_compile_features(Vault_Heist PRIVATE cxx_std_20)

# TODO: Add tests and install targets if needed.
