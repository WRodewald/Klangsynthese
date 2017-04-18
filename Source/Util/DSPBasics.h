#pragma once
#include <complex>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

namespace DSPBasics
{

	class SineGenComplex
	{
	public:
		SineGenComplex();

		inline void	 setFrequency(double normalizedFrequency);
		inline float tick();

	private:

		std::complex<double>  phase;
		std::complex<double>  phaseInc;
	};

	class SineGen
	{
	public:
		SineGen() = default;

		inline void	 setFrequency(double normalizedFrequency)
		{
			phaseInc = normalizedFrequency;
		}

		inline float tick()
		{
			if (!master)
			{
				phase += phaseInc;
				//phase -= (phase > 1);
			}
			else
			{
				phase = master->phase * (phaseInc / master->phaseInc);
			}

			return std::sin(2. * M_PI * phase);
		}

		void setMaster(SineGen* master)
		{
			this->master = (master != this) ? master : nullptr;
		}

	private:
		SineGen * master{ nullptr };

		float  phase{ 0 };
		float  phaseInc{ 0 };
	};


	class OnePole
	{
	public:
		OnePole() : a1(0), b0(0), z1(0) {};

		inline void  setCutoff(float normalizedCutoff);
		inline void  setState(float state);
		inline float tick(float);

	private:
		float a1;
		float b0;
		float z1;
	};

}




inline DSPBasics::SineGenComplex::SineGenComplex() :
	phase(std::complex<double>(1, 0)),
	phaseInc(std::complex<double>(0, 0))
{
}

inline void DSPBasics::SineGenComplex::setFrequency(double normalizedFrequency)
{
	this->phaseInc = std::polar<double>(1., normalizedFrequency * 2. * M_PI);
}

inline float DSPBasics::SineGenComplex::tick()
{
	phase *= phaseInc;
	return phase.imag();
}

void DSPBasics::OnePole::setCutoff(float normalizedCutoff)
{
	a1 = -std::exp(-2.0 * M_PI * normalizedCutoff);
	b0 = 1. + a1;
}

float DSPBasics::OnePole::tick(float in)
{
	return z1 = b0 * in - a1 * z1;
}

inline void DSPBasics::OnePole::setState(float state)
{
	z1 = state;
}