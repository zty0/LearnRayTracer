# CMakeList.txt : CMake project for RayTracerProgram, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (RayTracer WIN32 "main.cpp" )
#target_include_directories(RayTracerProgram PUBLIC C:\\Users\\jox\\source\\repos\\glm)
if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(RayTracer PROPERTIES
                          LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

set(glm_DIR ${CMAKE_SOURCE_DIR}/library/glm/lib/cmake/glm) # if necessary
find_package(glm REQUIRED)
target_link_libraries(RayTracer glm::glm)

# Amend manifest to tell Windows that the application is DPI aware (needed for Windows 8.1 and up)
IF (MSVC)
    IF (CMAKE_MAJOR_VERSION LESS 3)
        MESSAGE(WARNING "CMake version 3.0 or newer is required use build variable TARGET_FILE")
    ELSE()
        ADD_CUSTOM_COMMAND(
            TARGET RayTracer
            POST_BUILD
            COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\dpiaware.manifest\" -inputresource:\"$<TARGET_FILE:RayTracer>\"\;\#1 -outputresource:\"$<TARGET_FILE:RayTracer>\"\;\#1
            COMMENT "Adding display aware manifest..." 
        )
    ENDIF()
ENDIF(MSVC)

# TODO: Add tests and install targets if needed.
