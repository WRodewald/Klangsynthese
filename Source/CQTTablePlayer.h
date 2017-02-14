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
	CQTTablePlayer(unsigned int frameSize, float freqBinThreshold = 0);

	virtual void process(AudioIO::CallbackConfig & cfg, AudioIO::CallbackData & data) override;

	void setTable(CQTAdditiveTable *table);

	void prepare(float sampleRate);


	class SineGen
	{
	public:
		SineGen();

		inline void	 setFrequency(double normalizedFrequency);
		inline float tick();

	private:

		std::complex<double>  phase;
		std::complex<double>  phaseInc;
	};


private:

	CQTAdditiveTable *table;
	float readPos; // read position
	float readInc; // per sample read position increment
	int   tableLength;
	bool  tableEndReached;

	float				 freqBinThreshold; // threshold value over which frequency bins are processed
	std::vector<bool>	 freqBinMask;	   // mask specifies which frequency bins are being processed

	std::vector<SineGen> generators;

	SineGen				 masterGen;

	float				sampleRate;

	std::ofstream		debugFile;

	// fast access buffers for read positions
	int  *				readPosInt;
	float *				readPosFrac;



};

inline CQTTablePlayer::SineGen::SineGen():
	phase(std::complex<double>(1,0)),
	phaseInc(std::complex<double>(0, 0))
{
}

inline void CQTTablePlayer::SineGen::setFrequency(double normalizedFrequency)
{
	this->phaseInc = std::polar<double>(1.,normalizedFrequency * 2. * M_PI);
}

inline float CQTTablePlayer::SineGen::tick()
{
	phase *= phaseInc;
	return phase.imag();
}


