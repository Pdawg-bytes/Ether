#pragma once

#include "SceneObject.h"
#include "Material.h"
#include "MaterialLibrary.h"
#include "../Platform/FileLoader.h"
#include "../Math/Vector3.h"
#include "../Math/Vector2.h"
#include "../Math/Quaternion.h"

#include <vector>
#include <string>
#include <istream>
#include <memory>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cctype>

struct OBJMaterial
{
    std::string Name;
    
    Vector3 Albedo   = Vector3(0.8f);
    f32 Roughness    = 0.5f;
    f32 Metallic     = 0.0f;
    Vector3 Emission = Vector3::Zero;
    f32 IOR          = 1.5f;
    f32 Transmission = 0.0f;

    std::string AlbedoMap;
    std::string RoughnessMap;
    std::string EmissionMap;
    std::string BumpMap;
};

class OBJLoader
{
public:
    static std::vector<SceneObject> LoadFromFile(const std::string& relativeFilePath)
    {
        return LoadFromFile(relativeFilePath, Vector3::Zero, Quaternion::Identity, Vector3::One);
    }

    static std::vector<SceneObject> LoadFromFile(const std::string& relativeFilePath,
                                                 const Vector3& position,
                                                 const Quaternion& rotation = Quaternion::Identity,
                                                 const Vector3& scale = Vector3::One)
    {
        std::unique_ptr<std::istream> stream = FileLoader::OpenInputStream(relativeFilePath);

        if (!stream || !*stream)
            return std::vector<SceneObject>();

        std::string directory = DirectoryOf(relativeFilePath);
        return LoadFromStream(*stream, directory, position, rotation, scale);
    }

private:
    struct OBJVertex
    {
        Vector3 Position;
        Vector3 Normal   = Vector3::Zero;
        Vector2 TexCoord = Vector2::Zero;
    };

    struct OBJFace
    {
        std::vector<u32> VertexIndices;
        std::vector<u32> NormalIndices;
        std::vector<u32> TexCoordIndices;
        std::string      MaterialName;
    };

    static std::vector<SceneObject> LoadFromStream(std::istream& stream, const std::string& directory,
                                                   const Vector3& position, const Quaternion& rotation, const Vector3& scale)
    {
        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<Vector2> texCoords;
        std::vector<OBJFace> faces;
        std::unordered_map<std::string, OBJMaterial> materials;
        std::string currentMaterial;

        std::string line;

        while (std::getline(stream, line))
        {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "v")
            {
                Vector3 pos;
                iss >> pos.X >> pos.Y >> pos.Z;
                positions.push_back(pos);
            }
            else if (command == "vn")
            {
                Vector3 norm;
                iss >> norm.X >> norm.Y >> norm.Z;
                normals.push_back(norm.Normalized());
            }
            else if (command == "vt")
            {
                Vector2 tex;
                iss >> tex.X >> tex.Y;
                tex.Y = 1.0f - tex.Y;
                texCoords.push_back(tex);
            }
            else if (command == "f")
            {
                OBJFace face;
                face.MaterialName = currentMaterial;

                std::string vertexStr;
                while (iss >> vertexStr)
                {
                    ParseFaceVertex(vertexStr, face, positions.size(), texCoords.size(), normals.size());
                }

                if (!face.VertexIndices.empty())
                {
                    bool valid = true;
                    for (u32 index : face.VertexIndices)
                    {
                        if (index == static_cast<u32>(-1))
                        {
                            valid = false;
                            break;
                        }
                    }
                    if (valid)
                        faces.push_back(face);
                }
            }
            else if (command == "usemtl")
            {
                iss >> currentMaterial;
            }
            else if (command == "mtllib")
            {
                std::string materialFile;
                std::getline(iss, materialFile);
                Trim(materialFile);
                if (!materialFile.empty())
                {
                    std::string materialPath = JoinPath(directory, materialFile);

                    std::unique_ptr<std::istream> mtlStream = FileLoader::OpenInputStream(materialPath);
                    if (mtlStream && *mtlStream)
                    {
                        ParseMtl(*mtlStream, DirectoryOf(materialPath), materials);
                    }
                }
            }
        }

        for (Vector3& vertex : positions)
        {
            vertex = rotation.Rotate(vertex * scale) + position;
        }

        std::unordered_map<std::string, s32> materialIndices;
        MaterialLibrary& library = GetMaterialLibrary();

        for (auto& [matName, objMat] : materials)
        {
            Material mat;
            mat.Albedo       = objMat.Albedo;
            mat.Roughness    = objMat.Roughness;
            mat.Metallic     = objMat.Metallic;
            mat.Emission     = objMat.Emission;
            mat.IOR          = objMat.IOR;
            mat.Transmission = objMat.Transmission;
            mat.AlbedoMap    = LoadTexture(objMat.AlbedoMap);
            mat.RoughnessMap = LoadTexture(objMat.RoughnessMap);
            mat.EmissionMap  = LoadTexture(objMat.EmissionMap);
            mat.BumpMap      = LoadTexture(objMat.BumpMap);

            materialIndices[matName] = library.Add(mat);
        }

