#pragma once
#include <string>
#include <vector>
#include <cmath>
#include "cereal/access.hpp"
#include "cereal/types/vector.hpp"

#include "ATable.h"

class CQTTable : public ATable
{
	friend class cereal::access;

	// #################### CONSTRUCTOR ####################
public:
	
	CQTTable();
		

	
	// #################### GETTER // SETTER ####################
	
	virtual bool isValid() const override;
	void setBinsPerSemitone(unsigned int binsPerSemitone);

	// #################### MANIPULATION ####################

	virtual void shiftFrequencyTo(int midiTarget) override;

	// #################### SERIALIZATION ####################
	
	template <class Archive>
	void serialize(Archive & ar)
	{
		// We pass this cast to the base type for each base type we
		// need to serialize.  Do this instead of calling serialize functions
		// directly
		ar(cereal::base_class<ATable>(this), binsPerSemitone);
	}

private:
	unsigned int binsPerSemitone{ 0 };


	// Inherited via ATable
	virtual ATable * interpolateTable(const ATable & secondTable, int targetMidi) const override;


	// Inherited via ATable
	virtual ATable * createShiftedTable(int targetMidi) override;

};

