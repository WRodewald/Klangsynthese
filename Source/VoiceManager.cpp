#include "VoiceManager.h"
#include <algorithm>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

using namespace VoiceManager;

Control::Control()
	:
	currentTimeStamp(0),
	eventCounter(0),
	noteOnCounter(0),
	noteOffCounter(0),
	maxVoices(0),
	logic(NULL)
{

	notes.reserve(MaxNotes);
	StackConfig nullConfig = { 0,0 };
	setStackConfig(nullConfig);
}

void Control::setVoices(std::vector<AVoiceHandle*> &newVoices)
{
	maxVoices = newVoices.size();

	voices.reserve(maxVoices);
	stacks.reserve(maxVoices);
	pool.reserve(maxVoices);
	voices.reserve(maxVoices);


	for (int i = 0; i < maxVoices; i++)
	{
		voices.push_back(newVoices[i]);
		stacks.push_back(VoiceStack(i, maxVoices));
	}

	StackConfig config = { maxVoices,1 };
	this->setStackConfig(config);

}

void Control::noteOn(uint32_t timeStamp, unsigned int ch, unsigned int note, unsigned int vel)
{	
	if (notes.size() < MaxNotes)
	{
		this->currentTimeStamp = timeStamp;	

		// this is a ridiculous to prevent a wrap around to id a NoteEvent as "NoEvent" 
		if (eventCounter == NoteEvent::NoEvent) eventCounter++; 

		notes.push_back(NoteEvent(eventCounter++, ch, note, vel));
		
		NoteEvent * note = &notes.back();

		if (logic)
		{
			const VoiceStack *requestedStack = logic->allocOnNoteOn(note);
			
			if (requestedStack)
			{
				// check if pointer points to one of the stacks in pool
				auto requestedStackIt = findStackInPool(requestedStack);
			
				allocStack(note, *requestedStackIt);
			}
		}
	}
}

void Control::noteOff(uint32_t timeStamp, unsigned int ch, unsigned int note)
{
	this->currentTimeStamp = timeStamp;

	/*
		I'm not quite happy with that blob. In short, the stack currently has a pointer to the note but idealy, we 
	*/

	auto noteIt = findNoteInNotes(note, ch);
	if (noteIt == notes.end()) return;

	NoteEvent &oldNote = *noteIt;

	// we only check for possible new notes to play if the released note was played by a voice
	if (oldNote.getState() == NoteEvent::State::Playing)
	{
		auto stackIt = findStackInPool(oldNote);

		// free that voice
		freeStack(*stackIt);

		// remove old note
		notes.erase(noteIt);

		if (logic)
		{
			const NoteEvent *requestedNote = logic->allocOnNoteOff(*stackIt);

			if (requestedNote != NULL)
			{
				auto requestedNoteIt = findNoteInNotes(*requestedNote);

				NoteEvent &noteToAllocate = *requestedNoteIt;
				allocStack(&noteToAllocate, *stackIt, true);
			}
		}
	}
	else // oldNote.getState() == NoteEvent::State::Idle
	{
		// remove old note
		notes.erase(noteIt);
	}

}

void Control::allNotesOff(uint32_t timeStamp)
{
	this->currentTimeStamp = timeStamp;

	// go through all stacks
	for (auto &stack  : this->stacks)
	{
		if (stack.getState() == AVoiceHandle::State::Active)
		{
			freeStack(&stack);
		}
	}

	// go through all voices manually 
	for (auto &voice : this->voices)
	{
		if (voice->getState() == AVoiceHandle::State::Active)
		{
			voice->noteOff(timeStamp);
		}
	}
	
	// finaly, erase all notes
	notes.clear();
	
}


// ***********************************************************************************************************************

unsigned int		Control::getNumActiveNotes()
{
	int noteCount = 0;
	for (auto &note : notes)
	{
		noteCount += (note.getState() == NoteEvent::State::Playing);
	}
	return noteCount;
}

unsigned int		Control::getNumInactiveNotes()
{
	int noteCount = 0;
	for (auto &note : notes)
	{
		noteCount += (note.getState() == NoteEvent::State::Idle);
	}
	return noteCount;
}

const NoteEvent *	Control::getNoteEvent(unsigned int id)
{
	return &notes[id];
}

unsigned int		Control::getNumNoteEvents()
{
	return notes.size();
}

const VoiceStack *	Control::getStack(unsigned int i)
{
	return pool[i];
}

unsigned int		Control::getNumStacks() 
{
	return pool.size();
}

unsigned int		Control::getNumVoices()
{
	return voices.size();
}

