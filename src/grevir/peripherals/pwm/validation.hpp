#pragma once
#include <grevir/peripherals/pwm/model.hpp>
#include <algorithm>

namespace grevir::pwm {

constexpr bool candidate_less(const Candidate& a, const Candidate& b) {
  if (order(a) != order(b)) { return order(a) < order(b); }
  const auto common = std::min(a.count, b.count);
  for (unsigned i = 0; i < common; ++i) {
    const auto x = std::tuple{a.endpoints[i].request, a.endpoints[i].channel, a.endpoints[i].pin};
    const auto y = std::tuple{b.endpoints[i].request, b.endpoints[i].channel, b.endpoints[i].pin};
    if (x != y) { return x < y; }
  }
  return std::tuple{a.count, a.key} < std::tuple{b.count, b.key};
}

template <typename P>
constexpr bool normalize_model(P& p) {
  const auto is_kind = [&](unsigned id, Kind kind) {
    const auto* r = resource(p.resources, id);
    return r != nullptr && r->kind == kind;
  };
  for (const auto& r : p.resources) {
    if (r.id == 0) { return false; }
    unsigned duplicates = 0;
    for (const auto& other : p.resources) {
      if (r.id == other.id) { ++duplicates; }
    }
    if (duplicates != 1) { return false; }
    // This model supports a two-level timer/channel tree. It does not
    // claim to cover arbitrary hardware containment or aggregate constraints.
    if (r.kind == Kind::channel) {
      if (!is_kind(r.parent, Kind::timer)) { return false; }
    } else {
      if (r.parent != 0) { return false; }
    }
  }
  for (auto reserved : p.reservations) {
    if (resource(p.resources, reserved) == nullptr) { return false; }
  }
  for (std::size_t i = 0; i < p.reservations.size(); ++i) {
    for (std::size_t j = 0; j < i; ++j) {
      if (overlaps(p.resources, p.reservations[i], p.reservations[j])) { return false; }
    }
  }
  for (auto& c : p.candidates) {
    if (c.key == 0 || c.configuration == 0 || !is_kind(c.timer, Kind::timer)
        || !c.frequency.positive() || c.count == 0 || c.count > c.endpoints.size()) { return false; }
    if (c.domain != 0 && (!is_kind(c.domain, Kind::domain) || c.setting == 0)) { return false; }
    if (c.domain == 0 && c.setting != 0) { return false; }
    c.frequency = c.frequency.normalized();
    std::sort(c.endpoints.begin(), c.endpoints.begin() + c.count,
      [](const auto& a, const auto& b) { return a.request < b.request; });
    for (unsigned i = 0; i < c.count; ++i) {
      auto& e = c.endpoints[i];
      if (!is_kind(e.channel, Kind::channel) || !is_kind(e.pin, Kind::pin)
          || !descendant(p.resources, e.channel, c.timer)
          || !e.step.valid() || !at_most(e.step, {1, 1})) { return false; }
      bool known = false;
      for (const auto& request : p.requests) {
        if (request.key == e.request) { known = true; }
      }
      if (!known) { return false; }
      e.step = e.step.normalized();
      for (unsigned j = 0; j < i; ++j) {
        if (e.request == c.endpoints[j].request || e.channel == c.endpoints[j].channel
            || e.pin == c.endpoints[j].pin) { return false; }
      }
    }
    for (std::size_t i = c.count; i < c.endpoints.size(); ++i) { c.endpoints[i] = {}; }
  }
  for (const auto& a : p.candidates) {
    for (const auto& b : p.candidates) {
      if (a.key == b.key && a != b) { return false; }
      if (a.timer == b.timer && a.configuration == b.configuration
          && (a.frequency != b.frequency || a.waveform != b.waveform || a.source != b.source
            || a.domain != b.domain || a.setting != b.setting)) { return false; }
    }
  }
  std::sort(p.candidates.begin(), p.candidates.end(), candidate_less);
  return true;
}

template <typename P>
constexpr unsigned reserved_by(const P& p, const Candidate& c) {
  unsigned lowest = 0;
  for (auto r : p.reservations) {
    bool conflict = overlaps(p.resources, r, c.timer) || overlaps(p.resources, r, c.domain);
    for (unsigned i = 0; i < c.count; ++i) {
      conflict = conflict || overlaps(p.resources, r, c.endpoints[i].pin)
        || overlaps(p.resources, r, c.endpoints[i].channel);
    }
    if (conflict && (lowest == 0 || r < lowest)) { lowest = r; }
  }
  return lowest;
}

template <typename P>
constexpr bool compatible(const P& p, const Candidate& a, const Candidate& b) {
  if (overlaps(p.resources, a.timer, b.timer)) { return false; }
  if (a.domain != 0 && a.domain == b.domain && a.setting != b.setting) { return false; }
  for (unsigned i = 0; i < a.count; ++i) {
    for (unsigned j = 0; j < b.count; ++j) {
      if (overlaps(p.resources, a.endpoints[i].pin, b.endpoints[j].pin)
          || overlaps(p.resources, a.endpoints[i].channel, b.endpoints[j].channel)) { return false; }
    }
  }
  return true;
}

} // namespace grevir::pwm
