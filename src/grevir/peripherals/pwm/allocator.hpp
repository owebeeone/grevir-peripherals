#pragma once
#include <grevir/peripherals/pwm/validation.hpp>
#include <grevir/core/allocation/search.hpp>

namespace grevir::pwm {

// Value-based constexpr search; the public compile() wrapper requires constant
// evaluation. A runtime entry is retained solely for exhaustive HOST checking.
template <std::size_t R, std::size_t C, std::size_t H, std::size_t B>
constexpr Plan<R> solve(Problem<R, C, H, B> p, std::uint32_t budget = 100'000) {
  Plan<R> result;
  std::sort(p.requests.begin(), p.requests.end(),
    [](const auto& a, const auto& b) { return a.key < b.key; });
  for (std::size_t i = 0; i < R; ++i) { result.requests[i] = p.requests[i].key; }
  const auto fail = [&](Status status, Key key = {}, unsigned detail = 0) {
    result.diagnostic = {status, key, detail};
    result.candidates.fill(0);
    return result;
  };
  for (const auto& r : p.requests) {
    if (!identifier(r.key.instance) || !identifier(r.key.local)) {
      return fail(Status::invalid_identity, r.key);
    }
    unsigned count = 0;
    for (const auto& other : p.requests) {
      if (other.key == r.key) { ++count; }
    }
    if (count > 1) { return fail(Status::duplicate_identity, r.key, count); }
  }
  for (const auto& r : p.requests) {
    if (r.config.error != ConfigError::none) {
      return fail(Status::invalid_request, r.key, static_cast<unsigned>(r.config.error));
    }
    if (!r.config.frequency.valid() || !r.config.step.valid()
        || !at_most(r.config.step, {1, 1})
        || r.config.pin == 0) { return fail(Status::invalid_request, r.key); }
    if (r.config.frequency.empty()) {
      return fail(Status::invalid_request, r.key, static_cast<unsigned>(ConfigError::conflict));
    }
  }
  if (!normalize_model(p)) { return fail(Status::model_error); }
  for (const auto& r : p.requests) {
    const auto* pin = resource(p.resources, r.config.pin);
    if (pin == nullptr || pin->kind != Kind::pin) { return fail(Status::model_error, r.key); }
  }

  // Units are sorted by their least member key because requests are sorted.
  std::array<std::size_t, R> unit{};
  std::array<std::size_t, R> first{};
  std::size_t units = 0;
  for (std::size_t i = 0; i < R; ++i) {
    std::size_t found = units;
    if (p.requests[i].group != 0) {
      for (std::size_t j = 0; j < i; ++j) {
        if (p.requests[j].group == p.requests[i].group) { found = unit[j]; break; }
      }
    }
    if (found == units) { first[units++] = i; }
    unit[i] = found;
  }

  const auto fits = [&](const Candidate& c, std::size_t u) {
    unsigned members = 0;
    for (std::size_t i = 0; i < R; ++i) {
      if (unit[i] == u) {
        if (members == c.count) { return false; }
        const auto& r = p.requests[i];
        const auto& e = c.endpoints[members++];
        if (e.request != r.key || e.pin != r.config.pin
            || !r.config.frequency.contains(c.frequency)
            || !at_most(e.step, r.config.step)
            || (r.config.waveform != Waveform::any && r.config.waveform != c.waveform)
            || (r.config.source != Source::any && r.config.source != c.source)) { return false; }
      }
    }
    return members == c.count && (members == 1 || c.independent_duties);
  };

  std::array<std::array<bool, C>, R> eligible{};
  for (std::size_t u = 0; u < units; ++u) {
    bool any = false;
    unsigned blocker = 0;
    for (std::size_t c = 0; c < C; ++c) {
      if (c != 0 && p.candidates[c] == p.candidates[c - 1]) { continue; }
      if (fits(p.candidates[c], u)) {
        const auto reservation = reserved_by(p, p.candidates[c]);
        if (reservation != 0) {
          if (blocker == 0 || reservation < blocker) { blocker = reservation; }
        } else { eligible[u][c] = true; any = true; }
      }
    }
    if (!any) {
      return fail(blocker == 0 ? Status::no_candidate : Status::reserved,
        p.requests[first[u]].key, blocker);
    }
  }

  const auto search = grevir::allocation::search(eligible, units, [&](std::size_t a, std::size_t b) {
    return compatible(p, p.candidates[a], p.candidates[b]);
  }, budget);
  result.visited = search.visited;
  if (!search.success) { return fail(search.exhausted ? Status::exhausted : Status::conflict); }
  for (std::size_t i = 0; i < R; ++i) {
    result.candidates[i] = p.candidates[search.selected[unit[i]]].key;
  }
  return result;
}

template <typename P>
consteval auto compile(P problem, std::uint32_t budget = 100'000) { return solve(problem, budget); }

} // namespace grevir::pwm
