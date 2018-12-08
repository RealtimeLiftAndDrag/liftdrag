#include "PID.hpp"


PID::PID(float _k_p, float _k_i, float _k_d) {
	k_p = _k_p;
	k_i = _k_i;
	k_d = _k_d;
}

float PID::calcCorrection(float setPoint, float procVar, float dt) {
	//get error from previous calculation
	float preErr = err;

	//compute new error value
	err = setPoint - procVar;

	//proportional error
	float p_err = k_p * err;
	//integral error
	float i_err = k_i * (err * dt);
	//derivate error
	float d_err = k_d * ((err - preErr) / dt);

	return p_err + i_err + d_err;
}