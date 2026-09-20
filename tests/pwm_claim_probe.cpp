#include <GrevirPeripherals.h>

namespace {
struct GPIO {
  static void pinMode(unsigned, ardo::gpio::OutputPinMode);
  static void digitalWrite(unsigned, bool);
};
template <typename Claim>
struct Backend {
  static constexpr unsigned timer_bits = 9;
  using Claims = Claim;
  static void setPwm(std::uint16_t);
};
template <unsigned Pin, typename... Claims>
using Pwm = ardo::HardwarePwm<ardo::OutputPin<GPIO, Pin>, Backend<ardo::ResourceClaim<Claims...>>>;
template <typename... Params>
struct Module : ardo::ModuleBase<ardo::Parameters<Params...>> {};
struct Slow {};
struct Fast {};
struct Channel {};
using Timer = ardo::HardwareTimer<1>;
using SlowClaim = ardo::shared_use_claim<Timer, 0, Slow>;
using FastClaim = ardo::shared_use_claim<Timer, 0, Fast>;

template <int Id>
struct Case;
template <>
struct Case<0> {
  using App = ardo::Application<Module<Pwm<12, Timer>, Pwm<13, ardo::HardwareTimer<2>>>>;
};
template <>
struct Case<1> {
  using App = ardo::Application<Module<Pwm<12, Timer>, Pwm<12, ardo::HardwareTimer<2>>>>;
};
template <>
struct Case<2> {
  using App = ardo::Application<Module<Pwm<12, Timer>, Pwm<13, Timer>>>;
};
template <>
struct Case<3> {
  using App = ardo::Application<Module<Pwm<12, ardo::GPIOResource<12>>>>;
};
template <>
struct Case<4> {
  using App = ardo::Application<Module<
    Pwm<12, SlowClaim, ardo::range_claim<Channel, 0, 1>>,
    Pwm<13, SlowClaim, ardo::range_claim<Channel, 1, 2>>>>;
};
template <>
struct Case<5> {
  using App = ardo::Application<Module<Pwm<12, SlowClaim>, Pwm<13, FastClaim>>>;
};
template <>
struct Case<6> {
  using App = ardo::Application<Module<
    Pwm<12, SlowClaim, ardo::range_claim<Channel, 0, 2>>,
    Pwm<13, SlowClaim, ardo::range_claim<Channel, 1, 3>>>>;
};
struct ExternalTimer {
  using Claims = ardo::ResourceClaim<Timer>;
  static void runSetup() {}
  static void runLoop() {}
};
template <>
struct Case<7> {
  using App = ardo::Application<Module<Pwm<12, Timer>>, Module<ExternalTimer>>;
};
using App = typename Case<PWM_CASE>::App;
void instantiate() {
  App::runSetup();
  App::runLoop();
}
}
