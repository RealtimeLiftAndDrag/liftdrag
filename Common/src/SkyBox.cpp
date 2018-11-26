#include "SkyBox.hpp"

#include <iostream>
#include "glad/glad.h"
#include "stb/stb_image.h"
#include "Shader.hpp"
#include "GLInterface.hpp"



static bool s_isSetup;
static unq<Shader> s_shader;
static u32 s_vao, s_vbo, s_ibo;

static bool setup() {
    std::string shadersPath(g_resourcesDir + "/Common/shaders/");
    if (!(s_shader = Shader::load(shadersPath + "skybox.vert", shadersPath + "skybox.frag"))) {
        std::cerr << "Failed to load shader" << std::endl;
        return false;
    }

    vec3 positions[8]{
        { -1.0f, -1.0f, -1.0f },
        {  1.0f, -1.0f, -1.0f },
        { -1.0f,  1.0f, -1.0f },
        {  1.0f,  1.0f, -1.0f },
        { -1.0f, -1.0f,  1.0f },
        {  1.0f, -1.0f,  1.0f },
        { -1.0f,  1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f }
    };
    u32 indices[36]{
        0, 2, 3, 3, 1, 0,
        0, 1, 5, 5, 4, 0,
        0, 4, 6, 6, 2, 0,
        7, 6, 4, 4, 5, 7,
        7, 5, 1, 1, 3, 7,
        7, 3, 2, 2, 6, 7
    };

    // Create VBO
    glGenBuffers(1, &s_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (gli::checkError()) {
        std::cerr << "Failed to create vbo" << std::endl;
        return false;
    }

    // Create IBO
    glGenBuffers(1, &s_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    if (gli::checkError()) {
        std::cerr << "Failed to create ibo" << std::endl;
        return false;
    }

    // Create VAO
    glGenVertexArrays(1, &s_vao);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ibo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    if (gli::checkError()) {
        std::cerr << "Failed to create vao" << std::endl;
        return false;
    }

    return true;
}



unq<SkyBox> SkyBox::create(std::initializer_list<std::string_view> files) {
    if (!s_isSetup) {
        if (!setup()) {
            std::cerr << "Failed static setup" << std::endl;
            return {};
        }
    }

    // Create cubemap
    u32 cubemap(0);
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

    int size(0);
    for (int fi(0); fi < 6; ++fi) {
        std::string_view file(files.begin()[fi]);
        ivec2 texSize;
        unsigned char * texData(stbi_load(file.data(), &texSize.x, &texSize.y, nullptr, 3));
        if (!texData) {
            std::cerr << stbi_failure_reason() << std::endl;
            std::cerr << "Failed to load skybox texture: " << file << std::endl;
            return {};
        }
        if (texSize.x != texSize.y) {
            std::cerr << "Face textures must be square" << std::endl;
            return {};
        }
        if (fi == 0) size = texSize.x;
        if (texSize.x != size) {
            std::cerr << "All face textures must be the same size" << std::endl;
            return {};
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + fi, 0, GL_RGB, texSize.x, texSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
        stbi_image_free(texData);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    if (gli::checkError()) {
        return {};
    }

    return unq<SkyBox>(new SkyBox(cubemap));
}

void SkyBox::render(const mat4 & viewMat, const mat4 & projMat) const {
    s_shader->bind();
    s_shader->uniform("u_viewMat", viewMat);
    s_shader->uniform("u_projMat", projMat);

    glCullFace(GL_FRONT);
    glDepthFunc(GL_LEQUAL);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap);
    glBindVertexArray(s_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);

    s_shader->unbind();
}

SkyBox::SkyBox(u32 cubemap) :
    m_cubemap(cubemap)
{}