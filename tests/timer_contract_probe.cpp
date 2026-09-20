#include "mock_timer_backend.hpp"

namespace {
using namespace ardo::timers;
template <typename... Requests>
using Checked = CheckedTimerConfig<timer_mock::FlexibleBackend, TimerConfig<Requests...>>;
struct UnknownOption {};
template <int Id>
struct Case;
template <> struct Case<0> { using Type = Checked<Frequency<1000>, Resolution<16>>; };
template <> struct Case<1> { using Type = Checked<VariableFrequency<20000, 2>, Resolution<8>>; };
template <> struct Case<2> { using Type = Checked<Frequency<1, 0>, Resolution<8>>; };
template <> struct Case<3> { using Type = Checked<Frequency<0>, Resolution<8>>; };
template <> struct Case<4> { using Type = Checked<Frequency<1000>, Resolution<0>>; };
template <> struct Case<5> { using Type = Checked<Frequency<10001>, Resolution<8>>; };
template <> struct Case<6> { using Type = Checked<Frequency<5000>, Resolution<16>>; };
template <> struct Case<7> { using Type = Checked<Frequency<1000>, Resolution<32>>; };
template <> struct Case<8> {
  using Type = CheckedTimerConfig<timer_mock::FixedBackend,
    TimerConfig<VariableFrequency<1000>, Resolution<8>>>;
};
template <> struct Case<9> {
  using Type = Checked<Frequency<1000>, Resolution<8>, UnknownOption>;
};
template <> struct Case<10> { using Type = Checked<Frequency<1000>>; };
template <> struct Case<11> { using Type = Checked<Frequency<1000>, Frequency<2000>, Resolution<8>>; };
template <> struct Case<12> { using Type = Checked<Frequency<1000>, VariableFrequency<1000>, Resolution<8>>; };
template <> struct Case<13> { using Type = Checked<Frequency<1000>, Resolution<8>, Resolution<16>>; };
static_assert(sizeof(typename Case<CASE_ID>::Type) > 0);
}
