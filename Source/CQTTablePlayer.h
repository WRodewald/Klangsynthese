#pragma once

#include "AudioIO.h"
#include "CQTAdditiveTable.h"
#include <fstream>
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#include <complex>

class CQTTablePlayer : public AudioCallbackProvider
{
public:
	CQTTablePlayer();

	virtual void process(AudioIO::CallbackConfig & cfg, AudioIO::CallbackData & data) override;

	void setTable(CQTAdditiveTable *table);

	void prepare(float sampleRate);


	class SineGen
	{
	public:
		SineGen();

		inline void	 setFrequency(double normalizedFrequency);
		inline float tick();
		inline float tickWithFactor(double frequencyFactor);

	private:
		//double phase;
		//double phaseInc;

		std::complex<double>  phase;
		std::complex<double>  phaseInc;
	};


private:

	CQTAdditiveTable *table;
	float readPos; // read position
	float readInc; // per sample read position increment
	int   tableLength;
	bool  tableEndReached;

	std::vector<bool>	 frequencyMask; 
	std::vector<SineGen> generators;

	SineGen				 masterGen;

	float				sampleRate;

	std::ofstream		debugFile;

};

inline CQTTablePlayer::SineGen::SineGen():
	phase(std::complex<double>(1,0)),
	phaseInc(std::complex<double>(0, 0))
{
}

inline void CQTTablePlayer::SineGen::setFrequency(double normalizedFrequency)
{
	this->phaseInc = std::polar<double>(1.,normalizedFrequency * 2. * M_PI);
	//this->phaseInc = normalizedFrequency;
}

inline float CQTTablePlayer::SineGen::tick()
{
	//phase += phaseInc;
	//phase = phase - (phase >= 1); // phase wrap works for frequencies < nyquist

	phase *= phaseInc;

	return phase.imag();
}

inline float CQTTablePlayer::SineGen::tickWithFactor(double frequencyFactor)
{
	//return std::sin(frequencyFactor * phase * 2. * M_PI);


}
