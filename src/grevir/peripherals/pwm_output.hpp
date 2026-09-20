/**
 * PWM output support for ArdOinus.
 */

#ifndef ARDO_PWM_OUTPUT___H
#define ARDO_PWM_OUTPUT___H

#include <grevir/base/utility.hpp>
#include <grevir/base/compat/cstdint.hpp>
#include <grevir/base/int_scaler.hpp>
#include <grevir/core/resource_claims.hpp>

namespace ardo {

/**
 * Portable PWM scaling and resource claims. Backend supplies timer_bits, Claims
 * and static setPwm(value). Lifecycle calls are forwarded to Pin.
 */
template <
  typename w_Pin,
  typename Backend,
  typename w_ScalerSelector = setl::LinearScalerSelector<8>,
  typename... w_Bases >
class HardwarePwm : public w_Bases..., public setl::not_copyable {
protected:
  HardwarePwm() {}  // Only the one instance allowed
public:

  using Pin = w_Pin;
  using Pwm = Backend;
  static constexpr std::int8_t timer_bits = Pwm::timer_bits;
  using Scaler = typename w_ScalerSelector::template Scaler<timer_bits>;
  // Accept the scaler input width; narrowing to its output width loses bits.
  using value_type = typename w_ScalerSelector::in_value_type;

  static const HardwarePwm pwm_pin;

  // 
  using Claims = ardo::ConcatenateResourceClaims<
    typename w_Pin::Claims, typename Pwm::Claims>;

  inline static void setPwm(const value_type& value) {
    Pwm::setPwm(Scaler::scale(value));
  }

  inline static void runSetup() {
    Pin::runSetup();
  }

  inline static void runLoop() {
    Pin::runLoop();
  }

  // Overrides base class interface if given.
  void setPwmPin(const std::uint32_t value) const {
    setPwm(value_type(value));
  }
};

template <typename P, typename Backend, typename S, typename... B>
const HardwarePwm<P, Backend, S, B...> HardwarePwm<P, Backend, S, B...>::pwm_pin;

}
#endif // ARDO_PWM_OUTPUT___H
