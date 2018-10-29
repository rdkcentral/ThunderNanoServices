#ifndef __LINUX_GPIO_H__
#define __LINUX_GPIO_H__

#include "Module.h"

namespace WPEFramework {

namespace GPIO {

class Pin {
private:
    Pin () = delete;
    Pin (const Pin&) = delete;
    Pin& operator= (const Pin&) = delete;

    class Monitor : public Core::Thread {
    private:
	Monitor(const Monitor&) = delete;
	Monitor& operator= (const Monitor&) = delete;

	typedef std::map<int, Pin&> Subscribers;

    public:
	Monitor()
	{
	}
	~Monitor()
	{
	    Stop();
	    Wait(STOPPED, Core::infinite);
	}

    public:
        void Register(Pin& pin) {

            _adminLock.Lock();

            Subscribers::iterator index = _subscribers.find(pin.Descriptor());

            // Do not register a callback that is already registered!!!
            ASSERT (index == _subscribers.end());

            if (index == _subscribers.end()) {

                // It is the first subscriber, Add it..
                _subscribers.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(pin.Descriptor()),
                                         std::forward_as_tuple(pin));

                // Trigger a new evaluation of the worker thread.
                if (_subscribers.size() == 1) 
                    Run(); 
                else 
                    Break();
            }

            _adminLock.Unlock();
        }
	void Unregister(Pin& pin) {

            _adminLock.Lock();

            Subscribers::iterator index(_subscribers.find(pin.Descriptor()));

            // Do not unregister a callback that you did not register!!!
            ASSERT(index != _subscribers.end());

            if (index != _subscribers.end()) {

                // Now it is time to reevaluate, as the map is loosing an entry.
                _subscribers.erase(index);

                // Trigger a new evaluation of the worker thread.
                Break();
            }

            _adminLock.Unlock();
        }

    private:
        inline void Break() {
            Signal(SIGUSR2);
        }

        virtual bool Initialize() {
		int err;
		sigset_t sigset;

		/* Create a sigset of all the signals that we're interested in */
		err = sigemptyset(&sigset);
		ASSERT(err == 0);
		err = sigaddset(&sigset, SIGUSR2);
		ASSERT(err == 0);

		/* We must block the signals in order for signalfd to receive them */
		err = pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
		ASSERT(err == 0);

		/* Create the signalfd */
		_pollPins[0].fd = signalfd(-1, &sigset, 0);
		ASSERT(_pollPins[0].fd != -1);

		_pollPins[0].events = POLLIN;
		_pollPins[0].revents = 0;

		return (err == 0);
	}

	virtual uint32_t  Worker() {
		uint32_t delay = 0;

		// Add entries not in the Array before we start !!!
		_adminLock.Lock();

		uint32_t count = 1;
		Subscribers::iterator index = _subscribers.begin();

		ASSERT(_subscribers.size() < (sizeof(_pollPins)/sizeof(struct pollfd)));

		// Fill in all entries required/updated..
		while (index != _subscribers.end()) {

			_pollPins[count].fd = index->first;
			_pollPins[count].events = POLLERR|POLLPRI;
			_pollPins[count].revents = 0;
			count++;

			index++;
		}

		if (count > 1) {

			_adminLock.Unlock();

			int result = poll(_pollPins, count, -1);

			_adminLock.Lock();

			if (result != -1) {

				if (_pollPins[0].revents & POLLIN) {
					/* We have a valid signal, read the info from the fd */
					struct signalfd_siginfo info;

					uint32_t VARIABLE_IS_NOT_USED bytes = read(_pollPins[0].fd, &info, sizeof(info));

					ASSERT(bytes == sizeof(info));

					// Clear the signal port..
					_pollPins[0].revents = 0;
				}

				while (--count >= 1) {

					if (_pollPins[count].revents & (POLLPRI|POLLERR)) {

						unsigned char buffer[1];

						read(_pollPins[count].fd, &buffer, sizeof(buffer));

						// Find the pin that goes with this FD.
						index = _subscribers.find(_pollPins[count].fd);

						if (index != _subscribers.end()) {
							index->second.Notify();
						}
					}
				}
			}
		}

		if (_subscribers.size() == 0) {
			Block();
			delay = Core::infinite;
		}

		_adminLock.Unlock();

		return (delay);
	}

    private:
	Core::CriticalSection _adminLock;
	Subscribers _subscribers;
	struct pollfd _pollPins[32];
    };

public:
    enum pin_mode {
        INPUT,
        OUTPUT,
        PWM_TONE,
        PWM,
        CLOCK
    };

    enum pull_mode {
        OFF = 0,
        DOWN = 1,
        UP = 2
    };


    enum trigger_mode {
        NONE    = 0x00,
        FALLING = 0x01,
        RISING  = 0x02,
        BOTH    = 0x03,
        HIGH    = 0x04,
        LOW     = 0x08
    };

    struct IObserver {
        virtual ~IObserver() {};

        virtual void Activity (Pin&  pin) = 0;
    };

public:
    Pin(const uint8_t id);
    virtual ~Pin();
  
public:
    inline uint8_t Id () const {
        return (_pin);
    }

    void Register(IObserver* callback);
    void Unregister(IObserver* callback);

    bool Get () const;
    void Set (const bool value);

    void Trigger (const trigger_mode mode);
    void Mode (const pin_mode mode);
    void Pull (const pull_mode mode);

private: 
    int Descriptor() const;
    void Flush();

    void Notify () {

        Flush();

        _adminLock.Lock();

        std::list<IObserver*>::iterator index (_observers.begin());

        while (index != _observers.end()) {
            (*index)->Activity (*this);
	    index++;
        }

        _adminLock.Unlock();
    }

private:
    Core::CriticalSection _adminLock;
    const uint8_t _pin;
    mutable int _descriptor;
    std::list<IObserver*> _observers;

    static Monitor _monitor;
};

} } // namespace WPEFramework::GPIO

#endif // __LINUX_GPIO_H__
