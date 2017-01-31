


#include <stdio.h>
#include <math.h>
#include <string>
#include <iostream>
#include <istream>

#include "portaudio.h"
#include "AudioIO.h"


bool AudioIO::portAudioInitialized = false;
unsigned int AudioIO::audioIOInstances = 0;

int AudioIO::portAudioCallback(	const void *					inputBuffer,
								void *							outputBuffer,
								unsigned long					framesPerBuffer,
								const PaStreamCallbackTimeInfo* timeInfo,
								PaStreamCallbackFlags			statusFlags,
								void *							userData)
{
	StreamData *data = (StreamData*) userData;

	float * in  = (float*)inputBuffer;
	float * out = (float*)outputBuffer;
	
	data->ioObject->audioCallback(in, out, framesPerBuffer);
	return 0;

}

void AudioIO::audioCallback(const float * in, float * out, unsigned long framesPerBuffer)
{
	unsigned int samples = 0;
	const float * inSamples   = in;
	float * outSamples = out;
	while (samples < framesPerBuffer)
	{

		// write stream input to input buffer
		for (int ch = 0; ch < cfg->inChannels; ch++)
		{
			inputBuffer[ch].push_back(*inSamples++);
		}
				
		bufferSize++;

		// when we have en
		if (bufferSize == cfg->frameSize)
		{
			processFrame(); 
			for (int ch = 0; ch < cfg->outChannels; ch++)
			{
				int size = latencyBuffer[ch].size();
				for (int i = 0; i < outputBuffer[ch].size(); i++)
				{
					float val = outputBuffer[ch][i];
					latencyBuffer[ch].push_back(val);
				}
				size = latencyBuffer[ch].size();
			}

			for (int ch = 0; ch < cfg->inChannels; ch++)
			{				
				inputBuffer[ch].clear(); // clears buffer
			}
			bufferSize = 0;
		}
		// write latency buffer output to stream out
		for (int ch = 0; ch < cfg->outChannels; ch++)
		{
			*(outSamples++) = latencyBuffer[ch].pop_front();;
		}
		samples++;
	}
}

void AudioIO::processFrame()
{
	for (int i = 0; i <inputBuffer.size(); i++)
	{
		data->read[i] = &(inputBuffer[i][0]);
	}
	
	for (int i = 0; i < outputBuffer.size(); i++)
	{
		data->write[i] = &(outputBuffer[i][0]);
	}

	if (callbackProvider != NULL) callbackProvider->process(*cfg, *data);

}

bool AudioIO::evaluatePAError(PaError & err)
{
	if (err == paNoError) return true;

	std::cout << "PortAudio Error: " << Pa_GetErrorText(err) << std::endl;
	return false;
}

int AudioIO::queryNumber(int min, int max, std::string label)
{
	int number = min - 1;
	while ((number < min) ||(number > max))
	{
		std::cout << label << ": ";

		std::string input;
		std::getline(std::cin, input);

		number = min-1;
		try
		{
			number = std::stoi(input);
		}
		catch (std::exception &e)
		{
			number = min - 1;
		}
		if (((number < min) || (number > max))) std::cout << "Invalid Input" << std::endl;
	}
	return number;
}

int AudioIO::queryNumber(std::vector<int> aviable, std::string label)
{
	bool validInput = false;
	int number = 0;
	while (!validInput)
	{
		validInput = false;
		std::cout << label << ": ";

		std::string input;
		std::getline(std::cin, input);

		try
		{
			number = std::stoi(input);

			for (int i = 0; i < aviable.size(); i++)
			{
				if (aviable[i] == number)
				{
					validInput = true;
					break;
				}
			}
		}
		catch (std::exception &e)
		{
			std::cout << "Invalid Input" << std::endl;
		}
	}
	return number;
}

int AudioIO::askForDevice(std::string query)
{
	return 0;
}

