// Copyright (c) 2021-present Sparky Studios. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <catch2/catch_test_macros.hpp>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>

using namespace SparkyStudios::Audio::Amplitude;

TEST_CASE("DiskFileSystem Tests", "[filesystem][amplitude]")
{
    DiskFileSystem fileSystem;
    fileSystem.SetBasePath(AM_OS_STRING("./sample_project"));

    const auto& cp = std::filesystem::current_path() / AM_OS_STRING("sample_project");

    SECTION("can sets the base path")
    {
        REQUIRE(fileSystem.GetBasePath() == cp);
    }

    SECTION("can resolve paths")
    {
        REQUIRE(
            fileSystem.ResolvePath(AM_OS_STRING("sounds/test.wav")) ==
            (cp / AM_OS_STRING("sounds/test.wav")).lexically_normal().make_preferred().native());
        REQUIRE(
            fileSystem.ResolvePath(AM_OS_STRING("../sample_project/sounds/../test.wav")) ==
            (cp / AM_OS_STRING("test.wav")).lexically_normal().make_preferred().native());
        REQUIRE(
            fileSystem.ResolvePath(AM_OS_STRING("./sounds/../sounds/./test.wav")) ==
            (cp / AM_OS_STRING("sounds/test.wav")).lexically_normal().make_preferred().native());
    }

    SECTION("can check if files exists")
    {
        REQUIRE(fileSystem.Exists(AM_OS_STRING("audio_config.amconfig")));
        REQUIRE_FALSE(fileSystem.Exists(AM_OS_STRING("some_random_file.ext")));
    }

    SECTION("can detect if a file is a directory")
    {
        REQUIRE(fileSystem.IsDirectory(AM_OS_STRING("sounds")));
        REQUIRE_FALSE(fileSystem.IsDirectory(AM_OS_STRING("audio_config.amconfig")));
    }

    SECTION("can join paths")
    {
        REQUIRE(
            fileSystem.Join({ AM_OS_STRING("sounds"), AM_OS_STRING("test.wav") }) ==
            std::filesystem::path(AM_OS_STRING("sounds/test.wav")).lexically_normal().make_preferred().native());
        REQUIRE(
            fileSystem.Join({ AM_OS_STRING("../sample_project/sounds/../test.wav") }) ==
            std::filesystem::path(AM_OS_STRING("../sample_project/test.wav")).lexically_normal().make_preferred().native());
        REQUIRE(
            fileSystem.Join({ AM_OS_STRING("./sounds"), AM_OS_STRING("../sounds/"), AM_OS_STRING("./test.wav") }) ==
            std::filesystem::path(AM_OS_STRING("sounds/test.wav")).lexically_normal().make_preferred().native());
    }

    SECTION("can open files")
    {
        REQUIRE(fileSystem.OpenFile(AM_OS_STRING("audio_config.amconfig"))->IsValid());
        REQUIRE_FALSE(fileSystem.OpenFile(AM_OS_STRING("some_random_file.ext"))->IsValid());
    }
}

TEST_CASE("DiskFileSystem DiskFile Tests", "[filesystem][amplitude]")
{
    DiskFileSystem fileSystem;
    fileSystem.SetBasePath(AM_OS_STRING("./sample_project"));

    const auto& file = fileSystem.OpenFile(AM_OS_STRING("test_data/diskfile_read_test.txt"));

    SECTION("can open files")
    {
        REQUIRE(file->IsValid());
    }

    SECTION("can return the correct file path")
    {
        REQUIRE(file->GetPath() == fileSystem.ResolvePath(AM_OS_STRING("test_data/diskfile_read_test.txt")));
    }

    SECTION("can return the correct file size")
    {
        REQUIRE(file->Length() == 2);
    }

    SECTION("can read the file")
    {
        REQUIRE(file->Read8() == 'O');
        REQUIRE(file->Read8() == 'K');
    }

    SECTION("can seek the file")
    {
        file->Seek(1, SEEK_SET);
        REQUIRE(file->Position() == 1);
        REQUIRE(file->Read8() == 'K');
        file->Seek(-2, SEEK_END);
        REQUIRE(file->Position() == 0);
        REQUIRE(file->Read8() == 'O');
        file->Seek(-1, SEEK_CUR);
        REQUIRE(file->Position() == 0);
        REQUIRE(file->Read8() == 'O');
    }

    SECTION("can read the entire file")
    {
        file->Seek(0);
        auto* content = static_cast<AmUInt8Buffer>(ammalloc(2));
        REQUIRE(file->Read(content, 2) == 2);
        REQUIRE(content[0] == 'O');
        REQUIRE(content[1] == 'K');
        REQUIRE(file->Position() == file->Length());
        amfree(content);
    }

    SECTION("can close files")
    {
        static_cast<DiskFile*>(file.get())->Close();
        REQUIRE_FALSE(file->IsValid());
    }
}

