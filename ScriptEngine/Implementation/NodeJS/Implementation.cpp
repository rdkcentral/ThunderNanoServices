#include "../../Module.h"
#include "../../Implementation.h"

#include <messaging/messaging.h>

// Node uses the EXTERNAL as an enum value undef it before it kill everything
#undef EXTERNAL

#include <node.h>

#ifdef __WINDOWS__
#pragma comment(lib, "libnode.lib")
#else

using namespace Thunder;

class FileDescriptorRedirect : public Core::IResource {
public:
	FileDescriptorRedirect() = delete;
	FileDescriptorRedirect(FileDescriptorRedirect&&) = delete;
	FileDescriptorRedirect(const FileDescriptorRedirect&) = delete;
	FileDescriptorRedirect& operator= (const FileDescriptorRedirect&) = delete;

	FileDescriptorRedirect(int fileDescriptor)
		: _locator(fileDescriptor)
		, _original(-1)
		, _offset(0) {

		if (::pipe(_pipe) == 0) {          /* make a pipe */
			_original = ::dup(_locator);  /* save stdout for display later */
			::dup2(_pipe[1], _locator);   /* redirect stdout to the pipe */
			::close(_pipe[1]);
			::fcntl(_pipe[0], F_SETFL, ::fcntl(_pipe[0], F_GETFL) | O_NONBLOCK);
		}
	}
	~FileDescriptorRedirect() {
		if (_original != -1) {
			dup2(_original, _locator);  /* reconnect stdout for testing */
		}
	}

public:
	handle Descriptor() const override {
		return (static_cast<Core::IResource::handle>(_pipe[0]));
	}
	uint16_t Events() override {
		return (POLLHUP | POLLRDHUP | POLLIN);
	}
	void Handle(const uint16_t events) override {
		if (events & POLLIN) {
			ssize_t loaded = ::read(_pipe[0], &(_buffer[_offset]), sizeof(_buffer) - _offset); /* read from pipe into buffer */

			if ( (loaded != -1) && (loaded > 0) ) {
				_offset += loaded;

			}
		} 
	}

private:
	int _locator;
	int _original;
	uint32_t _offset;
	int _pipe[2];
	char _buffer[1024];
};

#endif

namespace Thunder {

	namespace Implementation {

		class Config : public Core::JSON::Container {
		public:
			Config(Config&&) = delete;
			Config(const Config&) = delete;
			Config& operator=(const Config&) = delete;

			Config()
				: Core::JSON::Container()
				, Workers(4) {
				Add(_T("workers"), &Workers);
			}
			~Config() override = default;

		public:
			Core::JSON::DecUInt8 Workers;

		};
	}
}

using namespace Thunder;

struct platform {
public:
	platform() = delete;
	platform(platform&&) = delete;
	platform(const platform&) = delete;
	platform& operator= (const platform&) = delete;

