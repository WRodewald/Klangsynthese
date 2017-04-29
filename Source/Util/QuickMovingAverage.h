#pragma once

#include <vector>

class QuickMovingAverage
{
public:
	inline QuickMovingAverage(unsigned int len = 1);

	inline void setLength(unsigned int len);
	inline void reserveLength(unsigned int len);

	inline float tick(float input);

	// refreshes the cached value to prevent growing errors
	inline void refreshCache();

	inline const float * getData() { return buffer.data(); };

private:


	void resize(unsigned int len);

	unsigned int readPos{ 0 };
	unsigned int writePos{ 0 };

	unsigned int mask{ 0 };
	float		 sumOfBuffer{ 0 };
	float		 iLength{ 1 };
	float		 length { 1 };

	std::vector<float> buffer;

};



inline QuickMovingAverage::QuickMovingAverage(unsigned int length)
{
	resize(length);
	setLength(length);
}

inline void QuickMovingAverage::setLength(unsigned int length)
{
	if(length == this->length) return;
	
	auto curLen = this->length;

	auto average = sumOfBuffer * iLength;

	this->length = length;
	iLength = 1.f / (float)length;
	
	resize(length);
	
	readPos = writePos = 0;
	for (int i = 0; i < length - 1; i++)
	{
		buffer[writePos] = average;
		writePos = (writePos + 1) & mask;
	}

	refreshCache();
}

inline void QuickMovingAverage::reserveLength(unsigned int len)
{
	unsigned int nextPow2 = 1;
	while (nextPow2 < len) nextPow2 *= 2;

	if(nextPow2 > buffer.capacity()) this->buffer.reserve(nextPow2);
}


inline float QuickMovingAverage::tick(float input)
{
	auto data = buffer.data();

	// write first, case len = 1 -> no filter
	data[writePos] = input;
	writePos = (writePos + 1) & mask;

	// read second
	auto out = data[readPos];
	readPos = (readPos + 1) & mask;
	
	sumOfBuffer += input;
	auto average = sumOfBuffer * iLength;
	sumOfBuffer -= out;
	return average;

}


inline void QuickMovingAverage::refreshCache()
{
	auto data = buffer.data();
	auto sum = 0.f;
	auto it = readPos;

	while (it != writePos)
	{
		sum += data[it];
		it = (it + 1) & mask;
	}
	sumOfBuffer = sum;
}

inline void QuickMovingAverage::resize(unsigned int len)
{
	unsigned int nextPow2 = 1;
	while (nextPow2 < len) nextPow2 *= 2;	
	buffer.resize(nextPow2, 0);
	mask = buffer.size() - 1;
}
