#pragma once

#include <grevir/interrupt/binding.hpp>
#include <grevir/base/compat/cstdint.hpp>

namespace grevir::timer {

using interrupt::EventIdentity;
using interrupt::FixedArray;

struct Request {
  EventIdentity event{};
  bool pwm_required = false;
};

struct Candidate {
  EventIdentity request{};
  std::string_view configuration{};
  std::string_view timer{};
  std::string_view owner{};
  std::string_view source{};
  std::string_view selector{};
  std::string_view entry{};
  std::string_view snapshot_policy{};
  std::string_view acknowledge_policy{};
  unsigned preference = 0;
  bool pwm = false;
  bool period_event = false;
  bool reserved = false;
};

enum class Error {
  none, invalid_model, duplicate_request, duplicate_candidate,
  no_candidate, conflict, exhausted
};

template <std::size_t R, std::size_t C>
struct Problem {
  FixedArray<Request, R> requests{};
  FixedArray<Candidate, C> candidates{};
};

template <std::size_t R>
struct Solution {
  Error error = Error::none;
  interrupt::SelectedPlan<R> interrupts{};
  FixedArray<std::string_view, R> configurations{};
  std::uint32_t visited = 0;
};

constexpr bool less(const Candidate& a, const Candidate& b) {
  if (a.request != b.request) { return a.request < b.request; }
  if (a.preference != b.preference) { return a.preference < b.preference; }
  if (a.timer != b.timer) { return a.timer < b.timer; }
  return a.configuration < b.configuration;
}

template <std::size_t R, std::size_t C, std::size_t D>
constexpr Solution<R> solve(Problem<R, C> problem,
    const interrupt::DemandSummary<D>& demands, std::uint32_t budget = 100000) {
  Solution<R> result{};
  const auto fail = [&](Error error) {
    result.error = error;
    result.interrupts = {};
    return result;
  };
  for (std::size_t i = 0; i < R; ++i) {
    for (std::size_t j = i + 1; j < R; ++j) {
      if (problem.requests[j].event < problem.requests[i].event) {
        const auto temporary = problem.requests[i];
        problem.requests[i] = problem.requests[j];
        problem.requests[j] = temporary;
      }
    }
    if (i != 0 && problem.requests[i - 1].event == problem.requests[i].event) {
      return fail(Error::duplicate_request);
    }
  }
  for (std::size_t i = 0; i < C; ++i) {
    for (std::size_t j = i + 1; j < C; ++j) {
      if (less(problem.candidates[j], problem.candidates[i])) {
        const auto temporary = problem.candidates[i];
        problem.candidates[i] = problem.candidates[j];
        problem.candidates[j] = temporary;
      }
    }
    const auto& candidate = problem.candidates[i];
    if (!interrupt::valid_component(candidate.configuration)
        || !interrupt::valid_component(candidate.timer)
        || !interrupt::valid_component(candidate.owner)
        || (candidate.period_event &&
            (!interrupt::valid_component(candidate.source)
             || !interrupt::valid_component(candidate.selector)
             || !interrupt::valid_component(candidate.entry)
             || !interrupt::valid_component(candidate.snapshot_policy)
             || !interrupt::valid_component(candidate.acknowledge_policy)))) {
      return fail(Error::invalid_model);
    }
    for (std::size_t j = 0; j < i; ++j) {
      if (candidate.request == problem.candidates[j].request
          && candidate.configuration == problem.candidates[j].configuration) {
        return fail(Error::duplicate_candidate);
      }
    }
  }
  FixedArray<std::size_t, R> selected{};
  FixedArray<bool, R> selected_demanded{};
  bool exhausted = false;
  const auto search = [&](auto&& self, std::size_t row) -> bool {
    if (row == R) { return true; }
    const auto& request = problem.requests[row];
    bool demanded = false;
    for (std::size_t d = 0; d < demands.count; ++d) {
      if (demands.keys[d] == request.event) { demanded = true; }
    }
    for (std::size_t c = 0; c < C; ++c) {
      const auto& candidate = problem.candidates[c];
      if (candidate.request != request.event || candidate.reserved
          || (request.pwm_required && !candidate.pwm)
          || (demanded && !candidate.period_event)) { continue; }
      if (result.visited == budget) { exhausted = true; return false; }
      ++result.visited;
      bool compatible = true;
      for (std::size_t earlier = 0; earlier < row; ++earlier) {
        const auto& used = problem.candidates[selected[earlier]];
        if (used.timer == candidate.timer
            || (demanded && selected_demanded[earlier] &&
                (used.source == candidate.source || used.entry == candidate.entry))) {
          compatible = false;
        }
      }
      if (!compatible) { continue; }
      selected[row] = c;
      selected_demanded[row] = demanded;
      if (self(self, row + 1)) { return true; }
      if (exhausted) { return false; }
    }
    return false;
  };
  if (!search(search, 0)) {
    if (exhausted) { return fail(Error::exhausted); }
    for (std::size_t row = 0; row < R; ++row) {
      bool eligible = false;
      for (std::size_t c = 0; c < C; ++c) {
        const auto& candidate = problem.candidates[c];
        bool demanded = false;
        for (std::size_t d = 0; d < demands.count; ++d) {
          if (demands.keys[d] == problem.requests[row].event) { demanded = true; }
        }
        if (candidate.request == problem.requests[row].event && !candidate.reserved
            && (!problem.requests[row].pwm_required || candidate.pwm)
            && (!demanded || candidate.period_event)) { eligible = true; }
      }
      if (!eligible) { return fail(Error::no_candidate); }
    }
    return fail(Error::conflict);
  }
  result.interrupts.allocation_ok = true;
  for (std::size_t row = 0; row < R; ++row) {
    const auto& request = problem.requests[row];
    const auto& candidate = problem.candidates[selected[row]];
    result.configurations[row] = candidate.configuration;
    for (std::size_t d = 0; d < demands.count; ++d) {
      if (demands.keys[d] == request.event) {
        auto& binding = result.interrupts.bindings[result.interrupts.count++];
        binding = {request.event, candidate.owner, candidate.configuration,
          candidate.source, candidate.selector, candidate.entry,
          candidate.snapshot_policy, candidate.acknowledge_policy, 0, false};
      }
    }
  }
  return result;
}

template <class Spec, std::size_t R, std::size_t C, std::size_t D>
constexpr bool validates(const Problem<R, C>& problem,
    const interrupt::DemandSummary<D>& demands,
    const interrupt::SelectedPlan<R>& plan) {
  constexpr auto catalog = interrupt::EventCatalog<Spec>::keys;
  if (R != catalog.size()) { return false; }
  for (std::size_t i = 0; i < R; ++i) {
    bool found = false;
    for (std::size_t j = 0; j < R; ++j) {
      if (problem.requests[j].event == catalog[i]) {
        if (found) { return false; }
        found = true;
      }
    }
    if (!found) { return false; }
  }
  const auto solution = solve(problem, demands);
  if (solution.error != Error::none || plan.count > R
      || plan.count != solution.interrupts.count
      || plan.allocation_ok != solution.interrupts.allocation_ok
      || plan.search_exhausted != solution.interrupts.search_exhausted) {
    return false;
  }
  for (std::size_t i = 0; i < plan.count; ++i) {
    if (!(plan.bindings[i] == solution.interrupts.bindings[i])) { return false; }
  }
  return true;
}

} // namespace grevir::timer
