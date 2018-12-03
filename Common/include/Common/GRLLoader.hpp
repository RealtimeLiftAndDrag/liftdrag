#pragma once

#include "Global.hpp"

#include <vector>

#include "Common/Model.hpp"



namespace grl {

    struct Object {
        std::string name;
        std::vector<HardMesh::Vertex> vertices;
        glm::mat4 originMat;
    };

    std::vector<Object> load(const std::string & filename);

}