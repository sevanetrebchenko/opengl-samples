
# Configure project files.
set(SAMPLE_NAME PathTracing)
set(SAMPLE_SOURCE
        "${PROJECT_SOURCE_DIR}/src/samples/path-tracing/src/main.cpp"
        "${PROJECT_SOURCE_DIR}/src/samples/path-tracing/src/triangle.cpp"
        "${PROJECT_SOURCE_DIR}/src/samples/path-tracing/src/model.cpp"
        "${PROJECT_SOURCE_DIR}/src/samples/path-tracing/src/material.cpp"
        "${PROJECT_SOURCE_DIR}/src/samples/path-tracing/src/primitives.cpp"
        )

set(SAMPLE_INCLUDE "${PROJECT_SOURCE_DIR}/src/samples/path-tracing/include")

include(setup_project)