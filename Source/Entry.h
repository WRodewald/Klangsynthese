#pragma once

#include "AudioIO.h"
#include "ConfigManager.h"

class SineGen : public AudioCallbackProvider
{
public:

	SineGen() : phase(0) {}

	virtual void process(AudioIO::CallbackConfig &cfg, AudioIO::CallbackData &data) override;

private:
	float phase;
};