#pragma once

#include <stdlib.h>
#include <cmath>
#include <algorithm>

template <typename T>
class StackBuffer
{
public:
	inline StackBuffer(unsigned int capacity);
	inline StackBuffer();
	inline ~StackBuffer();


	inline StackBuffer(const StackBuffer &other);


	inline void		    reserve(unsigned int count);
	inline unsigned int capacity() const;

	inline void			clear();
	inline unsigned int size() const;
	inline bool			empty() const;

	inline void  push_back(T val);
	inline T	 pop_back();




	// ACCESS

	inline T &operator[](unsigned int pos) const;
	inline T &at(unsigned int pos) const;

	inline T &front() const;
	inline T &back() const;



private:
	T *_buffer;
	unsigned int _capacity;

	unsigned int _back;


};

template<typename T>
inline StackBuffer<T>::StackBuffer(unsigned int capacity) : StackBuffer()
{
	reserve(capacity);
}

template <typename T>
inline StackBuffer<T>::StackBuffer()
	: _buffer(NULL), _back(0)
{
	_buffer = new T[1];
	_back = 0; 
	_capacity = 1;

}

template <typename T>
inline StackBuffer<T>::~StackBuffer()
{
	if (_buffer) delete[] _buffer;
}

template<typename T>
inline StackBuffer<T>::StackBuffer(const StackBuffer & other) :StackBuffer(other._capacity)
{
	for (int i = 0; i < other.size(); i++)
	{
		this->push_back(other[i]);
	}
}

template <typename T>
inline void StackBuffer<T>::reserve(unsigned int count)
{
	unsigned int newCapacity = count;

	if (_capacity != newCapacity)
	{
		T * newBuffer = new T[newCapacity];
		int     newBack = 0;

		unsigned int elementsToCopy = std::min(size(), newCapacity);
		for (int i = 0; i < elementsToCopy; i++)
		{
			newBuffer[newBack++] = _buffer[i];
		}

		delete[] _buffer;
		_back = newBack;
		_buffer = newBuffer;
		_capacity = newCapacity;
	}
}

template <typename T>
inline unsigned int StackBuffer<T>::capacity() const
{
	return _capacity;
}

template <typename T>
inline void StackBuffer<T>::clear()
{
	_back = 0;
}

template <typename T>
inline unsigned int StackBuffer<T>::size() const
{
	return _back;
}

template <typename T>
inline bool StackBuffer<T>::empty() const
{
	return _back == 0;
}

template <typename T>
inline void StackBuffer<T>::push_back(T val)
{
	_buffer[_back] = val;
	_back++;
}

template <typename T>
inline T StackBuffer<T>::pop_back()
{
	_back--;
	T val = _buffer[_back];
	return val;
}

template <typename T>
inline T &StackBuffer<T>::operator[](unsigned int pos) const
{
	return _buffer[pos];
}

template <typename T>
inline T &StackBuffer<T>::at(unsigned int pos) const
{
	return _buffer[pos];
}

template <typename T>
inline T &StackBuffer<T>::front()  const
{
	return _buffer[0];
}

template <typename T>
inline T &StackBuffer<T>::back()  const
{
	return _buffer[_back-1];
}
