#pragma once

#include "CQTTable.h"
#include <fstream>
#include <complex>
#include <memory>

#include "AudioIO.h"

#include "Util/DSPBasics.h"
#include "Util/EnvelopeGen.h"
#include "Util/QuickMovingAverage.h"

using DSPBasics::OnePole;
using DSPBasics::SineGen;
using DSPBasics::SineGenComplex;

class TablePlayer
{
public:
	TablePlayer();

	void prepare(const AudioIO::CallbackConfig & cfg);
	void setTable(const ATable *table);

	void process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData *data);

	
	void noteOn();
	void noteOff();

private:

	void prepareTablePlayback();

private:

	const ATable *table{ nullptr };

	float readPos{ 0 }; // read position
	float readInc{ 0 }; // per sample read position increment

	bool resetOnNextProcess{ false };
	bool tablePreparationRequired{ false };
	bool gated{ false };

	// sine generators
	std::vector<SineGenComplex>		generators;
	AREnvelope						masterEnv;
	
	// fast access buffers for read positions
	std::vector<int>	readPosInt;
	std::vector<float>	readPosFrac;
	std::vector<float>  writeBuffer;

	std::unique_ptr<AudioIO::CallbackConfig > cfg;
	
};
