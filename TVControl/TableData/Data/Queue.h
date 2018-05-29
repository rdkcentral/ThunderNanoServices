#ifndef __TVQUEUE_H
#define __TVQUEUE_H

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename T>
class Queue {
private:
    pthread_mutex_t _mutex;
    pthread_cond_t _condition;
    std::deque<T> _queue;

public:
    Queue()
        : _mutex(PTHREAD_MUTEX_INITIALIZER)
        , _condition(PTHREAD_COND_INITIALIZER)
    {
    }

public:
    void Push(T const& value)
    {
        pthread_mutex_lock(&_mutex);
        _queue.push_back(value);
        pthread_cond_signal(&_condition);
        pthread_mutex_unlock(&_mutex);
    }

    void PushPriority(T const& value)
    {
        pthread_mutex_lock(&_mutex);
        _queue.push_front(value);
        pthread_cond_signal(&_condition);
        pthread_mutex_unlock(&_mutex);
    }

    T Pop()
    {
        pthread_mutex_lock(&_mutex);
        if (_queue.empty())
            pthread_cond_wait(&_condition, &_mutex);
        T ret = _queue.front();
        _queue.pop_front();
        pthread_mutex_unlock(&_mutex);
        return ret;
    }

    bool Get(T& value)
    {
        pthread_mutex_lock(&_mutex);
        if (_queue.empty())
            return false;
        value = _queue.front();
        _queue.pop_front();
        pthread_mutex_unlock(&_mutex);
        return true;
    }

    bool Clear()
    {
        pthread_mutex_lock(&_mutex);
        while (!_queue.empty()) {
            T ret = _queue.front();
            _queue.pop_front();
            free(ret);
        }
        pthread_cond_signal(&_condition);
        pthread_mutex_unlock(&_mutex);
        return true;
    }

    pthread_mutex_t* GetMutex()
    {
        return &_mutex;
    }

    pthread_cond_t* GetMutexCondition()
    {
        return &_condition;
    }

    std::deque<T>& GetQueue()
    {
        return _queue;
    }
};
#endif
