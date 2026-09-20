#pragma once

#include <grevir/base/utility.hpp>
#include <grevir/core/resource_claims.hpp>
#include <grevir/peripherals/gpio/interfaces.hpp>
#include <grevir/peripherals/gpio/modes.hpp>

namespace ardo {

/**
 * An externally managed pin. This only provides resource claims. It expects that
 * an external library will manage the pin setup. No access to the GPIO functions
 * are provided.
 */
template <unsigned P>
class ExternalPin : public setl::not_copyable {
  private:
    ExternalPin() = delete;  // No instance provided.
  public:
    static constexpr unsigned PIN = P;

    // Claim resources.
    using Claims = ResourceClaim<GPIOResource<PIN>>;

    inline static void runSetup() {}

    inline static void runLoop() {}
};

/**
 * A digital output.
 */
template <typename Backend, unsigned P, 
    gpio::OutputPinMode M = gpio::OutputPinMode::Output, 
    typename... Base>
class OutputPin : public Base..., public setl::not_copyable {
protected:
  OutputPin() {}  // Only the one instance allowed.
public:
  static constexpr unsigned PIN = P;

  // Claim resources.
  using Claims = ResourceClaim<GPIOResource<PIN>>;

  static const OutputPin pin;

  inline static void runSetup() {
    Backend::pinMode(PIN, M);
  }

  inline static void runLoop() {}

  static void set(bool level) {
    Backend::digitalWrite(PIN, level);
  }

  // Overrides OutputPinIF base class interface.
  void setPin(bool level) const {
    set(level);
  }

  // May override base class.
  unsigned pinNo() const {
    return PIN;
  }
};

template <typename Backend, unsigned P, gpio::OutputPinMode M, typename... Base>
const OutputPin<Backend, P, M, Base...> OutputPin<Backend, P, M, Base...>::pin;

/**
 * A digital output for open collector/drain pulled low when active.
 */
template <typename Backend, unsigned P, typename... Base>
class OutputPin<Backend, P, gpio::OutputPinMode::OpenDrainLowOutput, Base...> 
    : public Base..., public setl::not_copyable {
protected:
  OutputPin() {}  // Only the one instance allowed.
public:
  static constexpr unsigned PIN = P;

  // Claim resources.
  using Claims = ResourceClaim<GPIOResource<PIN>>;

  static const OutputPin pin;

  inline static void runSetup() {
    Backend::pinMode(PIN, gpio::InputPinMode::Untied);
    Backend::digitalWrite(PIN, false);
  }

  inline static void runLoop() {}

  static void set(bool level) {
    // Switching between input (no pullup mode) and output (drive low) mode
    // emulates an open collector output.
    if (level) {
      Backend::pinMode(PIN, gpio::InputPinMode::Untied);
    } else {
      Backend::pinMode(PIN, gpio::OutputPinMode::Output);
    }
  }

  // Overrides OutputPinIF base class interface.
  void setPin(bool level) const {
    set(level);
  }

  // Open drain can be read as well.
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

template <typename Backend, unsigned P, typename... Base>
const OutputPin<Backend, P, gpio::OutputPinMode::OpenDrainLowOutput, Base...>
OutputPin<Backend, P, gpio::OutputPinMode::OpenDrainLowOutput, Base...>::pin;

/**
 * A digital output for open collector/drain pulled high when active.
 */
template <typename Backend, unsigned P, typename... Base>
class OutputPin<Backend, P, gpio::OutputPinMode::OpenDrainHighOutput, Base...>
  : public Base..., public setl::not_copyable {
protected:
  OutputPin() {}  // Only the one instance allowed.
public:
  static constexpr unsigned PIN = P;

  // Claim resources.
  using Claims = ResourceClaim<GPIOResource<PIN>>;

  static const OutputPin pin;

  inline static void runSetup() {
    Backend::pinMode(PIN, gpio::InputPinMode::Untied);
    Backend::digitalWrite(PIN, true);
  }

  inline static void runLoop() {}

  static void set(bool level) {
    // Switching between input (no pullup mode) and output (drive high) mode
    // emulates an open collector output.
    if (level) {
      Backend::pinMode(PIN, gpio::OutputPinMode::Output);
    } else {
      Backend::pinMode(PIN, gpio::InputPinMode::Untied);
    }
  }

  // Overrides OutputPinIF base class interface.
  void setPin(bool level) const {
    set(level);
  }

  // Open drain can be read as well.
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

template <typename Backend, unsigned P, typename... Base>
const OutputPin<Backend, P, gpio::OutputPinMode::OpenDrainHighOutput, Base...>
    OutputPin<Backend, P, gpio::OutputPinMode::OpenDrainHighOutput, Base...>::pin;

} // namespace ardo
