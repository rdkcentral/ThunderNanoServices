/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "Module.h"
#include <interfaces/IPower.h>

namespace WPEFramework {

class PowerImplementation : public Exchange::IPower,
							virtual public WPEFramework::Core::Thread {
private:
    PowerImplementation(const PowerImplementation&) = delete;
    PowerImplementation& operator=(const PowerImplementation&) = delete;

public:
    PowerImplementation()
		: _adminLock()
		, _timeout(0)
		, _wait(false, false)
    {
		struct stat properties;
		const char sEarlySuspendTriggerFile[] = "/sys/power/early_suspend_trigger";
		
		_currentState = PCState::On;
		_newState = PCState::On;
		TRACE(Trace::Warning, (_T("PowerImplementation::Construct()")));
		
		if (stat(sEarlySuspendTriggerFile, &properties) == -1)
			ASSERT(false);
		
		int fd = open(sEarlySuspendTriggerFile, O_WRONLY);
		if (fd > 0) {
			write(fd, "0", strlen("1"));
			close(fd);
		} else {
			TRACE(Trace::Error, (_T("Not able to access %s."),
						sEarlySuspendTriggerFile));
	    }
		Run();
	}

    virtual ~PowerImplementation()
    {
		Stop();
		_wait.SetEvent();
		Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
        TRACE(Trace::Warning, (_T("PowerImplementation::Destruct()")));
    }

	uint32_t Worker();
    BEGIN_INTERFACE_MAP(PowerImplementation)
    INTERFACE_ENTRY(Exchange::IPower)
    END_INTERFACE_MAP

    // IPower methods
	virtual void Register(Exchange::IPower::INotification* sink) override;
	virtual void Unregister(Exchange::IPower::INotification* sink) override;
    virtual PCState GetState() const override;
	virtual PCStatus SetState(const PCState state, const uint32_t waitTime) override;

	virtual void PowerKey() override
    {
        TRACE(Trace::Information, (_T("PowerImplementation::PowerKey()")));
    }

	virtual void Configure(const string& settings)
    {
        TRACE(Trace::Information, (_T("PowerImplementation::Configure()")));
    }

private:
	void NotifyStateChange(Exchange::IPower::PCState pState);

private:
	uint32_t _timeout;
    PCState _currentState;
    PCState _newState;
	Core::Event _wait;
	Core::CriticalSection _lock;	
	Core::CriticalSection _adminLock;
	std::list<Exchange::IPower::INotification*> _notificationClients;
};

// The essence of making the IPower interface available. This instantiates
// an object that can be created from the outside of the library by looking
// for the PowerImplementation class name, that realizes the IPower interface.
SERVICE_REGISTRATION(PowerImplementation, 1, 0);
}

#include <fstream>

using namespace WPEFramework;

