#include "SimObject.hpp"
#include "glm/gtc/matrix_transform.hpp"

SimObject::SimObject() {
    gravityOn = true;
    timeScale = 1.f;
    mass = 1.f;

    pos = vec3(0);
    vel = vec3(0);
    acc = vec3(0);

    a_pos = vec3(0);
    a_vel = vec3(0);
    a_acc = vec3(0);
}

mat4 SimObject::getTransform() {
    return getTranslate() * getRotate();
}

void SimObject::addTranslationalForce(vec3 force) {
    acc += (force / mass);
}

void SimObject::addAngularForce(vec3 force) {
    a_acc.x += force.x / 3613011.7;
    a_acc.y += force.y / 23766233.8;
    a_acc.z += force.z / 26696229.2;
}

void SimObject::applyThrust() {
	if (thrust < 0.01) {
		thrustVal = 0;
		return;
	}
	thrustVal = thrust * maxThrust; //maxThrust is in Newtons
	vec3 thrustForce = vec3(getRotate() * vec4(0, 0, -thrustVal, 1));
	addTranslationalForce(thrustForce);
}

void SimObject::update(float frametime) {
	applyThrust();

	vel += acc * frametime;
    pos += vel * frametime;

    a_vel += a_acc * frametime;
    a_pos += a_vel * frametime;

    acc = vec3(0);
    a_acc = vec3(0);

    if (gravityOn)
        acc += vec3(0, -9.8, 0);
}

void SimObject::setGravityOn(bool _gravityOn) {
    gravityOn = _gravityOn;
}

void SimObject::setMass(float _mass) {
    mass = _mass;
}

void SimObject::setMaxThrust(float _maxThrust)
{
	maxThrust = _maxThrust;
}

void SimObject::setTimeScale(float _timeScale)
{
    timeScale = _timeScale;
}

mat4 SimObject::getTranslate()
{
    return glm::translate(mat4(1), pos);
}

mat4 SimObject::getRotate()
{
    mat4 R = mat4(1);
    R = glm::rotate(R, a_pos.x, vec3(1, 0, 0));
    R = glm::rotate(R, a_pos.y, vec3(0, 1, 0));
    R = glm::rotate(R, a_pos.z, vec3(0, 0, 1));
    return R;
}

float SimObject::getThrustVal()
{
	return thrustVal;
}

