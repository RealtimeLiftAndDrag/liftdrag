#pragma once

#include "Common/Global.hpp"

class SimObject {
    vec3 acc;
    vec3 a_vel;
    vec3 a_acc;
    float mass;
    bool gravityOn;
    float maxThrust;
    float thrustVal;
public:
    vec3 pos;
    vec3 vel;
    vec3 a_pos;
    float thrust; //val between 0 and 1

    SimObject();
    mat4 getTransform();
    void addTranslationalForce(vec3 force);
    void addAngularForce(vec3 force);
    void applyThrust();
    void update(float time);
    void setGravityOn(bool _gravityOn);
    void setMass(float _mass);
    void setMaxThrust(float _maxThrust);
    mat4 getTranslate();
    mat4 getRotate();
    float getThrustVal();
};