#pragma once

#include <grevir/peripherals/timer/own.hpp>
#include <grevir/core/allocation/search.hpp>
#include <grevir/base/compat/algorithm.hpp>
#include <grevir/base/compat/array.hpp>
#include <grevir/base/compat/compare.hpp>
#include <grevir/base/compat/cstdint.hpp>
#include <grevir/base/compat/string_view.hpp>
#include <grevir/base/compat/tuple.hpp>

namespace grevir::timer {

struct UseKey {
  std::string_view owner;
  std::string_view local;
  constexpr auto operator<=>(const UseKey&) const = default;
};
struct UseDemand {
  UseKey key;
  UseKind kind;
};

template <typename... Instances>
constexpr auto demands() {
  constexpr std::size_t count = (Instances::count + ... + 0);
  std::array<UseDemand, count> result{};
  std::size_t next = 0;
  (Instances::visit([&]<typename Use>(Use*) {
    result[next++] = {{Instances::name.view(), Use::name.view()}, Use::kind};
  }), ...);
  return result;
}

// The backend guarantees each binding's behavior and carries its typed
// configuration payload beside this common envelope. Resources are canonical
// identities supplied by that backend; zero means no pin/source/role.
struct UseBinding {
  UseKey key{};
  UseKind kind = UseKind::pwm;
  unsigned endpoint = 0;
  unsigned pin = 0;
  constexpr bool operator==(const UseBinding&) const = default;
};
struct Candidate {
  std::string_view owner{};
  unsigned key = 0;
  unsigned timer = 0;
  unsigned preference = 0;
  std::array<UseBinding, 4> bindings{};
  unsigned binding_count = 0;
  std::array<unsigned, 8> exclusive_roles{};
  unsigned role_count = 0;
  constexpr bool operator==(const Candidate&) const = default;
};
template <typename Payload>
struct Choice {
  Candidate candidate;
  Payload payload;
};

enum class Status {
  success, invalid_identity, duplicate_identity, invalid_model,
  no_candidate, reserved, conflict, exhausted
};
struct Diagnostic {
  Status status = Status::success;
  UseKey use{};
  unsigned detail = 0;
};
template <std::size_t D>
struct Plan {
  Diagnostic diagnostic{};
  std::array<UseKey, D> uses{};
  std::array<unsigned, D> candidates{};
  std::uint32_t visited = 0;
  constexpr bool ok() const { return diagnostic.status == Status::success; }
};
template <std::size_t D, std::size_t C, std::size_t R>
struct Problem {
  std::array<UseDemand, D> demands;
  std::array<Candidate, C> candidates;
  std::array<unsigned, R> reservations;
};

constexpr bool valid_component(std::string_view name) {
  if (name.empty()) { return false; }
  for (char c : name) {
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9') || c == '_')) { return false; }
  }
  return true;
}

template <typename P>
constexpr bool valid_candidate(const P& problem, const Candidate& c) {
  if (!valid_component(c.owner) || c.key == 0 || c.timer == 0
      || c.binding_count == 0 || c.binding_count > c.bindings.size()
      || c.role_count > c.exclusive_roles.size()) { return false; }
  for (unsigned i = 0; i < c.binding_count; ++i) {
    const auto& b = c.bindings[i];
    if (b.key.owner != c.owner || !valid_component(b.key.local)
        || b.endpoint == 0) { return false; }
    bool known = false;
    for (const auto& demand : problem.demands) {
      if (demand.key == b.key && demand.kind == b.kind) { known = true; }
    }
    if (!known) { return false; }
    for (unsigned j = 0; j < i; ++j) {
      if (b.key == c.bindings[j].key || b.endpoint == c.bindings[j].endpoint
          || (b.pin != 0 && b.pin == c.bindings[j].pin)) { return false; }
    }
  }
  for (unsigned i = 0; i < c.role_count; ++i) {
    if (c.exclusive_roles[i] == 0) { return false; }
    for (unsigned j = 0; j < i; ++j) {
      if (c.exclusive_roles[i] == c.exclusive_roles[j]) { return false; }
    }
  }
  return true;
}

template <typename P>
constexpr bool fits(const P& p, const Candidate& c, std::string_view owner) {
  if (c.owner != owner) { return false; }
  unsigned matched = 0;
  for (const auto& demand : p.demands) {
    if (demand.key.owner != owner) { continue; }
    bool found = false;
    for (unsigned i = 0; i < c.binding_count; ++i) {
      if (c.bindings[i].key == demand.key && c.bindings[i].kind == demand.kind) {
        found = true;
      }
    }
    if (!found) { return false; }
    ++matched;
  }
  return matched == c.binding_count;
}

template <typename F>
constexpr void each_resource(const Candidate& candidate, F&& f) {
  f(candidate.timer);
  for (unsigned i = 0; i < candidate.binding_count; ++i) {
    f(candidate.bindings[i].endpoint);
    if (candidate.bindings[i].pin != 0) { f(candidate.bindings[i].pin); }
  }
  for (unsigned i = 0; i < candidate.role_count; ++i) {
    f(candidate.exclusive_roles[i]);
  }
}

constexpr bool compatible(const Candidate& a, const Candidate& b) {
  bool overlaps = false;
  each_resource(a, [&](unsigned left) {
    each_resource(b, [&](unsigned right) { overlaps |= left == right; });
  });
  return !overlaps;
}

template <std::size_t D, std::size_t C, std::size_t R>
constexpr Plan<D> solve(Problem<D, C, R> p, std::uint32_t budget = 100'000) {
  Plan<D> result;
  std::sort(p.demands.begin(), p.demands.end(),
    [](const auto& a, const auto& b) { return a.key < b.key; });
  std::sort(p.candidates.begin(), p.candidates.end(), [](const auto& a, const auto& b) {
    return std::tuple{a.owner, a.preference, a.timer, a.key}
      < std::tuple{b.owner, b.preference, b.timer, b.key};
  });
  for (std::size_t i = 0; i < D; ++i) { result.uses[i] = p.demands[i].key; }
  const auto fail = [&](Status status, UseKey use = {}, unsigned detail = 0) {
    result.diagnostic = {status,use,detail};
    result.candidates.fill(0);
    return result;
  };
  for (std::size_t i = 0; i < D; ++i) {
    if (!valid_component(p.demands[i].key.owner)
        || !valid_component(p.demands[i].key.local)) {
      return fail(Status::invalid_identity, p.demands[i].key);
    }
    if (i != 0 && p.demands[i].key == p.demands[i - 1].key) {
      return fail(Status::duplicate_identity, p.demands[i].key);
    }
  }
  for (std::size_t i = 0; i < C; ++i) {
    if (!valid_candidate(p, p.candidates[i])) { return fail(Status::invalid_model); }
    for (std::size_t j = 0; j < i; ++j) {
      if (p.candidates[i].key == p.candidates[j].key) {
        return fail(Status::invalid_model);
      }
    }
  }
  std::array<std::size_t, D> unit{};
  std::array<std::size_t, D> first{};
  std::size_t units = 0;
  for (std::size_t i = 0; i < D; ++i) {
    if (i == 0 || p.demands[i].key.owner != p.demands[i - 1].key.owner) {
      first[units++] = i;
    }
    unit[i] = units - 1;
  }
  std::array<std::array<bool, C>, D> eligible{};
  for (std::size_t u = 0; u < units; ++u) {
    bool any = false;
    unsigned blocked = 0;
    for (std::size_t c = 0; c < C; ++c) {
      if (!fits(p, p.candidates[c], p.demands[first[u]].key.owner)) { continue; }
      bool reserved = false;
      for (unsigned r : p.reservations) {
        each_resource(p.candidates[c], [&](unsigned occupied) {
          reserved |= r == occupied;
        });
      }
      if (reserved) { blocked = p.candidates[c].timer; }
      else { eligible[u][c] = true; any = true; }
    }
    if (!any) {
      return fail(blocked == 0 ? Status::no_candidate : Status::reserved,
        p.demands[first[u]].key, blocked);
    }
  }
  const auto search = grevir::allocation::search(eligible, units,
    [&](std::size_t a, std::size_t b) {
      return compatible(p.candidates[a], p.candidates[b]);
    }, budget);
  result.visited = search.visited;
  if (!search.success) {
    return fail(search.exhausted ? Status::exhausted : Status::conflict);
  }
  for (std::size_t i = 0; i < D; ++i) {
    result.candidates[i] = p.candidates[search.selected[unit[i]]].key;
  }
  return result;
}

template <typename P>
consteval auto compile(P problem, std::uint32_t budget = 100'000) {
  return solve(problem, budget);
}

} // namespace grevir::timer
