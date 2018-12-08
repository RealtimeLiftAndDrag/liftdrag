#pragma once

class PID {
	//error values
	float err;

	//coefficients
	float k_p;
	float k_i;
	float k_d;

public:
	PID(float _k_p, float _k_i, float _k_d);

	//setPoint is point we want to reach
	//procVar is point we are currently at
	float calcCorrection(float setPoint, float procVar, float dt);
};