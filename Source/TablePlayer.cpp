#include "TablePlayer.h"

TablePlayer::TablePlayer()
{	
}

void TablePlayer::prepare(const AudioIO::CallbackConfig & cfg)
{
	readPosInt  = std::vector<int>(cfg.frameSize, 0);
	readPosFrac = std::vector<float>(cfg.frameSize, 0);
	writeBuffer = std::vector<float>(cfg.frameSize, 0);

	masterEnv.setAttack(0);
	masterEnv.setRelease(44100 * 0.01);

	if (this->cfg == nullptr) this->cfg = std::unique_ptr<AudioIO::CallbackConfig>( new AudioIO::CallbackConfig(cfg));
	*this->cfg = cfg;

	prepareTablePlayback();
}

void TablePlayer::process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data)
{	
	// return if we don't have a table
	if (!table) return;

	// we bypass overhead from vectors by 
	auto readPosInt = this->readPosInt.data();
	auto readPosFrac = this->readPosFrac.data();
	auto writeBuffer = this->writeBuffer.data();

	// get Data
	auto &tableBins = table->getData();
	auto tableLength	   = table->getEnvelopeLength();
	auto lastValidReadPos = tableLength - 2;

	// prepare write channel, 
	for (int i = 0; i < cfg.frameSize; i++)
	{
		writeBuffer[i] = 0;
		readPos += readInc;
		if (readPos <= lastValidReadPos) 
		{
			readPosInt[i]  = readPos;
			readPosFrac[i] = readPos - readPosInt[i];
		}
		else
		{
			readPosInt[i]  = lastValidReadPos;
			readPosFrac[i] = 0;
		}
	}
	
	auto numBins = std::min(generators.size(), tableBins.size());
	
	for (int i = 0; i < cfg.frameSize; i++)
	{
		for (int f = 0; f <numBins; f++)
		{
			auto &harmonic = tableBins[f]; // create a reference to the amplitude table to lower access overhead
			auto &envelope = harmonic.envelope;
			auto generator = &generators[f];

			float amplitude = (1.f - readPosFrac[i]) * envelope[readPosInt[i]] + (readPosFrac[i]) * envelope[readPosInt[i] + 1];
			float sine = generator->tick();

			writeBuffer[i] += sine * amplitude;
		}
	}

	// apply envelope
	for (int i = 0; i < cfg.frameSize; i++)
	{
		data->write[0][i] += 0.5 * writeBuffer[i] * masterEnv.tick();
	}
}

void TablePlayer::setTable(const ATable * table)
{
	this->table = table; 
	prepareTablePlayback();
}

void TablePlayer::noteOn()
{
	this->readPos = 0;
	this->masterEnv.setGate(1);
}

void TablePlayer::noteOff()
{
	this->masterEnv.setGate(0);
}

void TablePlayer::prepareTablePlayback()
{
	if (cfg == nullptr) return;
	if (table == nullptr) return;

	auto numBins = table->getData().size();

	if (generators.size() < numBins) generators = std::vector<SineGenComplex>(numBins);


	for (int f = 0; f < numBins; f++)
	{
		generators[f] = SineGenComplex(); // reset sine gen
		auto freq = table->getData()[f].frequency * cfg->iSampleRate;
		generators[f].setFrequency((freq < 0.5) ? freq : 0);
		//generators[f].setMaster(&generators[0]);
	}

	readInc = table->getConfig().sampleRate / (cfg->sampleRate * table->getConfig().hopSize);
}


