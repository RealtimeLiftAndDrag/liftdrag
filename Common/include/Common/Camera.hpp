#pragma once



#include "Global.hpp"

#include "glm/gtx/quaternion.hpp"



class Camera {

    public:

    Camera();

    const vec3 & position() const { return m_position; }
    virtual void position(const vec3 & position);

    const quat & orientation() const { return m_orientation; }
    virtual void orientation(const quat & orientation);

    const mat3 & orientMatrix() const { return m_orientMatrix; }
    virtual void orientMatrix(const mat3 & orientMatrix);

    const vec3 & u() const { return m_u; }
    const vec3 & v() const { return m_v; }
    const vec3 & w() const { return m_w; }
    virtual void uvw(const vec3 & u, const vec3 & v, const vec3 & w) final;

    const vec2 & fov() const { return m_fov; }
    void fov(const vec2 & fov);

    float near() const { return m_near; }
    void near(float near);

    float far() const { return m_far; }
    void far(float far);

    const mat4 & viewMat() const;

    const mat4 & projMat() const;

    private:

    vec3 m_position;
    quat m_orientation;
    union {
        mat3 m_orientMatrix;
        struct { vec3 m_u, m_v, m_w; };
    };
    vec2 m_fov; // horizontal and vertical vield of view in radians
    float m_near, m_far; // distance to near and far clipping planes

    mutable mat4 m_viewMat;
    mutable bool m_isViewMatValid;
    mutable mat4 m_projMat;;
    mutable bool m_isProjMatValid;

};

class ThirdPersonCamera : public Camera {

    public:

    ThirdPersonCamera(float minDistance, float maxDistance);

    using Camera::position;
    virtual void position(const vec3 & position) override;

    using Camera::orientation;
    virtual void orientation(const quat & orientation) override;

    using Camera::orientMatrix;
    virtual void orientMatrix(const mat3 & orientMatrix) override;

    float distance() const { return m_distance; }
    void distance(float distance);

    float theta() const { return m_theta; }
    void theta(float theta) { return thetaPhi(theta, m_phi); }

    float phi() const { return m_phi; }
    void phi(float phi) { return thetaPhi(m_theta, phi); }

    void thetaPhi(float theta, float phi);

    float minDistance() const { return m_minDistance; }
    float maxDistance() const { return m_maxDistance; }

    float zoom() const { return m_zoom; }
    void zoom(float zoom);

    private:

    void updateUp();

    void updateDownPosition();

    void updateDownOrientation();

    float m_distance;
    float m_theta; // angle about y axis
    float m_phi; // angle off y axis
    float m_minDistance, m_maxDistance;
    float m_zoom;

};