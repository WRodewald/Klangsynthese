#pragma once

#include <stdlib.h>
#include <cmath>
#include <algorithm>

template <typename T>
class RingBuffer
{
public:
	inline RingBuffer(unsigned int capacity);
	inline RingBuffer();
	inline RingBuffer(RingBuffer & other);
	inline ~RingBuffer();


	inline void		    reserve(unsigned int count);
	inline unsigned int capacity() const;

	inline void			clear();
	inline unsigned int size() const;
	inline bool			empty() const;

	inline void push_back(T val);
	inline void push_front(T val);

	inline T pop_back();
	inline T pop_front();




// ACCESS

	inline T &operator[](unsigned int pos) const;
	inline T &at(unsigned int pos) const;

	inline T &front() const;
	inline T &back() const;

	

private:
	T *_buffer;
	unsigned int _mask;
	unsigned int _capacity;

	
	int _front;
	int _back;


};

template<typename T>
inline RingBuffer<T>::RingBuffer(unsigned int capacity) : RingBuffer()
{
	reserve(capacity);
}

template <typename T>
inline RingBuffer<T>::RingBuffer()
	: _buffer(NULL), _front(0), _back(0)
{
	_buffer = new T[1];
	_capacity = 1;
	_mask =  0;

}

template<typename T>
inline RingBuffer<T>::RingBuffer(RingBuffer & other) : RingBuffer(other._capacity)
{
	unsigned int size = other.size();
	for (int i = 0; i < size; i++)
	{
		this->push_back(other[i]);
	}
}

template <typename T>
inline RingBuffer<T>::~RingBuffer()
{
	if (_buffer) delete[] _buffer;
}

template <typename T>
inline void RingBuffer<T>::reserve(unsigned int count)
{
	unsigned int newCapacity = std::pow(2, std::ceil(std::log2(count)));

	if (_capacity != newCapacity)
	{		
		T * newBuffer = new T[newCapacity];
		int     newFront = 0;
		int     newBack = 0;

		unsigned int elementsToCopy = std::min(size(), newCapacity);
		for (int i = 0; i < elementsToCopy; i++)
		{
			newBuffer[newBack++] = pop_front();
		}

		delete[] _buffer;
		_front    = newFront;
		if(newBack >0) _back = newBack - 1;
		

		_buffer   = newBuffer;
		_capacity = newCapacity;
		_mask     = _capacity -1;
	}
}

template <typename T>
inline unsigned int RingBuffer<T>::capacity() const
{
	return _capacity;
}

template <typename T>
inline void RingBuffer<T>::clear()
{
	_back = _front;
}

template <typename T>
inline unsigned int RingBuffer<T>::size() const
{
	int delta = _back;
	delta -= _front;
	return (delta >= 0) ? delta : delta + _capacity;
}

template <typename T>
inline bool RingBuffer<T>::empty() const
{
	return _front == _back;
}

template <typename T>
inline void RingBuffer<T>::push_back(T val)
{
	_back = (_back + 1) & _mask;
	_buffer[_back] = val;

}

template <typename T>
inline void RingBuffer<T>::push_front(T val)
{
	_front = (_front - 1 + _capacity) & _mask;
	_buffer[_front] = val;
}

template <typename T>
inline T RingBuffer<T>::pop_back()
{
	T val = _buffer[_back];
	_back = (_back - 1 + _capacity) & _mask;
	return val;
}

template <typename T>
inline T RingBuffer<T>::pop_front()
{
	T val = _buffer[_front];
	_front = (_front + 1) & _mask;
	return val;
}

template <typename T>
inline T &RingBuffer<T>::operator[](unsigned int pos) const
{
	return at(pos);
}

template <typename T>
inline T &RingBuffer<T>::at(unsigned int pos) const
{
	int index = (_front + pos) & _mask;
	return _buffer[index];
}

template <typename T>
inline T &RingBuffer<T>::front()  const
{
	return _buffer[_front];
}

template <typename T>
inline T &RingBuffer<T>::back()  const
{
	return _buffer[_back];
}
