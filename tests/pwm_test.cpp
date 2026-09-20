#include "mock_devices.hpp"
#include <catch2/catch_test_macros.hpp>

namespace {
using namespace peripheral_mock;
using Pin = ardo::OutputPin<GPIO, 12>;

template <unsigned Bits>
struct PwmBackend {
  static constexpr unsigned timer_bits = Bits;
  using Claims = ardo::ResourceClaim<ardo::HardwareTimer<1>>;
  static void setPwm(std::uint32_t value) {
    GPIO::record(12, "pwm:" + std::to_string(value));
  }
};

struct PwmIF {
  virtual ~PwmIF() = default;
  virtual void setPwmPin(std::uint32_t value) const = 0;
};

struct TrackingPin : Pin {
  static void runLoop() { GPIO::record(12, "loop"); }
};
using Output = ardo::HardwarePwm<TrackingPin, PwmBackend<9>>;
struct Module : ardo::ModuleBase<ardo::Parameters<Output>> {
  static void runSetup() { Output::setPwm(0); }
  static void runLoop() { Output::setPwm(255); }
};
}

TEST_CASE_METHOD(Fixture, "PWM expands the eight bit range including endpoints and interior values", "[pwm]") {
  using Pwm = ardo::HardwarePwm<Pin, PwmBackend<9>>;
  Pwm::setPwm(0);
  Pwm::setPwm(1);
  Pwm::setPwm(127);
  Pwm::setPwm(128);
  Pwm::setPwm(254);
  Pwm::setPwm(255);
  REQUIRE(GPIO::events == std::vector<std::string>{
    "12:pwm:0", "12:pwm:2", "12:pwm:254", "12:pwm:257", "12:pwm:509", "12:pwm:511"});
}

TEST_CASE_METHOD(Fixture, "PWM matching resolutions preserve duty values", "[pwm]") {
  using Pwm = ardo::HardwarePwm<Pin, PwmBackend<8>>;
  Pwm::setPwm(0);
  Pwm::setPwm(128);
  Pwm::setPwm(255);
  REQUIRE(GPIO::events == std::vector<std::string>{"12:pwm:0", "12:pwm:128", "12:pwm:255"});
}

TEST_CASE_METHOD(Fixture, "PWM accepts the complete input range before reducing resolution", "[pwm]") {
  using Pwm = ardo::HardwarePwm<Pin, PwmBackend<8>, setl::LinearScalerSelector<16>>;
  Pwm::setPwm(0);
  Pwm::setPwm(255);
  Pwm::setPwm(256);
  Pwm::setPwm(32768);
  Pwm::setPwm(65535);
  REQUIRE(GPIO::events == std::vector<std::string>{
    "12:pwm:0", "12:pwm:0", "12:pwm:1", "12:pwm:128", "12:pwm:255"});
}

TEST_CASE_METHOD(Fixture, "PWM virtual interface scales the input width before backend delivery", "[pwm]") {
  using Pwm = ardo::HardwarePwm<Pin, PwmBackend<8>, setl::LinearScalerSelector<16>, PwmIF>;
  const PwmIF& output = Pwm::pwm_pin;
  output.setPwmPin(32768);
  output.setPwmPin(65535);
  REQUIRE(GPIO::events == std::vector<std::string>{"12:pwm:128", "12:pwm:255"});
}

TEST_CASE_METHOD(Fixture, "PWM application configures the pin before writes and forwards pin loop", "[pwm]") {
  using App = ardo::Application<Module>;
  App::runSetup();
  REQUIRE(GPIO::events == std::vector<std::string>{"12:output", "12:pwm:0"});
  App::runLoop();
  REQUIRE(GPIO::events == std::vector<std::string>{"12:output", "12:pwm:0", "12:loop", "12:pwm:511"});
}