void AudioIO::printDevice(int deviceID)
{

	if ((deviceID < 0) || (deviceID > Pa_GetDeviceCount())) return;

	int defaultInput = Pa_GetDefaultInputDevice();
	int defaultOutput = Pa_GetDefaultOutputDevice();

	auto device = Pa_GetDeviceInfo(deviceID);
	auto api = Pa_GetHostApiInfo(device->hostApi);
	std::cout << "[" << deviceID << "]";

	if (deviceID == defaultInput)
	{
		std::cout << " (I) ";
	}
	else if (deviceID == defaultOutput)
	{
		std::cout << " (O) ";
	}
	else
	{
		std::cout << "\t";
	}

	std::cout << device->name;
	std::cout << std::endl;
	std::cout << "\t";
	std::cout << device->defaultSampleRate << " @ " << device->defaultLowInputLatency * 1000 << "/" << device->defaultLowOutputLatency * 1000 << "ms";
	std::cout << std::endl;
}


AudioIO::AudioIO():
	stream(NULL),
	cfg(NULL),
	data(NULL),
	callbackProvider(NULL)
	
{
	streamData.ioObject = this; // used as callbac pointer
	audioIOInstances++;

	if (!portAudioInitialized)
	{
		PaError err = Pa_Initialize();
		if (evaluatePAError(err)) portAudioInitialized = true;
	}
}

AudioIO::~AudioIO()
{
	if (stream) closeStream();
	audioIOInstances--;

	if (data) delete data;
	if (cfg)  delete cfg;
	data = NULL;
	cfg = NULL;

	if(audioIOInstances == 0) Pa_Terminate();
}

bool AudioIO::initStream(CallbackConfig cfg)
{
	if (cfg.outChannels < 1)		  return false;
	if (cfg.frameSize < 1)			  return false;
	if (cfg.frameSize > MaxFrameSize) return false;
	if (!portAudioInitialized)		  return false;
	
	streamData.ioObject = this;
	PaError err;

	int inputDeviceID  = cfg.inDevice;
	int outputDeviceID = cfg.outDevice;

	
	if ((cfg.inDevice   == DefaultDevice)  || (cfg.inDevice  > Pa_GetDeviceCount())) inputDeviceID  = Pa_GetDefaultInputDevice();
	if ((cfg.outDevice  == DefaultDevice)  || (cfg.outDevice > Pa_GetDeviceCount())) outputDeviceID = Pa_GetDefaultOutputDevice();
	if ((cfg.inDevice == NoDevice)  || (cfg.inChannels < 1))  inputDeviceID  = -1;
	if ((cfg.outDevice == NoDevice) || (cfg.outChannels < 1)) outputDeviceID = -1;

	auto inputDevice  = Pa_GetDeviceInfo(inputDeviceID);
	auto outputDevice = Pa_GetDeviceInfo(outputDeviceID);

	PaStreamParameters inParameter, outParameter;
	PaStreamParameters *outParameterPtr = NULL;
	PaStreamParameters *inParameterPtr = NULL;

	if (inputDevice != NULL)
	{
		inParameterPtr = &inParameter;
		inParameter.channelCount = cfg.inChannels;
		inParameter.device = inputDeviceID;
		inParameter.hostApiSpecificStreamInfo = NULL;
		inParameter.sampleFormat = paFloat32;
		inParameter.suggestedLatency = inputDevice->defaultLowInputLatency;
	}

	if (outputDevice != NULL)
	{
		outParameterPtr = &outParameter;
		outParameter.channelCount = cfg.outChannels;
		outParameter.device = outputDeviceID;
		outParameter.hostApiSpecificStreamInfo = NULL;
		outParameter.sampleFormat = paFloat32;
		outParameter.suggestedLatency = outputDevice->defaultLowOutputLatency;
	}



	err = Pa_OpenStream(&stream,
								inParameterPtr,			// no input channels
								outParameterPtr,		// stereo output
								cfg.sampleRate,			// requested sample rate
								0,						// use a variable frame size
								paNoFlag,
								portAudioCallback,
								&streamData);
	

	if (!evaluatePAError(err)) return false;

	if (this->cfg != NULL)  delete this->cfg;	
	if (this->data != NULL) delete this->data;
	this->cfg = NULL;
	this->data = NULL;

	bufferSize = 0;
	inputBuffer.clear();
	outputBuffer.clear();
	latencyBuffer.clear();

	for (int ch = 0; ch < cfg.inChannels; ch++)
	{
		inputBuffer.push_back(StackBuffer<float>());
		inputBuffer.back().reserve(cfg.frameSize);
	}

	for (int ch = 0; ch < cfg.outChannels; ch++)
	{
		outputBuffer.push_back(StackBuffer<float>());
		outputBuffer.back().reserve(cfg.frameSize);
		latencyBuffer.push_back(RingBuffer<float>(cfg.frameSize+1));
		for (int i = 0; i < cfg.frameSize; i++)
		{
			outputBuffer[ch].push_back(0);
			latencyBuffer[ch].push_back(0);
		}
	}

	this->data = new CallbackData(cfg.inChannels, cfg.outChannels);
	
	// copy over stream
	this->cfg  = new CallbackConfig;
	*this->cfg = cfg;

	return true;
}

