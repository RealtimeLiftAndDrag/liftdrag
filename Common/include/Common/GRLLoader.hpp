#include "Global.hpp"

#include <vector>



namespace grl {

    struct Object {
        std::string name;
        std::vector<glm::vec3> posData;
        std::vector<glm::vec3> norData;
        std::vector<glm::vec2> texData;
        glm::mat4 originMat;
    };

    std::vector<Object> load(const std::string & filename);

}