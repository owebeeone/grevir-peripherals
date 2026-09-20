#include <GrevirPeripherals.h>
#include <cstdint>

namespace {
struct Backend {
  inline static bool configured = false;
  inline static bool level = false;
  inline static bool inputLevel = true;
  inline static bool inputConfigured = false;
  static void pinMode(unsigned pin, ardo::gpio::InputPinMode mode) {
    inputConfigured = pin == 4 && mode == ardo::gpio::InputPinMode::PullUp;
  }
  static bool digitalRead(unsigned) { return inputLevel; }
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
using Input = ardo::DebounceInput<ardo::InputPin<Backend, 4>, Clock, 10>;
using Button = ardo::ButtonEventModule<Input, Clock>;
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
  if (!Backend::level) {
    return 3;
  }
  using ButtonApp = ardo::Application<Button>;
  ButtonApp::runSetup();
  if (!Backend::inputConfigured || Button::get() != Button::None) {
    return 4;
  }
  Backend::inputLevel = false;
  Clock::ticks = 20;
  ButtonApp::runLoop();
  Clock::ticks = 31;
  ButtonApp::runLoop();
  Clock::ticks = 331;
  ButtonApp::runLoop();
  if (Button::get() != Button::LongClick || Button::get() != Button::None) {
    return 5;
  }
  const setl::Period<int> duration(12);
  return (duration / 3).get() == 4 ? 0 : 6;
}
