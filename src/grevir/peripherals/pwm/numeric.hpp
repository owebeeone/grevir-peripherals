#pragma once
#include <grevir/base/compat/cstdint.hpp>
#include <grevir/base/compat/numeric.hpp>

namespace grevir::pwm {

// Bounded allocation metadata arithmetic. Values up to one million make the
// three-factor ppm comparison fit uint64_t (at most 10^18). Used during constant evaluation; not duty-update arithmetic.
struct Ratio {
  std::uint32_t numerator = 0;
  std::uint32_t denominator = 1;
  constexpr bool positive() const { return numerator > 0 && denominator > 0; }
  constexpr bool valid() const {
    return numerator > 0 && numerator <= 1'000'000
      && denominator > 0 && denominator <= 1'000'000;
  }
  constexpr Ratio normalized() const {
    const auto divisor = std::gcd(numerator, denominator);
    return {numerator / divisor, denominator / divisor};
  }
  constexpr bool operator==(const Ratio&) const = default;
};

// Compare arbitrary uint64 ratios without overflowing a cross product.
// Euclidean quotients reverse the comparison at each reciprocal step.
constexpr bool fraction_at_most(std::uint64_t an, std::uint64_t ad,
    std::uint64_t bn, std::uint64_t bd) {
  bool reversed = false;
  while (true) {
    const auto aq = an / ad;
    const auto bq = bn / bd;
    if (aq != bq) { return reversed ? aq > bq : aq < bq; }
    const auto ar = an % ad;
    const auto br = bn % bd;
    if (ar == 0 || br == 0) { return reversed ? br == 0 : ar == 0; }
    an = ad; ad = ar; bn = bd; bd = br;
    reversed = !reversed;
  }
}

constexpr bool at_most(Ratio a, Ratio b) {
  return std::uint64_t{a.numerator} * b.denominator
    <= std::uint64_t{b.numerator} * a.denominator;
}

constexpr bool within(Ratio actual, Ratio requested, std::uint32_t ppm) {
  const auto a = std::uint64_t{actual.numerator} * requested.denominator;
  const auto b = std::uint64_t{requested.numerator} * actual.denominator;
  const auto difference = a > b ? a - b : b - a;
  return difference * 1'000'000 <= b * ppm;
}

} // namespace grevir::pwm
