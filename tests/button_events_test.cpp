#include "mock_devices.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

namespace {
using namespace peripheral_mock;
using Raw = ardo::InputPin<GPIO, 10>;
using Input = ardo::DebounceInput<Raw, Clock, 10>;
using Button = ardo::ButtonEventModule<Input, Clock>;
using App = ardo::Application<Button>;

struct ButtonFixture : Fixture {
  ButtonFixture() {
    GPIO::inputs[10] = true;
    App::runSetup();
  }
  static void sample(std::uint32_t tick, bool released) {
    Clock::set(tick);
    GPIO::inputs[10] = released;
    App::runLoop();
  }
};

struct MicroClock {
  using TimeType = setl::Time<std::uint32_t, setl::TimeUnit::MICROS>;
  inline static std::uint32_t ticks = 0;
  static TimeType now() { return TimeType(ticks); }
};
}

TEST_CASE_METHOD(ButtonFixture, "button idle startup and bouncing press release produce one delayed click", "[button]") {
  sample(0, true);
  sample(500, true);
  REQUIRE(Button::get() == Button::None);
  sample(501, false);
  sample(505, true);
  sample(520, false);
  sample(530, false);
  REQUIRE(Button::get() == Button::None);
  sample(531, false); // accepted press
  sample(550, true);
  sample(555, false);
  sample(570, true);
  sample(580, true);
  sample(581, true); // accepted release
  sample(980, true);
  REQUIRE(Button::get() == Button::None);
  sample(981, true); // 400 ms after accepted release
  REQUIRE(Button::get() == Button::Click);
  REQUIRE(Button::get() == Button::None);
  sample(982, true);
  sample(1500, true);
  REQUIRE(Button::get() == Button::None);
}

TEST_CASE_METHOD(ButtonFixture, "second debounced press produces one double click", "[button]") {
  sample(1, false);
  sample(12, false);
  sample(50, true);
  sample(61, true);
  sample(200, false);
  sample(211, false);
  REQUIRE(Button::get() == Button::DoubleClick);
  REQUIRE(Button::get() == Button::None);
  sample(800, false);
  REQUIRE(Button::get() == Button::None);
  sample(801, true);
  sample(812, true);
  sample(1300, true);
  REQUIRE(Button::get() == Button::None);
}

TEST_CASE_METHOD(ButtonFixture, "held button produces one long click at the threshold", "[button]") {
  sample(1, false);
  sample(12, false);
  sample(311, false);
  REQUIRE(Button::get() == Button::None);
  sample(312, false);
  REQUIRE(Button::get() == Button::LongClick);
  sample(1000, false);
  REQUIRE(Button::get() == Button::None);
  sample(1001, true);
  sample(1012, true);
  sample(1500, true);
  REQUIRE(Button::get() == Button::None);
}

TEST_CASE_METHOD(ButtonFixture, "button setup clears processing state and an unread event", "[button]") {
  sample(1, false);
  sample(12, false);
  sample(312, false); // leave LongClick unread
  GPIO::inputs[10] = true;
  App::runSetup();
  REQUIRE(Button::get() == Button::None);
  sample(400, false);
  sample(411, false);
  sample(711, false);
  REQUIRE(Button::get() == Button::LongClick);
  GPIO::inputs[10] = true;
  App::runSetup();
  sample(800, false);
  sample(811, false);
  GPIO::inputs[10] = true;
  App::runSetup();
  sample(1300, true);
  REQUIRE(Button::get() == Button::None);
}

TEST_CASE_METHOD(Fixture, "button clock wraparound and microsecond clock retain millisecond thresholds", "[button]") {
  using MicroButton = ardo::ButtonEventModule<Raw, MicroClock>;
  using MicroApp = ardo::Application<MicroButton>;
  MicroClock::ticks = std::numeric_limits<std::uint32_t>::max() - 99999;
  GPIO::inputs[10] = true;
  MicroApp::runSetup();
  GPIO::inputs[10] = false;
  MicroApp::runLoop();
  MicroClock::ticks = 199999; // 299999 microseconds elapsed
  MicroApp::runLoop();
  REQUIRE(MicroButton::get() == MicroButton::None);
  MicroClock::ticks = 200000;
  MicroApp::runLoop();
  REQUIRE(MicroButton::get() == MicroButton::LongClick);
  GPIO::inputs[10] = true;
  MicroApp::runLoop();
  GPIO::inputs[10] = false;
  MicroClock::ticks = 210000;
  MicroApp::runLoop();
  GPIO::inputs[10] = true;
  MicroClock::ticks = 220000;
  MicroApp::runLoop();
  MicroClock::ticks = 619999;
  MicroApp::runLoop();
  REQUIRE(MicroButton::get() == MicroButton::None);
  MicroClock::ticks = 620000;
  MicroApp::runLoop();
  REQUIRE(MicroButton::get() == MicroButton::Click);
}
