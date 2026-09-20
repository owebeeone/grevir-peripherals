#pragma once

#include <GrevirPeripherals.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace peripheral_mock {
struct GPIO {
  inline static std::array<bool, 32> inputs{};
  inline static std::vector<std::string> events;

  static void record(unsigned pin, const std::string& action) {
    events.push_back(std::to_string(pin) + ":" + action);
  }
  static void pinMode(unsigned pin, ardo::gpio::InputPinMode mode) {
    record(pin, mode == ardo::gpio::InputPinMode::PullUp ? "pullup" : "input");
  }
  static void pinMode(unsigned pin, ardo::gpio::OutputPinMode mode) {
    if (mode != ardo::gpio::OutputPinMode::Output) {
      throw "Open-drain emulation must select input or output mode";
    }
    record(pin, "output");
  }
  static void digitalWrite(unsigned pin, bool level) {
    record(pin, level ? "high" : "low");
  }
  static bool digitalRead(unsigned pin) {
    return inputs.at(pin);
  }
};

struct Clock {
  using TimeType = setl::Time<std::uint32_t>;
  inline static TimeType current{};
  static TimeType now() { return current; }
  static void set(std::uint32_t ticks) { current = TimeType(ticks); }
};
using Period = Clock::TimeType::period_type;

struct Fixture {
  Fixture() {
    Clock::set(0);
    GPIO::inputs.fill(false);
    GPIO::events.clear();
  }
};
} // namespace peripheral_mock