uint32_t PowerImplementation::Worker()
{
	int fd = -1;
	uint32_t timeOutInSec = 0;
	bool triggerStateChange = false;
	struct stat properties;
	const char sStateFile[] = "/sys/power/state";
	const char sEarlySuspendTriggerFile[] = "/sys/power/early_suspend_trigger";

	while (IsRunning() == true) {
		_wait.Lock();
		_lock.Lock();
		if ((_currentState != _newState) && (true == IsRunning()) &&
				((Exchange::IPower::SuspendToRAM == _newState) ||
				 (Exchange::IPower::On == _newState))) {
			timeOutInSec = _timeout;
			_timeout = 0;
			triggerStateChange = true;
		}
		_lock.Unlock();
		if (triggerStateChange) {
			NotifyStateChange(_newState);
			/* Timeout specified is in seconds. */
			SleepMs(timeOutInSec * 1000);

			if (Exchange::IPower::SuspendToRAM == _newState) {
				/* Since only ON & STR is supported; query can be done when in ON. */
				/* Kept for future extension. */
				//_currentState = _newState;
				if (stat(sEarlySuspendTriggerFile, &properties) == -1)
					ASSERT(false);
				if (stat(sStateFile, &properties) == -1)
					ASSERT(false);

				fd = open(sEarlySuspendTriggerFile, O_WRONLY);
				if (fd > 0) {
					write(fd, "1", strlen("1"));
					close(fd);
				} else {
					TRACE(Trace::Error, (_T("Not able to access %s."),
								sEarlySuspendTriggerFile));
				}

				fd = open(sStateFile, O_WRONLY);
				if (fd > 0) {
					/* We will be able to write state only if we are in 'On' State. */
					write(fd, "mem", strlen("mem"));
					close(fd);
				} else {
					TRACE(Trace::Error, (_T("Not able to access %s."), sStateFile));
				}

				fd = open(sEarlySuspendTriggerFile, O_WRONLY);
				if (fd > 0) {
					write(fd, "0", strlen("1"));
					close(fd);
				} else {
					TRACE(Trace::Error, (_T("Not able to access %s."),
								sEarlySuspendTriggerFile));
				}
				/*** `STATE_REVERT_LOGIC` ***/
				/* STR FREEZE happens only after ~100ms from trigger. */
				/* This sleep will make the code to execute after wake-up. */
				_newState = Exchange::IPower::On;
				SleepMs(80);
				/* Broadcast wake-up state change event. */
				NotifyStateChange(_currentState);
			} else {
				/* No implementation needed as ON state is already taken care after
				   triggering STR. See `STATE_REVERT_LOGIC`. */
			}
			triggerStateChange = false;
		}
	}
	Block();
	TRACE(Trace::Warning, (_T("Worker thread to Block state")));
	return (WPEFramework::Core::infinite);
}

void PowerImplementation::Register(Exchange::IPower::INotification* sink)
{
	_adminLock.Lock();
	// Make sure a sink is not registered multiple times.
	ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), sink)
			== _notificationClients.end());
	_notificationClients.push_back(sink);
	sink->AddRef();
	_adminLock.Unlock();
	TRACE(Trace::Information, (_T("Registered a sink on the power")));
}

void PowerImplementation::Unregister(Exchange::IPower::INotification* sink)
{
	_adminLock.Lock();
	std::list<Exchange::IPower::INotification*>::iterator
		index(std::find(_notificationClients.begin(),
					_notificationClients.end(), sink));
	// Make sure you do not unregister something you did not register !!!
	ASSERT(index != _notificationClients.end());
	if (index != _notificationClients.end()) {
		(*index)->Release();
		_notificationClients.erase(index);
		TRACE(Trace::Information, (_T("Unregistered a sink on the power")));
	}
	_adminLock.Unlock();
}

void PowerImplementation::NotifyStateChange(Exchange::IPower::PCState pState)
{
	TRACE(Trace::Information, (_T("Sending Event: new power state :%d"), pState));
	_adminLock.Lock();
	std::list<Exchange::IPower::INotification*>::iterator
		index(_notificationClients.begin());
	while (index != _notificationClients.end()) {
		(*index)->StateChange(pState);
		index++;
	}
	_adminLock.Unlock();
}

Exchange::IPower::PCStatus PowerImplementation::SetState(const Exchange::IPower::PCState state,
		const uint32_t timeout)
{
	Exchange::IPower::PCStatus retStatus = PCFailure;
	TRACE(Trace::Information,
			(_T("NewImpl: PowerImplementation::SetState(%d, %d)"), state, timeout));
	/* Check if supported mode */
	if ((Exchange::IPower::SuspendToRAM == state) ||
			(Exchange::IPower::On == state)) {
		_timeout = timeout;
		_newState = state;
		_lock.Lock();
		_wait.SetEvent();
		_lock.Unlock();
		retStatus = PCSuccess;
	}
	return (retStatus);
}

Exchange::IPower::PCState PowerImplementation::GetState() const
{
	TRACE(Trace::Information, (_T("PowerImplementation::GetState() => %d"),
				_currentState));
	/* No implementation needed as ON & STR is the only supported modes. */
	return (_currentState);
}

