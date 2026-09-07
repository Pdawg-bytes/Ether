#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <memory>

#ifdef __3DS__
    #include <3ds.h>
#else
    #include <windows.h>
#endif

class FileLoader
{
public:
    static std::vector<u8> LoadFile(const std::string& relativePath)
    {
        std::string fullPath = GetFullPath(relativePath);
        
        FILE* file = fopen(fullPath.c_str(), "rb");

        if (!file)
            return std::vector<u8>();

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::vector<u8> buffer(size);
        size_t bytesRead = fread(buffer.data(), 1, size, file);
        fclose(file);

        if (bytesRead != size)
            return std::vector<u8>();

        return buffer;
    }

    static std::string LoadFileAsString(const std::string& relativePath)
    {
        auto data = LoadFile(relativePath);

        if (data.empty())
            return "";

        return std::string(reinterpret_cast<const char*>(data.data()), data.size());
    }

    static std::unique_ptr<std::ifstream> OpenInputStream(const std::string& relativePath)
    {
        std::string fullPath = GetFullPath(relativePath);

        auto stream = std::make_unique<std::ifstream>(fullPath, std::ios::in | std::ios::binary);

        if (!stream->is_open())
            return nullptr;

        return stream;
    }

private:
    static std::string GetFullPath(const std::string& relativePath)
    {
#ifdef __3DS__
        return "romfs:/" + relativePath;
#else
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        
        std::string exeDir(exePath);

        size_t lastSlash = exeDir.find_last_of('\\');
        if (lastSlash != std::string::npos)
            exeDir = exeDir.substr(0, lastSlash + 1);
        
        std::string normalizedPath = relativePath;
        for (char& c : normalizedPath)
        {
            if (c == '/')
                c = '\\';
        }
        
        return exeDir + "Data\\" + normalizedPath;
#endif
    }
};