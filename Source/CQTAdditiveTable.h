#pragma once
#include <string>
#include <vector>

#include "cereal/access.hpp"
#include "cereal/types/vector.hpp"

class CQTAdditiveTable
{
	friend class cereal::access;
public:
	
	class Config
	{
	public:
		float sampleRate{ 0 };
		float hopSize{ 0 };
		float binsPerSemitone{ 0 };

		bool operator==(const Config & rhs) const;
		bool operator!=(const Config & rhs) const;

		bool isValid() const;

		template<class Archive>
		void serialize(Archive & archive)
		{
			archive(sampleRate, hopSize, binsPerSemitone);
		}
	};

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
	
public:
	// #################### CONSTRUCTOR ####################
	
	CQTAdditiveTable();
		
	static CQTAdditiveTable * interpolateTable(const CQTAdditiveTable &t1, const CQTAdditiveTable &t2, int targetMidi);

	
	// #################### GETTER // SETTER ####################

	inline unsigned int getMidiNote() const;
	inline void			setMidiNote(unsigned int note);

	inline unsigned int	estimateMidiNote() const;
	inline float		getSampleRate() const;
	inline float		getHopSize() const;
	inline float		getBinsPerSemitone() const;

	inline unsigned int	getEnvelopeLength() const;


	inline Config		getConfig() const;
	inline void 		setConfig(Config cfg);

	inline const std::vector<Bin> & getData()  const;

	inline void		setBins(std::vector<Bin> &&		 bins);
	inline void		setBins(const std::vector<Bin> & bins);

	bool			isValid() const;


	// #################### MANIPULATION ####################

	void shiftFrequencyTo(int midiTarget);

	// #################### SERIALIZATION ####################
	
	template<class Archive>
	void serialize(Archive & archive)
	{
		archive(midiNote, cfg, bins);
	}


private:
	// #################### MEMBER ####################
		

	std::vector<Bin> bins; // list of frequencies

	int	  midiNote{ -1 }; // -1: invalid

	Config cfg;

};


inline unsigned int CQTAdditiveTable::getMidiNote() const
{
	return midiNote;
}

inline void CQTAdditiveTable::setMidiNote(unsigned int note)
{
	midiNote = note;
}

inline unsigned int CQTAdditiveTable::estimateMidiNote() const
{
	auto findMaxSample = [](const std::vector<float> & data) -> float
	{
		auto max = data[0];
		for (const float & sample : data)
		{
			if (sample > max) max = sample;
		}
		return max;
	};
	
	float freqAtMaxSample = 0;
	float maxSample = 0;

	for (const auto & bin : bins)
	{
		auto maxInEnvelope = findMaxSample(bin.envelope);
		
		if (maxInEnvelope > maxSample)
		{
			maxSample = maxInEnvelope;
			freqAtMaxSample = bin.frequency;
		}
	}

	auto noteAtMaxSample = std::log2(freqAtMaxSample / 440.) * 12 + 69;
	return std::round(noteAtMaxSample);
}

inline float CQTAdditiveTable::getSampleRate() const
{
	return cfg.sampleRate;
}

inline float CQTAdditiveTable::getHopSize() const
{
	return cfg.hopSize;
}

inline float CQTAdditiveTable::getBinsPerSemitone() const
{
	return cfg.binsPerSemitone;
}

inline unsigned int CQTAdditiveTable::getEnvelopeLength() const
{
	return bins[0].envelope.size();
}

inline CQTAdditiveTable::Config CQTAdditiveTable::getConfig() const
{
	return cfg;
}

inline void CQTAdditiveTable::setConfig(Config cfg)
{
	this->cfg = cfg;
}

inline const std::vector<CQTAdditiveTable::Bin> & CQTAdditiveTable::getData() const
{
	return bins;
}

inline void CQTAdditiveTable::setBins(std::vector<Bin>&& bins)
{
	this->bins = std::move(bins);
}

inline void CQTAdditiveTable::setBins(const std::vector<Bin>& bins)
{
	this->bins = bins;
}
