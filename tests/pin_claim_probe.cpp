#include <GrevirPeripherals.h>

namespace {
struct Backend {
  static void pinMode(unsigned, ardo::gpio::InputPinMode);
  static void pinMode(unsigned, ardo::gpio::OutputPinMode);
  static bool digitalRead(unsigned);
  static void digitalWrite(unsigned, bool);
};
using Input = ardo::InputPin<Backend, 1>;
using Output = ardo::OutputPin<Backend, OUTPUT_PIN>;
struct Module : ardo::ModuleBase<ardo::Parameters<Input, Output>> {};
void instantiate_lifecycle() {
  ardo::Application<Module>::runSetup();
  ardo::Application<Module>::runLoop();
}
}
