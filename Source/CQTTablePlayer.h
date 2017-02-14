#pragma once

#include "AudioIO.h"
#include "MidiIo.h"
#include "CQTAdditiveTable.h"
#include <fstream>
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#include <complex>


class CQTTablePlayer : public AudioCallbackProvider , public MidiListener
{
public:
	CQTTablePlayer(unsigned int frameSize, float freqBinThreshold = 0);

	virtual void process(AudioIO::CallbackConfig & cfg, AudioIO::CallbackData & data) override;

	void setTable(CQTAdditiveTable *table);

	void prepare(float sampleRate);

	virtual void noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel);
	virtual void noteOff(double timeStamp, unsigned char ch, unsigned char note);
	virtual void midiEvent(double timeStamp, std::vector<unsigned char> *message);

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

private:

	CQTAdditiveTable *table;
	float readPos; // read position
	float readInc; // per sample read position increment
	int   tableLength;
	bool  tableEndReached;

	float				 freqBinThreshold; // threshold value over which frequency bins are processed
	std::vector<bool>	 freqBinMask;	   // mask specifies which frequency bins are being processed

	// sine generators
	std::vector<SineGen> generators;
	OnePole				 masterAmp;

	float				sampleRate;


	// fast access buffers for read positions
	int  *				readPosInt;
	float *				readPosFrac;

	bool				resetOnNextProcess;




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



void CQTTablePlayer::OnePole::setCutoff(float normalizedCutoff)
{
	a1 = -std::exp(-2.0 * M_PI * normalizedCutoff);
	b0 = 1. + a1;
}

float CQTTablePlayer::OnePole::tick(float in)
{
	return z1 = b0 * in - a1 * z1;
}

inline void CQTTablePlayer::OnePole::setState(float state)
{
	z1 = state;
}