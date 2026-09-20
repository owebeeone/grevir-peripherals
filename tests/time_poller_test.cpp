#include "mock_devices.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

using namespace peripheral_mock;
using Sequence = ardo::Sequence<std::uint32_t, 10, 20>;

TEST_CASE_METHOD(Fixture, "poller retains strict expiry and catches up one step per call", "[clock]") {
  ardo::TimePoller<unsigned, Clock> poller;
  Clock::set(10);
  REQUIRE_FALSE(poller.poll(Period(10)));
  REQUIRE(poller.getState() == 0);
  Clock::set(31);
  REQUIRE(poller.poll(Period(10)));
  REQUIRE(poller.getState() == 1);
  REQUIRE(poller.poll(Period(10)));
  REQUIRE(poller.getState() == 2);
  REQUIRE(poller.poll(Period(10)));
  REQUIRE(poller.getState() == 3);
  REQUIRE_FALSE(poller.poll(Period(10)));
  poller.reset();
  REQUIRE(poller.getState() == 0);
  Clock::set(41);
  REQUIRE_FALSE(poller.poll(Period(10)));
  Clock::set(42);
  REQUIRE(poller.poll(Period(10)));
}

TEST_CASE_METHOD(Fixture, "poller expires correctly across unsigned clock wraparound", "[clock]") {
  Clock::set(std::numeric_limits<std::uint32_t>::max() - 4);
  ardo::TimePoller<unsigned, Clock> poller;
  Clock::set(5);
  REQUIRE_FALSE(poller.poll(Period(10)));
  Clock::set(6);
  REQUIRE(poller.poll(Period(10)));
  REQUIRE(poller.getState() == 1);
  Clock::set(15);
  REQUIRE_FALSE(poller.poll(Period(10)));
  Clock::set(16);
  REQUIRE(poller.poll(Period(10)));
}

TEST_CASE_METHOD(Fixture, "cyclic sequence repeats periods and reset restarts state", "[clock]") {
  ardo::CyclicTimeSequencePoller<Sequence, Clock> poller;
  Clock::set(11);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 1);
  Clock::set(30);
  REQUIRE_FALSE(poller.poll());
  Clock::set(31);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 0);
  Clock::set(41);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 1);
  Clock::set(100);
  poller.init();
  REQUIRE(poller.state() == 1);
  REQUIRE_FALSE(poller.poll());
  Clock::set(121);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 0);
  poller.reset();
  REQUIRE_FALSE(poller.poll());
  Clock::set(132);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 1);
}

TEST_CASE_METHOD(Fixture, "finite sequence stops at the end until reset", "[clock]") {
  ardo::TimerSequencePoller<Sequence, Clock> poller;
  REQUIRE_FALSE(poller.hasFinished());
  Clock::set(11);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 1);
  Clock::set(31);
  REQUIRE(poller.poll());
  REQUIRE(poller.state() == 2);
  REQUIRE(poller.hasFinished());
  Clock::set(100);
  REQUIRE_FALSE(poller.poll());
  poller.init();
  REQUIRE(poller.hasFinished());
  poller.reset();
  REQUIRE_FALSE(poller.hasFinished());
  REQUIRE(poller.state() == 0);
  Clock::set(111);
  REQUIRE(poller.poll());
}
