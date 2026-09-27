#include <grevir/peripherals/pwm/requirements.hpp>
#include <grevir/peripherals/timer/owner_allocator.hpp>

namespace {
namespace t = grevir::timer;
namespace p = grevir::pwm;

using Clock = t::Instance<"clock",t::Own<t::PeriodEventUse<"tick">>>;
using Drive = t::Instance<"drive",t::Own<
  p::PwmRequest<"output",p::Frequency<p::Hertz<1000>,p::Exact>,
    p::DutyStepAtMost<1,256>,p::Pin<301>>,
  t::PeriodEventUse<"period">>>;

constexpr auto input = t::demands<Drive,Clock>();
static_assert(input.size() == 3);
static_assert(input[2].kind == t::UseKind::period_event);

constexpr t::Candidate clock{
  "clock", 11, 1, 0,
  {{{{"clock","tick"},t::UseKind::period_event,101,0}}}, 1,
  {401}, 1
};
constexpr t::Candidate drive{
  "drive", 22, 2, 0,
  {{{{"drive","output"},t::UseKind::pwm,201,301},
    {{"drive","period"},t::UseKind::period_event,202,0}}}, 2,
  {402,403}, 2
};
constexpr auto combined = t::compile(t::Problem{input,
  std::array{drive,clock},std::array<unsigned,0>{}});
constexpr auto reversed = t::compile(t::Problem{t::demands<Clock,Drive>(),
  std::array{clock,drive},std::array<unsigned,0>{}});
static_assert(combined.ok() && reversed.ok());
static_assert(combined.uses == reversed.uses);
static_assert(combined.candidates == reversed.candidates);
static_assert(combined.candidates[0] == clock.key);
static_assert(combined.candidates[1] == drive.key);
static_assert(combined.candidates[2] == drive.key);

constexpr auto event_only = t::compile(t::Problem{
  t::demands<Clock>(),std::array{clock},std::array<unsigned,0>{}});
static_assert(event_only.ok());

static_assert(t::compile(t::Problem{t::demands<Drive>(),
  std::array{drive},std::array{301u}}).diagnostic.status == t::Status::reserved);
static_assert(t::compile(t::Problem{t::demands<Drive>(),
  std::array{drive},std::array{201u}}).diagnostic.status == t::Status::reserved);
static_assert(t::compile(t::Problem{t::demands<Drive>(),
  std::array{drive},std::array{999u}}).ok());
static_assert(t::compile(t::Problem{t::demands<Clock,Drive>(),
  std::array{drive,clock},std::array{301u}}).diagnostic.status == t::Status::reserved);

constexpr auto clock_role_on_drive_pin = [] {
  auto candidate = clock;
  candidate.exclusive_roles[0] = drive.bindings[0].pin;
  return candidate;
}();
constexpr auto clock_role_on_drive_endpoint = [] {
  auto candidate = clock;
  candidate.exclusive_roles[0] = drive.bindings[0].endpoint;
  return candidate;
}();
static_assert(!t::compatible(clock_role_on_drive_pin,drive));
static_assert(!t::compatible(drive,clock_role_on_drive_endpoint));
static_assert(t::compile(t::Problem{input,
  std::array{clock_role_on_drive_pin,drive},std::array<unsigned,0>{}})
  .diagnostic.status == t::Status::conflict);
static_assert(t::compile(t::Problem{t::demands<Clock,Drive>(),
  std::array{drive,clock_role_on_drive_endpoint},std::array<unsigned,0>{}})
  .diagnostic.status == t::Status::conflict);

constexpr auto bad_role = [] {
  auto candidate = drive;
  candidate.exclusive_roles[1] = candidate.exclusive_roles[0];
  return candidate;
}();
static_assert(t::compile(t::Problem{t::demands<Drive>(),
  std::array{bad_role},std::array<unsigned,0>{}}).diagnostic.status
  == t::Status::invalid_model);

constexpr auto same_timer = [] {
  auto candidate = drive;
  candidate.timer = clock.timer;
  return candidate;
}();
static_assert(t::compile(t::Problem{input,
  std::array{clock,same_timer},std::array<unsigned,0>{}}).diagnostic.status
  == t::Status::conflict);

constexpr auto empty = t::compile(t::Problem{
  std::array<t::UseDemand,0>{},std::array<t::Candidate,0>{},std::array<unsigned,0>{}});
static_assert(empty.ok() && empty.candidates.empty());
}
