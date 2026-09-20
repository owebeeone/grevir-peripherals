#include "mock_devices.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <type_traits>

namespace {
using namespace peripheral_mock;
using Raw = ardo::InputPin<GPIO, 9, ardo::gpio::InputPinMode::PullUp, ardo::InputPinIF>;
using Pin = ardo::DebounceInput<Raw, Clock, 10>;
static_assert(std::is_same_v<Pin::Claims, Raw::Claims>);
}

TEST_CASE_METHOD(Fixture, "debounce setup samples the initial level without a false transition", "[debounce]") {
  GPIO::inputs[9] = true;
  Pin::runSetup();
  const ardo::InputPinIF& input = Pin::pin;
  REQUIRE(input.getPin());
  REQUIRE(input.pinNo() == 9);
  REQUIRE(GPIO::events == std::vector<std::string>{"9:pullup"});
}

TEST_CASE_METHOD(Fixture, "debounce requires a continuous stable interval for each edge", "[debounce]") {
  GPIO::inputs[9] = false;
  Pin::runSetup();
  GPIO::inputs[9] = true;
  Clock::set(1);
  REQUIRE_FALSE(Pin::get());
  Clock::set(5);
  GPIO::inputs[9] = false;
  REQUIRE_FALSE(Pin::get());
  Clock::set(20);
  GPIO::inputs[9] = true;
  REQUIRE_FALSE(Pin::get());
  Clock::set(30);
  REQUIRE_FALSE(Pin::get());
  Clock::set(31);
  REQUIRE(Pin::get());
  GPIO::inputs[9] = false;
  Clock::set(32);
  REQUIRE(Pin::get());
  GPIO::inputs[9] = true;
  Clock::set(35);
  REQUIRE(Pin::get());
  GPIO::inputs[9] = false;
  Clock::set(50);
  REQUIRE(Pin::get());
  Clock::set(60);
  REQUIRE(Pin::get());
  Clock::set(61);
  REQUIRE_FALSE(Pin::get());
}

TEST_CASE_METHOD(Fixture, "debounce setup discards a pending change and prior accepted level", "[debounce]") {
  GPIO::inputs[9] = false;
  Pin::runSetup();
  GPIO::inputs[9] = true;
  REQUIRE_FALSE(Pin::get());
  Clock::set(11);
  REQUIRE(Pin::get());
  GPIO::inputs[9] = false;
  REQUIRE(Pin::get());
  Pin::runSetup();
  REQUIRE_FALSE(Pin::get());
  GPIO::inputs[9] = true;
  Clock::set(100);
  REQUIRE_FALSE(Pin::get());
  Clock::set(110);
  REQUIRE_FALSE(Pin::get());
  Clock::set(111);
  REQUIRE(Pin::get());
}

TEST_CASE_METHOD(Fixture, "debounce interval works across unsigned clock wraparound", "[debounce]") {
  Clock::set(std::numeric_limits<std::uint32_t>::max() - 4);
  GPIO::inputs[9] = true;
  Pin::runSetup();
  GPIO::inputs[9] = false;
  REQUIRE(Pin::get());
  Clock::set(5);
  REQUIRE(Pin::get());
  Clock::set(6);
  REQUIRE_FALSE(Pin::get());
}