        std::vector<SceneObject> sceneObjects;
        for (const auto& face : faces)
        {
            s32 matIndex = -1;
            if (!face.MaterialName.empty() && materialIndices.count(face.MaterialName))
            {
                matIndex = materialIndices[face.MaterialName];
            }

            if (face.VertexIndices.size() == 3)
            {
                Vector3 v0 = positions[face.VertexIndices[0]];
                Vector3 v1 = positions[face.VertexIndices[1]];
                Vector3 v2 = positions[face.VertexIndices[2]];

                if (HasValidTexCoords(face, 3))
                {
                    Vector2 uv0 = texCoords[face.TexCoordIndices[0]];
                    Vector2 uv1 = texCoords[face.TexCoordIndices[1]];
                    Vector2 uv2 = texCoords[face.TexCoordIndices[2]];

                    sceneObjects.push_back(SceneObject::CreateTriangle(v0, v1, v2, uv0, uv1, uv2, matIndex));
                }
                else
                {
                    sceneObjects.push_back(SceneObject::CreateTriangle(v0, v1, v2, matIndex));
                }
            }
            else if (face.VertexIndices.size() == 4)
            {
                Vector3 v0 = positions[face.VertexIndices[0]];
                Vector3 v1 = positions[face.VertexIndices[1]];
                Vector3 v2 = positions[face.VertexIndices[2]];
                Vector3 v3 = positions[face.VertexIndices[3]];

                if (HasValidTexCoords(face, 4))
                {
                    Vector2 uv0 = texCoords[face.TexCoordIndices[0]];
                    Vector2 uv1 = texCoords[face.TexCoordIndices[1]];
                    Vector2 uv2 = texCoords[face.TexCoordIndices[2]];
                    Vector2 uv3 = texCoords[face.TexCoordIndices[3]];
                    auto quads  = SceneObject::CreateQuad(v0, v1, v2, v3, uv0, uv1, uv2, uv3, matIndex);

                    sceneObjects.insert(sceneObjects.end(), quads.begin(), quads.end());
                }
                else
                {
                    auto quads = SceneObject::CreateQuad(v0, v1, v2, v3, matIndex);
                    sceneObjects.insert(sceneObjects.end(), quads.begin(), quads.end());
                }
            }
            else if (face.VertexIndices.size() > 4)
            {
                bool hasUV  = HasValidTexCoords(face, face.VertexIndices.size());
                Vector2 uv0 = hasUV ? texCoords[face.TexCoordIndices[0]] : Vector2::Zero;
                
                for (size_t i = 1; i < face.VertexIndices.size() - 1; i++)
                {
                    Vector3 v0 = positions[face.VertexIndices[0]];
                    Vector3 v1 = positions[face.VertexIndices[i]];
                    Vector3 v2 = positions[face.VertexIndices[i + 1]];
                    
                    if (hasUV)
                    {
                        Vector2 uv1 = texCoords[face.TexCoordIndices[i]];
                        Vector2 uv2 = texCoords[face.TexCoordIndices[i + 1]];
                        sceneObjects.push_back(SceneObject::CreateTriangle(v0, v1, v2, uv0, uv1, uv2, matIndex));
                    }
                    else
                    {
                        sceneObjects.push_back(SceneObject::CreateTriangle(v0, v1, v2, matIndex));
                    }
                }
            }
        }

