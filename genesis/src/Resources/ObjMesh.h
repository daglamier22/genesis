#pragma once

#include <glm/glm.hpp>

namespace Genesis {
    class ObjMesh {
        public:
            std::vector<float> vertices;
            std::vector<uint32_t> indices;
            std::unordered_map<std::string, uint32_t> history;
            std::unordered_map<std::string, glm::vec3> colors;
            glm::vec3 brushColor;

            std::vector<glm::vec3> v, vn;
            std::vector<glm::vec2> vt;
            glm::mat4 preTransform;

            ObjMesh(std::string objFilepath, std::string mltFilepath, glm::mat4 preTransform);

            void readVertexData(const std::vector<std::string>& words);
            void readTexcoordData(const std::vector<std::string>& words);
            void readNormalData(const std::vector<std::string>& words);
            void readFaceData(const std::vector<std::string>& words);
            void readCorner(const std::string& vertexDescription);
    };
}  // namespace Genesis
