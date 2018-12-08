#pragma once

#include "glm/gtx/quaternion.hpp"

#include "Common/Global.hpp"

class SimObject {

    public:

    // `direction` must be normalized and should not be +-z
    SimObject(float mass, const vec3 & momentsOfInertia, float dryThrust, const vec3 & position, const vec3 & direction, float speed, float thrust);

    void update(float time);

    void reset(const vec3 & position, const vec3 & direction, float speed);

    // `force` is in world space
    void addTranslationalForce(const vec3 & force);

    // `force` is in world space
    void addAngularForce(const vec3 & force);

    const vec3 & position() const { return m_position; }

    const vec3 & velocity() const { return m_velocity; }

    const vec3 & acceleration() const { return m_acceleration; }

    const quat & orientation() const { return m_orientation; }

    const mat3 & orientMatrix() const { return m_orientMatrix; }

    const vec3 & u() const { return m_u; }
    const vec3 & v() const { return m_v; }
    const vec3 & w() const { return m_w; }

    void thrust(float thrust);
    float thrust() const { return m_thrust; }

    const float k_mass, k_invMass;
    const vec3 k_momentsOfInertia; // moments of inertia for pitch, yaw, and roll
    const float k_dryThrust; // TODO: wet thrust

    private:

    vec3 m_position;
    vec3 m_velocity;
    vec3 m_acceleration;
    quat m_orientation;
    union {
        mat3 m_orientMatrix;
        struct { vec3 m_u, m_v, m_w; };
    };
    vec3 m_angularVel;
    vec3 m_angularAcc;
    float m_thrust; // Between 0 and 1

};