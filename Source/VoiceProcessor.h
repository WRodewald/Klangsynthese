#pragma once

#include "VoiceManager.h"
#include "TableManager.h"
#include "TablePlayer.h"

#include <atomic>
#include <mutex>

class VoiceProcessor  : public VoiceManager::AVoiceHandle
{
public:
	VoiceProcessor(unsigned int voiceID, const TableManager * const tableManager);

public:

	// Inherited via AVoiceHandle
	virtual void noteOn(uint32_t timeStamp, unsigned int ch, unsigned int note, unsigned int vel, bool wasStolen) override;
	virtual void noteOff(uint32_t timeStamp) override;

	inline void process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data);

	inline void prepare(const AudioIO::CallbackConfig &cfg);
	
private:

	enum class AsyncEvent
	{
		NoEvent = 0,
		NoteOff = 1,
		NoteOn = 2
	};
	
private:


	TablePlayer player;
	const TableManager * const tableManager;

	AsyncEvent asyncEvent{ AsyncEvent::NoEvent };
	std::mutex asyncEventMutex;

};

inline void VoiceProcessor::process(const AudioIO::CallbackConfig & cfg, AudioIO::CallbackData * data)
{
	{
		std::lock_guard<std::mutex> lock(asyncEventMutex);
		if (asyncEvent != AsyncEvent::NoEvent)
		{
			switch (asyncEvent)
			{
				case AsyncEvent::NoteOn:
				{
					player.setTable(tableManager->getTable(this->getCurrentNote()));
					player.noteOn();
				} break;
				case AsyncEvent::NoteOff:
				{
					player.noteOff();
				} break;
			}
			asyncEvent = AsyncEvent::NoEvent;
		}
	}
	
	player.process(cfg, data);
}

inline void VoiceProcessor::prepare(const AudioIO::CallbackConfig & cfg)
{
	player.prepare(cfg);
}