// ***********************************************************************************************************************




void		Control::setLogic(AVoiceLogic * logic)
{
	// suspend old logic
	if (this->logic)
	{
		this->logic->suspend();
		this->logic->setControl(NULL);
	}

	//swap logic
	this->logic = logic;

	// prepare new logic
	if (this->logic)
	{
		this->logic->setControl(this);
		this->logic->prepare();
	}
}

StackConfig Control::setStackConfig(StackConfig requestedConfig)
{
	/*
	Function tries to set up stacks / stack size accordingly. If the requested configuration can't be achived, the function will return what was configured instead
	Rules:
	- numStacks has priority, when it's set to 6 voices and we have 6 voices, it's provide one stack with six voices
	- if num Stacks > MaxVoices, it'll prepare one stack with all voices
	- otherwise it will set up as many stacks as possible with the requested stack size
	*/

	StackConfig adjustedConfig = requestedConfig;
	if (adjustedConfig.stackSize > voices.size()) adjustedConfig.stackSize = voices.size();

	unsigned int maxStacks = std::floor((float)voices.size() / (float)adjustedConfig.stackSize);
	if (adjustedConfig.numStacks > maxStacks) adjustedConfig.numStacks = maxStacks;


	/*
	Following code distributes every voice to a stack. The stacks get every N (stackSize) voices, i.e.
	VoiceID: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11
	StackID: 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2

	Reason being, if you use powers of two, voices will keep their assigned stack and thus
	less voices have to be killed because they get reassigned.

	*/

	for (int i = 0; i < adjustedConfig.numStacks * adjustedConfig.stackSize; i++)
	{
		AVoiceHandle * voice = voices[i];
		VoiceStack  * stack = &stacks[i % adjustedConfig.numStacks];
		if (stack != voice->getStack())
		{
			moveVoice(voice, stack);
		}
	}

	for (int i = adjustedConfig.numStacks * adjustedConfig.stackSize; i < voices.size(); i++)
	{
		moveVoice(voices[i], NULL);
	}

	pool.clear();

	// move active stacks to pool
	for (int i = 0; i < adjustedConfig.numStacks; i++)
	{
		pool.push_back(&stacks[i]);
	}

	// free all non-active stacks
	for (int i = adjustedConfig.numStacks; i < stacks.size(); i++)
	{
		if (stacks[i].getNoteID() != NoteEvent::NoEvent)
		{
			freeStack(&stacks[i]);
		}
	}


	return adjustedConfig;
}

void		Control::allocStack(NoteEvent * note, VoiceStack * stack, bool forceSteal)
{
	if (stack->getNoteID() == note->getEventID()) return; // ignored

	// invalid scenarios: note is already being played
	// remaining scenarios:
	// 1. stack is idle: normal alloc
	// 1. stack is playing: note steal

	bool wasStolen = false;
	if (stack->getNoteID() != NoteEvent::NoEvent)
	{
		freeStack(stack);
		wasStolen = true;
	}

	stack->_noteOn(++noteOnCounter, currentTimeStamp, *note, wasStolen || forceSteal);
	note->setState(NoteEvent::State::Playing);

	auto noteIt = findNoteInNotes(*note);

}

void		Control::freeStack(VoiceStack * stack)
{
	// following asserts might point out logic issues

	auto noteID = stack->getNoteID();
	auto noteIt = findNoteInNotes(noteID);
	
	noteIt->setState(NoteEvent::State::Idle);
	stack->_noteOff(++noteOffCounter, currentTimeStamp);
}

std::vector<NoteEvent>::iterator Control::findNoteInNotes(unsigned int note, unsigned int ch)
{
	for (auto it = notes.begin(); it != notes.end(); it++)
	{
		if ((it->getChannel() == ch) && (it->getNote() == note))
		{
			return it;
		}
	}
	return notes.end();
}

std::vector<NoteEvent>::iterator Control::findNoteInNotes(const NoteEvent & e)
{
	for (auto it = notes.begin(); it != notes.end(); it++)
	{
		if (it->getEventID() == e.getEventID())
		{
			return it;
		}
	}
	return notes.end();
}

std::vector<NoteEvent>::iterator Control::findNoteInNotes(NoteEvent::ID noteID)
{
	for (auto it = notes.begin(); it != notes.end(); it++)
	{
		if (it->getEventID() == noteID)
		{
			return it;
		}
	}
	return notes.end();
}


