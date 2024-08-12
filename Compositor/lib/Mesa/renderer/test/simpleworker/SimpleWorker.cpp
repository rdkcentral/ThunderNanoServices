#include "simpleworker/SimpleWorker.h"

MODULE_NAME_ARCHIVE_DECLARATION

namespace Thunder {
	namespace Core {

		SimpleWorker::SimpleWorker()
			: _timer(Core::Thread::DefaultStackSize(), _T("SimpleWorker")) {
		}

		SimpleWorker::~SimpleWorker() {
			_timer.Flush();
		}

		uint32_t SimpleWorker::Schedule(ICallback* callback, const Core::Time& triggerTime) {
			uint32_t result = Core::ERROR_NONE;

			_timer.Schedule(triggerTime, Dispatcher(callback));

			return (result);
		}

		uint32_t SimpleWorker::Revoke(ICallback* callback) {
			return (_timer.Revoke(Dispatcher(callback)) ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY);
		}
	}
}
