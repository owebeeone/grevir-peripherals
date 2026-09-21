#pragma once
#include <grevir/peripherals/pwm/requirements.hpp>
#include <grevir/base/compat/array.hpp>
#include <grevir/base/compat/tuple.hpp>

namespace grevir::pwm {

enum class Kind { timer, channel, pin, domain };
struct Resource { unsigned id; unsigned parent; Kind kind; };
struct Endpoint {
  Key request;
  unsigned channel = 0;
  unsigned pin = 0;
  Ratio step{};
  constexpr bool operator==(const Endpoint&) const = default;
};
struct Candidate {
  unsigned key = 0;
  unsigned preference = 0;
  unsigned timer = 0;
  unsigned configuration = 0;
  Ratio frequency{};
  Waveform waveform = Waveform::any;
  Source source = Source::any;
  std::array<Endpoint, 4> endpoints{};
  unsigned count = 0;
  unsigned domain = 0;
  unsigned setting = 0;
  bool independent_duties = true;
  constexpr bool operator==(const Candidate&) const = default;
};

enum class Status {
  success, invalid_identity, duplicate_identity, invalid_request, model_error,
  no_candidate, reserved, conflict, exhausted
};
struct Diagnostic {
  Status status = Status::success;
  Key request{};
  unsigned detail = 0;
  constexpr bool operator==(const Diagnostic&) const = default;
};
template <std::size_t N>
struct Plan {
  Diagnostic diagnostic{};
  // Canonical bindings indexed by sorted request key. Shared requests point at
  // the same candidate, representing one configuration owner.
  std::array<Key, N> requests{};
  std::array<unsigned, N> candidates{};
  std::uint32_t visited = 0;
  constexpr bool ok() const { return diagnostic.status == Status::success; }
};

template <std::size_t R, std::size_t C, std::size_t H, std::size_t B>
struct Problem {
  std::array<Request, R> requests;
  std::array<Candidate, C> candidates;
  std::array<Resource, H> resources;
  std::array<unsigned, B> reservations;
};

template <typename Resources>
constexpr const Resource* resource(const Resources& resources, unsigned id) {
  for (const auto& r : resources) {
    if (r.id == id) { return &r; }
  }
  return nullptr;
}

template <typename Resources>
constexpr bool descendant(const Resources& resources, unsigned child, unsigned ancestor) {
  // Bounded even for malformed input; validation separately rejects cycles.
  for (std::size_t i = 0; child != 0 && i <= resources.size(); ++i) {
    if (child == ancestor) { return true; }
    const auto* r = resource(resources, child);
    if (r == nullptr) { return false; }
    child = r->parent;
  }
  return false;
}

template <typename Resources>
constexpr bool overlaps(const Resources& resources, unsigned a, unsigned b) {
  return a != 0 && b != 0 && (descendant(resources, a, b) || descendant(resources, b, a));
}

constexpr auto order(const Candidate& c) {
  // Endpoint identities are compared separately after canonical member sorting.
  return std::tuple{c.preference, c.timer, c.configuration};
}

template <Status S>
consteval void require_success() {
  static_assert(S == Status::success, "GREVIR_PWM_ALLOCATION_FAILED");
}

} // namespace grevir::pwm
