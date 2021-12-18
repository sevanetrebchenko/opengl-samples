
# Add STB as a library.
add_library(stb INTERFACE)

# Anything that targets stb will need to see the directories for STB includes.
target_include_directories(stb INTERFACE "${PROJECT_SOURCE_DIR}/lib/stb/include")