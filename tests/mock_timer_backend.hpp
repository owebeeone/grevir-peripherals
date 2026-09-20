#pragma once

#include <grevir/peripherals/timer/requirements.hpp>

namespace timer_mock {
using namespace ardo::timers;

template <typename Request>
constexpr double frequency() {
  if constexpr (std::is_base_of_v<Parameter<ParameterClass::frequency>, Request>
      || std::is_base_of_v<Parameter<ParameterClass::frequency_variable>, Request>) {
    return double(Request::frequency_hz) / Request::frequency_divider_hz;
  } else {
    return 0;
  }
}

template <typename Request>
constexpr unsigned resolution() {
  if constexpr (std::is_base_of_v<Parameter<ParameterClass::resolution>, Request>) {
    return Request::resolution;
  } else {
    return 0;
  }
}

template <typename Config>
struct Limits;
template <typename... Requests>
struct Limits<TimerConfig<Requests...>> {
  static constexpr double hertz = (0.0 + ... + frequency<Requests>());
  static constexpr unsigned bits = (0u + ... + resolution<Requests>());
  static constexpr bool variable = (false || ... ||
    std::is_base_of_v<Parameter<ParameterClass::frequency_variable>, Requests>);
  // This synthetic device requires both settings and restricts combinations.
  static constexpr bool fits = hertz > 0 && hertz <= 10000 && bits > 0 && bits <= 16
    && (hertz <= 1000 || bits <= 8);
};

struct FlexibleBackend {
  using AllowedParameters = ParameterClasses<ParameterClass::frequency,
    ParameterClass::frequency_variable, ParameterClass::resolution>;
  template <typename Config>
  static constexpr bool accepts = Limits<Config>::fits;
};
struct FixedBackend {
  using AllowedParameters = ParameterClasses<ParameterClass::frequency, ParameterClass::resolution>;
  template <typename Config>
  static constexpr bool accepts = Limits<Config>::fits && !Limits<Config>::variable;
};
} // namespace timer_mock
