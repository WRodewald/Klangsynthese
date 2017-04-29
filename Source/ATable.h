#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "cereal/access.hpp"
#include "cereal/types/vector.hpp"

class ATable
{
public:
	struct Bin
	{
		float			   frequency;
		std::vector<float> envelope;

		template<class Archive>
		void serialize(Archive & archive)
		{
			archive(frequency, envelope);
		}
	};
	
	class Config
	{
	public:
		float sampleRate{ 0 };
		float hopSize{ 0 };

		inline bool operator==(const Config & rhs) const;
		inline bool operator!=(const Config & rhs) const;

		inline bool isValid() const;

		template<class Archive>
		void serialize(Archive & archive)
		{
			archive(sampleRate, hopSize);
		}
	};
	
	template<class Archive>
	void serialize(Archive & archive)
	{
		archive(midiNote, cfg, bins);
	
	}
	// #################### CONSTRUCTOR ####################
public:

	// #################### MANIPULATION ####################
public:
	virtual void shiftFrequencyTo(int midiTarget) = 0;
	
	inline void normalizeBinSizes();
	
	virtual ATable * interpolateTable(const ATable &t2, int targetMidi) const = 0;
	virtual ATable * createShiftedTable(int targetMidi) = 0;

	// #################### GETTER // SETTER ####################
public:

	virtual bool isValid() const = 0;

	inline const std::vector<Bin> & getData()  const;

	inline void		setBins(std::vector<Bin> &&		 bins);
	inline void		setBins(const std::vector<Bin> & bins);
	
	inline unsigned int getMidiNote() const;
	inline void			setMidiNote(unsigned int note);

	inline Config		getConfig() const;
	inline void 		setConfig(Config cfg);

	inline unsigned int	getEnvelopeLength() const;

protected:
	// #################### MEMBER ####################

	Config cfg;

	std::vector<Bin> bins; // list of frequencies
	int	  midiNote{ -1 };  // -1: invalid
};


inline unsigned int ATable::getMidiNote() const
{
	return midiNote;
}

inline void ATable::setMidiNote(unsigned int note)
{
	midiNote = note;
}

inline const std::vector<ATable::Bin> & ATable::getData() const
{
	return bins;
}

inline void ATable::setBins(std::vector<Bin>&& bins)
{
	this->bins = std::move(bins);
}

inline void ATable::setBins(const std::vector<Bin>& bins)
{
	this->bins = bins;
}

inline unsigned int ATable::getEnvelopeLength() const
{
	return bins[0].envelope.size();
}

inline ATable::Config ATable::getConfig() const
{
	return cfg;
}

inline void ATable::setConfig(Config cfg)
{
	this->cfg = cfg;
}

void ATable::normalizeBinSizes()
{
	int maxLength = 0;
	for (auto & bin : bins)
	{
		if (bin.envelope.size() > maxLength) maxLength = bin.envelope.size();
	}

	for (auto & bin : bins)
	{
		// the current implementation is very time consluming because all bins have to be reallocated
		auto lengthDiff = maxLength - bin.envelope.size();
		auto zeroFill = std::vector<float>(lengthDiff, 0);
		bin.envelope.insert(bin.envelope.end(), zeroFill.begin(), zeroFill.end());
	}
}

// #################### Config ####################

bool ATable::Config::operator==(const ATable::Config & rhs) const
{
	auto lhs = *this;
	return ((lhs.hopSize == rhs.hopSize) && (lhs.sampleRate == rhs.sampleRate));
}

bool ATable::Config::operator!=(const ATable::Config & rhs) const
{
	auto lhs = *this;
	return !(lhs == rhs);
}

bool ATable::Config::isValid() const
{
	if (sampleRate <= 0)		return false;
	if (hopSize <= 0)			return false;
	return true;
}
