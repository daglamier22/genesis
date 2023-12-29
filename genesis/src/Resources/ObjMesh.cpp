#include "ObjMesh.h"

#include "Utils.h"

namespace Genesis {
    ObjMesh::ObjMesh(std::string objFilepath, std::string mltFilepath, glm::mat4 preTransform) {
        this->preTransform = preTransform;

        std::ifstream file;
        file.open(mltFilepath);
        std::string line;
        std::string materialName;
        std::vector<std::string> words;

        while (std::getline(file, line)) {
            words = split(line, " ");

            if (words[0].compare("newmtl") == 0) {
                materialName = words[1];
            }

            if (words[0].compare("Kd") == 0) {
                brushColor = glm::vec3(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]));
                colors.insert({materialName, brushColor});
            }
        }
        file.close();

        file.open(objFilepath);

        while (std::getline(file, line)) {
            words = split(line, " ");

            if (words[0].compare("v") == 0) {
                readVertexData(words);
            }

            if (words[0].compare("vt") == 0) {
                readTexcoordData(words);
            }

            if (words[0].compare("vn") == 0) {
                readNormalData(words);
            }

            if (words[0].compare("usemtl") == 0) {
                if (colors.contains(words[1])) {
                    brushColor = colors[words[1]];
                } else {
                    brushColor = glm::vec3(1.0f);
                }
            }

            if (words[0].compare("f") == 0) {
                readFaceData(words);
            }
        }
        file.close();
    }

    void ObjMesh::readVertexData(const std::vector<std::string>& words) {
        glm::vec4 newVertex = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 1.0f);
        glm::vec3 transformedVertex = glm::vec3(preTransform * newVertex);
        v.push_back(transformedVertex);
    }

    void ObjMesh::readTexcoordData(const std::vector<std::string>& words) {
        glm::vec2 newTexcoord = glm::vec2(std::stof(words[1]), std::stof(words[2]));
        vt.push_back(newTexcoord);
    }

    void ObjMesh::readNormalData(const std::vector<std::string>& words) {
        glm::vec4 newNormal = glm::vec4(std::stof(words[1]), std::stof(words[2]), std::stof(words[3]), 0.0f);
        glm::vec3 transformedNormal = glm::vec3(preTransform * newNormal);
        vn.push_back(transformedNormal);
    }

    void ObjMesh::readFaceData(const std::vector<std::string>& words) {
        size_t triangleCount = words.size() - 3;

        for (int i = 0; i < triangleCount; ++i) {
            readCorner(words[1]);
            readCorner(words[2 + i]);
            readCorner(words[3 + i]);
        }
    }

    void ObjMesh::readCorner(const std::string& vertexDescription) {
        if (history.contains(vertexDescription)) {
            indices.push_back(history[vertexDescription]);
            return;
        }

        uint32_t index = static_cast<uint32_t>(history.size());
        history.insert({vertexDescription, index});
        indices.push_back(index);

        std::vector<std::string> v_vt_vn = split(vertexDescription, "/");

        // position
        glm::vec3 pos = v[std::stol(v_vt_vn[0]) - 1];
        vertices.push_back(pos[0]);
        vertices.push_back(pos[1]);
        vertices.push_back(pos[2]);

        // color
        vertices.push_back(brushColor[0]);
        vertices.push_back(brushColor[1]);
        vertices.push_back(brushColor[2]);

        // texcoord
        glm::vec2 texcoord = glm::vec2(0.0f, 0.0f);
        if (v_vt_vn.size() == 3 && v_vt_vn[1].size() > 0) {
            texcoord = vt[std::stol(v_vt_vn[1]) - 1];
        }
        vertices.push_back(texcoord[0]);
        vertices.push_back(texcoord[1]);

        // normal
        glm::vec3 normal = vn[std::stol(v_vt_vn[2]) - 1];
        vertices.push_back(normal[0]);
        vertices.push_back(normal[1]);
        vertices.push_back(normal[2]);
    }
}  // namespace Genesis