TEST_CASE("Native DiskFile Tests", "[filesystem][amplitude]")
{
    DiskFileSystem fileSystem;
    fileSystem.SetBasePath(AM_OS_STRING("./sample_project"));

    DiskFile file(fileSystem.ResolvePath(AM_OS_STRING("test_data/diskfile_read_test.txt")), eFOM_READWRITE, eFOK_BINARY);

    SECTION("can open files")
    {
        REQUIRE(file.IsValid());
    }

    SECTION("can return the correct file path")
    {
        REQUIRE(file.GetPath() == fileSystem.ResolvePath(AM_OS_STRING("test_data/diskfile_read_test.txt")));
    }

    SECTION("can return the correct file size")
    {
        REQUIRE(file.Length() == 0);
    }

    SECTION("can write the file")
    {
        file.Seek(0, SEEK_SET);
        REQUIRE(file.Write8('K') == 1);
        REQUIRE(file.Write8('O') == 1);

        SECTION("can seek the file")
        {
            file.Seek(1, SEEK_SET);
            REQUIRE(file.Position() == 1);
            REQUIRE(file.Read8() == 'O');
        }

        SECTION("can read the entire file")
        {
            file.Seek(0, SEEK_SET);
            auto* content = static_cast<AmUInt8Buffer>(ammalloc(2));
            REQUIRE(file.Read(content, 2) == 2);
            REQUIRE(content[0] == 'K');
            REQUIRE(content[1] == 'O');
            REQUIRE(file.Position() == file.Length());
            amfree(content);
        }

        SECTION("can write the entire file")
        {
            file.Seek(0, SEEK_SET);
            auto* content = static_cast<AmUInt8Buffer>(ammalloc(2));
            content[0] = 'O';
            content[1] = 'K';
            REQUIRE(file.Write(content, 2) == 2);
            REQUIRE(file.Position() == file.Length());
            amfree(content);
        }
    }

    SECTION("can close files")
    {
        file.Seek(0, SEEK_SET);
        REQUIRE(file.Write8('O') == 1);
        REQUIRE(file.Write8('K') == 1);

        file.Close();
        REQUIRE_FALSE(file.IsValid());
    }
}

TEST_CASE("MemoryFile Tests", "[filesystem][amplitude]")
{
    MemoryFile file;
    file.Open(2);

    file.Seek(0, SEEK_SET);
    file.Write8('O');
    file.Write8('K');
    file.Seek(0, SEEK_SET);

    SECTION("can open files")
    {
        DiskFileSystem fileSystem;
        fileSystem.SetBasePath(AM_OS_STRING("./sample_project"));

        REQUIRE(file.IsValid());

        char ok[] = "OK";

        file.Close();
        REQUIRE(file.OpenMem(nullptr, 2) == AM_ERROR_INVALID_PARAMETER);
        REQUIRE_FALSE(file.IsValid());
        REQUIRE(file.OpenMem(reinterpret_cast<AmConstUInt8Buffer>(ok), 2, false, false) == AM_ERROR_NO_ERROR);
        REQUIRE(file.IsValid());
        REQUIRE(file.GetPtr() == ok);

        file.Close();
        REQUIRE(ok[0] == 'O');
        REQUIRE(ok[1] == 'K');
        REQUIRE(file.OpenMem(reinterpret_cast<AmConstUInt8Buffer>(ok), 2, true, true) == AM_ERROR_NO_ERROR);
        REQUIRE(file.IsValid());
        REQUIRE(file.Read(reinterpret_cast<AmUInt8Buffer>(ok), 2) == 2);
        REQUIRE(ok[0] == 'O');
        REQUIRE(ok[1] == 'K');

        file.Close();
        REQUIRE(file.OpenToMem("") == AM_ERROR_INVALID_PARAMETER);
        REQUIRE_FALSE(file.IsValid());
        REQUIRE(file.OpenToMem(fileSystem.ResolvePath(AM_OS_STRING("test_data/diskfile_read_test.txt"))) == AM_ERROR_NO_ERROR);
        REQUIRE(file.IsValid());
        REQUIRE(file.Read(reinterpret_cast<AmUInt8Buffer>(ok), 2) == 2);
        REQUIRE(ok[0] == 'O');
        REQUIRE(ok[1] == 'K');

        file.Close();
        DiskFile df(fileSystem.ResolvePath(AM_OS_STRING("test_data/diskfile_read_test.txt")), eFOM_READ, eFOK_BINARY);
        REQUIRE(file.OpenFileToMem(nullptr) == AM_ERROR_INVALID_PARAMETER);
        REQUIRE_FALSE(file.IsValid());
        REQUIRE(file.OpenFileToMem(&df) == AM_ERROR_NO_ERROR);
        REQUIRE(file.IsValid());
        REQUIRE(file.Read(reinterpret_cast<AmUInt8Buffer>(ok), 2) == 2);
        REQUIRE(ok[0] == 'O');
        REQUIRE(ok[1] == 'K');
    }

    SECTION("can return the correct file path")
    {
        REQUIRE(file.GetPath().empty());
    }

    SECTION("can return the correct file size")
    {
        REQUIRE(file.Length() == 2);
    }

    SECTION("can read the file")
    {
        REQUIRE(file.Read8() == 'O');
        REQUIRE(file.Read8() == 'K');
    }

    SECTION("can seek the file")
    {
        file.Seek(1, SEEK_SET);
        REQUIRE(file.Position() == 1);
        REQUIRE(file.Read8() == 'K');
        file.Seek(-2, SEEK_END);
        REQUIRE(file.Position() == 0);
        REQUIRE(file.Read8() == 'O');
        file.Seek(-1, SEEK_CUR);
        REQUIRE(file.Position() == 0);
        REQUIRE(file.Read8() == 'O');
        file.Seek(1234, SEEK_SET);
        REQUIRE(file.Position() == 1234);
        REQUIRE(file.Read8() == 0);
    }

    SECTION("can read the entire file")
    {
        file.Seek(0, SEEK_SET);
        auto* content = static_cast<AmUInt8Buffer>(ammalloc(2));
        REQUIRE(file.Read(content, 2) == 2);
        REQUIRE(content[0] == 'O');
        REQUIRE(content[1] == 'K');
        REQUIRE(file.Position() == file.Length());
        REQUIRE(file.Eof());
        amfree(content);
    }

    SECTION("can close files")
    {
        file.Close();
        REQUIRE_FALSE(file.IsValid());
        REQUIRE(file.GetPtr() == nullptr);
    }
}