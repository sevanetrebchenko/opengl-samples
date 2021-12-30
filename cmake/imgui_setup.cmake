# Set Sources and Header files.
set(SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/lib/imgui/imgui.cpp"
        "${PROJECT_SOURCE_DIR}/lib/imgui/imgui_draw.cpp"
        "${PROJECT_SOURCE_DIR}/lib/imgui/imgui_widgets.cpp"
        "${PROJECT_SOURCE_DIR}/lib/imgui/imgui_demo.cpp"
        "${PROJECT_SOURCE_DIR}/lib/imgui/imgui_tables.cpp"
        "${PROJECT_SOURCE_DIR}/lib/imgui/misc/cpp/imgui_stdlib.cpp"
        # Dependencies
        "${PROJECT_SOURCE_DIR}/lib/imgui/backends/imgui_impl_opengl3.cpp"
        "${PROJECT_SOURCE_DIR}/lib/imgui/backends/imgui_impl_glfw.cpp"
        )

set(IMGUI_INCLUDE_DIRS
        "${PROJECT_SOURCE_DIR}/lib/imgui"
        )

# Add ImGui as a library.
add_library(imgui STATIC "${SOURCE_FILES}")

# Anything that targets ImGui will need to see the directories for ImGui includes.
target_include_directories(imgui PUBLIC "${IMGUI_INCLUDE_DIRS}")

# Use Glad for ImGui.
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

message(STATUS "Linking Glad to ImGui.")
target_link_libraries(imgui glad) # Link Glad to ImGui
message(STATUS "Linking GLFW to ImGui.")
target_link_libraries(imgui glfw) # Link GLFW to ImGui