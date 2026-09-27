#pragma once

#include <grevir/base/text.hpp>
#include <grevir/base/compat/type_traits.hpp>

namespace grevir::timer {

enum class UseKind { pwm, period_event };
enum class Target { avr, atmega328p, esp32 };
template <Target T, typename... Options> struct For {};
template <unsigned Bits> struct CounterBitsAtLeast {};
namespace atmega328p {
struct Timer0 {};
struct Timer1 {};
struct Timer2 {};
}
template <typename DeviceTimer> struct RequireTimer {};

template <typename T, typename = void> struct IsUse : std::false_type {};
template <typename T>
struct IsUse<T, std::void_t<decltype(T::is_timer_use)>>
  : std::bool_constant<T::is_timer_use> {};

// This declares an owner-local period event, independently of any PWM pin.
// A backend without a joint event/PWM candidate must reject the active use.
template <grevir::Text Name, typename... Options>
struct PeriodEventUse {
  inline static constexpr auto name = Name;
  static constexpr bool is_timer_use = true;
  static constexpr UseKind kind = UseKind::period_event;
  template <typename F> static constexpr void visit(F&& f) {
    (f(static_cast<Options*>(nullptr)), ...);
  }
};

template <typename... Items>
struct Own {
  static constexpr std::size_t count = (std::size_t{0} + ... + IsUse<Items>::value);
  static_assert(count > 0, "GREVIR_TIMER_OWNER_HAS_NO_USES");
  template <typename Item, typename F> static constexpr void visit_use(F& f) {
    if constexpr (IsUse<Item>::value) { f(static_cast<Item*>(nullptr)); }
  }
  template <typename Item, typename F> static constexpr void visit_option(F& f) {
    if constexpr (!IsUse<Item>::value) { f(static_cast<Item*>(nullptr)); }
  }
  template <typename F> static constexpr void visit(F&& f) {
    (visit_use<Items>(f), ...);
  }
  template <typename F> static constexpr void visit_options(F&& f) {
    (visit_option<Items>(f), ...);
  }
};

// The application gives each module instance a stable name. All uses in Own
// are solved against one physical timer and one selected configuration.
template <grevir::Text Name, typename Demand>
struct Instance {
  inline static constexpr auto name = Name;
  using demand = Demand;
  static constexpr std::size_t count = Demand::count;
  template <typename F> static constexpr void visit(F&& f) { Demand::visit(f); }
  template <typename F> static constexpr void visit_options(F&& f) {
    Demand::visit_options(f);
  }
};

} // namespace grevir::timer
