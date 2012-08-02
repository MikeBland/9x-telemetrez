// ring_buffer.h

#ifndef RING_BUFFER_INCLUDED
#define RING_BUFFER_INCLUDED

#include <avr/io.h>

#define rb_size 60

typedef class ring_buffer ring_buffer;
class ring_buffer
{
	public:
		ring_buffer(); // constructor
        bool full() const volatile; // returns true is the buffer is full
		bool empty() const volatile; // returns true if the buffer is empty
		uint8_t front() const volatile; // returns the next element in the array
		void pop() volatile; // removes the first element from the array
		void push(uint8_t c) volatile; // add an element to the array
		uint8_t bytesFree() volatile; // return the number of free bytes
	private:
		uint8_t array[rb_size];
		uint8_t head, tail;
		uint8_t freeBytes;
};

#endif

