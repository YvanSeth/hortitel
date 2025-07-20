cmake_minimum_required(VERSION 3.12)

# Specify the project name
project(HortiTel)

# Pull in SDK (must be before project)
include(pico/pico_sdk_import.cmake)
include(pico/pico_extras_import_optional.cmake)

# Initialize the Pico SDK
pico_sdk_init()

# Point this to whever you have downloaded the Melopero source from:
# https://github.com/melopero/Melopero_Perpetuo_Lora/tree/main/C%2B%2B/src
add_subdirectory(../Pico/Melopero/Melopero_Perpetuo_Lora/C++/src/ build)

# Add our src directory
add_subdirectory(src)
