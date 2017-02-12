#include "AudioIO.h"
#include "ConfigManager.h"
#include "CQTAdditiveTable.h"
#include "CQTTablePlayer.h"

#include <iostream>
#include <fstream>
#include <regex>
#include <future>

// function blocks process by waiting for enter key
void waitForStdIn()
{
	std::cin.sync();
	std::cin.get();
}


int main(int argc, char **argv)
{
	
	bool configMode = false;
	bool debugMode  = false;
	std::string fileName;


	// #################### read arguments ####################
	int argIdx = 1;
	while (argIdx < argc)
	{
		std::string argument = std::string(argv[argIdx]);
		std::regex configRegex("[\\\\\\/-]?[cC](onfig)?"); // yes
		std::regex debugRegex( "[\\\\\\/-]?[dD](ebug)?"); 
		if(std::regex_match(argument, configRegex))
		{
			configMode = true;
		}
		else if (std::regex_match(argument, debugRegex))
		{
			debugMode = true;
		}
		else 
		{
			if (fileName.size() != 0) // fileName already set
			{
				std::cout << "Unexpected Argument" << std::endl;
				return 0;
			}
			fileName = argument;
			std::ifstream fileStream(fileName.c_str());
			if (!fileStream.good())
			{
				std::cout << "File could not be opened" << std::endl;
				return 0;
			}
		}
		argIdx++;
	}


	// #################### config stuff ####################

	std::cout << "Reading Config.xml" << std::endl;

	// load config manager, read config.xml if existing
	ConfigManager config("config.xml");	

	// create audio context, start in debug mode if debugMode==true
	AudioIO audio(debugMode);

	// prepare audiocontext configuration
	AudioIO::CallbackConfig cfg;
	cfg.inChannels = 0;
	cfg.outChannels = 2;
	cfg.frameSize = 64;
	cfg.sampleRate = 44100;
	cfg.inDevice = AudioIO::NoDevice;
	cfg.outDevice = AudioIO::DefaultDevice;
	
	// if config is true update current variables with getConfiguration dialog
	if (configMode)
	{
		AudioIO::ConfigKeys keys =(AudioIO::ConfigKeys)( AudioIO::ConfigKeys::Conf_OutputDevice | AudioIO::ConfigKeys::Conf_FrameSize | AudioIO::ConfigKeys::Conf_SampleRate);
		audio.getConfiguration(cfg, keys);

		config.setValue(ConfigManager::ID("Audio", "SampleRate"),	std::to_string(cfg.sampleRate));
		config.setValue(ConfigManager::ID("Audio", "FrameSize"),	std::to_string(cfg.frameSize));
		config.setValue(ConfigManager::ID("Audio", "OutputDevice"), std::to_string(cfg.outDevice));
	}
	
	// get audio configuration from config.xml
	std::string cfgValue;
	if (config.getValue(ConfigManager::ID("Audio", "SampleRate"), cfgValue))
	{
		cfg.sampleRate = std::atof(cfgValue.c_str());
	}
	if (config.getValue(ConfigManager::ID("Audio", "FrameSize"), cfgValue))
	{
		cfg.frameSize = std::atof(cfgValue.c_str());
	}
	if (config.getValue(ConfigManager::ID("Audio", "OutputDevice"), cfgValue))
	{
		cfg.outDevice = std::atof(cfgValue.c_str());
	}
	
	// store changes to configuration
	config.writeChanges(); 
	

	// #################### create CQG table ####################
	
	std::cout << "Import CQT Table" << std::endl;

	CQTAdditiveTable * table = CQTAdditiveTable::createTableFromFile(fileName);

	if (table == NULL)
	{
		std::cout << "Could not import CQT Table" << std::endl;
		std::cin.sync();
		std::cin.get();
		return 0;
	}

	CQTTablePlayer     player;
	player.setTable(table);
	
	// #################### start stream ####################


	std::cout << "Starting Audio Callback" << std::endl;

	bool state = true;
	state &= audio.initStream(cfg);
	state &= audio.startStream();
	audio.setCallback(&player);

	if (!state)
	{
		std::cout << "Faild to Start Audio Callback" << std::endl;
		std::cin.sync();
		std::cin.get();
		return 0;
	}


	// #################### debug loop ####################

	std::cout << "Audio Callback Running" << std::endl;
	auto blocker = std::async(waitForStdIn);
	while (std::future_status::timeout == blocker.wait_for(std::chrono::seconds(1))) // wait_for waits for a second and repeats following loop
	{
		if(debugMode)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "## Stream State ##" << std::endl;
			auto state = audio.getDebugData();
			std::cout << "CPU Load: \t\t" <<  (int) std::round(state->cpuLoad * 100) << "%" << std::endl;
			std::cout << "Frame Size (Int/Ext): \t" << state->internFrameSize << "/" << state->externFrameSize << std::endl;
			std::cout << "Frames Processed:\t" << state->processedSamples << std::endl;
			std::cout << std::endl;
		}
	}



	// #################### deconstruction ####################
	audio.closeStream();

	std::cout << "Audio Callback Stopped" << std::endl;

	std::cin.get();
	std::cin.sync();
	return 0;
	
}