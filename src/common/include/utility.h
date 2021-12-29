
#ifndef OPENGL_SAMPLES_UTILITY_H
#define OPENGL_SAMPLES_UTILITY_H

#include "pch.h"

#define PI 3.14159265359

namespace OpenGL {

    [[nodiscard]] std::string ConvertToNativeSeparators(std::string path);
    [[nodiscard]] std::string GetDirectory(std::string path);
    [[nodiscard]] std::string GetAssetName(std::string path);
    [[nodiscard]] std::string GetAssetExtension(std::string path);

}

#endif //OPENGL_SAMPLES_UTILITY_H
