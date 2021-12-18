
#include "utility.h"

namespace OpenGL {

    [[nodiscard]] char GetSeparator() {
        #ifdef _WIN32
            return '/';
        #else
            return '\\';
        #endif
    }

    [[nodiscard]] char GetNativeSeparator() {
        #ifdef _WIN32
            return '\\';
        #else
            return '/';
        #endif
    }

    std::string ConvertToNativeSeparators(std::string path) {
        std::replace(path.begin(), path.end(), GetSeparator(), GetNativeSeparator());
        return path;
    }

    std::string GetDirectory(std::string path) {
        std::string directory;
        path = ConvertToNativeSeparators(path);

        std::size_t last_separator_index = path.rfind(GetNativeSeparator());

        if (last_separator_index != std::string::npos) {
            // Found position of separator.
            directory = path.substr(0, last_separator_index + 1); // Keep slash.
        }

        return directory;
    }

    std::string GetAssetName(std::string path) {
        std::string asset_name;
        path = ConvertToNativeSeparators(path);

        // Substring everything after the last slash.
        std::size_t last_separator_index = path.rfind(GetNativeSeparator());
        if (last_separator_index != std::string::npos) {
            asset_name = path.substr(last_separator_index + 1);
        }

        // Substring everything before the '.' of the extension.
        std::size_t dot = asset_name.find_last_of('.');
        if (dot != std::string::npos) {
            asset_name = asset_name.substr(0, dot);
        }

        return asset_name;
    }

    std::string GetAssetExtension(std::string path) {
        std::size_t dot = path.find_last_of('.');
        if (dot != std::string::npos) {
            return path.substr(dot + 1);
        }

        return path;
    }

}