#pragma once

#include <grevir/peripherals/pwm/requirements.hpp>

namespace grevir::timer {

// One module-local demand for an entire physical timer. The first backend
// supports PWM uses; other use categories require their own candidate builder.
template <typename... Items>
struct Own {
  static_assert((std::size_t{0} + ... + pwm::IsPwmRequest<Items>::value) > 0,
    "GREVIR_TIMER_OWNER_HAS_NO_SUPPORTED_USES");
};

// The application gives each module instance a stable name. All uses in Own
// are solved against one physical timer and one selected configuration.
template <pwm::Text Name, typename Demand>
struct Instance;

template <pwm::Text Name, typename... Items>
struct Instance<Name, Own<Items...>> : pwm::Instance<Name, Items...> {};

template <unsigned Bits>
using CounterBitsAtLeast = pwm::CounterBitsAtLeast<Bits>;
template <typename DeviceTimer>
using RequireTimer = pwm::RequireTimer<DeviceTimer>;
template <pwm::Target Target, typename... Options>
using For = pwm::For<Target, Options...>;
namespace atmega328p {
using Timer0 = pwm::atmega328p::Timer0;
using Timer1 = pwm::atmega328p::Timer1;
using Timer2 = pwm::atmega328p::Timer2;
}

} // namespace grevir::timer
