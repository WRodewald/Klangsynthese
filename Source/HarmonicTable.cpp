#include "HarmonicTable.h"
#include <fstream>
#include <regex>
#include <iostream>
#include <array>

// #################### CONSTRUCTOR ####################

HarmonicTable::HarmonicTable()
{
}


bool HarmonicTable::isValid() const
{
	// check for all arguments
	bool validTable = cfg.isValid();
	if (bins.size() == 0)	validTable = false;
	
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

void HarmonicTable::shiftFrequencyTo(int midiTarget)
{
	if (midiTarget == midiNote) return;
	
	auto fundamental = 440 * pow(2., (static_cast<double>(midiTarget) - 69.) / 12.);

	for (int i = 0; i < bins.size(); i++)
	{
		bins[i].frequency = (i+1) * fundamental;
	}
	
	setMidiNote(midiTarget);
}

ATable * HarmonicTable::interpolateTable(const ATable & secondTable, int targetMidi) const
{
	auto table2 = dynamic_cast<const HarmonicTable*>(&secondTable);

	// tables need to have the same config, otherwise bad stuff will happen
	bool validInerpolation = true;
	if (this->cfg != table2->cfg) validInerpolation = false;
	if (validInerpolation == false) return nullptr;

	// swap tables so that table1.midiNote < table2.midiNote
	auto & t1 = (this->midiNote < table2->midiNote) ? *this : *table2;
	auto & t2 = (this->midiNote < table2->midiNote) ? *table2 : *this;

	// return a copy of the provided table, if midi notes align
	if (t1.midiNote == targetMidi) return (new HarmonicTable(t1));
	if (t2.midiNote == targetMidi) return (new HarmonicTable(t2));

	// aboart if target midi isn't in range of midi notes
	if (t1.midiNote > targetMidi) return nullptr;
	if (t2.midiNote < targetMidi) return nullptr;

	// at t1.midiNote < targetMidi < t2.midiNote

	// calculate fraction for linear interpolation
	float midiRange = t2.midiNote - t1.midiNote;
	float t2Frac = static_cast<float>(targetMidi - t1.midiNote) / midiRange;
	float t1Frac = 1. - t2Frac;

	// create the new table object with cfg etc
	auto newTable = new HarmonicTable();
	newTable->cfg = t1.cfg;

	int binIdx = 0;
	while ((binIdx < t1.bins.size()) && (binIdx < t2.bins.size()))
	{
		Bin bin;

		const auto &bin1 = t1.bins[binIdx];
		const auto &bin2 = t2.bins[binIdx];

		bin.envelope.resize(std::min(bin1.envelope.size(), bin2.envelope.size()));

		int sampleIdx = 0;
		while ((sampleIdx < bin1.envelope.size()) && (sampleIdx < bin2.envelope.size()))
		{
			bin.envelope[sampleIdx] = t1Frac * bin1.envelope[sampleIdx] + t2Frac * bin2.envelope[sampleIdx];
			sampleIdx++;
		}
		newTable->bins.push_back(std::move(bin));
		binIdx++;
	}

	newTable->shiftFrequencyTo(targetMidi); // just sets the frequencys correctly
	return newTable;
}

ATable * HarmonicTable::createShiftedTable(int targetMidi)
{
	auto table = new HarmonicTable(*this);
	table->shiftFrequencyTo(targetMidi);
	return table;
}



