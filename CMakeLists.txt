cmake_minimum_required(VERSION 3.16)
project(mollerpol)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH
        "Compton installation directory" FORCE)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Override with -DINSTALL_NAMESPACE=my_name at cmake configure time if needed
set(INSTALL_NAMESPACE "" CACHE STRING
    "Subdirectory namespace used in all install paths (include/, lib/cmake/, config/, etc.)" FORCE)

# Find dependencies
find_package(jana2_common_extensions REQUIRED)
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Hist)

# Add subdirectories
install(DIRECTORY config/ DESTINATION config)
add_subdirectory(src/plugins)