std::vector<VoiceStack*>::iterator Control::findStackInPool(const NoteEvent & e)
{
	for (auto it = pool.begin(); it != pool.end(); it++)
	{
		if (((*it)->getNoteID() == e.getEventID()))
		{
			return it;
		}
	}
	return pool.end();
}

std::vector<VoiceStack*>::iterator Control::findStackInPool(const VoiceStack * s)
{
	for (auto it = pool.begin(); it != pool.end(); it++)
	{
		if (((*it) == s))
		{
			return it;
		}
	}
	return pool.end();
}

void							   Control::moveVoice(AVoiceHandle *voice, VoiceStack *stack)
{
	// helper function moves one voice from it's current stack (if set) to given stack

	if (VoiceStack *stack = voice->getStack()) // this is actually an assignment!
	{
		stack->removeVoice(voice);
		voice->_noteOff(0); // kill reassigned notes}
	}

	if (stack)
	{
		stack->addVoice(voice);
	}
	else
	{
		voice->setStack(NULL);
		voice->setStackId(0);
	}

}



// ***********************************************************************************************************************

AVoiceHandle::AVoiceHandle(unsigned int id)
	:
	state(AVoiceHandle::State::Idle),
	note(0),
	ch(0),
	vel(0),
	id(id),
	stackID(0),
	voiceStack(NULL)
{
}

void AVoiceHandle::_noteOn(uint32_t timeStamp, unsigned int ch, unsigned int note, unsigned int vel, bool wasStolen)
{
	
	state = State::Active;
	this->note = note;
	this->vel  = vel;
	this->ch   = ch;

	noteOn(timeStamp, ch, note, vel, wasStolen);

}

void AVoiceHandle::_noteOff(uint32_t timeStamp)
{
	state = State::Idle;
	noteOff(timeStamp);
}

bool AVoiceHandle::isPlaying(const NoteEvent & e) const
{
	return ((e.getNote() == this->note) && (e.getChannel() == this->ch));
}

// ***********************************************************************************************************************


VoiceStack::VoiceStack(unsigned int id, unsigned int maxNumVoices)
	:
	id(id),
	noteOnCounter(0),
	noteOffCounter(0),
	noteID(NoteEvent::NoEvent)
{
	voices.reserve(maxNumVoices);
}

void					VoiceStack::_noteOn(uint64_t counter, uint32_t timeStamp, const NoteEvent & note, bool wasStolen)
{
	this->noteOnCounter = counter;
	this->noteID = note.getEventID();
	for (int i = 0; i < voices.size(); i++)
	{
		voices[i]->_noteOn(timeStamp, note.getChannel(), note.getNote(), note.getVelocity(), wasStolen);
	}
}

void					VoiceStack::_noteOff(uint64_t counter, uint32_t timeStamp)
{
	this->noteOffCounter = counter;
	noteID = NoteEvent::NoEvent; // yes, sorry
	for (int i = 0; i < voices.size(); i++)
	{
		this->voices[i]->_noteOff(timeStamp);
	}
}

AVoiceHandle *			VoiceStack::getMasterVoice()
{
	return voices[0];
}

const AVoiceHandle *	VoiceStack::getMasterVoice() const
{
	return voices[0];
}

AVoiceHandle *			VoiceStack::getVoice(unsigned int id)
{
	return voices[id];
}

NoteEvent::ID 			VoiceStack::getNoteID() const
{
	return noteID;
}

AVoiceHandle::State		VoiceStack::getState() const
{
	AVoiceHandle::State state = AVoiceHandle::State::Idle;
	for (auto voice : voices)
	{
		if (voice->getState() == AVoiceHandle::State::Active) return AVoiceHandle::State::Active;
	}
	return state;
}

unsigned int			VoiceStack::getNumVoices(void) const
{
	return voices.size();
}

void					VoiceStack::clearVoices()
{
	for (int i = 0; i < voices.size(); i++)
	{
		voices[i]->setStack(NULL);
		voices[i]->setStackId(0);
	}
	voices.clear();
}

void					VoiceStack::addVoice(AVoiceHandle * voice)
{
	voices.push_back(voice);
	voices[voices.size() - 1]->setStack(this);
	voices[voices.size() - 1]->setStackId(voices.size()-1);
}

void					VoiceStack::removeVoice(AVoiceHandle * voice)
{
	auto it = voices.begin();
	while (it != voices.end())
	{
		if ((*it) == voice)
		{
			it = voices.erase(it);
		}
		else
		{
			it++;
		}
	}

	voice->setStack(NULL);
	voice->setStackId(0);

	for (unsigned int i = 0; i < voices.size(); i++)
	{
		voices[i]->setStackId(i);
	}
}
