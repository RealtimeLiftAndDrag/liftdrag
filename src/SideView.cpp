#include "SideView.hpp"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp"

#include "WindowManager.h"
#include "Program.h"



namespace SideView {



static constexpr int k_nSlices = 50;
static constexpr int k_width(720), k_height(480);
static constexpr float k_sliceSize(0.01f); // z distance between slices

static WindowManager * windowManager = nullptr;
static unsigned int * nulldata;
static uint side_geo_tex;
static std::shared_ptr<Program> outlineProg, texProg;
uint outlineTopVAO, outlineBotVAO, outlineTopVBO, outlineBotVBO;
uint boardVAO, boardVBO, boardTexVBO, boardIndVBO;
vec3 outlineTopVerts[k_nSlices];
vec3 outlineBotVerts[k_nSlices];
vec4 screenSpec; // screen width, screen height, x aspect factor, y aspect factor

static bool setupShaders(const std::string & resourcesDir) {
    std::string shadersDir(resourcesDir + "/shaders");

    // Foil Shader -------------------------------------------------------------

    outlineProg = std::make_shared<Program>();
    outlineProg->setVerbose(true);
    outlineProg->setShaderNames(shadersDir + "/lineVert.glsl", shadersDir + "/lineFrag.glsl");
    if (!outlineProg->init()) {
        std::cerr << "Failed to initialize foil shader" << std::endl;
        return false;
    }
    outlineProg->addAttribute("topVertPos");
    outlineProg->addAttribute("botVertPos");
    outlineProg->addUniform("P");
    outlineProg->addUniform("V");
    outlineProg->addUniform("M");

    // FB Shader ---------------------------------------------------------------
    texProg = std::make_shared<Program>();
    texProg->setVerbose(true);
    texProg->setShaderNames(shadersDir + "/fb.vert.glsl", shadersDir + "/fb.frag.glsl");
    if (!texProg->init()) {
        std::cerr << "Failed to initialize fb shader" << std::endl;
        return false;
    }
    texProg->addUniform("tex");
    glUseProgram(texProg->pid);
    glUniform1i(texProg->getUniform("tex"), 0);
    glUseProgram(0);

    return true;
}

bool setup(const std::string & resourcesDir, int sideTexID, GLFWwindow * mainWindow) {
    windowManager = new WindowManager();
    if (!windowManager->init(k_width, k_height, mainWindow)) {
        std::cerr << "Failed to initialize window manager" << std::endl;
        return false;
    }
    side_geo_tex = (uint)sideTexID;

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // Setup shaders
    if (!setupShaders(resourcesDir)) {
        std::cerr << "Failed to setup shaders" << std::endl;
        return false;
    }

    for (int i = 0; i < k_nSlices; i++) {
        outlineTopVerts[i] = vec3(0, 0, -1);
        outlineBotVerts[i] = vec3(0, 0, -1);
    }

    screenSpec.x = float(k_width);
    screenSpec.y = float(k_height);
    if (k_width >= k_height) {
        screenSpec.z = float(k_height) / float(k_width);
        screenSpec.w = 1.0f;
    }
    else {
        screenSpec.z = 1.0f;
        screenSpec.w = float(k_width) / float(k_height);
    }

        


    initGeom();

    return true;
}

bool initGeom() {
    glGenVertexArrays(1, &outlineTopVAO);
    glBindVertexArray(outlineTopVAO);
    glGenBuffers(1, &outlineTopVBO);
    glBindBuffer(GL_ARRAY_BUFFER, outlineTopVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glGenVertexArrays(1, &outlineBotVAO);
    glBindVertexArray(outlineBotVAO);
    glGenBuffers(1, &outlineBotVBO);
    glBindBuffer(GL_ARRAY_BUFFER, outlineBotVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);;

    glGenVertexArrays(1, &boardVAO);
    glBindVertexArray(boardVAO);

    glGenBuffers(1, &boardVBO);
    glBindBuffer(GL_ARRAY_BUFFER, boardVBO);
    GLfloat board_vertices[] = {
        -1.0f, -1.0f, 0.0f, //LD
        1.0f, -1.0f, 0.0f, //RD
        1.0f,  1.0f, 0.0f, //RU
        -1.0f,  1.0f, 0.0f, //LU
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(board_vertices), board_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


    // color
    vec2 board_tex[] = {
        // front colors
        vec2(0.0f, 0.0f),
        vec2(1.0f, 0.0f),
        vec2(1.0f, 1.0f),
        vec2(0.0f, 1.0f),
    };
    glGenBuffers(1, &boardTexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, boardTexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(board_tex), board_tex, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &boardIndVBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boardIndVBO);
    GLushort board_elements[] = {
        // front
        0, 1, 2,
        2, 3, 0,
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(board_elements), board_elements, GL_STATIC_DRAW);
    glBindVertexArray(0);
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }
    return true;
}

void copyOutlineData() {
    glBindVertexArray(outlineTopVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outlineTopVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, &outlineTopVerts, GL_DYNAMIC_DRAW);

    glBindVertexArray(outlineBotVAO);
    glBindBuffer(GL_ARRAY_BUFFER, outlineBotVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, &outlineBotVerts, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void render() {

    glfwMakeContextCurrent(windowManager->getHandle());
    glViewport(0, 0, k_width, k_height);
    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    texProg->bind();
    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, side_geo_tex);
    int x = glGetError();
    //if (glGetError() != GL_NO_ERROR) {
        //std::cerr << "OpenGL error: " << glGetError() << std::endl;
    //}
    glBindVertexArray(boardVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
    texProg->unbind();
        

    ////don't draw outline until we have filled the outline verts at least once
    //if (outlineTopVerts[k_nSlices - 1].z != 0)
    //	return;
    //if (outlineBotVerts[k_nSlices - 1].z != 0)
    //	return;
    //int test;
        

    //copyOutlineData();

    //mat4 V, M, P;

    //P = glm::ortho(-1.f / screenSpec.z, 1.f / screenSpec.z, //left and right
    //	-1.f / screenSpec.w, 1.f / screenSpec.w, //bottom and top
    //	-10.f, 10.f); //near and far

    //V = mat4(1);
    //M = mat4(1);
    //M = glm::translate(mat4(1), vec3(-1, 0, 1));

    //outlineProg->bind();
    //glUniformMatrix4fv(outlineProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
    //glUniformMatrix4fv(outlineProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
    //glUniformMatrix4fv(outlineProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);

    //glBindVertexArray(outlineBotVAO);
    //glDrawArrays(GL_LINE_STRIP, 0, k_nSlices);

    //glBindVertexArray(outlineTopVAO);
    //glDrawArrays(GL_LINE_STRIP, 0, k_nSlices);

    glBindVertexArray(0);
    //uint clearColor[4]{};

    glfwSwapBuffers(windowManager->getHandle());
}

/*void submitOutline(int sliceNum, std::pair<vec3, vec3> outlinePoints)
{
    outlineTopVerts[sliceNum] = vec3(sliceNum*k_sliceSize, outlinePoints.first.y, 0.f);
    outlineBotVerts[sliceNum] = vec3(sliceNum*k_sliceSize, outlinePoints.second.y, 0.f);
}*/

GLFWwindow * getWindow() {
    return windowManager->getHandle();
}



}
