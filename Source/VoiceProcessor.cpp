#include "VoiceProcessor.h"

VoiceProcessor::VoiceProcessor(unsigned int voiceID, const CQTTableManager * const tableManager)
	:
	tableManager(tableManager),
	AVoiceHandle(voiceID)
{

}

void VoiceProcessor::noteOn(uint32_t timeStamp, unsigned int ch, unsigned int note, unsigned int vel, bool wasStolen)
{
	std::lock_guard<std::mutex> lock(asyncEventMutex);
	asyncEvent = AsyncEvent::NoteOn;
}

void VoiceProcessor::noteOff(uint32_t timeStamp)
{
	std::lock_guard<std::mutex> lock(asyncEventMutex);
	asyncEvent = AsyncEvent::NoteOff;
}

