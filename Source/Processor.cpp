#include "Processor.h"
#include "VoiceLogic.h"
#include <memory>

Processor::Processor(unsigned int numVoices, const CQTTableManager * tableManager)
	:
	tableManager(tableManager),
	numVoices(numVoices)
{
	// create voices
	std::vector<VoiceManager::AVoiceHandle*> voiceHandles;
	for (int i = 0; i < numVoices; i++)
	{
		voices.push_back(std::make_unique<VoiceProcessor>(i, tableManager));
		voiceHandles.push_back(voices[i].get());
	}

	voiceControl.setVoices(voiceHandles);

	// set logic
	voiceLogic = std::make_unique<SimpleVoiceLogic>();
	voiceControl.setLogic(voiceLogic.get());
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
