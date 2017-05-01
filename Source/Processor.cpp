#include "Processor.h"
#include "VoiceLogic.h"
#include <memory>

Processor::Processor(unsigned int numVoices, const TableManager * tableManager, bool doAuto)
	:
	tableManager(tableManager),
	numVoices(numVoices),
	doAuto(doAuto)
{
	// create voices
	std::vector<VoiceManager::AVoiceHandle*> voiceHandles;
	for (int i = 0; i < numVoices; i++)
	{
		voices.push_back(std::unique_ptr<VoiceProcessor>(new VoiceProcessor(i, tableManager)));
		voiceHandles.push_back(voices[i].get());
	}

	voiceControl.setVoices(voiceHandles);

	// set logic
	voiceLogic = std::unique_ptr<SimpleVoiceLogic>(new SimpleVoiceLogic());
	voiceControl.setLogic(voiceLogic.get());


	for (int i = 0; i < 128; i++)
	{
		auto table = tableManager->getTable(i);
		if (table != nullptr)
		{
			if (autoData.lowerRange == 0) autoData.lowerRange = i;
			autoData.upperRange = i;
		}
	}
	autoData.lastNote = autoData.lowerRange;
}

void Processor::noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel)
{
	voiceControl.noteOn(timeStamp, ch, note, vel);
}

void Processor::noteOff(double timeStamp, unsigned char ch, unsigned char note)
{
	voiceControl.noteOff(timeStamp, ch, note);
}

void Processor::midiEvent(double timeStamp, std::vector<unsigned char>* message) 
{
}

void Processor::process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data)
{
	if (doAuto)
	{
		float ms = 1000. * (cfg.frameSize * cfg.iSampleRate);
		playAuto(ms);
	}


	for (int c = 0; c < cfg.outChannels; c++)
	{
		for (int i = 0; i < cfg.frameSize; i++)
		{
			data->write[c][i] = 0;
		}
	}

	
	for (auto & voice : voices)
	{
		voice->process(cfg, data);		
	}

	for (int c = 1; c < cfg.outChannels; c++)
	{
		for (int i = 0; i < cfg.frameSize; i++)
		{
			data->write[c][i] = data->write[0][i];
		}
	}

}



void Processor::prepare(const AudioIO::CallbackConfig & cfg)
{
	for (auto &voice : voices)
	{
		voice->prepare(cfg);
	}
}


void Processor::playAuto(float msIncrement)
{
	autoData.timerMS += msIncrement;
	if (autoData.timerMS > autoData.intervalMS)
	{
		autoData.timerMS -= autoData.intervalMS;

		if (autoData.lastNote > 0) noteOff(0, 0, autoData.lastNote);
		autoData.curStep++;
		if (autoData.curStep > 2) autoData.curStep = 0;
		static const unsigned int offsets[] = { 4, 3, 5};
		auto interval = offsets[autoData.curStep];

		auto note = autoData.lastNote += interval;

		if (note > autoData.upperRange)
		{
			note = autoData.lowerRange;
			autoData.curStep = 0;
		}
				
		
		autoData.lastNote = note;

		noteOn(0, 0, autoData.lastNote, 127);
	}
}