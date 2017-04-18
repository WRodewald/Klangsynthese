#include "AudioIO.h"
#include "MidiIO.h"
#include "ConfigManager.h"
#include "CQTAdditiveTable.h"
#include "CQTTablePlayer.h"
#include "CQTTableManager.h"
#include "Processor.h"

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

#define  returnFail    { waitForStdIn(); return -1;}
#define  returnSuccess { waitForStdIn(); return -0;}


int main(int argc, char **argv)
{
	
	bool multiThreading = false;
	bool configMode		= false;
	bool debugMode		= false;
	bool cache			= false;
	float cqtBinThreshold = 0;
	std::string fileName;


	// #################### read arguments ####################
	int argIdx = 1;
	while (argIdx < argc)
	{
		std::string argument = std::string(argv[argIdx]);
		std::regex configRegex("[\\\\\\/-]?C(onfig)?",	std::regex::icase); 
		std::regex debugRegex( "[\\\\\\/-]?D(ebug)?",	std::regex::icase);
		std::regex thresRegex("[\\\\\\/-]?T(hreshold)?",std::regex::icase);
		std::regex mtRegex("[\\\\\\/-]?MT",				std::regex::icase);
		std::regex cacheRegex("[\\\\\\/-]?cache",		std::regex::icase);

		if(std::regex_match(argument,		configRegex))	configMode	   = true;
		else if (std::regex_match(argument, debugRegex))	debugMode	   = true;
		else if (std::regex_match(argument, mtRegex))		multiThreading = true;
		else if (std::regex_match(argument, cacheRegex))	cache		   = true;
		else if (std::regex_match(argument, thresRegex))
		{
			argIdx++;
			if (argIdx >= argc)
			{
				std::cout << "Unexpected Argument (Threshold)" << std::endl;
				returnFail;
			}
			std::string thresholdString = std::string(argv[argIdx]);

			try 
			{
				cqtBinThreshold = std::stof(thresholdString);
			}
			catch (const std::exception &e)
			{
				std::cout << "Unexpected Argument (Threshold)" << std::endl;
				returnFail;
			}

		}
		else 
		{
			if (fileName.size() != 0) // fileName already set
			{
				std::cout << "Unexpected Argument" << std::endl;
				returnFail;
			}
			fileName = argument;
		}
		argIdx++;
	}


	

	// #################### Audio IO ####################

	// create audio context, start in debug mode if debugMode==true
	AudioIO audio(debugMode);

	// prepare audiocontext configuration
	AudioIO::CallbackConfig cfg;
	cfg.inChannels = 0;
	cfg.outChannels = 2;
	cfg.frameSize = 64;
	cfg.sampleRate = 44100;
	cfg.iSampleRate = 1. / cfg.sampleRate;
	cfg.inDevice = AudioIO::NoDevice;
	cfg.outDevice = AudioIO::DefaultDevice;



	// #################### config stuff ####################

	std::cout << "Reading Config.xml" << std::endl;

	// load config manager, read config.xml if existing
	ConfigManager config("config.xml");	
		
	// if config is true update current variables with getConfiguration dialog
	if (false)
	{

		std::cout << std::endl << "Audio Configuration" << std::endl;

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
		cfg.outDevice = std::atoi(cfgValue.c_str());
	}

	
	// store changes to configuration
	config.writeChanges(); 
	

	// #################### create CQT manager ####################
	
	std::cout << "Create CQT Manager" << std::endl;

	CQTTableManager tableManager(debugMode); 
	
	if (fileName.size() > 0)
	{
		std::cout << "Loading CQT table file: ";
		bool success = tableManager.importTextFile(fileName, debugMode);
		if (success)
		{
			std::cout << "success" << std::endl;
		}
		else
		{
			std::cout << "failed" << std::endl;
			returnFail;
		}

		std::cout << "Interpolating CQT Tables" << std::endl;
		tableManager.prepareTables({ 0, 128 });
	}
	else
	{
		std::cout << "Loading CQT table cache: ";
		try
		{
			tableManager.loadBinaryCache("CQTTable.cache");
			std::cout << "success" << std::endl;
		}
		catch (const std::exception & e)
		{
			std::cout << "failed" << std::endl;
			returnFail;
		}
	}
		
	std::cout << "CQT Manager Initialized" << std::endl;

	if (cache)
	{
		std::cout << "Caching CQT table: ";
		try
		{
			tableManager.storeBinaryCache("CQTTable.cache");
			std::cout << "success" << std::endl;
			//returnSuccess;
		}
		catch (const std::exception & e)
		{
			std::cout << "failed" << std::endl;
			returnFail;
		}
	}

	if (tableManager.sanity() == false)
	{
		std::cout << "CQT Manger contains inconistent tables" << std::endl;
		returnFail;
	}

	// #################### create Processor ####################
	
	const auto NumVoices = 1;
	Processor processor(NumVoices, &tableManager);
	processor.prepare(cfg);
	audio.setCallback(&processor);
	
	// #################### MIDI Part 2 ####################

	MidiCaster midiCaster;

	midiCaster.addListener(&processor);
	midiCaster.open();

	// #################### start stream ####################


	std::cout << "Starting Audio Callback" << std::endl;

	bool state = true;
	state &= audio.initStream(cfg);
	state &= audio.startStream();

	if (!state)
	{
		std::cout << "Faild to Start Audio Callback" << std::endl;
		returnFail;
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

	returnSuccess;
	
}