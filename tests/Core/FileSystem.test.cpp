//
// Path: tests/Core/FileSystem.test.cpp
//
// No GPU needed: these run on every CI job, so the Linux branch of
// GetExecutableDirectory (/proc/self/exe) is covered by the GCC/Clang jobs.
//

#include <doctest.h>

#include <Engine/Core/FileSystem.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace GEF;

namespace
{
    // A file in the system temp directory, removed when the test ends
    class TempFile
    {
    public:
        TempFile(const fs::path& name, const std::string& content)
            : path_(fs::temp_directory_path() / name)
        {
            std::ofstream(path_, std::ios::binary) << content;
        }

        ~TempFile()
        {
            std::error_code ignored;
            fs::remove(path_, ignored);
        }

        TempFile(const TempFile&) = delete;
        TempFile& operator=(const TempFile&) = delete;

        [[nodiscard]] const fs::path& Path() const { return path_; }

    private:
        fs::path path_;
    };
}

TEST_CASE("GetExecutableDirectory is the directory holding the test executable")
{
    const fs::path& directory = FileSystem::GetExecutableDirectory();

    REQUIRE_FALSE(directory.empty());
    CHECK(directory.is_absolute());
    CHECK(fs::is_directory(directory));

    // A directory, not the executable itself (the parent_path() trap)
    bool containsTestExecutable = false;
    for (const auto& entry : fs::directory_iterator(directory))
    {
        if (entry.path().stem() == "GEF_Tests")
            containsTestExecutable = true;
    }
    CHECK(containsTestExecutable);
}

TEST_CASE("GetExecutableDirectory is computed once and stays stable")
{
    const fs::path& first = FileSystem::GetExecutableDirectory();
    const fs::path& second = FileSystem::GetExecutableDirectory();

    CHECK(&first == &second);
    CHECK(first == second);
}

TEST_CASE("AssetsDirectory and AssetPath are built from the executable directory")
{
    const fs::path assets = FileSystem::AssetsDirectory();

    CHECK(assets == FileSystem::GetExecutableDirectory() / "assets");
    CHECK(FileSystem::AssetPath("shaders/basic.vert") ==
          assets / "shaders" / "basic.vert");
}

TEST_CASE("ReadTextFile returns the exact content of a file")
{
    // Binary read: line endings and trailing spaces come back unchanged
    const std::string content = "line 1\r\nline 2\n  trailing  \n";
    const TempFile file("gef_filesystem_test.txt", content);

    const auto read = FileSystem::ReadTextFile(file.Path());

    REQUIRE(read.has_value());
    CHECK(*read == content);
}

TEST_CASE("ReadTextFile tells an empty file apart from a missing one")
{
    const TempFile empty("gef_filesystem_empty.txt", "");

    const auto read = FileSystem::ReadTextFile(empty.Path());

    REQUIRE(read.has_value()); // exists...
    CHECK(read->empty());      // ...and is empty
}

TEST_CASE("ReadTextFile returns nullopt for a missing file")
{
    const fs::path missing =
        fs::temp_directory_path() / "gef_this_file_does_not_exist.txt";
    REQUIRE_FALSE(fs::exists(missing));

    CHECK_FALSE(FileSystem::ReadTextFile(missing).has_value());
}

TEST_CASE("ReadTextFile handles non-ASCII paths")
{
    // The bug the wchar_t -> char copy had: accented characters in the path.
    // A u8"" literal is char8_t in C++20, which std::filesystem::path reads as
    // UTF-8 on every platform (a plain std::string would use the Windows
    // ANSI code page instead).
    const TempFile file(fs::path(u8"gef_été_shader.txt"), "ok");

    REQUIRE(fs::exists(file.Path()));
    const auto read = FileSystem::ReadTextFile(file.Path());

    REQUIRE(read.has_value());
    CHECK(*read == "ok");
}
