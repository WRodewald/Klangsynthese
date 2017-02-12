#include "CQTTablePlayer.h"
#include <iostream>
#include <fstream>
CQTTablePlayer::CQTTablePlayer()
	:
	table(NULL),
	sampleRate(44100),
	readPos(-128),
	readInc(0),
	tableLength(0),
	tableEndReached(false)
{
	debugFile.open("output.txt");
}

void CQTTablePlayer::process(AudioIO::CallbackConfig & cfg, AudioIO::CallbackData & data)
{
	
	// update stuff if sample rate changed
	if (cfg.sampleRate != sampleRate) prepare(cfg.sampleRate);

	// return if we don't have a table
	if (!table) return;

	// following code might just get the price for least efficient additive synth there ever was
	for (int i = 0; i < cfg.frameSize; i++)
	{
		if (!tableEndReached)
		{
			data.write[0][i] = 0;

			readPos += readInc;
			int   readInt = readPos;
			float readFrac = readPos - readInt;
			if (readInt + 1 >= tableLength)
			{
				tableEndReached = true;
				continue;
			}
			if (readPos < 0)
			{
				continue;
			}

			int selectedFreq = 48 + table->binsPerSemitone * 12;
			for (int f = 0; f < generators.size(); f++)
			{
				double amplitude = (1.f - readFrac) * table->amplitudeTable[f][readInt] + (readFrac)* table->amplitudeTable[f][readInt + 1];
				float sine		= masterGen.tickWithFactor(table->frequencyTable[f] / table->frequencyTable[0]);
					
				data.write[0][i] += sine * amplitude;
			}
			masterGen.tick();
		}
		else
		{
			data.write[0][i] = 0;
		}
		
	}

	// copy first channel over to second channel (or any other if available)
	for (int c = 1; c < cfg.outChannels; c++)
	{
		memcpy((void*)data.write[c], (void*)data.write[0], cfg.frameSize * sizeof(float));
	}

	

}

void CQTTablePlayer::setTable(CQTAdditiveTable * table)
{
	this->table = table;

	generators.clear();
	frequencyMask.clear();

	if (table)
	{
		unsigned int numFrequencies = table->frequencyTable.size();
		for (int f = 0; f < numFrequencies; f++)
		{
			frequencyMask.push_back(true);
			generators.push_back(SineGen());
		}

		tableEndReached = false;
		tableLength = table->amplitudeTable[0].size();



		for (int f = 0; f < table->amplitudeTable.size(); f++)
		{
			for (int i = 0; i < table->amplitudeTable[f].size(); i++)
			{
				debugFile << table->amplitudeTable[f][i] << ", ";
			}
			debugFile << std::endl;
		}
	}
	


	prepare(sampleRate);
}

void CQTTablePlayer::prepare(float sampleRate)
{
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

		masterGen.setFrequency(table->frequencyTable[0] * iSampleRate);
	}

	readInc = table->sampleRate / (this->sampleRate * table->hopSize);	
}