        return sceneObjects;
    }

    static u32 ParseIndex(const std::string& value, size_t count)
    {
        if (value.empty())
            return static_cast<u32>(-1);

        s32 index = static_cast<s32>(std::stoi(value));
        if (index < 0)
            index += static_cast<s32>(count);
        else
            --index;

        return index >= 0 && index < static_cast<s32>(count) ? static_cast<u32>(index) : static_cast<u32>(-1);
    }

    static bool HasValidTexCoords(const OBJFace& face, size_t count)
    {
        if (face.TexCoordIndices.size() < count)
            return false;

        for (size_t i = 0; i < count; ++i)
        {
            if (face.TexCoordIndices[i] == static_cast<u32>(-1))
                return false;
        }

        return true;
    }

    static void Trim(std::string& value)
    {
        size_t begin = value.find_first_not_of(" \t\r\n");
        size_t end   = value.find_last_not_of(" \t\r\n");
        value        = begin == std::string::npos ? std::string() : value.substr(begin, end - begin + 1);
    }

    static std::string DirectoryOf(const std::string& path)
    {
        size_t slash = path.find_last_of("/\\");
        return slash == std::string::npos ? std::string() : path.substr(0, slash);
    }

    static std::string JoinPath(const std::string& directory, const std::string& path)
    {
        if (directory.empty())
            return path;

        if (path.empty())
            return directory;

        return directory + "/" + path;
    }

    static TexturePtr LoadTexture(const std::string& path)
    {
        return path.empty() ? nullptr : Texture::LoadFromFile(path);
    }

    static void ParseFaceVertex(const std::string& vertexStr, OBJFace& face, size_t positionCount, size_t texCoordCount, size_t normalCount)
    {
        size_t pos1 = vertexStr.find('/');
        if (pos1 == std::string::npos)
        {
            face.VertexIndices.push_back(ParseIndex(vertexStr, positionCount));
            face.TexCoordIndices.push_back(static_cast<u32>(-1));
            face.NormalIndices.push_back(static_cast<u32>(-1));
            return;
        }

        face.VertexIndices.push_back(ParseIndex(vertexStr.substr(0, pos1), positionCount));

        size_t pos2 = vertexStr.find('/', pos1 + 1);
        if (pos2 == std::string::npos)
        {
            std::string texStr = vertexStr.substr(pos1 + 1);
            face.TexCoordIndices.push_back(texStr.empty() ? static_cast<u32>(-1) : ParseIndex(texStr, texCoordCount));
            face.NormalIndices.push_back(static_cast<u32>(-1));
            return;
        }

        std::string texStr = vertexStr.substr(pos1 + 1, pos2 - pos1 - 1);
        face.TexCoordIndices.push_back(texStr.empty() ? static_cast<u32>(-1) : ParseIndex(texStr, texCoordCount));
        face.NormalIndices.push_back(ParseIndex(vertexStr.substr(pos2 + 1), normalCount));
    }

    static void ParseMtl(std::istream& stream, const std::string& directory, std::unordered_map<std::string, OBJMaterial>& materials)
    {
        std::string line;
        OBJMaterial currentMat;
        bool inMaterial = false;

        while (std::getline(stream, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "newmtl")
            {
                if (inMaterial && !currentMat.Name.empty())
                {
                    materials[currentMat.Name] = currentMat;
                }

                std::string matName;
                iss >> matName;

                currentMat      = OBJMaterial();
                currentMat.Name = matName;
                inMaterial      = true;
            }
            else if (command == "Kd" && inMaterial)
            {
                iss >> currentMat.Albedo.X >> currentMat.Albedo.Y >> currentMat.Albedo.Z;
            }
            else if (command == "Ka" && inMaterial)
            {
                Vector3 ambient;
                iss >> ambient.X >> ambient.Y >> ambient.Z;
                if (currentMat.Albedo == Vector3(0.8f))
                {
                    currentMat.Albedo = ambient;
                }
            }
            else if (command == "Ns" && inMaterial)
            {
                f32 shininess;
                iss >> shininess;
                currentMat.Roughness = 1.0f - (shininess / 1000.0f);
                currentMat.Roughness = std::clamp(currentMat.Roughness, 0.0f, 1.0f);
            }
            else if (command == "Ni" && inMaterial)
            {
                iss >> currentMat.IOR;
            }
            else if (command == "Tr" && inMaterial)
            {
                iss >> currentMat.Transmission;
            }
            else if (command == "d" && inMaterial)
            {
                f32 dissolve;
                iss >> dissolve;
                currentMat.Transmission = 1.0f - dissolve;
            }
            else if (command == "Ke" && inMaterial)
            {
                iss >> currentMat.Emission.X >> currentMat.Emission.Y >> currentMat.Emission.Z;
            }
            else if ((command == "map_Kd" || command == "map_kd") && inMaterial)
            {
                std::getline(iss, currentMat.AlbedoMap);
                Trim(currentMat.AlbedoMap);
                currentMat.AlbedoMap = JoinPath(directory, LastToken(currentMat.AlbedoMap));
            }
            else if ((command == "map_Ns" || command == "map_ns") && inMaterial)
            {
                std::getline(iss, currentMat.RoughnessMap);
                Trim(currentMat.RoughnessMap);
                currentMat.RoughnessMap = JoinPath(directory, LastToken(currentMat.RoughnessMap));
            }
            else if ((command == "map_Ke" || command == "map_ke") && inMaterial)
            {
                std::getline(iss, currentMat.EmissionMap);
                Trim(currentMat.EmissionMap);
                currentMat.EmissionMap = JoinPath(directory, LastToken(currentMat.EmissionMap));
            }
            else if (command == "bump" || command == "map_Bump" || command == "map_bump")
            {
                std::getline(iss, currentMat.BumpMap);
                Trim(currentMat.BumpMap);
                currentMat.BumpMap = JoinPath(directory, LastToken(currentMat.BumpMap));
            }
        }

        if (inMaterial && !currentMat.Name.empty())
        {
            materials[currentMat.Name] = currentMat;
        }

        if (materials.empty())
        {
            OBJMaterial defaultMat;
            defaultMat.Name      = "default";
            materials["default"] = defaultMat;
        }
    }

    static std::string LastToken(const std::string& value)
    {
        std::istringstream stream(value);
        std::string token;
        std::string last;

        while (stream >> token)
            last = token;

        return last;
    }
};