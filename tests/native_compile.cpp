#include <GrevirPeripherals.h>
#include <cstdint>

namespace {
struct Backend {
  static void pinMode(unsigned, ardo::gpio::InputPinMode);
  static void pinMode(unsigned, ardo::gpio::OutputPinMode);
  static bool digitalRead(unsigned);
  static void digitalWrite(unsigned, bool);
};
struct Clock {
  using TimeType = setl::Time<std::uint32_t>;
  static TimeType now();
};
using Input = ardo::InputPin<Backend, 1>;
using Output = ardo::OutputPin<Backend, 2>;
struct Module : ardo::ModuleBase<ardo::Parameters<Input, Output>> {};
using Debounced = ardo::DebounceInput<Input, Clock, 10>;
using Button = ardo::ButtonEventModule<Debounced, Clock>;
struct PwmBackend {
  static constexpr unsigned timer_bits = 9;
  using Claims = ardo::ResourceClaim<ardo::HardwareTimer<1>>;
  static void setPwm(std::uint16_t);
};
using Pwm = ardo::HardwarePwm<Output, PwmBackend>;
struct PwmModule : ardo::ModuleBase<ardo::Parameters<Pwm>> {};
using Sequence = ardo::Sequence<std::uint32_t, 10, 20>;
void instantiate() {
  ardo::Application<Module>::runSetup();
  ardo::Application<Module>::runLoop();
  Output::set(Input::get());
  ardo::Application<PwmModule>::runSetup();
  ardo::Application<PwmModule>::runLoop();
  Pwm::setPwm(255);
  Pwm::pwm_pin.setPwmPin(128);
  ardo::Application<Button>::runSetup();
  ardo::Application<Button>::runLoop();
  Button::get();
  Debounced::pin.getPin();
  ardo::TimePoller<unsigned, Clock> timer;
  timer.poll(Clock::TimeType::period_type(10));
  ardo::CyclicTimeSequencePoller<Sequence, Clock> cyclic;
  cyclic.poll();
  ardo::TimerSequencePoller<Sequence, Clock> finite;
  finite.poll();
}
}
