#pragma once

#include <cmath>


class AREnvelope
{
public:
	AREnvelope();

	inline void setGate(bool gate);

	inline void setAttack(float attack);
	inline void setRelease(float release);

	inline float tick();


private:
	
	double value{ 0 };
	bool   gate{ 0 };
	double attackCoeff{ 1. };
	double releaseCoeff{ 1. };
	
};

inline AREnvelope::AREnvelope()
{
	setAttack(1);
	setRelease(1);
}

void AREnvelope::setGate(bool gate)
{
	this->gate = gate;
}

inline void AREnvelope::setAttack(float attack)
{
	if (attack < 0) attack = 0.00001;

	float exponent = std::log10(0.8) / attack;
	attackCoeff = 1. - std::pow(10., exponent);
}

inline void AREnvelope::setRelease(float release)
{
	if (release < 0) release = 0.00001;

	float exponent = std::log10(0.8) / release;
	releaseCoeff = 1. - std::pow(10., exponent);
}

inline float AREnvelope::tick()
{
	auto coeff =  gate * attackCoeff + (!gate) * releaseCoeff;
	return value += (gate - value) * coeff;
}
