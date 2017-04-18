#pragma once

#include <vector>
#include <list>

// function schedules note events and distributes them to Voices

namespace VoiceManager
{
	static const int MaxNotes = 16 * 128; // wow, much notes.
	
// ***********************************************************************************************************************

	class AVoiceHandle;
	class VoiceStack;
	class NoteEvent;


// ***********************************************************************************************************************

	/*
		A NoteEvent represents a held down key, it's created with a MIDI noteOn() event and it's destroyed with a corosponding noteOff.
		Every NoteEvent can be identified by it's eventID, that is, at leaat in pratice, unique. Collisions with events might occure but only when the first note is held down while playing 2^64 others
		
		Comments:
			- The EventID collision thingy can be adjusted by using the first free "ID" (i.e. resuse "1" when there is no note with ID 1)
			- NoteEvent::ID 0 is used as "NoEvent"
	*/
	class NoteEvent
	{
	public:

		using ID = std::uint64_t;
		static const ID NoEvent = 0;
		
		NoteEvent() : NoteEvent(0, 0, 0, 0) {};
		NoteEvent(ID eventID, unsigned int ch, unsigned int note, unsigned int vel) : note(note), ch(ch), vel(vel), eventID(eventID), state(State::Idle) 
		{
		};

		enum class State
		{
			Playing,
			Idle
		};

		unsigned int	getNote()		const { return note; };
		unsigned int	getVelocity()	const { return vel; };
		unsigned int	getChannel()	const { return ch; };
		ID				getEventID()	const { return eventID; };
		State			getState()		const { return state; }

		void			setState(State s) { this->state = s; }


	private:
		ID			eventID;
		State		 state;

		//public:
		unsigned int note;
		unsigned int vel;
		unsigned int ch;
	};

// ***********************************************************************************************************************

	/*
		Abstract AVoiceHandle represents the "handle" of a voice that receives the NoteOn / NoteOff messages. They can be viewed as the output adapapter of the AM_VoiceManager::Control
		A Handle is bound to a VoiceStack and only receives noteOn / noteOff messages from the stack. (Exception: reassigning voices to stacks)
	*/
	class AVoiceHandle
	{
	public:

		enum class State
		{
			Active,		// voice is playing an active note
			Idle		// voice is not being processed
		};
		
		AVoiceHandle(unsigned int id);

		void _noteOn(uint32_t timeStamp, unsigned int ch, unsigned int note, unsigned int vel, bool wasStolen);
		void _noteOff(uint32_t timeStamp);

		virtual void noteOn(uint32_t timeStamp, unsigned int ch, unsigned int note, unsigned int vel, bool wasStolen) = 0;
		virtual void noteOff(uint32_t timeStamp) = 0;

		bool  isPlaying(const NoteEvent &e) const;

		State getState() const { return state; };

		int getCurrentNote()			   const { return note; };
		int getCurrentVelocity()		   const { return vel; };
		int getCurrentChannel()			   const { return ch; };
		
		void setStackId(unsigned int id)
		{
			stackID = id;
		}

		unsigned int  getStackID()
		{
			return stackID;
		}

		void setStack(VoiceStack *stack)
		{
			this->voiceStack = stack;
		}

		VoiceStack * getStack()
		{
			return voiceStack;
		}

		const unsigned int id;

	private:
		VoiceStack		*  voiceStack;
		unsigned int	   stackID; // id of the voice within the stack

		State  state;

		int note;
		int ch;
		int vel;
	};

	// ***********************************************************************************************************************

	/*
		The configuration of voice stacks in the voice manager. The struct describes how many stacks with how many voices each should be used
	*/
	struct StackConfig
	{
		unsigned int numStacks;
		unsigned int stackSize;
	};

	/*
		The VoiceStack class consists of multiple AVoiceHandles, with the first one being regarded the "MasterVoice". (Exception: when being stored in a vector but not used, a stack is empty and doesn't own voices)
		Stacks receive noteOn / noteOff messages from the Control and forward them to their voices. 
		A Stack stores the ID of the NoteEvent that is currently played. (Or NoteEvent::NoEvent if the stack is idle)
		
		The Stack is the main object that the Control forwards noteOn / noteOff messages to. Even when the stack size is 1, the stack is used as an interface between control and voice
	*/
	class VoiceStack
	{
	public:
		VoiceStack(unsigned int id, unsigned int maxNumVoices);

		void _noteOn(uint64_t counter, uint32_t timeStamp, const NoteEvent & note, bool wasStolen);
		void _noteOff(uint64_t counter, uint32_t timeStamp);
				

		AVoiceHandle *			getMasterVoice();
		const AVoiceHandle *	getMasterVoice() const;
		AVoiceHandle *			getVoice(unsigned int id);

		NoteEvent::ID 			getNoteID() const;
		AVoiceHandle::State     getState()  const;

		
		unsigned int getNumVoices() const;

		uint64_t getCurrentNoteOnCounter()   const { return noteOnCounter; };
		uint64_t getCurrentNoteOffCounter()  const { return noteOffCounter; };

		void clearVoices();
		void addVoice(AVoiceHandle    *voice);
		void removeVoice(AVoiceHandle *voice);
		
		const unsigned int		 id;

	private:

		NoteEvent::ID noteID;

