#ifndef __DESIGNATORPARSER_H
#define __DESIGNATORPARSER_H

#include "Module.h"
#include <broadcast/broadcast.h>

namespace WPEFramework {

namespace Broadcast {

// Convert a string to the seperate elements, needed for DVB.
// Syntax: tune://frequency=714000000;modulation=8;pgmno=813;symbol=6875000
class Designator {
private:
  Designator() = delete;
  Designator(const Designator &) = delete;
  Designator &operator=(const Designator &) = delete;

public:
  Designator(const string &designator)
      : _header(), _frequency(0), _symbolRate(0), _modulation(),
        _spectral(SpectralInversion::Auto) {
    uint16_t offset = 0;
    bool found = false;
    while ((found == false) && (offset < designator.length())) {
      while ((designator[offset] != ':') && (offset < designator.length())) {
        offset++;
      }
      if ((designator[offset + 0] != ':') || (designator[offset + 1] != '/') ||
          (designator[offset + 2] != '/')) {
        offset++;
      } else {
        found = true;
      }
    }
    if (found == true) {
      if (offset > 0) {
        _header = Core::TextFragment(designator, 0, offset - 1);
      }
      Core::TextSegmentIterator iterator(Core::TextSegmentIterator(
          Core::TextFragment(designator, offset + 3,
                             (designator.length() - offset - 3)),
          false, ';'));
      while (iterator.Next() == true) {
        Core::TextSegmentIterator index(iterator.Current(), false, '=');

        // See if we have a key.
        if (index.Next() == true) {
          Core::TextFragment key(index.Current());

          // See if we have a value..
          if (index.Next() == true) {
            // Yes we have, let absorb the value
            if (key == _T("frequency")) {
              _frequency = Core::NumberType<uint32_t>(index.Current()).Value();
            } else if (key == _T("modulation")) {
              _modulation = static_cast<Broadcast::Modulation>(
                  Core::NumberType<uint16_t>(index.Current()).Value());
            } else if (key == _T("symbol")) {
              _symbolRate = Core::NumberType<uint32_t>(index.Current()).Value();
            } else if (key == _T("pgmno")) {
              _program = Core::NumberType<uint16_t>(index.Current()).Value();
            } else if (key == _T("spectral")) {
              _symbolRate = static_cast<Broadcast::SpectralInversion>(
                  Core::NumberType<uint32_t>(index.Current()).Value());
            }
          }
        }
      }
    }
  }
  ~Designator() {}

public:
  inline uint32_t Frequency() const { return (_frequency); }
  inline Broadcast::Modulation Modulation() const { return (_modulation); }
  inline uint32_t SymbolRate() const { return (_symbolRate); }
  inline uint16_t ProgramNumber() const { return (_program); }
  inline Broadcast::SpectralInversion Spectral() const { return (_spectral); }

private:
  Core::TextFragment _header;
  uint32_t _frequency;
  uint32_t _symbolRate;
  uint16_t _program;
  Broadcast::Modulation _modulation;
  Broadcast::SpectralInversion _spectral;
};
} // namespace Broadcast
} // namespace WPEFramework

#endif // __DESIGNATORPARSER_H
