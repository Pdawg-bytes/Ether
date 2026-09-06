#pragma once

#include "SceneObject.h"
#include "Material.h"
#include "MaterialLibrary.h"
#include "../Platform/FileLoader.h"
#include "../Math/Vector3.h"
#include "../Math/Vector2.h"

#include <vector>
#include <string>
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
};

class OBJLoader
{
public:
    static std::vector<SceneObject> LoadFromFile(const std::string& relativeFilePath)
    {
        std::string content = FileLoader::LoadFileAsString(relativeFilePath);

        if (content.empty())
            return std::vector<SceneObject>();

        return LoadFromString(content);
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

    static std::vector<SceneObject> LoadFromString(const std::string& content)
    {
        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<Vector2> texCoords;
        std::vector<OBJFace> faces;
        std::unordered_map<std::string, OBJMaterial> materials;
        std::string currentMaterial;

        std::istringstream stream(content);
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
                    ParseFaceVertex(vertexStr, face);
                }

                if (!face.VertexIndices.empty())
                {
                    faces.push_back(face);
                }
            }
            else if (command == "usemtl")
            {
                iss >> currentMaterial;
            }
            else if (command == "mtllib")
            {
            }
        }

        ParseMaterialsFromContent(content, materials);

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

                if (!face.TexCoordIndices.empty() && face.TexCoordIndices.size() == 3)
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

                if (!face.TexCoordIndices.empty() && face.TexCoordIndices.size() == 4)
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
                for (size_t i = 1; i < face.VertexIndices.size() - 1; i++)
                {
                    Vector3 v0 = positions[face.VertexIndices[0]];
                    Vector3 v1 = positions[face.VertexIndices[i]];
                    Vector3 v2 = positions[face.VertexIndices[i + 1]];
                    
                    sceneObjects.push_back(SceneObject::CreateTriangle(v0, v1, v2, matIndex));
                }
            }
        }

        return sceneObjects;
    }

    static u32 StringToU32(const std::string& str)
    {
        return static_cast<u32>(std::stoul(str));
    }

    static void ParseFaceVertex(const std::string& vertexStr, OBJFace& face)
    {
        size_t pos1 = vertexStr.find('/');
        if (pos1 == std::string::npos)
        {
            face.VertexIndices.push_back(StringToU32(vertexStr) - 1);
            return;
        }

        u32 posIndex = StringToU32(vertexStr.substr(0, pos1)) - 1;
        face.VertexIndices.push_back(posIndex);

        size_t pos2 = vertexStr.find('/', pos1 + 1);
        if (pos2 == std::string::npos)
        {
            std::string texStr = vertexStr.substr(pos1 + 1);
            if (!texStr.empty())
            {
                face.TexCoordIndices.push_back(StringToU32(texStr) - 1);
            }
            return;
        }

        std::string texStr = vertexStr.substr(pos1 + 1, pos2 - pos1 - 1);
        if (!texStr.empty())
        {
            face.TexCoordIndices.push_back(StringToU32(texStr) - 1);
        }

        u32 normIndex = StringToU32(vertexStr.substr(pos2 + 1)) - 1;
        face.NormalIndices.push_back(normIndex);
    }

    static void ParseMaterialsFromContent(const std::string& content, std::unordered_map<std::string, OBJMaterial>& materials)
    {
        std::istringstream stream(content);
        std::string line;
        OBJMaterial currentMat;
        bool inMaterial = false;

        while (std::getline(stream, line))
        {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

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
                currentMat = OBJMaterial();
                currentMat.Name = matName;
                inMaterial = true;
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
            else if (command == "Ke" && inMaterial)
            {
                iss >> currentMat.Emission.X >> currentMat.Emission.Y >> currentMat.Emission.Z;
            }
        }

        if (inMaterial && !currentMat.Name.empty())
        {
            materials[currentMat.Name] = currentMat;
        }

        if (materials.empty())
        {
            OBJMaterial defaultMat;
            defaultMat.Name = "default";
            materials["default"] = defaultMat;
        }
    }
};

#undef std_stui