// ring_buffer.cpp

#include "ringbuffer.h"

ring_buffer::ring_buffer() {
	head = tail = 0;
	freeBytes = rb_size;
}
bool ring_buffer::full() const volatile {
    if(freeBytes == 0) {
	        return 1;
    }
    return 0; // buffer has space in it
}
bool ring_buffer::empty() const volatile {
    if(head == tail) {
        if(freeBytes == rb_size)
	        return 1;
    }
    return 0; // buffer has data in it
}
uint8_t ring_buffer::front() const volatile {
    if(empty())
        return 0;
    else
	    return array[head];
}
void ring_buffer::pop() volatile {
    if(!empty()) {
    	if (++head >= rb_size)
	    	head = 0;
    	freeBytes++;
    }
}
uint8_t ring_buffer::bytesFree() volatile {
	return freeBytes;
}

// if the ringbuffer is full the byte won't get added
void ring_buffer::push(uint8_t c) volatile {
	uint8_t new_tail = tail;

	if (++new_tail >= rb_size)
		new_tail = 0;
	if (new_tail != head) {
		array[tail] = c;
		tail = new_tail;
		freeBytes--;
	}
}

