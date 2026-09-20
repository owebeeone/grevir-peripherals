#include <GrevirPeripherals.h>
#include <cstdint>
#include <array>

namespace {
struct Backend {
  inline static bool configured = false;
  inline static bool level = false;
  inline static bool inputLevel = true;
  inline static bool inputConfigured = false;
  static void pinMode(unsigned pin, ardo::gpio::InputPinMode mode) {
    inputConfigured = pin == 4 && mode == ardo::gpio::InputPinMode::PullUp;
  }
  static bool digitalRead(unsigned) { return inputLevel; }
  static void pinMode(unsigned pin, ardo::gpio::OutputPinMode mode) {
    configured = pin == 3 && mode == ardo::gpio::OutputPinMode::Output;
  }
  static void digitalWrite(unsigned, bool value) { level = value; }
};
struct Clock {
  using TimeType = setl::Time<std::uint32_t>;
  inline static std::uint32_t ticks = 0;
  static TimeType now() { return TimeType(ticks); }
};
using Pin = ardo::OutputPin<Backend, 3>;
using Input = ardo::DebounceInput<ardo::InputPin<Backend, 4>, Clock, 10>;
using Button = ardo::ButtonEventModule<Input, Clock>;
struct PwmBackend {
  static constexpr unsigned timer_bits = 8;
  using Claims = ardo::ResourceClaim<ardo::HardwareTimer<1>>;
  inline static std::uint8_t value = 0;
  static void setPwm(std::uint8_t duty) { value = duty; }
};
using Pwm = ardo::HardwarePwm<Pin, PwmBackend, setl::LinearScalerSelector<16>>;
struct PwmModule : ardo::ModuleBase<ardo::Parameters<Pwm>> {};
struct StorageBackend {
  using Resource = ardo::EepromResource;
  static constexpr std::size_t capacity = 16;
  inline static std::array<std::uint8_t, capacity> bytes{};
  static std::uint8_t read(std::size_t address) { return bytes.at(address); }
  static void update(std::size_t address, std::uint8_t value) { bytes.at(address) = value; }
};
using SavedValue = ardo::EepromReaderWriter<std::uint32_t, 12, StorageBackend>;
struct TimerBackend {
  using AllowedParameters = ardo::timers::ParameterClasses<
    ardo::timers::ParameterClass::frequency, ardo::timers::ParameterClass::resolution>;
  template <typename Config>
  static constexpr bool accepts = std::is_same_v<typename Config::Parameters,
    std::tuple<ardo::timers::Frequency<1000>, ardo::timers::Resolution<8>>>;
};
using Checked = ardo::timers::CheckedTimerConfig<TimerBackend,
  ardo::timers::TimerConfig<ardo::timers::Frequency<1000>, ardo::timers::Resolution<8>>>;
static_assert(std::tuple_size_v<typename Checked::Parameters> == 2);
struct Module : ardo::ModuleInstanceBase<Module, ardo::Parameters<Pin>> {
  ardo::TimePoller<unsigned, Clock> timer;
  void instanceSetup() {
    timer.reset();
    Pin::set(false);
  }
  void instanceLoop() {
    if (timer.poll(Clock::TimeType::period_type(10))) {
      Pin::set(true);
    }
  }
};
}
int main() {
  ardo::Application<Module>::runSetup();
  if (!Backend::configured || Backend::level) {
    return 1;
  }
  Clock::ticks = 10;
  ardo::Application<Module>::runLoop();
  if (Backend::level) {
    return 2;
  }
  Clock::ticks = 11;
  ardo::Application<Module>::runLoop();
  if (!Backend::level) {
    return 3;
  }
  using ButtonApp = ardo::Application<Button>;
  ButtonApp::runSetup();
  if (!Backend::inputConfigured || Button::get() != Button::None) {
    return 4;
  }
  Backend::inputLevel = false;
  Clock::ticks = 20;
  ButtonApp::runLoop();
  Clock::ticks = 31;
  ButtonApp::runLoop();
  Clock::ticks = 331;
  ButtonApp::runLoop();
  if (Button::get() != Button::LongClick || Button::get() != Button::None) {
    return 5;
  }
  const setl::Period<int> duration(12);
  if ((duration / 3).get() != 4) {
    return 6;
  }
  Backend::configured = false;
  ardo::Application<PwmModule>::runSetup();
  Pwm::setPwm(65535);
  if (!Backend::configured || PwmBackend::value != 255) {
    return 7;
  }
  Pwm::pwm_pin.setPwmPin(32768);
  if (PwmBackend::value != 128) {
    return 8;
  }
  SavedValue::write(0xff807f01u);
  if (StorageBackend::bytes[11] != 0) {
    return 9;
  }
  return SavedValue::read() == 0xff807f01u ? 0 : 10;
}