bool AudioIO::startStream()
{
	if (stream == NULL) return false;

	PaError err = Pa_StartStream(stream); 
	if (!evaluatePAError(err)) return false;
	return true;
}

bool AudioIO::closeStream()
{
	PaError err;
	err = Pa_StopStream(stream);
	if (err != paNoError) return false;

	err = Pa_CloseStream(stream);
	if (err != paNoError) return false;
	
	stream = NULL;

	inputBuffer.clear();
	outputBuffer.clear();
	latencyBuffer.clear();	

	delete cfg;
	cfg = NULL;

	return true;
}

AudioIO::CallbackConfig AudioIO::getConfiguration()
{
	CallbackConfig cfg;
	if (!portAudioInitialized) return cfg;
	unsigned int numDevices = Pa_GetDeviceCount();
		

	std::cout << "Aviable Devices" << std::endl;
	std::cout << std::endl;

	std::cout << "\t" << "Device Name (API Name)" << std::endl;
	std::cout << "\t" << "SampleRate @ Latency (I/O)" << std::endl;

	std::cout << std::endl;

	std::cout << "[" << -2 << "]" << "\t" << "No Device (Input Only)" << std::endl;
	std::cout << "[" << -1 << "]" << "\t" << "Default Device" << std::endl;

	for (int i = 0; i < numDevices; i++)
	{
		printDevice(i);
	}

	cfg.inDevice  = queryNumber(-2, numDevices, "Input Device");
	cfg.outDevice = queryNumber(-1, numDevices, "Output Device");
	cfg.inChannels = 0;
	if (cfg.inDevice != NoDevice)
	{
		cfg.inChannels = queryNumber(1, 2, "Input Channels");
	}

	cfg.outChannels = queryNumber(1, 2, "Output Channels");
	std::vector<int> validSampleRates;
	validSampleRates.push_back(44100);
	validSampleRates.push_back(48000);
	validSampleRates.push_back(88200);
	validSampleRates.push_back(92000);
	validSampleRates.push_back(176400);
	validSampleRates.push_back(192000);
	cfg.sampleRate = queryNumber(validSampleRates , "Sample Rate");
	cfg.frameSize = queryNumber(1, MaxFrameSize, "Frame Size");
	


	std::cin.sync();
	std::cin.get();
}

AudioIO::CallbackData::CallbackData(unsigned int inChannels, unsigned int outChannels)
{
	read  = NULL;
	write = NULL;
	if (inChannels  > 0) read  = new float*[inChannels];
	if (outChannels > 0) write = new float*[outChannels];
}

AudioIO::CallbackData::~CallbackData()
{
	if (read)  delete[] read;
	if (write) delete[] write;
}
