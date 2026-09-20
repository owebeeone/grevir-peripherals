#pragma once

#include <grevir/base/utility.hpp>
#include <grevir/core/resource_claims.hpp>
#include <grevir/peripherals/gpio/interfaces.hpp>
#include <grevir/peripherals/gpio/modes.hpp>

namespace ardo {

/**
 * The basic digital input pin.
 */
template <typename Backend, unsigned P,  // The digital/gpio pin number.
  gpio::InputPinMode M = gpio::InputPinMode::PullUp, 
  typename... Base>
class InputPin : public Base..., public setl::not_copyable {
protected:
  InputPin() {}  // Private constructor.
public:
  static constexpr unsigned PIN = P;

  // Claim resources.
  using Claims = ResourceClaim<GPIOResource<PIN>>;

  static const InputPin pin;

  inline static void runSetup() {
    Backend::pinMode(PIN, M);
  }

  inline static void runLoop() {}

  static bool get() {
    return Backend::digitalRead(PIN);
  }

  // May override base class.
  bool getPin() const {
    return get();
  }

  // May override base class.
  unsigned pinNo() const {
    return PIN;
  }
};

template <typename Backend, unsigned P, gpio::InputPinMode M, typename... Base>
const InputPin<Backend, P, M, Base...> InputPin<Backend, P, M, Base...>::pin;

} // namespace ardo
