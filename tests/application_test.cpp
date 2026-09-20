#include "mock_devices.hpp"
#include <catch2/catch_test_macros.hpp>

namespace {
using namespace peripheral_mock;
using Output = ardo::OutputPin<GPIO, 8>;
using Sequence = ardo::Sequence<std::uint32_t, 10, 20>;
struct Blink : ardo::ModuleInstanceBase<Blink, ardo::Parameters<Output>> {
  ardo::CyclicTimeSequencePoller<Sequence, Clock> timer;
  void instanceSetup() {
    timer.reset();
    Output::set(false);
  }
  void instanceLoop() {
    if (timer.poll()) {
      Output::set(timer.state() != 0);
    }
  }
};
}

TEST_CASE_METHOD(Fixture, "application drives a blinking output from the controlled clock", "[application][gpio][clock]") {
  using App = ardo::Application<Blink>;
  App::runSetup();
  REQUIRE(GPIO::events == std::vector<std::string>{"8:output", "8:low"});
  Clock::set(10);
  App::runLoop();
  REQUIRE(GPIO::events.size() == 2);
  Clock::set(11);
  App::runLoop();
  Clock::set(30);
  App::runLoop();
  REQUIRE(GPIO::events == std::vector<std::string>{"8:output", "8:low", "8:high"});
  Clock::set(31);
  App::runLoop();
  REQUIRE(GPIO::events == std::vector<std::string>{"8:output", "8:low", "8:high", "8:low"});
  GPIO::events.clear();
  App::runSetup();
  Clock::set(42);
  App::runLoop();
  REQUIRE(GPIO::events == std::vector<std::string>{"8:output", "8:low", "8:high"});
}