		std::vector<AVoiceHandle *> voices;
		uint64_t noteOnCounter;	// counter value of last note on event this voice got 
		uint64_t noteOffCounter;	// counter value  of last note off event this voice got

	};



// ***********************************************************************************************************************

	/*
		The IControlStateProvider provices (no pun intented) an interface for a AVoiceLogic to query information from the control. 
		The only active function (non-getter) is the "setStackConfig", that can be used by the logic to change the stack arrangement.

		Very AVoiceLogic owns a pointer to the Control as a IControlStateProvider. 
	*/
	class IControlStateProvider
	{
		public:
			virtual const NoteEvent * getNoteEvent(unsigned int i) = 0;
			virtual unsigned int	  getNumNoteEvents()		   = 0;

			virtual const VoiceStack * getStack(unsigned int i) = 0;
			virtual unsigned int	   getNumStacks() = 0;
			virtual unsigned int	   getNumVoices() = 0;

			virtual StackConfig			setStackConfig(StackConfig config) = 0;
			
	};


// ***********************************************************************************************************************

	/*
		The AVoiceLogic class represents the allocation logic of the AM_VoiceManager. The core functionallity of the logic is to decide which stack to allocate on a note on and
		which note to play when a stack becomes idle after a noteOff. Additionally, the logic has to implement prepare and suspend methods that are called when
		a control switches it's logic. Suspend is called in the old logic, prepare in the new logic.
	*/
	class AVoiceLogic
	{
	public:

		void setControl(IControlStateProvider * control)
		{
			this->control = control;
		}

		// allocation callbacks, return true when the allocation event is valid and should be executed
		virtual const VoiceStack * allocOnNoteOn(const NoteEvent *e) = 0;
		virtual const NoteEvent * allocOnNoteOff(const VoiceStack *e) = 0;

		virtual void prepare() = 0; // called before logic is used in voice manager
		virtual void suspend() = 0; // called when logic is not longer in use
		
	protected:
		IControlStateProvider * control;

	};

// ***********************************************************************************************************************

	/*
		The Control class represents the core of the AM_VoiceManager. The Control takes three ingreedients.
		1) it receives polyphonic noteOn / noteOff messages
		2) it takes a vector of AVoiceHandles that those messages are forwarded to
		3) it takes a logic that decides, which voice (or stack) is allocated for incomming notes

		
		Comments:
			- the Control will be extended to forward other midi MEssages to a corrosponding voice,
	*/
	class Control : public IControlStateProvider
	{
	private:
		
	// ***************************************************

	public:
		

	public:
		Control();

		void setVoices(std::vector<AVoiceHandle*> &newVoice);

		void noteOn(uint32_t timeStamp,  unsigned int ch, unsigned int note, unsigned int vel);
		void noteOff(uint32_t timeStamp, unsigned int ch, unsigned int note);
		void allNotesOff(uint32_t timeStamp);

		AVoiceHandle* getVoiceHandlePtr(unsigned id)
		{
			if (id < voices.size())
			{
				return voices[id];
			}
			return NULL;
		}
		
		unsigned int getNumActiveNotes();
		unsigned int getNumInactiveNotes();
	
		virtual const NoteEvent * getNoteEvent(unsigned int i)  override;
		virtual unsigned int	  getNumNoteEvents() override;
		
		virtual const VoiceStack * getStack(unsigned int i)	override;

		virtual unsigned int	  getNumStacks() override;
		virtual unsigned int	  getNumVoices() override;

		void setLogic(AVoiceLogic *logic);
		
		virtual StackConfig setStackConfig(StackConfig config) override;


		// ***************************************************
	private:
		
		void freeStack (VoiceStack *e);
		void allocStack(NoteEvent *e, VoiceStack *stack, bool forceSteal = false);
		
		std::vector<NoteEvent>::iterator findNoteInNotes(unsigned int note, unsigned int ch);		// returns iterator of notes vector 
		std::vector<NoteEvent>::iterator findNoteInNotes(NoteEvent::ID noteID);					// returns iterator of notes vector 
		std::vector<NoteEvent>::iterator findNoteInNotes(const NoteEvent & e);					// returns iterator of notes vector 
		std::vector<VoiceStack*>::iterator findStackInPool(const NoteEvent & e);					// returns iterator of pool vector (or ::end()
		std::vector<VoiceStack*>::iterator findStackInPool(const VoiceStack *s);				// returns iterator of pool vector (or ::end()

		void moveVoice(AVoiceHandle *voice, VoiceStack *stack);



		// ***************************************************
	private:
		
		std::vector<NoteEvent>  notes;

		std::vector<VoiceStack*>  pool; // voices currently enabled for allocation


		std::vector<AVoiceHandle*>   voices;	   // list of all aviable voices
		std::vector<VoiceStack>		 stacks;	   // list of all aviable stacks

		unsigned int	maxVoices;

		// Logic

		AVoiceLogic * logic;

		// State

		uint64_t eventCounter;	// these count incomming note on's
		uint64_t noteOnCounter;	// counts note on's  forwarded to voices
		uint64_t noteOffCounter;	// counts note off's forwarded to voices

		uint32_t currentTimeStamp; // stores the current time stemp, updatet with incomming Note Event


	

	};

};



