#include "mock_devices.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace peripheral_mock;
using namespace ardo::gpio;

TEST_CASE_METHOD(Fixture, "input modes and interface read the supplied backend", "[gpio]") {
  using PullUp = ardo::InputPin<GPIO, 2, InputPinMode::PullUp, ardo::InputPinIF>;
  using Untied = ardo::InputPin<GPIO, 3, InputPinMode::Untied>;
  PullUp::runSetup();
  Untied::runSetup();
  REQUIRE(GPIO::events == std::vector<std::string>{"2:pullup", "3:input"});
  const ardo::InputPinIF& pin = PullUp::pin;
  REQUIRE(pin.pinNo() == 2);
  REQUIRE_FALSE(pin.getPin());
  GPIO::inputs[2] = true;
  REQUIRE(pin.getPin());
  REQUIRE_FALSE(Untied::get());
  PullUp::runLoop();
  REQUIRE(GPIO::events.size() == 2);
}

TEST_CASE_METHOD(Fixture, "output setup and interface preserve write order", "[gpio]") {
  using Output = ardo::OutputPin<GPIO, 4, OutputPinMode::Output, ardo::OutputPinIF>;
  Output::runSetup();
  const ardo::OutputPinIF& pin = Output::pin;
  REQUIRE(pin.pinNo() == 4);
  Output::set(true);
  pin.setPin(false);
  Output::runLoop();
  REQUIRE(GPIO::events == std::vector<std::string>{"4:output", "4:high", "4:low"});
}

TEST_CASE_METHOD(Fixture, "open drain low preloads low and switches drive versus release", "[gpio]") {
  using Pin = ardo::OutputPin<GPIO, 5, OutputPinMode::OpenDrainLowOutput,
    ardo::OutputPinIF, ardo::InputPinIF>;
  Pin::runSetup();
  Pin::pin.setPin(false);
  Pin::pin.setPin(true);
  REQUIRE(GPIO::events == std::vector<std::string>{"5:input", "5:low", "5:output", "5:input"});
  GPIO::inputs[5] = true;
  const ardo::InputPinIF& input = Pin::pin;
  REQUIRE(input.getPin());
  GPIO::inputs[5] = false;
  REQUIRE_FALSE(input.getPin());
}

TEST_CASE_METHOD(Fixture, "open drain high preloads high and switches drive versus release", "[gpio]") {
  using Pin = ardo::OutputPin<GPIO, 6, OutputPinMode::OpenDrainHighOutput,
    ardo::InputOutputPinIF>;
  Pin::runSetup();
  Pin::pin.setPin(true);
  Pin::pin.setPin(false);
  REQUIRE(GPIO::events == std::vector<std::string>{"6:input", "6:high", "6:output", "6:input"});
  GPIO::inputs[6] = true;
  REQUIRE(Pin::pin.getPin());
  REQUIRE(Pin::pin.pinNo() == 6);
}

TEST_CASE_METHOD(Fixture, "external pin participates without performing GPIO operations", "[gpio]") {
  struct ExternalUser : ardo::ModuleBase<ardo::Parameters<ardo::ExternalPin<7>>> {};
  ardo::Application<ExternalUser>::runSetup();
  ardo::Application<ExternalUser>::runLoop();
  REQUIRE(GPIO::events.empty());
}
