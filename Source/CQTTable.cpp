#include "CQTTable.h"
#include <fstream>
#include <regex>
#include <iostream>
#include <array>
#include "cereal/types/polymorphic.hpp"


// #################### CONSTRUCTOR ####################

CQTTable::CQTTable()
{
}



bool CQTTable::isValid() const
{
	// check for all arguments
	bool validTable = cfg.isValid();
	if (bins.size() == 0)	validTable = false;

	if (binsPerSemitone < 1) validTable = false;

	if (!validTable)
	{
		std::cout << "Invalid table: missing arguments or data." << std::endl;
	}
		
	// check number of frames
	int numFrames = bins[0].envelope.size();
	if (numFrames < 1)
	{
		validTable = false;
		std::cout << "Invalid table: missing arguments or data." << std::endl;
	}

	for (int i = 1; i < bins.size(); i++)
	{
		if (bins[i].envelope.size() != numFrames)
		{
			validTable = false;
			std::cout << "Invalid table: amplitude arrays don't have the same size." << std::endl;
			break;
		}
	}
	
	return validTable;
}

void CQTTable::setBinsPerSemitone(unsigned int binsPerSemitone)
{
	this->binsPerSemitone = binsPerSemitone;
}

void CQTTable::shiftFrequencyTo(int midiTarget)
{
	if (midiTarget == midiNote) return;
	int binOffset = (midiTarget - midiNote) * binsPerSemitone;

	// move the whole data struct to a temp location to prevent overrides
	auto tempBins = std::move(bins);

	// empty buffer
	std::vector<float> emptyEnvelope;
	emptyEnvelope.resize(tempBins[0].envelope.size(), 0);

	bins.clear();
	
	for (int i = 0; i < tempBins.size(); i++)
	{
		bins.push_back(Bin());
		Bin &bin = bins.back();

		bin.frequency = tempBins[i].frequency;

		int offsetedIndx = i + binOffset;

		bool outOfRange = (offsetedIndx < 0) || (offsetedIndx >= bins.size());
		if (outOfRange)
		{
			bin.envelope = emptyEnvelope;
		}
		else
		{
			bin.envelope = std::move(tempBins[offsetedIndx].envelope);
		}
	}
	setMidiNote(midiTarget);
}

ATable * CQTTable::interpolateTable(const ATable & secondTable, int targetMidi) const
{
	auto table2 = dynamic_cast<const CQTTable*>(&secondTable);

	// tables need to have the same config, otherwise bad stuff will happen
	bool validInerpolation = true;
	if (this->cfg != table2->cfg) validInerpolation = false;
	if (this->binsPerSemitone != table2->binsPerSemitone) validInerpolation = false;
	if (validInerpolation == false) return nullptr;

	// swap tables so that table1.midiNote < table2.midiNote
	auto & t1 = (this->midiNote < table2->midiNote) ? *this   : *table2;
	auto & t2 = (this->midiNote < table2->midiNote) ? *table2 : *this;

	// return a copy of the provided table, if midi notes align
	if (t1.midiNote == targetMidi) return (new CQTTable(t1));
	if (t2.midiNote == targetMidi) return (new CQTTable(t2));

	// aboart if target midi isn't in range of midi notes
	if (t1.midiNote > targetMidi) return nullptr;
	if (t2.midiNote < targetMidi) return nullptr;

	// at t1.midiNote < targetMidi < t2.midiNote
	int t1Offset = (targetMidi - t1.midiNote) * t1.binsPerSemitone;
	int t2Offset = (targetMidi - t2.midiNote) * t2.binsPerSemitone;

	// calculate fraction for linear interpolation
	float midiRange = t2.midiNote - t1.midiNote;
	float t2Frac = static_cast<float>(targetMidi - t1.midiNote) / midiRange;
	float t1Frac = 1. - t2Frac;

	// create the new table object with cfg etc
	auto newTable = new CQTTable;
	newTable->cfg = t1.cfg;
	newTable->setBinsPerSemitone(t1.binsPerSemitone);
	newTable->setMidiNote(targetMidi);

	// prepare all bins
	for (auto &bin : t1.bins)
	{
		Bin newBin;
		newBin.frequency = bin.frequency;
		newTable->bins.push_back(newBin);
	}

	auto &bins = newTable->bins;

	// interpolate over data
	for (int i = 0; i < bins.size(); i++)
	{
		int  t1BinIdx = i - t1Offset;
		int  t2BinIdx = i - t2Offset;

		bool t1InRange = (0 <= t1BinIdx) && (t1BinIdx < t1.bins.size());
		bool t2InRange = (0 <= t2BinIdx) && (t2BinIdx < t2.bins.size());

		if (t1InRange && t2InRange)
		{
			auto &t1Bin = t1.bins[t1BinIdx];
			auto &t2Bin = t2.bins[t2BinIdx];
			auto &bin = bins[i];
			auto maxBinIdx = std::min(t1Bin.envelope.size(), t2Bin.envelope.size());

			int envIdx = 0;
			while (envIdx < maxBinIdx)
			{
				bin.envelope.push_back(t1Bin.envelope[envIdx] * t1Frac + t2Bin.envelope[envIdx] * t2Frac);
				envIdx++;
			}

		}
		else if (t1InRange)
		{
			auto &t1Bin = t1.bins[t1BinIdx];
			auto &bin = bins[i];
			bin.envelope = t1Bin.envelope;

		}
		else if (t2InRange)
		{
			auto &t2Bin = t2.bins[t2BinIdx];
			auto &bin = bins[i];
			bin.envelope = t2Bin.envelope;
		}
		else
		{
			return nullptr;
		}
	}
	newTable->normalizeBinSizes();
	return newTable;
}

ATable * CQTTable::createShiftedTable(int targetMidi)
{
	auto table = new CQTTable(*this);
	table->shiftFrequencyTo(targetMidi);
	return table;
}

