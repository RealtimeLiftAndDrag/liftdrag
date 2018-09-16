#include "glad/glad.h"
#include "Common/GLSL.h"
#include "stb_image.h"
#include "Common/Program.h"
#include "ProgTerrain.hpp"
#include "Common/Model.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace ProgTerrain {
    // Our shader program
    std::shared_ptr<Program> heightshader, progSky, progWater;

    unq<Model> skySphere;
    static const bool k_drawLines(false);
    static const bool k_drawGrey(false);

    // Contains vertex information for OpenGL
    GLuint TerrainVertexArrayID;
    GLuint WaterVertexArrayID;

    // Data necessary to give our box to OpenGL
    GLuint TerrainPosID, TerrainTexID, IndexBufferIDBox;
    GLuint WaterPosID, WaterTexID, WaterIndexBufferIDBox;

    //texture data
    GLuint GrassTexture, SnowTexture, SandTexture, CliffTexture;
    GLuint SkyTexture, NightTexture;
    GLuint GrassNormal, SnowNormal, SandNormal, CliffNormal;
    float time = 1.0;
    namespace {

        static const int k_meshSize(300);
        static const float k_meshRes(2.f); // Higher value = less verticies per unit of measurement
        void init_mesh() {
            //generate the VAO
            glGenVertexArrays(1, &TerrainVertexArrayID);
            glBindVertexArray(TerrainVertexArrayID);

            //generate vertex buffer to hand off to OGL
            glGenBuffers(1, &TerrainPosID);
            glBindBuffer(GL_ARRAY_BUFFER, TerrainPosID);

            // Size of the net mesh squared (grid) times 4 (verticies per rectangle)
            vec3 *vertices = new vec3[k_meshSize * k_meshSize * 4];

            for (int x = 0; x < k_meshSize; x++)
                for (int z = 0; z < k_meshSize; z++)
                {
                    vertices[x * 4 + z * k_meshSize * 4 + 0] = vec3(0.0, 0.0, 0.0) * k_meshRes + vec3(x, 0, z) * k_meshRes;
                    vertices[x * 4 + z * k_meshSize * 4 + 1] = vec3(1.0, 0.0, 0.0) * k_meshRes + vec3(x, 0, z) * k_meshRes;
                    vertices[x * 4 + z * k_meshSize * 4 + 2] = vec3(1.0, 0.0, 1.0) * k_meshRes + vec3(x, 0, z) * k_meshRes;
                    vertices[x * 4 + z * k_meshSize * 4 + 3] = vec3(0.0, 0.0, 1.0) * k_meshRes + vec3(x, 0, z) * k_meshRes;
                }
            glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * k_meshSize * k_meshSize * 4, vertices, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            //tex coords
            float t = k_meshRes / 100;
            //float t = k_meshRes / 100;

            vec2 *tex = new vec2[k_meshSize * k_meshSize * 4];
            for (int x = 0; x < k_meshSize; x++)
                for (int y = 0; y < k_meshSize; y++)
                {
                    tex[x * 4 + y * k_meshSize * 4 + 0] = vec2(0.0, 0.0) + vec2(x, y) * t;
                    tex[x * 4 + y * k_meshSize * 4 + 1] = vec2(t, 0.0) + vec2(x, y) * t;
                    tex[x * 4 + y * k_meshSize * 4 + 2] = vec2(t, t) + vec2(x, y) * t;
                    tex[x * 4 + y * k_meshSize * 4 + 3] = vec2(0.0, t) + vec2(x, y) * t;
                }
            glGenBuffers(1, &TerrainTexID);
            //set the current state to focus on our vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, TerrainTexID);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * k_meshSize * k_meshSize * 4, tex, GL_STATIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            //free(tex);

            glGenBuffers(1, &IndexBufferIDBox);
            //set the current state to focus on our vertex buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);

            GLuint *elements = new GLuint[k_meshSize * k_meshSize * 6];
            int ind = 0;
            for (int i = 0; i < k_meshSize * k_meshSize * 6; i += 6, ind += 4)
            {
                elements[i + 0] = ind + 0;
                elements[i + 1] = ind + 1;
                elements[i + 2] = ind + 2;
                elements[i + 3] = ind + 0;
                elements[i + 4] = ind + 2;
                elements[i + 5] = ind + 3;
            }
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * k_meshSize * k_meshSize * 6, elements, GL_STATIC_DRAW);
            glBindVertexArray(0);
        }

        static const int k_waterSize(1);
        static const float k_waterRes(600.f);
        void init_water()
        {
            //generate the VAO
            glGenVertexArrays(1, &WaterVertexArrayID);
            glBindVertexArray(WaterVertexArrayID);

            //generate vertex buffer to hand off to OGL
            glGenBuffers(1, &WaterPosID);
            glBindBuffer(GL_ARRAY_BUFFER, WaterPosID);

            // Size of the net mesh squared (grid) times 4 (verticies per rectangle)
            vec3 *vertices = new vec3[k_waterSize * k_waterSize * 4];

            for (int x = 0; x < k_waterSize; x++)
                for (int z = 0; z < k_waterSize; z++)
                {
                    vertices[x * 4 + z * k_waterSize * 4 + 0] = vec3(0.0, 0.0, 0.0) * k_waterRes + vec3(x, 0, z) * k_waterRes;
                    vertices[x * 4 + z * k_waterSize * 4 + 1] = vec3(1.0, 0.0, 0.0) * k_waterRes + vec3(x, 0, z) * k_waterRes;
                    vertices[x * 4 + z * k_waterSize * 4 + 2] = vec3(1.0, 0.0, 1.0) * k_waterRes + vec3(x, 0, z) * k_waterRes;
                    vertices[x * 4 + z * k_waterSize * 4 + 3] = vec3(0.0, 0.0, 1.0) * k_waterRes + vec3(x, 0, z) * k_waterRes;
                }
            glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * k_waterSize * k_waterSize * 4, vertices, GL_DYNAMIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            //tex coords
            float t = k_waterRes / 100;
            //float t = k_waterRes / 100;

            vec2 *tex = new vec2[k_waterSize * k_waterSize * 4];
            for (int x = 0; x < k_waterSize; x++)
                for (int y = 0; y < k_waterSize; y++)
                {
                    tex[x * 4 + y * k_waterSize * 4 + 0] = vec2(0.0, 0.0) + vec2(x, y) * t;
                    tex[x * 4 + y * k_waterSize * 4 + 1] = vec2(t, 0.0) + vec2(x, y) * t;
                    tex[x * 4 + y * k_waterSize * 4 + 2] = vec2(t, t) + vec2(x, y) * t;
                    tex[x * 4 + y * k_waterSize * 4 + 3] = vec2(0.0, t) + vec2(x, y) * t;
                }
            glGenBuffers(1, &WaterTexID);
            //set the current state to focus on our vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, WaterTexID);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * k_waterSize * k_waterSize * 4, tex, GL_STATIC_DRAW);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            //free(tex);

            glGenBuffers(1, &WaterIndexBufferIDBox);
            //set the current state to focus on our vertex buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WaterIndexBufferIDBox);

            GLuint *elements = new GLuint[k_waterSize * k_waterSize * 6];
            int ind = 0;
            for (int i = 0; i < k_waterSize * k_waterSize * 6; i += 6, ind += 4)
            {
                elements[i + 0] = ind + 0;
                elements[i + 1] = ind + 1;
                elements[i + 2] = ind + 2;
                elements[i + 3] = ind + 0;
                elements[i + 4] = ind + 2;
                elements[i + 5] = ind + 3;
            }
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * k_waterSize * k_waterSize * 6, elements, GL_STATIC_DRAW);
            glBindVertexArray(0);
        }

        void init_textures(const std::string & textureDir) {
            int width, height, channels;
            char filepath[1000];
            std::string str;
            unsigned char* data;

            // Grass texture
            str = textureDir + "/grass.jpg";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &GrassTexture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, GrassTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Grass normal map
            str = textureDir + "/grass_normal.png";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &GrassNormal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, GrassNormal);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);



            // Snow texture
            str = textureDir + "/snow.jpg";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &SnowTexture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, SnowTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Snow normal map
            str = textureDir + "/snow_normal.png";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &SnowNormal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, SnowNormal);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);



            // Sand texture
            str = textureDir + "/sand.jpg";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &SandTexture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, SandTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Sand normal map
            str = textureDir + "/sand_normal.png";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &SandNormal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, SandNormal);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Cliff texture
            str = textureDir + "/cliff.jpg";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &CliffTexture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, CliffTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Cliff normal map
            str = textureDir + "/cliff_normal.png";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &CliffNormal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, CliffNormal);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Skybox Texture
            str = textureDir + "/sky.jpg";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &SkyTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, SkyTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Skybox Texture
            str = textureDir + "/sky2.jpg";
            strcpy_s(filepath, str.c_str());
            data = stbi_load(filepath, &width, &height, &channels, 4);
            glGenTextures(1, &NightTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, NightTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        void assign_textures() {
            //[TWOTEXTURES]
            //set the 2 textures to the correct samplers in the fragment shader:
            GLuint TextureLocation, GrassTextureLocation, SnowTextureLocation, SandTextureLocation, CliffTextureLocation, SkyTextureLocation, NightTextureLocation;
            GLuint GrassNormalLocation, SnowNormalLocation, SandNormalLocation, CliffNormalLocation;

            GrassTextureLocation = glGetUniformLocation(heightshader->pid, "grassSampler");
            GrassNormalLocation = glGetUniformLocation(heightshader->pid, "grassNormal");
            SnowTextureLocation = glGetUniformLocation(heightshader->pid, "snowSampler");
            SnowNormalLocation = glGetUniformLocation(heightshader->pid, "snowNormal");
            SandTextureLocation = glGetUniformLocation(heightshader->pid, "sandSampler");
            SandNormalLocation = glGetUniformLocation(heightshader->pid, "sandNormal");
            CliffTextureLocation = glGetUniformLocation(heightshader->pid, "cliffSampler");
            CliffNormalLocation = glGetUniformLocation(heightshader->pid, "cliffNormal");
            // Then bind the uniform samplers to texture units:
            glUseProgram(heightshader->pid);
            glUniform1i(GrassTextureLocation, 0);
            glUniform1i(SnowTextureLocation, 1);
            glUniform1i(SandTextureLocation, 2);
            glUniform1i(CliffTextureLocation, 3);
            glUniform1i(CliffNormalLocation, 4);
            glUniform1i(SnowNormalLocation, 5);
            glUniform1i(GrassNormalLocation, 6);
            glUniform1i(SandNormalLocation, 7);

            SkyTextureLocation = glGetUniformLocation(progSky->pid, "dayTexSampler");
            NightTextureLocation = glGetUniformLocation(progSky->pid, "nightTexSampler");
            glUseProgram(progSky->pid);
            glUniform1i(SkyTextureLocation, 0);
            glUniform1i(NightTextureLocation, 1);
        }
    }
    void init_shaders(const std::string &resourceDir) {
        GLSL::checkVersion();

        const std::string shaderDir(resourceDir + "/shaders");


        // Initialize the GLSL program.
        heightshader = std::make_shared<Program>();
        heightshader->setVerbose(true);
        heightshader->setShaderNames(shaderDir + "/height_vertex.glsl", shaderDir + "/height_frag.glsl", shaderDir + "/tesscontrol.glsl", shaderDir + "/tesseval.glsl");
        if (!heightshader->init())
        {
            std::cerr << "Heightmap shaders failed to compile... exiting!" << std::endl;
            int hold;
            std::cin >> hold;
            exit(1);
        }
        heightshader->addUniform("P");
        heightshader->addUniform("V");
        heightshader->addUniform("M");
        heightshader->addUniform("camoff");
        heightshader->addUniform("campos");
        heightshader->addUniform("time");
        heightshader->addUniform("resolution");
        heightshader->addUniform("meshsize");
        heightshader->addUniform("drawGrey");
        heightshader->addAttribute("vertPos");
        heightshader->addAttribute("vertTex");

        // Initialize the GLSL progSkyram.
        progSky = std::make_shared<Program>();
        progSky->setVerbose(true);
        progSky->setShaderNames(shaderDir + "/skyvertex.glsl", shaderDir + "/skyfrag.glsl");
        if (!progSky->init())
        {
            std::cerr << "Skybox shaders failed to compile... exiting!" << std::endl;
            int hold;
            std::cin >> hold;
            exit(1);
        }
        progSky->addUniform("P");
        progSky->addUniform("V");
        progSky->addUniform("M");
        progSky->addUniform("campos");
        progSky->addUniform("time");
        progSky->addAttribute("vertPos");
        progSky->addAttribute("vertTex");

        // Initialize the GLSL program.
        progWater = std::make_shared<Program>();
        progWater->setVerbose(true);
        progWater->setShaderNames(shaderDir + "/water_vertex.glsl", shaderDir + "/water_fragment.glsl");
        if (!progWater->init())
        {
            std::cerr << "Water shaders failed to compile... exiting!" << std::endl;
            int hold;
            std::cin >> hold;
            exit(1);
        }
        progWater->addUniform("P");
        progWater->addUniform("V");
        progWater->addUniform("M");
        progWater->addUniform("camoff");
        progWater->addUniform("campos");
        progWater->addUniform("time");
        progWater->addAttribute("vertPos");
        progWater->addAttribute("vertTex");
    }



    void init_geom(const std::string & resourceDir)
    {
        const std::string shaderDir = resourceDir + "/shaders";

        init_mesh();
        init_water();

        skySphere = Model::load(resourceDir + "/models/sphere.obj");

        init_textures(resourceDir + "/textures");
        assign_textures();

    }

    void drawSkyBox(const mat4 &V, const mat4 &P, const vec3 &camPos) {
        float sangle = 3.1415926 / 2.;

        mat4 RotateX = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
        vec3 camp = -camPos;
        mat4 TransXYZ = glm::translate(glm::mat4(1.0f), camPos);
        mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 3.0f));

        mat4 M = TransXYZ * S;


        progSky->bind();
        glUniformMatrix4fv(progSky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progSky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(progSky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform3fv(progSky->getUniform("campos"), 1, &camPos[0]);
        glUniform1f(progSky->getUniform("time"), time);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SkyTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, NightTexture);
        skySphere->draw();

        progSky->unbind();
    }

    void drawWater(const mat4 &V, const mat4 &P, const vec3 &camPos, const float &centerOffset, const vec3 &offset) {
        // Draw the Water -----------------------------------------------------------------

        progWater->bind();
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
        mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(centerOffset, 2.0f, centerOffset));

        glUniformMatrix4fv(progWater->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(progWater->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(progWater->getUniform("V"), 1, GL_FALSE, &V[0][0]);

        glUniform3fv(progWater->getUniform("camoff"), 1, &offset[0]);
        glUniform3fv(progWater->getUniform("campos"), 1, &camPos[0]);
        glUniform1f(progWater->getUniform("time"), time);
        glBindVertexArray(WaterVertexArrayID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, WaterIndexBufferIDBox);
        //glDisable(GL_DEPTH_TEST);
        // Must use gl_patches w/ tessalation
        //glPatchParameteri(GL_PATCH_VERTICES, 3.0f);
        glDrawElements(GL_TRIANGLES, k_meshSize*k_meshSize * 6, GL_UNSIGNED_INT, (void*)0);
        //glEnable(GL_DEPTH_TEST);
        progWater->unbind();
    }

    void drawTerrain(const mat4 &V, const mat4 &P, const vec3 &camPos, const float &centerOffset, const vec3 &offset) {
        // Draw the terrain -----------------------------------------------------------------
        heightshader->bind();
        mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(centerOffset, 0.0f, centerOffset));
        glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniformMatrix4fv(heightshader->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(heightshader->getUniform("V"), 1, GL_FALSE, &V[0][0]);



        glUniform3fv(heightshader->getUniform("camoff"), 1, &offset[0]);
        glUniform3fv(heightshader->getUniform("campos"), 1, &camPos[0]);
        glUniform1f(heightshader->getUniform("time"), time);
        glUniform1i(heightshader->getUniform("meshsize"), k_meshSize);
        glUniform1f(heightshader->getUniform("resolution"), k_meshRes);
        glUniform1i(heightshader->getUniform("drawGrey"), k_drawGrey);
        glBindVertexArray(TerrainVertexArrayID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GrassTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, SnowTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, SandTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, CliffTexture);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, CliffNormal);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, SnowNormal);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, GrassNormal);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, SandNormal);

        glPatchParameteri(GL_PATCH_VERTICES, 3.0f);
        glDrawElements(GL_PATCHES, k_meshSize*k_meshSize * 6, GL_UNSIGNED_INT, (void*)0);


        heightshader->unbind();

    }

    void render(const mat4 &V, const mat4 &P, const vec3 &camPos) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawSkyBox(V, P, camPos);
        glEnable(GL_DEPTH_TEST);
        float centerOffset = -k_meshSize * k_meshRes / 2.0f;
        vec3 offset = camPos;
        offset.y = 0;
        offset.x = ((int)(offset.x / k_meshRes)) * k_meshRes;
        offset.z = ((int)(offset.z / k_meshRes)) * k_meshRes;

        if (!k_drawGrey || !k_drawLines) {
            drawWater(V, P, camPos, centerOffset, offset);
        }
        drawTerrain(V, P, camPos, centerOffset, offset);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    }
}
