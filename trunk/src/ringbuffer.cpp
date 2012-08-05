// ring_buffer.cpp

#include "ringbuffer.h"

ring_buffer::ring_buffer() {
	head = 0;
    tail = maxSize - 1;
	count=0;
}
bool ring_buffer::full() const volatile {
    return (count == maxSize);
}
bool ring_buffer::empty() const volatile {
    return (count == 0);
}
uint8_t ring_buffer::front() const volatile {
	    return array[head];
}
void ring_buffer::pop() volatile {
    count--;
    head = (head +1) % maxSize;
}
uint8_t ring_buffer::bytesFree() volatile {
	return (maxSize - count);
}

void ring_buffer::push(uint8_t c) volatile {
    tail = (tail + 1) % maxSize;
    count++;
    array[tail] = c;
}

