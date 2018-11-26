#include "Viewer.hpp"

#include <iostream>

#include "glad/glad.h"

#include "Common/Shader.hpp"
#include "Common/Camera.hpp"



namespace Viewer {

    static const ivec2 k_minCompSize(100);
    static const float k_fov(glm::radians(90.0f));
    static const float k_camPanAngle(0.01f);
    static const float k_camZoomAmount(0.1f);
    static const float k_camMinDist(0.05f);
    static const float k_camMaxDist(10.0f);

    static unq<Shader> s_objectShader;
    static ThirdPersonCamera s_camera;
    static shr<ViewerComponent> s_component;



    ViewerComponent::ViewerComponent() :
        Single(k_minCompSize, ivec2())
    {}

    void ViewerComponent::render() const {
        glEnable(GL_DEPTH_TEST);
        setViewport();

        s_objectShader->bind();

        s_objectShader->uniform("u_viewMat", s_camera.viewMat());
        s_objectShader->uniform("u_projMat", s_camera.projMat());
        model().draw(modelMat(), normalMat(), s_objectShader->uniformLocation("u_modelMat"), s_objectShader->uniformLocation("u_normalMat"));

        s_objectShader->unbind();
        glDisable(GL_DEPTH_TEST);
    }

    void ViewerComponent::pack() {
        if (size().y <= size().x) {
            s_camera.fov(vec2(k_fov * float(size().x) / float(size().y), k_fov));
        }
        else {
            s_camera.fov(vec2(k_fov, k_fov * float(size().y) / float(size().x)));
        }
    }

    void ViewerComponent::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
        if (ui::isMouseButtonPressed(0)) {
            s_camera.thetaPhi(
                s_camera.theta() + float(delta.x) * k_camPanAngle,
                s_camera.phi() - float(delta.y) * k_camPanAngle
            );
        }
    }

    void ViewerComponent::scrollEvent(const ivec2 & delta) {
        float distP((s_camera.distance() - k_camMinDist) / (k_camMaxDist - k_camMinDist));
        float zoom(std::sqrt(distP));
        zoom = glm::clamp(zoom - float(delta.y) * k_camZoomAmount, 0.0f, 1.0f);
        distP = zoom * zoom;
        s_camera.distance(k_camMinDist + distP * (k_camMaxDist - k_camMinDist));
    }

    bool setup() {
        // Setup object shader
        std::string shaderPath(g_resourcesDir + "/Visualizer/shaders/");
        if (!(s_objectShader = Shader::load(shaderPath + "viewer_object.vert", shaderPath + "viewer_object.frag"))) {
            std::cerr << "Failed to load object shader" << std::endl;
            return false;
        }

        // Setup camera
        s_camera.distance(2.0f);
        s_camera.near(0.01f);
        s_camera.far(100.0f);
        s_camera.fov(vec2(glm::pi<float>() * 0.5f));

        s_component.reset(new ViewerComponent());
    }

    shr<ViewerComponent> component() {
        return s_component;
    }

}