#pragma once
#include <Arduino.h>

template <size_t SIZE>
class FIFOQueue
{
public:
    struct Item
    {
        uint8_t channel;  // индекс канала 0..3
    };

    FIFOQueue() : head(0), tail(0) {}

    bool push(const Item &it)
    {
        uint8_t next = (tail + 1) % SIZE;
        if (next == head)
            return false; // очередь полная

        buffer[tail] = it;
        tail = next;
        return true;
    }

    bool pop(Item &it)
    {
        if (isEmpty())
            return false;

        it = buffer[head];
        head = (head + 1) % SIZE;
        return true;
    }

    bool isEmpty() const
    {
        return head == tail;
    }

    bool isFull() const
    {
        return ((tail + 1) % SIZE) == head;
    }

private:
    Item buffer[SIZE];
    uint8_t head;
    uint8_t tail;
};
