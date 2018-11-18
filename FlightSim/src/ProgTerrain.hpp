#include "Common/Global.hpp"
#include <iostream>
namespace ProgTerrain {
    namespace { //private functions
        void init_mesh();
        void init_water();
        void init_textures();
        void assign_textures();
    }
    void init_shaders();
    void init_geom();
    // Expects blending and depth testing to be enabled
    void render(const mat4 &V, const mat4 &P, const vec3 &camPos);
}