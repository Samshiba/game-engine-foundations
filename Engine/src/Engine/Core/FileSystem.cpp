//
// Created by genin on 05/10/2026.
// Path: Engine/src/Engine/Core/FileSystem.cpp
//

#include <Engine/Core/FileSystem.hpp>
#include "Commons.hpp"

#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace GEF::FileSystem
{
    namespace
    {
        std::filesystem::path QueryExecutablePath()
        {
#if defined(_WIN32)
            std::wstring buffer(MAX_PATH, L'\0');
            DWORD length = 0;

            while ((length = GetModuleFileNameW(nullptr, buffer.data(),
                                                static_cast<DWORD>(buffer.
                                                    size()))) == buffer.size())
            {
                buffer.resize(buffer.size() * 2);
            }

            buffer.resize(length);

            return buffer;

#elif defined(__linux__)

            std::error_code error;
            auto path = std::filesystem::read_symlink("/proc/self/exe", error);

            return error ? std::filesystem::path{} : path;
#else
#error "GetExecutableDirectory is not implemented on this platform"
#endif
        }
    }

    const std::filesystem::path& GetExecutableDirectory()
    {
        static const std::filesystem::path directory = QueryExecutablePath().
            parent_path();
        return directory;
    }

    std::filesystem::path AssetsDirectory()
    {
        return GetExecutableDirectory() / "assets";
    }

    std::filesystem::path AssetPath(const std::filesystem::path& relative)
    {
        return AssetsDirectory() / relative;
    }

    std::optional<std::string> ReadTextFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            GEF_ENGINE_ERROR("Failed to open file: {}", path.string());
            return std::nullopt;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}