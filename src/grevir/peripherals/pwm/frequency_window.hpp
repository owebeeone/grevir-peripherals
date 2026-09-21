#pragma once
#include <grevir/peripherals/pwm/numeric.hpp>

namespace grevir::pwm {

// Exact rational microhertz, NOT rounded fixed point. For the request input
// bounds (N,D,ppm <= 10^6), N*(10^6 +/- ppm)/D has numerator <= 2*10^12
// and denominator <= 10^6. Cross products fit uint64_t (<= 2*10^18).
// This is host/constant-evaluation metadata, not a firmware arithmetic policy.
struct FrequencyBound {
  std::uint64_t numerator = 0;
  std::uint32_t denominator = 1;
  constexpr bool valid() const {
    return numerator <= 2'000'000'000'000 && denominator > 0 && denominator <= 1'000'000;
  }
  constexpr FrequencyBound normalized() const {
    const auto divisor = std::gcd(numerator, std::uint64_t{denominator});
    return {numerator / divisor, static_cast<std::uint32_t>(denominator / divisor)};
  }
  constexpr bool at_most(FrequencyBound other) const {
    return fraction_at_most(numerator, denominator, other.numerator, other.denominator);
  }
  constexpr bool operator==(const FrequencyBound&) const = default;
};

struct FrequencyWindow {
  FrequencyBound lower{};
  FrequencyBound upper{};
  bool specified = false;

  static constexpr FrequencyWindow around(Ratio rate, std::uint32_t ppm = 0) {
    if (!rate.valid() || ppm > 1'000'000) { return {}; }
    return {
      FrequencyBound{std::uint64_t{rate.numerator} * (1'000'000 - ppm), rate.denominator}.normalized(),
      FrequencyBound{std::uint64_t{rate.numerator} * (1'000'000 + ppm), rate.denominator}.normalized(),
      true
    };
  }
  // Well-formed bounds can still represent an empty intersection (lower > upper).
  constexpr bool valid() const {
    return specified && lower.valid() && upper.valid() && upper.numerator > 0;
  }
  constexpr bool empty() const { return valid() && !lower.at_most(upper); }
  constexpr FrequencyWindow intersect(FrequencyWindow other) const {
    // Unspecified is the identity for accumulating clauses. Callers reject any
    // malformed clause before intersection, rather than treating it as absent.
    if (!specified) { return other; }
    if (!other.specified) { return *this; }
    return {lower.at_most(other.lower) ? other.lower : lower,
      upper.at_most(other.upper) ? upper : other.upper, true};
  }
  constexpr bool contains(Ratio actual) const {
    if (!valid() || empty() || !actual.positive()) { return false; }
    const FrequencyBound value{std::uint64_t{actual.numerator} * 1'000'000, actual.denominator};
    return lower.at_most(value) && value.at_most(upper);
  }
  constexpr bool operator==(const FrequencyWindow&) const = default;
};

} // namespace grevir::pwm
