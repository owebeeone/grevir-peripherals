#include "mock_timer_backend.hpp"

namespace {
using namespace ardo::timers;
static_assert(!Frequency<1000>::is_variable);
static_assert(VariableFrequency<1000>::is_variable);
static_assert(Frequency<2001, 2, double>::frequency_hz == 2001);
static_assert(Frequency<2001, 2, double>::frequency_divider_hz == 2);
static_assert(std::is_same_v<Frequency<2001, 2, double>::frequency_type, double>);
static_assert(std::is_same_v<ParameterClasses<ParameterClass::frequency>::ParameterTuple,
  setl::ValueTuple<ParameterClass, ParameterClass::frequency>>);
static_assert(std::is_same_v<ToTupleFromParameterClasses<Parameter, ParameterClass::resolution>::type,
  std::tuple<Parameter<ParameterClass::resolution>>>);

// Adapted from the embedded legacy filter assertion, without AVR option types.
struct BackendOnlyOption {};
using Filtered = TimerConfigFilter<ParameterClasses<ParameterClass::frequency>,
  TimerConfig<Frequency<1000>, Resolution<16>, BackendOnlyOption>>::Config;
static_assert(std::is_same_v<Filtered, std::tuple<Frequency<1000>>>);
using Empty = TimerConfigFilter<ParameterClasses<>, TimerConfig<Frequency<1000>>>::Config;
static_assert(std::is_same_v<Empty, std::tuple<>>);

using Fixed = TimerConfig<Frequency<10000>, Resolution<8>>;
using Variable = TimerConfig<VariableFrequency<1000>, Resolution<16>>;
using Fractional = TimerConfig<Frequency<2001, 2, double>, Resolution<8>>;
static_assert(std::is_same_v<CheckedTimerConfig<timer_mock::FixedBackend, Fixed>::Config, Fixed>);
static_assert(std::is_same_v<CheckedTimerConfig<timer_mock::FlexibleBackend, Variable>::Config, Variable>);
static_assert(std::is_same_v<CheckedTimerConfig<timer_mock::FlexibleBackend, Fractional>::Parameters,
  std::tuple<Frequency<2001, 2, double>, Resolution<8>>>);
}
