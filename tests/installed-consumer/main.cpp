#include <GrevirPeripherals.h>
#include <cstdint>

namespace {
struct Backend {
  inline static bool configured = false;
  inline static bool level = false;
  static void pinMode(unsigned pin, ardo::gpio::OutputPinMode mode) {
    configured = pin == 3 && mode == ardo::gpio::OutputPinMode::Output;
  }
  static void digitalWrite(unsigned, bool value) { level = value; }
};
struct Clock {
  using TimeType = setl::Time<std::uint32_t>;
  inline static std::uint32_t ticks = 0;
  static TimeType now() { return TimeType(ticks); }
};
using Pin = ardo::OutputPin<Backend, 3>;
struct Module : ardo::ModuleInstanceBase<Module, ardo::Parameters<Pin>> {
  ardo::TimePoller<unsigned, Clock> timer;
  void instanceSetup() {
    timer.reset();
    Pin::set(false);
  }
  void instanceLoop() {
    if (timer.poll(Clock::TimeType::period_type(10))) {
      Pin::set(true);
    }
  }
};
}
int main() {
  ardo::Application<Module>::runSetup();
  if (!Backend::configured || Backend::level) {
    return 1;
  }
  Clock::ticks = 10;
  ardo::Application<Module>::runLoop();
  if (Backend::level) {
    return 2;
  }
  Clock::ticks = 11;
  ardo::Application<Module>::runLoop();
  return Backend::level ? 0 : 3;
}
