#include "CQTTablePlayer.h"
#include <iostream>
#include <fstream>
#include <cstring>
CQTTablePlayer::CQTTablePlayer(unsigned int frameSize, float freqBinThreshold)
	:
	table(NULL),
	sampleRate(44100),
	readPos(0),
	readInc(0),
	tableLength(0),
	tableEndReached(false),
	freqBinThreshold(freqBinThreshold),
	resetOnNextProcess(false)
{
	readPosInt  = new int[frameSize];
	readPosFrac = new float[frameSize];
	for (int i = 0; i < frameSize; i++)
	{
		readPosInt[i]  = 0;
		readPosFrac[i] = 0;
	}
}

void CQTTablePlayer::process(AudioIO::CallbackConfig & cfg, AudioIO::CallbackData & data)
{
	
	// update stuff if sample rate changed
	if (cfg.sampleRate != sampleRate) prepare(cfg.sampleRate);

	// prepare write channel, 
	for (int i = 0; i < cfg.frameSize; i++)
	{
		data.write[0][i] = 0;
		readPos += readInc;
		readPosInt[i] = readPos;
		readPosFrac[i] = readPos - readPosInt[i];
	}

	// return if we don't have a table
	if (!table) return;
	
	if (resetOnNextProcess)
	{
		resetOnNextProcess = false;
		tableEndReached = false;
		readPos = 0;
		masterAmp.setState(0);
	}

	// following bracket calculates for the first sine, updating the read positions for all the other sines (as they are the same
	tableEndReached = (readPos >= tableLength);
	if (tableEndReached) readPos = tableLength; 
	
	// process sines
	if (!tableEndReached)
	{
		float *write = data.write[0];
		for (int f = 0; f < generators.size(); f++)
		{
			if (!freqBinMask[f]) continue; // skip bin if mased out

			std::vector<float> *fTableAmplitudeVector = &table->amplitudeTable[f]; // create a reference to the amplitude table to lower access overhead
			SineGen * generator = &generators[f];

			for (int i = 0; i < cfg.frameSize; i++)
			{
				float amplitude = (1.f - readPosFrac[i]) * (*fTableAmplitudeVector)[readPosInt[i]] + (readPosFrac[i]) * (*fTableAmplitudeVector)[readPosInt[i] + 1];
				float sine = generator->tick();

				write[i] += sine * amplitude;
			}
		}
	}

	for (int i = 0; i < cfg.frameSize; i++)
	{
		data.write[0][i] *= masterAmp.tick(1);
	}

	// copy first channel over to second channel (or any other if available)
	for (int c = 1; c < cfg.outChannels; c++)
	{
		std::memcpy((void*)data.write[c], (void*)data.write[0], 
cfg.frameSize * sizeof(float));
	}

	

}

void CQTTablePlayer::setTable(CQTAdditiveTable * table)
{
	this->table = table;

	generators.clear();
	freqBinMask.clear();

	if (table)
	{
		unsigned int numFrequencies = table->frequencyTable.size();
		for (int f = 0; f < numFrequencies; f++)
		{
			freqBinMask.push_back(true);
			generators.push_back(SineGen());
		}

		tableEndReached = false;
		tableLength = table->amplitudeTable[0].size();

		if (freqBinThreshold > 0)
		{
			for (int f = 0; f < table->amplitudeTable.size(); f++)
			{
				float max = 0;
				for (int i = 0; i < table->amplitudeTable[f].size(); i++)
				{
					if (table->amplitudeTable[f][i] > max) max = table->amplitudeTable[f][i];
				}
				if (max < freqBinThreshold) 
				{
					freqBinMask[f] = false;
				}
			}
		}
	}
	std::cout << std::endl;
	std::cout << "CQT Table Player: Table prepared" << std::endl;
	if (freqBinThreshold)
	{
		int activeBinCount = 0;
		for (int f = 0; f < freqBinMask.size(); f++) activeBinCount += freqBinMask[f];
		std::cout << "Threshold set to: " << freqBinThreshold << " leaving " <<  activeBinCount << "/" << freqBinMask.size()  <<" bins active." << std::endl;

	}

	std::cout << std::endl;
	
	prepare(sampleRate);
}

void CQTTablePlayer::prepare(float sampleRate)
{
	masterAmp.setCutoff(10.f / sampleRate);
	masterAmp.setState(0);
	
	if (!table) return;

	this->sampleRate = sampleRate;

	double iSampleRate = 1. / sampleRate;

	if (table)
	{
		unsigned int numFrequencies = std::min(table->frequencyTable.size(), generators.size());

		for (int f = 0; f < numFrequencies; f++)
		{
			generators[f].setFrequency(table->frequencyTable[f] * iSampleRate);
		}
	}

	readInc = table->sampleRate / (this->sampleRate * table->hopSize);	
}

void CQTTablePlayer::noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel)
{
	if (table)
	{
		this->resetOnNextProcess = true;
	}
}

void CQTTablePlayer::noteOff(double timeStamp, unsigned char ch, unsigned char note)
{
}

void CQTTablePlayer::midiEvent(double timeStamp, std::vector<unsigned char>* message)
{
}

