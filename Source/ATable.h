#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "cereal/access.hpp"
#include "cereal/types/vector.hpp"
#include <utility>
#include <algorithm>

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

	inline void refreshActiveBins();
	inline void applyThreshold(float val);
	inline void limitNumActiveBins(unsigned int num);

	// #################### GETTER // SETTER ####################
public:

	virtual bool isValid() const = 0;

	inline const std::vector<Bin> & getBins()  const;
	inline const std::vector<const Bin*> & getActiveBins()  const;

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

	std::vector<const Bin*> activeBins;
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

inline const std::vector<ATable::Bin> & ATable::getBins() const
{
	return bins;
}
inline const std::vector<const ATable::Bin*> & ATable::getActiveBins() const
{
	return activeBins;
}

inline void ATable::setBins(std::vector<Bin>&& bins)
{
	this->bins = std::move(bins);
	activeBins.clear();
	for(auto & bin: this->bins)
	{
		activeBins.push_back(&bin);
	}
}

inline void ATable::setBins(const std::vector<Bin>& bins)
{
	this->bins = bins;
	activeBins.clear();
	for (auto & bin : this->bins)
	{
		activeBins.push_back(&bin);
	}
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

inline void ATable::refreshActiveBins()
{
	activeBins.clear();
	for (const auto & bin : this->bins) activeBins.push_back(&bin);
}

inline void ATable::applyThreshold(float val)
{
	activeBins.clear();

	for (const auto & bin : bins)
	{
		float max = 0;
		for (auto sample : bin.envelope)
			if (sample > max) max = sample;
			
		if (max > val)
			activeBins.push_back(&bin);
	}
}

inline void ATable::limitNumActiveBins(unsigned int num)
{
	if (num > bins.size()) return;
	std::vector<std::pair<Bin*, float>> map;
	map.reserve(bins.size());

	for (auto & bin : bins)
	{
		float max = 0;
		for (auto sample : bin.envelope)
			if (sample > max) max = sample;

		map.push_back({ &bin, max });
	}

	auto sortByMax = [](const std::pair<Bin*, float>& lhs, std::pair<Bin*, float> rhs) -> bool
	{
		return rhs.second < lhs.second;
	};

	std::sort(map.begin(), map.end(), sortByMax);

	activeBins.clear();
	activeBins.reserve(num);
	for (int i = 0; i < num; i++)
	{
		activeBins.push_back(map[i].first);
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