	platform(const char* configuration)
		: _platform()
		, _environment() {

		Implementation::Config config; config.FromString(string(configuration));

		std::vector<std::string> args = { "embeddednode" };

		// Parse Node.js CLI options, and print any errors that have occurred while
		// trying to parse them.
		std::unique_ptr<node::InitializationResult> result =
			node::InitializeOncePerProcess(
				args, 
				{
					node::ProcessInitializationFlags::kNoInitializeV8,
					node::ProcessInitializationFlags::kNoInitializeNodeV8Platform

				}
			);

		if (result->exit_code() != 0) {
			for (const std::string& err : result->errors())
				TRACE(Trace::Error, (_T("NodeJS platform Initialization: %s"), err.c_str()));
		}
		else {
			if (config.Workers.Value() > 0) {
				// Create a v8::Platform instance. `MultiIsolatePlatform::Create()` is a way
				// to create a v8::Platform instance that Node.js can use when creating
				// Worker threads. When no `MultiIsolatePlatform` instance is present,
				// Worker threads are disabled.
				_platform = node::MultiIsolatePlatform::Create(config.Workers.Value());
				v8::V8::InitializePlatform(_platform.get());
			}
	
			v8::V8::Initialize();

			// Setup up a libuv event loop, v8::Isolate, and Node.js Environment.
			std::vector<std::string> exec_args;
			std::vector<std::string> errors;

			_environment =
				node::CommonEnvironmentSetup::Create(_platform.get(), &errors, args, exec_args);

			for (const std::string& err : result->errors())
				TRACE(Trace::Error, (_T("NodeJS environment Initialization: %s"), err.c_str()));

			if (_environment != nullptr) {
				{
					// obtain + lock iolsate
					v8::Isolate* isolate = _environment->isolate();
					node::Environment* node_env = _environment->env();

					v8::Locker locker(isolate);
					v8::Isolate::Scope isolateScope(isolate);
					v8::HandleScope handle_scope(isolate);
					v8::Context::Scope context_scope(_environment->context());

					v8::MaybeLocal<v8::Value> loadenv_ret = node::LoadEnvironment(
						node_env,
						[&](const node::StartExecutionCallbackInfo& info) -> v8::MaybeLocal<v8::Value> {
							_require.Reset(isolate, info.native_require);
							_process.Reset(isolate, info.process_object);
							return(v8::Null(isolate));
						}
					);
				}
			}
		}
	}
	~platform() {
		node::Stop(_environment->env());

		_require.Reset();
		_process.Reset();
		_environment.reset();
		_platform.reset();

		v8::V8::Dispose();
		v8::V8::DisposePlatform();
	}

public:
	bool IsValid() const {
		return (_environment != nullptr);
	}
	uint32_t Prepare(const uint32_t length, const char script[]) {
		_script = string(script, length);
		return (Core::ERROR_NONE);
	}
	uint32_t Execute() {
		uint32_t result = Core::ERROR_UNAVAILABLE;

		// obtain + lock iolsate
		v8::Isolate* isolate = _environment->isolate();

		v8::Locker locker(isolate);
		v8::Isolate::Scope isolateScope(isolate);
		v8::HandleScope handle_scope(isolate);
		v8::Context::Scope context_scope(_environment->context());

		v8::Local<v8::Object> vm = LoadRequire(isolate, _T("vm"));
		v8::Local<v8::Value>  object = Execute(isolate, vm, _T("runInThisContext"), { _script });

		return (result);
	}
	uint32_t Abort() {
		if (_environment != nullptr) {
			node::Stop(_environment->env());
		}
		_script.clear();
		return (Core::ERROR_NONE);
	}

private:
	inline v8::Local<v8::Value> Execute(v8::Isolate * isolate, v8::Local<v8::Object>& classObject, const string & method, const std::vector<string>& parameters) {
		int index = 0;
		v8::Local<v8::String> methodName = v8::String::NewFromUtf8(isolate, method.c_str(), v8::NewStringType::kNormal, static_cast<int>(method.length())).ToLocalChecked();
		v8::Local<v8::Function> methodFunction = classObject->Get(isolate->GetCurrentContext(), methodName).ToLocalChecked().As<v8::Function>();
		v8::Local<v8::Value>* function_args = static_cast< v8::Local<v8::Value>* >(::alloca(sizeof(v8::Local<v8::Value>) * parameters.size()));

		for (const string& entry : parameters) {
			function_args[index] = v8::String::NewFromUtf8(isolate, entry.c_str(), v8::NewStringType::kNormal, static_cast<int>(entry.length())).ToLocalChecked();
			index++;
		}

		return (methodFunction->Call(
			isolate->GetCurrentContext(),
			v8::Null(isolate),
			1,
			function_args).ToLocalChecked());
	}
	v8::Local<v8::Object> LoadRequire(v8::Isolate* isolate, const string& require) {
		v8::Local<v8::String> vm_string = v8::String::NewFromUtf8(isolate, require.c_str(), v8::NewStringType::kNormal, static_cast<int>(require.length())).ToLocalChecked();
		v8::Local<v8::Value> function_args[1];
		function_args[0] = vm_string;

		v8::Local<v8::Value> vm = _require.Get(isolate)->Call(
			isolate->GetCurrentContext(),
			v8::Null(isolate),
			1,
			function_args).ToLocalChecked();

		return (vm.As<v8::Object>());
	}
private:
	std::unique_ptr<node::MultiIsolatePlatform> _platform;
	std::unique_ptr<node::CommonEnvironmentSetup> _environment;
	v8::Global<v8::Function> _require;
	v8::Global<v8::Object> _process;
	string _script;
};

platform* script_create_platform(const char configuration[]) {
	platform* instance = new platform(configuration);

	if ((instance != nullptr) && (instance->IsValid() == false)) {
		delete instance;
		instance = nullptr;
	}

	return (instance);
}

void script_destroy_platform(platform* instance) {
	if (instance != nullptr) {
		delete instance;
	}
}

uint32_t script_prepare(platform* instance, const uint32_t length, const char script[]) {
	uint32_t result = Core::ERROR_BAD_REQUEST;

	if (instance != nullptr) {
		result = instance->Prepare(length, script);
	}

	return (result);
}

uint32_t script_execute(platform* instance) {
	uint32_t result = Core::ERROR_BAD_REQUEST;

	if (instance != nullptr) {
		result = instance->Execute();
	}

	return (result);
}

uint32_t script_abort(platform* instance) {
	uint32_t result = Core::ERROR_BAD_REQUEST;

	if (instance != nullptr) {
		result = instance->Abort();
	}
	return (result);
}

