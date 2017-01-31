#include "Entry.h"
#define _USE_MATH_DEFINES 1
#include <cmath>
#include <iostream>
#include <regex>
#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

void SineGen::process(AudioIO::CallbackConfig &cfg, AudioIO::CallbackData &data)
{
	float pi = 400. / cfg.sampleRate;

	for (int i = 0; i < cfg.frameSize; i++)
	{
		phase += pi;
		if (phase >= 1) phase = phase - 1;
		float val1 = std::sin(2 * M_PI * phase);
		float val2 = std::sin(2 * M_PI * phase);

		data.write[0][i] = 0.1 * val1;
		if (cfg.outChannels > 1) data.write[1][i] = 0.1 *  val2;
	}

}


int main(int argc, char **argv)
{
	bool configMode = false;
	int argIdx = 1;
	while (argIdx < argc)
	{
		std::string argument = std::string(argv[argIdx]);
		std::regex configRegex("[\\\\\\/-]?[cC](onfig)?"); // yes
		if(std::regex_match(argument, configRegex))
		{
			configMode = true;
		}
		argIdx++;
	}

	AudioIO audio;

	AudioIO::CallbackConfig cfg;
	cfg.inChannels = 0;
	cfg.outChannels = 2;
	cfg.frameSize = 64;
	cfg.sampleRate = 44100;
	cfg.inDevice = AudioIO::NoDevice;
	cfg.outDevice = AudioIO::DefaultDevice;

	if (configMode)
	{
		AudioIO::ConfigKeys keys =(AudioIO::ConfigKeys)( AudioIO::ConfigKeys::Conf_OutputDevice | AudioIO::ConfigKeys::Conf_FrameSize | AudioIO::ConfigKeys::Conf_SampleRate);
		audio.getConfiguration(cfg, keys);
	}

	SineGen gen;
	

	std::cout << "Starting Audio Callback" << std::endl;

	bool state = true;
	state &= audio.initStream(cfg);
	state &= audio.startStream();
	audio.setCallback(&gen);

	if (state)
	{
		std::cout << "Audio Callback Running" << std::endl;
		std::cin.sync();
		std::cin.get();

		audio.closeStream();
	}
	else
	{
		std::cout << "Faild to Start Audio Callback" << std::endl;
		std::cin.sync();
		std::cin.get();
	}
}