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
using Sequence = ardo::Sequence<std::uint32_t, 10, 20>;
void instantiate() {
  ardo::Application<Module>::runSetup();
  ardo::Application<Module>::runLoop();
  Output::set(Input::get());
  ardo::TimePoller<unsigned, Clock> timer;
  timer.poll(Clock::TimeType::period_type(10));
  ardo::CyclicTimeSequencePoller<Sequence, Clock> cyclic;
  cyclic.poll();
  ardo::TimerSequencePoller<Sequence, Clock> finite;
  finite.poll();
}
}
