#ifndef _PLAYER_IMPLEMENTATION_H
#define _PLAYER_IMPLEMENTATION_H

#include "Module.h"
#include <interfaces/ITVControl.h>
#include <plugins/plugins.h>
#include <tracing/tracing.h>
#include <broadcast/broadcast.h>

#include "Designator.h"

namespace WPEFramework {

namespace Player {

namespace Implementation {

class PlayerPlatform {
private:
  PlayerPlatform() = delete;
  PlayerPlatform(const PlayerPlatform &) = delete;
  PlayerPlatform &operator=(const PlayerPlatform &) = delete;

public:
  PlayerPlatform(const Exchange::IStream::streamtype type, const uint8_t index,
                 ICallback *callbacks)
      : _state(Exchange::IStream::NotAvailable), _streamType(type),
        _drmType(Exchange::IStream::Unknown), _speed(0), _absoluteTime(0),
        _begin(0), _end(~0), _rectangle(), _z(0), _callback(callbacks),
        _player(Broadcast::ITuner::Create(Core::NumberType<uint8_t>(index).Text())) {}
  virtual ~PlayerPlatform() { Terminate(); }

public:
  static uint32_t Initialize(const string& configuration) {
    Broadcast::ITuner::Initialize(configuration);
    return (Core::ERROR_NONE);
  }
  static uint32_t Deinitialize() {
    Broadcast::ITuner::Deinitialize();
    return (Core::ERROR_NONE);
  }
  inline string Metadata() const { return string(); }
  inline Exchange::IStream::streamtype Type() const {
    return (Exchange::IStream::streamtype)_streamType;
  }
  inline Exchange::IStream::drmtype DRM() const {
    return (Exchange::IStream::drmtype)_drmType;
  }
  inline Exchange::IStream::state State() const {
    return (Exchange::IStream::state)_state;
  }
  inline uint32_t Load(const string &configuration) {

    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (_player != nullptr) {

      Broadcast::Designator parser(configuration);

      result = _player->Tune(parser.Frequency(), parser.Modulation(),
                             parser.SymbolRate(), Broadcast::FEC_INNER_UNKNOWN, parser.Spectral());

      if (result != Core::ERROR_NONE) {
        TRACE(Trace::Error, (_T("Error in player load :%d"), result));
      } else {
        _player->Prepare(parser.ProgramNumber());
        _state = Exchange::IStream::Paused;
        if (_callback != nullptr) {
          _callback->StateChange(_state);
        }
      }
    }

    return result;
  }
  inline uint32_t Speed(const int32_t request) {

    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (_player != nullptr) {

      Exchange::IStream::state newState = _state;

      result = Core::ERROR_NONE; // PLAYER_RESULT status =
                                 // _player->setSpeed(request);
      if (result == Core::ERROR_NONE) {
        _speed = request;
        if (_speed != 0) {
          newState = Exchange::IStream::state::Playing;
        } else {
          newState = Exchange::IStream::state::Paused;
        }
      } else {
        TRACE(Trace::Error, (_T("Error in pause playback:%d"), result));
        newState = Exchange::IStream::state::Error;
      }

      if ((newState != _state) && (_callback != nullptr)) {
        _state = newState;
        _callback->StateChange(_state);
      }
    }

    return (result);
  }
  inline int32_t Speed() const { return _speed; }
  inline void Position(const uint64_t absoluteTime) {
    _absoluteTime = absoluteTime;
  }
  inline uint64_t Position() const { return _absoluteTime; }
  inline void TimeRange(uint64_t &begin, uint64_t &end) const {
    begin = _begin;
    end = _end;
  }
  inline const Rectangle &Window() const { return (_rectangle); }
  inline void Window(const Rectangle &rectangle) { _rectangle = rectangle; }
  inline uint32_t Order() const { return (_z); }
  inline void Order(const uint32_t order) { _z = order; }
  inline uint32_t AttachDecoder(const uint8_t index) {
    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (_player != nullptr) {

      Exchange::IStream::state newState = _state;

      result = _player->Attach(index);
      if (result != Core::ERROR_NONE) {
        TRACE(Trace::Error, (_T("Error in attach decoder %d"), result));
        newState = Exchange::IStream::state::Error;
      }

      if ((newState != _state) && (_callback != nullptr)) {
        _state = newState;
        _callback->StateChange(_state);
      }
    }
    return (result);
  }

  inline uint32_t DetachDecoder(const uint8_t index) {
    uint32_t result = Core::ERROR_UNAVAILABLE;

    if (_player != nullptr) {

      Exchange::IStream::state newState = _state;

      result = _player->Detach(index);
      if (result != Core::ERROR_NONE) {
        TRACE(Trace::Error, (_T("Error in detach decoder %d"), result));
        newState = Exchange::IStream::state::Error;
      }

      if ((newState != _state) && (_callback != nullptr)) {
        _state = newState;
        _callback->StateChange(_state);
      }
    }
    return (result);
  }

  inline void Terminate() { delete _player; }

private:
  Exchange::IStream::state _state;
  Exchange::IStream::drmtype _drmType;
  Exchange::IStream::streamtype _streamType;

  int32_t _speed;
  uint64_t _absoluteTime;
  uint64_t _begin;
  uint64_t _end;
  Rectangle _rectangle;
  uint32_t _z;

  ICallback *_callback;
  Broadcast::ITuner* _player;
};

} // namespace Implementation
} // namespace Player
} // namespace WPEFramework

#endif // _PLAYER_IMPLEMENTATION_H
