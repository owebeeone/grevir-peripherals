#include <GrevirPeripherals.h>
#include <cstdint>
#include <type_traits>

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
using Raw = ardo::InputPin<Backend, 1>;
using Input = std::conditional_t<DEBOUNCE_INPUT, ardo::DebounceInput<Raw, Clock, 10>, Raw>;
using Output = ardo::OutputPin<Backend, OUTPUT_PIN>;
struct Module : ardo::ModuleBase<ardo::Parameters<Input, Output>> {};
void instantiate_lifecycle() {
  ardo::Application<Module>::runSetup();
  ardo::Application<Module>::runLoop();
}
}
