#include "CQTTablePlayer.h"

CQTTablePlayer::CQTTablePlayer()
{	
}

void CQTTablePlayer::prepare(const AudioIO::CallbackConfig & cfg)
{
	readPosInt  = std::vector<int>(cfg.frameSize, 0);
	readPosFrac = std::vector<float>(cfg.frameSize, 0);
	writeBuffer = std::vector<float>(cfg.frameSize, 0);

	masterEnv.setAttack(44100  * 0.01);
	masterEnv.setRelease(44100 * 0.01);

	if (this->cfg == nullptr) this->cfg = std::make_unique<AudioIO::CallbackConfig>(cfg);
	*this->cfg = cfg;

	prepareTablePlayback();
}

void CQTTablePlayer::process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data)
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
			SineGen * generator = &generators[f];

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

void CQTTablePlayer::setTable(const CQTAdditiveTable * table)
{
	this->table = table; 
	prepareTablePlayback();
}

void CQTTablePlayer::noteOn()
{
	this->readPos = 0;
	this->masterEnv.setGate(1);
}

void CQTTablePlayer::noteOff()
{
	this->masterEnv.setGate(0);
}

void CQTTablePlayer::prepareTablePlayback()
{
	if (cfg == nullptr) return;
	if (table == nullptr) return;

	auto numBins = table->getData().size();

	if (generators.size() < numBins) generators = std::vector<SineGen>(numBins);


	for (int f = 0; f < numBins; f++)
	{
		generators[f] = SineGen(); // reset sine gen
		generators[f].setFrequency(table->getData()[f].frequency * cfg->iSampleRate);
		generators[f].setMaster(&generators[0]);
	}

	readInc = table->getSampleRate() / (cfg->sampleRate * table->getHopSize());
}


