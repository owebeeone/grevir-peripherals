#pragma once
#include <grevir/peripherals/pwm/frequency_window.hpp>
#include <array>
#include <compare>
#include <cstddef>
#include <string_view>

namespace grevir::pwm {

template <std::size_t N>
struct Text {
  char value[N];
  constexpr Text(const char (&input)[N]) {
    for (std::size_t i = 0; i < N; ++i) { value[i] = input[i]; }
  }
  constexpr std::string_view view() const { return {value, N - 1}; }
};

struct Key {
  std::string_view instance;
  std::string_view local;
  constexpr auto operator<=>(const Key&) const = default;
};

constexpr bool identifier(std::string_view value) {
  if (value.empty()) { return false; }
  for (char c : value) {
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_')) { return false; }
  }
  return true;
}

enum class Target { avr, atmega328p, esp32 };
enum class Waveform { any, fast, phase_correct };
enum class Source { any, icr, apb, built_in, ocra };
enum class ConfigError { none, invalid_value, unsupported_option, conflict };

struct Config {
  FrequencyWindow frequency{};
  Ratio step{};
  unsigned pin = 0;
  Waveform waveform = Waveform::any;
  Source source = Source::any;
  ConfigError error = ConfigError::none;
  constexpr void fail(ConfigError value) {
    // Stable precedence independent of active option declaration order.
    if (value > error) { error = value; }
  }
};

struct Request { Key key; Config config; unsigned group = 0; };
template <std::uint32_t N, std::uint32_t D = 1> struct Hertz {};
struct Exact {};
template <std::uint32_t Ppm> struct WithinPpm {};
template <typename Rate, typename Accuracy> struct Frequency {};
template <std::uint32_t N, std::uint32_t D> struct DutyStepAtMost {};
template <unsigned Physical> struct Pin {};
template <Target T, typename... Options> struct For {};
namespace avr { struct FastPwm {}; struct PhaseCorrectPwm {}; struct TopFromIcr {}; struct BuiltInTop {}; struct TopFromOcra {}; }
namespace esp32 { struct ApbClock {}; }

template <Text Name, typename... Options>
struct PwmRequest {
  inline static constexpr auto name = Name;
  template <typename F> static constexpr void visit(F&& f) {
    (f(static_cast<Options*>(nullptr)), ...);
  }
};
template <Text Name, typename Pwm, unsigned Group = 0>
struct Instance {
  inline static constexpr auto name = Name;
  using request = Pwm;
  static constexpr unsigned group = Group;
};

constexpr bool matches(Target resident, Target section) {
  return resident == section || (resident == Target::atmega328p && section == Target::avr);
}

template <typename T>
constexpr void constrain(Config& config, T& field, T unset, T value) {
  if (field != unset && field != value) { config.fail(ConfigError::conflict); }
  field = value;
}

// Options stay inert until selected. Unknown active options become diagnostics;
// an inactive option can even be an incomplete type.
template <Target Resident, typename Option>
struct Apply {
  static constexpr void run(Config& c) { c.fail(ConfigError::unsupported_option); }
};
template <Target Resident, Target Section, typename... Options>
struct Apply<Resident, For<Section, Options...>> {
  static constexpr void run(Config& c) {
    if constexpr (matches(Resident, Section)) { (Apply<Resident, Options>::run(c), ...); }
  }
};
template <typename Policy> struct Accuracy {
  static constexpr bool supported = false;
  static constexpr std::uint32_t ppm = 0;
};
template <> struct Accuracy<Exact> {
  static constexpr bool supported = true;
  static constexpr std::uint32_t ppm = 0;
};
template <std::uint32_t P> struct Accuracy<WithinPpm<P>> {
  static constexpr bool supported = true;
  static constexpr std::uint32_t ppm = P;
};

template <Target Resident, std::uint32_t N, std::uint32_t D, typename Policy>
struct Apply<Resident, Frequency<Hertz<N, D>, Policy>> {
  static constexpr void run(Config& c) {
    constexpr Ratio raw{N, D};
    constexpr auto ppm = Accuracy<Policy>::ppm;
    if constexpr (!Accuracy<Policy>::supported) { c.fail(ConfigError::unsupported_option); }
    else if constexpr (!raw.valid() || ppm > 1'000'000) { c.fail(ConfigError::invalid_value); }
    else {
      c.frequency = c.frequency.intersect(FrequencyWindow::around(raw, ppm));
      if (c.frequency.empty()) { c.fail(ConfigError::conflict); }
    }
  }
};
template <Target Resident, std::uint32_t N, std::uint32_t D>
struct Apply<Resident, DutyStepAtMost<N, D>> {
  static constexpr void run(Config& c) {
    constexpr Ratio raw{N, D};
    if constexpr (!raw.valid() || N > D) { c.fail(ConfigError::invalid_value); }
    else {
      if (c.step.numerator == 0 || at_most(raw, c.step)) { c.step = raw.normalized(); }
    }
  }
};
template <Target Resident, unsigned P>
struct Apply<Resident, Pin<P>> {
  static constexpr void run(Config& c) {
    if constexpr (P == 0) { c.fail(ConfigError::invalid_value); }
    else { constrain(c, c.pin, 0u, P); }
  }
};

template <Target Resident, Waveform W>
struct AvrWaveform {
  static constexpr void run(Config& c) {
    if constexpr (!matches(Resident, Target::avr)) { c.fail(ConfigError::unsupported_option); }
    else { constrain(c, c.waveform, Waveform::any, W); }
  }
};
template <Target R> struct Apply<R, avr::FastPwm> : AvrWaveform<R, Waveform::fast> {};
template <Target R> struct Apply<R, avr::PhaseCorrectPwm> : AvrWaveform<R, Waveform::phase_correct> {};
template <Target R, Source S> struct AvrSource {
  static constexpr void run(Config& c) {
    if constexpr (!matches(R, Target::avr)) { c.fail(ConfigError::unsupported_option); }
    else { constrain(c, c.source, Source::any, S); }
  }
};
template <Target R> struct Apply<R, avr::TopFromIcr> : AvrSource<R, Source::icr> {};
template <Target R> struct Apply<R, avr::BuiltInTop> : AvrSource<R, Source::built_in> {};
template <Target R> struct Apply<R, avr::TopFromOcra> : AvrSource<R, Source::ocra> {};
template <Target R> struct Apply<R, esp32::ApbClock> {
  static constexpr void run(Config& c) {
    if constexpr (R != Target::esp32) { c.fail(ConfigError::unsupported_option); }
    else { constrain(c, c.source, Source::any, Source::apb); }
  }
};

template <Target Resident, typename I>
constexpr Request request() {
  Config config;
  I::request::visit([&]<typename O>(O*) { Apply<Resident, O>::run(config); });
  if (!config.frequency.valid() || !config.step.valid() || config.pin == 0) {
    config.fail(ConfigError::invalid_value);
  }
  return {{I::name.view(), I::request::name.view()}, config, I::group};
}

template <Target Resident, typename... Instances>
constexpr auto requests() {
  constexpr std::array<Request, sizeof...(Instances)> identities{
    Request{{Instances::name.view(), Instances::request::name.view()}, {}, Instances::group}...
  };
  constexpr bool valid_identities = [](const auto& values) {
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (!identifier(values[i].key.instance) || !identifier(values[i].key.local)) { return false; }
      for (std::size_t j = 0; j < i; ++j) {
        if (values[i].key == values[j].key) { return false; }
      }
    }
    return true;
  }(identities);
  if constexpr (valid_identities) {
    return std::array<Request, sizeof...(Instances)>{request<Resident, Instances>()...};
  } else {
    return identities;
  }
}

} // namespace grevir::pwm
