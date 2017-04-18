#include "MidiIO.h"

void MidiCaster::rtMidiCallback(double timeStamp, std::vector<unsigned char>* message, void * userData)
{
	MidiCaster * obj = (MidiCaster*)userData;

	unsigned char b1 = message->at(0);
	unsigned char b2 = message->at(1);

	static unsigned char channelMask = 1 + 2 + 4 + 8;
	static unsigned char typeMask = 16 + 32 + 64 + 128;

	unsigned char channel = b1 & channelMask;
	unsigned char type = (b1 & typeMask) >> 4; // we move the 4 bits to the start

	static unsigned char TypeNoteOn = 9;
	static unsigned char TypeNoteOff = 8;

	if (type == TypeNoteOn)
	{
		unsigned char b3 = 127;
		if (message->size() >= 3) b3 = message->at(2);
		obj->noteOn(timeStamp, channel, b2, b3);
	}
	else if (type == TypeNoteOff)
	{
		obj->noteOff(timeStamp, channel, b2);
	}
	else
	{
		// everything else is just forwarded as is
		obj->midiEvent(timeStamp, message);
	}

}

MidiCaster::MidiCaster()
{
	RtMidiIn tempMidiIn;
	auto  numPorts = tempMidiIn.getPortCount();
	
	for (int i = 0; i < numPorts; i++)
	{
		auto midiIn = new RtMidiIn;
		midiIn->setCallback(rtMidiCallback, this);
		inputs.push_back(midiIn);
	}
}

MidiCaster::~MidiCaster()
{
}

void MidiCaster::open()
{
	
	for (int i = 0; i < inputs.size(); i++)
	{
		inputs[i]->openPort(i);
	}
}

void MidiCaster::addListener(MidiListener * listener)
{
	if (!listener) return;

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i] == listener) return;
	}

	this->listeners.push_back(listener);
}

void MidiCaster::noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel)
{
	for (int i = 0; i < listeners.size(); i++)
	{
		listeners[i]->noteOn(timeStamp, ch, note, vel);
	}
}

void MidiCaster::noteOff(double timeStamp, unsigned char ch, unsigned char note)
{
	for (int i = 0; i < listeners.size(); i++)
	{
		listeners[i]->noteOff(timeStamp, ch, note);
	}
}

void MidiCaster::midiEvent(double timeStamp, std::vector<unsigned char>* message)
{
	for (int i = 0; i < listeners.size(); i++)
	{
		listeners[i]->midiEvent(timeStamp, message);
	}
}
