#pragma once
#include <grevir/peripherals/pwm/frequency_window.hpp>
#include <grevir/base/compat/array.hpp>
#include <grevir/base/compat/compare.hpp>
#include <grevir/base/compat/cstddef.hpp>
#include <grevir/base/compat/string_view.hpp>
#include <grevir/base/compat/type_traits.hpp>

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
  unsigned required_timer = 0; // Physical timer identity, zero when unconstrained.
  unsigned counter_bits_at_least = 0;
  ConfigError error = ConfigError::none;
  constexpr void fail(ConfigError value) {
    // Stable precedence independent of active option declaration order.
    if (value > error) { error = value; }
  }
};

struct Request { Key key; Config config; };
template <std::uint32_t N, std::uint32_t D = 1> struct Hertz {};
struct Exact {};
template <std::uint32_t Ppm> struct WithinPpm {};
template <typename Rate, typename Accuracy> struct Frequency {};
template <std::uint32_t N, std::uint32_t D> struct DutyStepAtMost {};
template <unsigned Physical> struct Pin {};
template <Target T, typename... Options> struct For {};
template <unsigned Bits> struct CounterBitsAtLeast {};
namespace atmega328p {
struct Timer0 {};
struct Timer1 {};
struct Timer2 {};
}
template <typename Timer> struct RequireTimer {};
namespace avr { struct FastPwm {}; struct PhaseCorrectPwm {}; struct TopFromIcr {}; struct BuiltInTop {}; struct TopFromOcra {}; }
namespace esp32 { struct ApbClock {}; }

template <Text Name, typename... Options>
struct PwmRequest {
  inline static constexpr auto name = Name;
  template <typename F> static constexpr void visit(F&& f) {
    (f(static_cast<Options*>(nullptr)), ...);
  }
};
template <typename T> struct IsPwmRequest : std::false_type {};
template <Text Name, typename... Options>
struct IsPwmRequest<PwmRequest<Name, Options...>> : std::true_type {};

template <Text Name, typename... Items>
struct Instance {
  inline static constexpr auto name = Name;
  static constexpr std::size_t count = (std::size_t{0} + ... + IsPwmRequest<Items>::value);
  template <typename Item, typename F> static constexpr void visit_use(F& f) {
    if constexpr (IsPwmRequest<Item>::value) { f(static_cast<Item*>(nullptr)); }
  }
  template <typename Item, typename F> static constexpr void visit_option(F& f) {
    if constexpr (!IsPwmRequest<Item>::value) { f(static_cast<Item*>(nullptr)); }
  }
  template <typename F> static constexpr void visit(F&& f) {
    (visit_use<Items>(f), ...);
  }
  template <typename F> static constexpr void visit_options(F&& f) {
    (visit_option<Items>(f), ...);
  }
};

constexpr bool matches(Target resident, Target section) {
  return resident == section || (resident == Target::atmega328p && section == Target::avr);
}

template <typename T>
constexpr void merge_constraint(Config& config, T& field, T unset, T value) {
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
    else { merge_constraint(c, c.pin, 0u, P); }
  }
};
template <Target Resident, unsigned Bits>
struct Apply<Resident, CounterBitsAtLeast<Bits>> {
  static constexpr void run(Config& c) {
    if constexpr (Bits == 0) { c.fail(ConfigError::invalid_value); }
    else if (Bits > c.counter_bits_at_least) { c.counter_bits_at_least = Bits; }
  }
};
template <typename Timer> struct TimerIdentity { static constexpr unsigned id = 0; };
template <> struct TimerIdentity<atmega328p::Timer0> { static constexpr unsigned id = 1; };
template <> struct TimerIdentity<atmega328p::Timer1> { static constexpr unsigned id = 2; };
template <> struct TimerIdentity<atmega328p::Timer2> { static constexpr unsigned id = 3; };
template <Target Resident, typename Timer>
struct Apply<Resident, RequireTimer<Timer>> {
  static constexpr void run(Config& c) {
    if constexpr (Resident != Target::atmega328p || TimerIdentity<Timer>::id == 0) {
      c.fail(ConfigError::unsupported_option);
    } else {
      merge_constraint(c, c.required_timer, 0u, TimerIdentity<Timer>::id);
    }
  }
};

template <Target Resident, Waveform W>
struct AvrWaveform {
  static constexpr void run(Config& c) {
    if constexpr (!matches(Resident, Target::avr)) { c.fail(ConfigError::unsupported_option); }
    else { merge_constraint(c, c.waveform, Waveform::any, W); }
  }
};
template <Target R> struct Apply<R, avr::FastPwm> : AvrWaveform<R, Waveform::fast> {};
template <Target R> struct Apply<R, avr::PhaseCorrectPwm> : AvrWaveform<R, Waveform::phase_correct> {};
template <Target R, Source S> struct AvrSource {
  static constexpr void run(Config& c) {
    if constexpr (!matches(R, Target::avr)) { c.fail(ConfigError::unsupported_option); }
    else { merge_constraint(c, c.source, Source::any, S); }
  }
};
template <Target R> struct Apply<R, avr::TopFromIcr> : AvrSource<R, Source::icr> {};
template <Target R> struct Apply<R, avr::BuiltInTop> : AvrSource<R, Source::built_in> {};
template <Target R> struct Apply<R, avr::TopFromOcra> : AvrSource<R, Source::ocra> {};
template <Target R> struct Apply<R, esp32::ApbClock> {
  static constexpr void run(Config& c) {
    if constexpr (R != Target::esp32) { c.fail(ConfigError::unsupported_option); }
    else { merge_constraint(c, c.source, Source::any, Source::apb); }
  }
};

template <Target Resident, typename I, typename Use>
constexpr Request request() {
  Config config;
  I::visit_options([&]<typename O>(O*) { Apply<Resident, O>::run(config); });
  Use::visit([&]<typename O>(O*) { Apply<Resident, O>::run(config); });
  if (!config.frequency.valid() || !config.step.valid() || config.pin == 0) {
    config.fail(ConfigError::invalid_value);
  }
  return {{I::name.view(), Use::name.view()}, config};
}

template <Target Resident, typename... Instances>
constexpr auto requests() {
  constexpr std::size_t count = (Instances::count + ... + 0);
  constexpr auto identities = [] {
    std::array<Request, count> values{};
    std::size_t next = 0;
    (Instances::visit([&]<typename Use>(Use*) {
      values[next++] = {{Instances::name.view(), Use::name.view()}, {}};
    }), ...);
    return values;
  }();
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
    std::array<Request, count> values{};
    std::size_t next = 0;
    (Instances::visit([&]<typename Use>(Use*) {
      values[next++] = request<Resident, Instances, Use>();
    }), ...);
    return values;
  } else {
    return identities;
  }
}

} // namespace grevir::pwm
