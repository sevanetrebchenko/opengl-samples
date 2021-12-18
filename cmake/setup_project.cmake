
# Variables to configure for a new project:
#   - SAMPLE_NAME - Target name
#   - SAMPLE_SOURCE - Source files
#   - SAMPLE_INCLUDE - Include directories

# Make target for project.
message(STATUS "Building Project: ${SAMPLE_NAME}")

# Configure shared source files / include directories.
set(SHARED_SOURCE
        "${PROJECT_SOURCE_DIR}/src/common/src/camera.cpp"
        "${PROJECT_SOURCE_DIR}/src/common/src/transform.cpp"
        "${PROJECT_SOURCE_DIR}/src/common/src/object_loader.cpp"
        "${PROJECT_SOURCE_DIR}/src/common/src/shader.cpp"
        "${PROJECT_SOURCE_DIR}/src/common/src/pch.cpp"
        "${PROJECT_SOURCE_DIR}/src/common/src/utility.cpp"
        )
set(SHARED_INCLUDE "${PROJECT_SOURCE_DIR}/src/common/include")

add_executable(${SAMPLE_NAME} ${SHARED_SOURCE} ${SAMPLE_SOURCE})
target_include_directories(${SAMPLE_NAME} PRIVATE ${SHARED_INCLUDE} ${SAMPLE_INCLUDE})
target_precompile_headers(${SAMPLE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src/common/include/pch.h")

# Link with dependencies.

# OpenGL
find_package(OpenGL REQUIRED) # Ensure OpenGL exists on the system.
message(STATUS "Linking OpenGL to project.")
target_link_libraries(${SAMPLE_NAME} OpenGL::GL)

# Glad.
message(STATUS "Linking Glad to project.")
target_link_libraries(${SAMPLE_NAME} glad)

# GLFW
message(STATUS "Linking GLFW to project.")
target_link_libraries(${SAMPLE_NAME} glfw)

# GLM.
message(STATUS "Linking GLM to project.")
target_link_libraries(${SAMPLE_NAME} glm)

# STB.
message(STATUS "Linking STB to project.")
target_link_libraries(${SAMPLE_NAME} stb)

# tinyobjloaders.
message(STATUS "Linking tinyobjloader to project.")
target_link_libraries(${SAMPLE_NAME} tinyobjloader)