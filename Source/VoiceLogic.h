#pragma once

#include "VoiceManager.h"


class SimpleVoiceLogic : public VoiceManager::AVoiceLogic
{
public:
	virtual const VoiceManager::VoiceStack * allocOnNoteOn(const VoiceManager::NoteEvent * e) override;
	virtual const VoiceManager::NoteEvent * allocOnNoteOff(const VoiceManager::VoiceStack * e) override;
	virtual void prepare() override;
	virtual void suspend() override;
};