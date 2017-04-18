#pragma once

#include "CQTAdditiveTable.h"
#include <fstream>
#include <complex>
#include <memory>

#include "AudioIO.h"

#include "Util/DSPBasics.h"
#include "Util/EnvelopeGen.h"

using DSPBasics::OnePole;
using DSPBasics::SineGen;

class CQTTablePlayer
{
public:
	CQTTablePlayer();

	void prepare(const AudioIO::CallbackConfig & cfg);
	void setTable(const CQTAdditiveTable *table);

	void process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData *data);

	
	void noteOn();
	void noteOff();

private:

	void prepareTablePlayback();

private:

	const CQTAdditiveTable *table{ nullptr };

	float readPos{ 0 }; // read position
	float readInc{ 0 }; // per sample read position increment

	bool resetOnNextProcess{ false };
	bool tablePreparationRequired{ false };
	bool gated{ false };

	// sine generators
	std::vector<SineGen> generators;
	AREnvelope			 masterEnv;
	
	// fast access buffers for read positions
	std::vector<int>	readPosInt;
	std::vector<float>	readPosFrac;
	std::vector<float>  writeBuffer;

	std::unique_ptr<AudioIO::CallbackConfig > cfg;
	
};
