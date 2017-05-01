#pragma once

#include "VoiceManager.h"
#include "TableManager.h"
#include "TablePlayer.h"
#include "AudioIO.h"
#include "MidiIO.h"
#include "VoiceProcessor.h"
#include "VoiceLogic.h"

class Processor : public AudioCallbackProvider, public MidiListener
{
public:
	Processor(unsigned int numVoices, const TableManager *tableManager, bool doAuto);


	virtual void noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel) override;
	virtual void noteOff(double timeStamp, unsigned char ch, unsigned char note) override;
	virtual void midiEvent(double timeStamp, std::vector<unsigned char> *message) override;
	
	virtual void process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data) override;

	void playAuto(float msIncrement);

	void prepare(const AudioIO::CallbackConfig & cfg);


private:

	std::unique_ptr<SimpleVoiceLogic> voiceLogic;

	VoiceManager::Control		  voiceControl;
	const TableManager * const tableManager;

	std::vector<std::unique_ptr<VoiceProcessor>> voices;

	const unsigned int			  numVoices;

	struct AutoData
	{
		unsigned int lowerRange{ 0 };
		unsigned int upperRange{ 0 };
		unsigned int curStep{ 0 };
		
		const float intervalMS{ 500 };
		float		timerMS{ 0 };

		int lastNote = -1;
	};

	bool doAuto{ false };
	AutoData autoData;

};
