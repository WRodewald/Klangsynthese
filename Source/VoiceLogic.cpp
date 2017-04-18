#include "VoiceLogic.h"

using namespace VoiceManager;

const VoiceStack * SimpleVoiceLogic::allocOnNoteOn(const NoteEvent * e)
{

	auto numStacks = control->getNumStacks();
	const VoiceStack * selectedStack = nullptr;


	// find oldest with lowest noteOff id

	for (int i = 0; i < numStacks; i++)
	{
		auto stack = control->getStack(i);
		if (stack->getState() == AVoiceHandle::State::Idle)
		{
			if (selectedStack == nullptr)
			{
				selectedStack = stack;
			}
			else
			{
				if (stack->getCurrentNoteOffCounter() < selectedStack->getCurrentNoteOffCounter())
				{
					selectedStack = stack;
				}
			}
		}
	}

	if (selectedStack != nullptr) return selectedStack;

	// find oldest note on

	selectedStack = control->getStack(0);
	for (int i = 0; i < numStacks; i++)
	{
		auto stack = control->getStack(i);
		if (stack->getCurrentNoteOnCounter() < selectedStack->getCurrentNoteOnCounter())
		{
			selectedStack = stack;
		}
	}

	return selectedStack;
}

const NoteEvent * SimpleVoiceLogic::allocOnNoteOff(const VoiceStack * e)
{
	return nullptr;
}

void SimpleVoiceLogic::prepare()
{
	auto numVoices = control->getNumVoices();
	control->setStackConfig({ numVoices,1 });
}

void SimpleVoiceLogic::suspend()
{
}
