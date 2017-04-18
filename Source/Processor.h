#pragma once

#include "VoiceManager.h"
#include "CQTTableManager.h"
#include "CQTTablePlayer.h"
#include "AudioIO.h"
#include "MidiIO.h"
#include "VoiceProcessor.h"
#include "VoiceLogic.h"

class Processor : public AudioCallbackProvider, public MidiListener
{
public:
	Processor(unsigned int numVoices, const CQTTableManager *tableManager);


	virtual void noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel) override;
	virtual void noteOff(double timeStamp, unsigned char ch, unsigned char note) override;
	virtual void midiEvent(double timeStamp, std::vector<unsigned char> *message) override;
	
	virtual void process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data) override;

	void prepare(const AudioIO::CallbackConfig & cfg);


private:

	std::unique_ptr<SimpleVoiceLogic> voiceLogic;

	VoiceManager::Control		  voiceControl;
	const CQTTableManager * const tableManager;

	std::vector<std::unique_ptr<VoiceProcessor>> voices;

	const unsigned int			  numVoices;

};
