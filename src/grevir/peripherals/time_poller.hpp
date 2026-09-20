/**
 * Time polling function.
 */
#ifndef ARDO_TIME_POLLER__H
#define ARDO_TIME_POLLER__H

#include <grevir/time/time.hpp>
#include <grevir/base/compat/cstdint.hpp>
#include <grevir/base/cyclic_int.hpp>

namespace ardo {
/**
 * Provide basic time period based polling.
 *
 * @param w_StateType Incremented when a period expires.
 * @param Clock Supplies TimeType and now(); unsigned time supports wraparound.
 */
template <typename w_StateType, typename Clock>
class TimePoller {
public:
  using StateType = w_StateType;
  using TimeType = typename Clock::TimeType;

  TimePoller() = default;

  TimePoller(TimeType from) : from(from) {}

  /** Sets the start time with now() time. */
  void setNow() {
    setFrom(Clock::now());
  }

  /** Sets the start time with the given time. */
  void setFrom(const TimeType& from) {
    this->from = from;
  }

  /** Polls to see if the given period passed since from. 
   * NOTE: This implementation assumes that the period of time between the from
   * time and the given period is smaller than half of the timer resolution.
   * This is intended to be called in a tight loop only a relatively small
   * period between successive calls to poll.
   * Returns true when period has expired and increments the state.
   */
  bool poll(const typename TimeType::period_type& period) {
    TimeType to = Clock::now();

    auto passed = to - from;

    if (passed > period) {
      from += period;
      ++state;
      return true;
    }
    return false;
  }

  const StateType& getState() const {
    return state;
  }

  void reset() {
    setNow();
    state = StateType{};
  }

private:
  StateType state = {};
  TimeType from = Clock::now();
};

/**
 * Provides a sequencing periods for a specific set of times.
 */
template <typename Tick, Tick... Seq>
class Sequence;

template <typename Tick, Tick SN, Tick... R>
class Sequence<Tick, SN, R...> {
public:
  using type = Tick;
  static constexpr unsigned count = 1 + Sequence<Tick, R...>::count;

  inline static type get(unsigned index) {
    if (index == 0) {
      return SN;
    }
    return Sequence<Tick, R...>::get(index - 1);
  }
};

template <typename Tick, Tick L>
class Sequence<Tick, L> {
public:
  using type = Tick;
  static constexpr unsigned count = 1;

  inline static type get(unsigned) {
    return L;
  }
};

/**
 * Provides a poller that will sequence through the given periods cyclicly.
 */
template <typename w_Seq, typename Clock>
class CyclicTimeSequencePoller {
public:
  using Sequence = w_Seq;
  using TimeType = typename Clock::TimeType;

  static_assert(
    Sequence::count > 1, 
    "Sequence is too small, add more sequnce items (min 2).");

  bool poll() {
    return poller.poll(typename TimeType::period_type(Sequence::get(state())));
  }

  unsigned state() const {
    return poller.getState().get();
  }

  void init() {
    poller.setNow();
  }

  void reset() {
    poller.reset();
  }

  TimePoller<setl::CyclicInt<Sequence::count>, Clock> poller;
};

/**
 * Provides a poller that will sequence throgh the given periods once.
 */
template <
  typename w_Seq, 
  typename Clock,
  typename w_StateType = unsigned>
class TimerSequencePoller {
public:
  using Sequence = w_Seq;
  using StateType = w_StateType;
  using TimeType = typename Clock::TimeType;

  bool poll() {
    // Terminate if we reach the end of the sequence.
    if (hasFinished()) {
      return false;
    }
    return poller.poll(typename TimeType::period_type(Sequence::get(state())));
  }

  bool hasFinished() {
    return Sequence::count <= poller.getState();
  }

  StateType state() const {
    return poller.getState();
  }

  void reset() {
    poller.reset();
  }

  void init() {
    poller.setNow();
  }

  TimePoller<StateType, Clock> poller;
};

}  // namespace

#endif

