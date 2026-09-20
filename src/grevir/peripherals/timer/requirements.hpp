#pragma once

#include <grevir/base/meta/type_algorithms.hpp>
#include <grevir/base/meta/tuple_types.hpp>
#include <grevir/base/compat/cstdint.hpp>
#include <grevir/base/compat/tuple.hpp>
#include <grevir/base/compat/type_traits.hpp>

namespace ardo::timers {

// Portable request categories; MCU-specific modes belong in backend packages.
enum class ParameterClass {
  frequency,
  frequency_variable,
  resolution
};

template <ParameterClass Class>
struct Parameter {
  static constexpr ParameterClass parameter_class = Class;
};

template <ParameterClass... Classes>
struct ParameterClasses {
  using ParameterTuple = setl::ValueTuple<ParameterClass, Classes...>;
  using BaseTypes = std::tuple<Parameter<Classes>...>;
};

template <typename... Requests>
struct TimerConfig {
  static_assert((0u + ... + (std::is_base_of_v<Parameter<ParameterClass::frequency>, Requests>
    || std::is_base_of_v<Parameter<ParameterClass::frequency_variable>, Requests>)) <= 1,
    "GREVIR_TIMER_CONFLICTING_FREQUENCY_REQUESTS");
  static_assert((0u + ... + std::is_base_of_v<Parameter<ParameterClass::resolution>, Requests>) <= 1,
    "GREVIR_TIMER_DUPLICATE_RESOLUTION_REQUESTS");
  using Parameters = std::tuple<Requests...>;
};

template <ParameterClass Class, std::uint32_t Hertz,
  std::uint32_t Divider = 1, typename FrequencyType = float>
struct FrequencyBase : Parameter<Class> {
  static_assert(Hertz > 0, "GREVIR_TIMER_ZERO_FREQUENCY");
  static_assert(Divider > 0, "GREVIR_TIMER_ZERO_FREQUENCY_DIVIDER");
  using frequency_type = FrequencyType;
  static constexpr std::uint32_t frequency_hz = Hertz;
  static constexpr std::uint32_t frequency_divider_hz = Divider;
  static constexpr bool is_variable = Class == ParameterClass::frequency_variable;
};

template <std::uint32_t Hertz, std::uint32_t Divider = 1, typename FrequencyType = float>
struct Frequency : FrequencyBase<ParameterClass::frequency, Hertz, Divider, FrequencyType> {};

template <std::uint32_t Hertz, std::uint32_t Divider = 1, typename FrequencyType = float>
struct VariableFrequency : FrequencyBase<ParameterClass::frequency_variable, Hertz, Divider, FrequencyType> {};

template <std::uint32_t Bits>
struct Resolution : Parameter<ParameterClass::resolution> {
  static_assert(Bits > 0, "GREVIR_TIMER_ZERO_RESOLUTION");
  static constexpr std::uint32_t resolution = Bits;
};

template <template <ParameterClass> typename Base, ParameterClass... Classes>
struct ToTupleFromParameterClasses {
  using type = std::tuple<Base<Classes>...>;
};


// Explicit opt-in projection retained from the legacy API. This is not validation.
template <typename Allowed, typename Input>
struct TimerConfigFilter {
  using AllowedParameters = Allowed;
  using InputConfig = Input;
  using Config = setl::tuple_select_by_base_t<
    typename InputConfig::Parameters, typename AllowedParameters::BaseTypes>;
};

/** Validate a complete request against a caller-selected backend.
 * Backend supplies AllowedParameters::BaseTypes and accepts<Config> (constexpr bool).
 * accepts checks values and combinations, not just the request categories.
 * No allocation, register configuration or timer selection is performed here.
 */
template <typename Backend, typename Input>
struct CheckedTimerConfig {
  using Parameters = typename Input::Parameters;
  using Supported = typename TimerConfigFilter<typename Backend::AllowedParameters, Input>::Config;
  static_assert(std::tuple_size_v<Supported> == std::tuple_size_v<Parameters>,
    "GREVIR_TIMER_UNSUPPORTED_REQUIREMENT");
  static_assert(Backend::template accepts<Input>, "GREVIR_TIMER_UNSATISFIED_CONFIG");
  using Config = Input;
};

} // namespace ardo::timers
