#ifndef __TVDATAQUEUE_H
#define __TVDATAQUEUE_H

#include "Queue.h"
#define PACKET_TYPE_MASK 0x03

class DataQueue : public Queue<std::pair<uint8_t*, uint16_t> > {
private:
    DataQueue() = default;
    ~DataQueue() = default;

public:
    DataQueue(const DataQueue&) = delete;
    DataQueue& operator=(const DataQueue&) = delete;

    static DataQueue& GetInstance()
    {
        static DataQueue instance;
        return instance;
    }

    bool Clear()
    {
        pthread_mutex_lock(GetMutex());
        for (auto& data : GetQueue()) {
            free(std::get<0>(data));
            GetQueue().pop_front();
        }
        pthread_cond_signal(GetMutexCondition());
        pthread_mutex_unlock(GetMutex());
        return true;
    }
};
#endif
