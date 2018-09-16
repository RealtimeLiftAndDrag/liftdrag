#include "Common/Global.hpp"
#include <iostream>
namespace ProgTerrain {
    namespace { //private functions
        void init_mesh();
        void init_water();
        void init_textures(const std::string & resourceDirectory);
        void assign_textures();
    }
    void init_shaders(const std::string & resourceDirectory);
    void init_geom(const std::string & resourceDirectory);
    void render(const mat4 &V, const mat4 &P, const vec3 &camPos);
}