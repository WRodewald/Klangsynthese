#pragma once

#include <vector>
#include <memory>
#include "RtMidi.h"

// MIDI I/O interface, only does input

class MidiListener
{
public:
	virtual void noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel) {};
	virtual void noteOff(double timeStamp, unsigned char ch, unsigned char note) {};
	virtual void midiEvent(double timeStamp, std::vector<unsigned char> *message) {};
};

class MidiCaster : protected MidiListener
{
public:
	MidiCaster();
	~MidiCaster();
		
	void open();
	
	void addListener(MidiListener * listener);

	static void rtMidiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData);
	
	virtual void noteOn(double timeStamp, unsigned char ch, unsigned char note, unsigned char vel);	
	virtual void noteOff(double timeStamp, unsigned char ch, unsigned char note);
	virtual void midiEvent(double timeStamp, std::vector<unsigned char> *message);

private:
	std::vector<MidiListener*> listeners;
	std::vector<RtMidiIn*>		inputs;
};




