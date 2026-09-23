# Grevir Peripherals

**Public API:** [Grevir Peripherals](https://github.com/owebeeone/grevir-wz/blob/main/docs/api/peripherals.md).
See [installation](https://github.com/owebeeone/grevir-wz/blob/main/docs/install.md) and
[supported platforms](https://github.com/owebeeone/grevir-wz/blob/main/docs/supported.md).
The workspace `/docs` is the current user-facing contract; development
checkpoints below are historical.

Portable GPIO, polling, buttons, storage and timer requirements.

## Development record (historical)

Portable GPIO, button handling and timing extracted from Ardoinus. This local development
snapshot contains digital input/output pins, both open-drain variants, external
pin claims, pin interfaces, debounce, button events, elapsed-time polling and
cyclic/finite period sequences, a backend-bound PWM wrapper, typed storage regions
and portable timer requirements.
It depends on Grevir Base, Time and Core. Hardware validation is on hold.

Include `<GrevirPeripherals.h>` or an individual `<grevir/peripherals/...hpp>`
header. Existing class names and the `ardo` namespace remain, with explicit
hardware/service bindings replacing the old implicit `CoreIF` dependency:

```cpp
using Input = ardo::InputPin<BoardGPIO, 2>; // pull-up by default
using Output = ardo::OutputPin<BoardGPIO, 3>;
using Intervals = ardo::Sequence<std::uint32_t, 100, 400>;
using Poller = ardo::CyclicTimeSequencePoller<Intervals, MillisecondClock>;
```

`BoardGPIO` supplies static `pinMode(unsigned, ardo::gpio::InputPinMode)`,
`pinMode(unsigned, ardo::gpio::OutputPinMode)`, `digitalRead(unsigned)` returning
bool, and `digitalWrite(unsigned, bool)`. Each instantiated wrapper requires only
the operations it uses. Logical input modes are `PullUp` and `Untied`; output modes
are `Output`, `OpenDrainLowOutput` and `OpenDrainHighOutput`. The open-drain
specializations translate their modes into input/output changes and a preloaded
output latch. A future Arduino/MCU adapter translates these logical operations to
its native API; these headers do not import Arduino macros or fake Arduino headers.

A clock supplies `TimeType` (a typed Grevir time) and static `now()`. Its tick type
and units must match the intended periods. The signatures are
`TimePoller<State, Clock>`, `Sequence<Tick, values...>`,
`CyclicTimeSequencePoller<Sequence, Clock>` and
`TimerSequencePoller<Sequence, Clock, State = unsigned>`. These template signatures
are deliberate migration changes and require updates to legacy call sites.

The original poller behavior is preserved: expiry requires elapsed time **greater
than** the period, and each call catches up one interval while retaining the prior
schedule. `init()` restarts the time origin without resetting sequence state;
`reset()` resets both. Unsigned clock subtraction supports wraparound within the
legacy polling interval constraints. Pin claims still identify resources by pin
number, independent of backend type; separate GPIO controllers are not modeled.

## Debounce and button events

```cpp
using RawButton = ardo::InputPin<BoardGPIO, 2>;
using StableButton = ardo::DebounceInput<RawButton, MillisecondClock, 10>;
using Button = ardo::ButtonEventModule<StableButton, MillisecondClock>;
using App = ardo::Application<Button>;
// Call App::runSetup(), then regularly call App::runLoop() and Button::get().
```

`DebounceInput<InputPin, Clock, debounceTime = 300>` retains the raw pin's claims
and optional virtual input interface. The interval is in **clock ticks**; a
microsecond clock with 300 ticks matches the old default. Setup configures the pin,
samples its initial level and clears pending transitions. A different sampled
level must persist for **more than** the interval before acceptance. Returning to
the accepted level cancels the pending change, fixing the old bounce-timer bug.
Only observed samples count; transitions between reads are not detected.

`ButtonEventModule<InputPin, Clock>` expects active-low input, normally debounced.
It preserves the original click classifier: a held press emits `LongClick` at
300 ms; release of a short press starts a 400 ms double-click window; a second
press emits `DoubleClick`, otherwise expiry emits `Click`. Thresholds are converted
into the supplied clock's units, which must represent those durations. The legacy
decision order is retained: a sampled second press takes priority over timeout
when both are observed in the same loop. Poll regularly. `get()` returns and clears
the latest pending event (`None` when empty); this is one slot, not an event queue.
Setup resets processing and pending events, making repeated application setup safe.
The template's singleton storage is now defined in the header for linked consumers.

## PWM output

`HardwarePwm<Pin, Backend, ScalerSelector = setl::LinearScalerSelector<8>, Bases...>`
accepts a concrete, caller-selected PWM backend. `Backend` provides `timer_bits`,
`Claims` and static `setPwm(scaledValue)`. It is already bound to the relevant pin,
timer and configuration; the portable wrapper does not select a timer or configure
hardware registers. `HwPwmParameters` and the implicit `ardo_system` resource lookup
are replaced by that explicit backend type.

```cpp
using PwmPin = ardo::OutputPin<BoardGPIO, 3>;
using Led = ardo::HardwarePwm<PwmPin, BoardPwmForPin3>;
// Use Led as a module parameter; setup/loop are forwarded to PwmPin.
// Led::setPwm(128) scales an 8-bit duty input to the backend resolution.
```

`Claims` combines the pin and backend resource claims. Compatible explicit sharing
is allowed; repeated exclusive pins/timers and conflicting ranges are rejected.
`runSetup()` and `runLoop()` retain the legacy forwarding to `Pin` only: backend
configuration must already be arranged by the caller's pin/application binding.
No new backend lifecycle protocol is assumed.

A scaler selector provides `in_value_type` and `Scaler<timer_bits>`; its scaler
provides `scale(input)`. Public `value_type` is now the **input** type, fixing legacy
narrowing when input resolution exceeds output resolution. For example, 16-bit
65535 now scales to 8-bit 255 instead of zero. The default input remains 0–255.
Callers supply values in the selector's supported range; no clamping is added.
`pwm_pin.setPwmPin(uint32_t)` preserves the optional virtual-interface entry point.

The unused `ardo_sequencer.h` mapping is superseded by `time_poller.hpp`. No separate
`sequencer.hpp` is created. The original source and old IDE header listings remain
unchanged; no C/C++ callers were found. Use the extracted `ardo` poller types with
explicit clock/tick bindings.

## Typed storage regions

`EepromReaderWriter<T, Address, Backend>` reads/writes a trivially copyable value in
an explicitly bound byte store. The backend supplies:

- `using Resource = ...`: the physical store's resource identity. Multiple adapters
  for the same store must share this type; independent stores use different types.
  `ardo::EepromResource` remains available for a single EEPROM store.
- `static constexpr std::size_t capacity`: capacity in bytes.
- Static `read(address)` returning a byte and `update(address, byte)` writing it.
  Avoiding writes to unchanged bytes is the backend's responsibility.

The region claims `[Address, Address + sizeof(T))`. Negative addresses, capacity
violations, unrepresentable claim endpoints and nontrivial value types fail at
compile time. The exclusive end must fit Core's signed `int` range representation.
Same-store overlaps conflict; adjacent regions and regions in distinct stores do
not. Setup/loop perform no I/O.

The extracted write operation sends each **byte value**, fixing the legacy pointer
argument to `EEPROM.update`. Reads/writes preserve native object representation,
including its byte order and any padding. This is not cross-platform serialization;
read bytes must be a valid representation of `T`, and `read()` also needs `T{}`.
No Arduino EEPROM provider is imported; that adapter remains a separate extraction.

## Portable timer requirements

`timer/requirements.hpp` provides `Frequency<Hertz, Divider = 1, Type = float>`,
`VariableFrequency<...>`, `Resolution<Bits>`, `TimerConfig<Requests...>`, parameter
classes and the existing explicit `TimerConfigFilter`. The portable enum contains
only frequency, variable frequency and resolution. AVR modes/pin options and default
board inventory are absent. `VariableFrequency::is_variable` is now true, correcting
its inherited false value. Zero frequency/divider/resolution and repeated or
conflicting frequency/resolution requests are rejected when instantiated.

Use `CheckedTimerConfig<Backend, Config>::Config` to validate mandatory requests.
The backend supplies `AllowedParameters::BaseTypes` (usually through
`ParameterClasses<...>`) and `template<typename Config> static constexpr bool
accepts`. The latter checks the complete configuration's values and combinations,
including required missing settings. Unsupported categories and rejected combinations
produce explicit diagnostics. The checked wrapper preserves every supplied request.

`TimerConfigFilter<Allowed, Config>::Config` retains deliberate projection to a
`std::tuple`; it does not prove that the original request is supported. Do not
substitute that filter for `CheckedTimerConfig` when requirements are mandatory.

This increment validates an explicitly chosen backend. It does not assign timers,
apply registers, calculate MCU prescalers, or implement the old pass-through
`TimerSelector`/`SelectionResolver` scaffolding. `timer/selection.hpp` remains planned.
Synthetic backend tests validate this contract, not any actual timer capability.

## Build and validation

Install Base, Time and Core into a prefix first, then:

```sh
cmake -S . -B build/native -DCMAKE_PREFIX_PATH=/your/grevir-install \
  -DGREVIR_BUILD_COMPILE_CHECKS=ON
cmake --build build/native
cmake --install build/native --prefix /your/grevir-install
```

Consumers use `find_package(grevir-peripherals CONFIG REQUIRED)` and link
`grevir::peripherals`. Eleven public headers compile independently, and concrete pin,
application, debounce/button, PWM, storage and poller types compile without Catch2. Compiler
probes accept separate pins and reject same-pin conflicts for both raw and
debounced inputs (two positive and two negative cases). `tests/installed-consumer` builds and runs against installed
packages without sibling checkout includes.

For host behavior tests, also install Grevir Test Support and provision Catch2
3.8.1, then enable `GREVIR_BUILD_HOST_TESTS=ON` and run CTest. Twenty-nine cases execute the
production wrappers/pollers using a recorded GPIO backend and controlled 32-bit
clock: input modes, writes, both open-drain variants, external pin lifecycle,
strict expiry, catch-up, wraparound, reset, cyclic/finite sequences and a composed
blinking application. Additional cases cover bouncing presses/releases, startup,
repeat setup, single/double/long clicks and microsecond-clock thresholds across
wraparound. Five PWM cases check scaling up/down, equal resolutions, the optional
virtual interface and pin lifecycle forwarding. PWM compiler probes pass two valid
compositions and reject six pin/timer/range conflicts with the intended diagnostics.
Five storage cases cover object bytes, offsets, untouched neighbors, update
semantics, adjacent/end-of-store regions and lifecycle. Storage compiler probes pass
three positive cases and reject eight invalid types/regions/claims. Timer contracts
have twelve static assertions, two positive backend configurations and twelve
expected rejections, including invalid values, unsupported categories and combinations.
Mocks record software operations; they do not model pull-up
voltages, electrical behavior, interrupt timing or MCU registers.

Timer inventory selection/allocation, MCU/Arduino adapters and board validation
remain deferred.

Source: [owebeeone/ardoinus](https://github.com/owebeeone/ardoinus). The original
MIT license notice is copied unchanged in `LICENSE.txt`.

## Installed portable PWM integration

The fixed-frequency ATmega328P PWM MVP now uses installed Core, Peripherals and AVR
headers. Core collects module requests and existing resource claims; AVR supplies
candidates and typed endpoints. Application setup initializes the selected owners
before parameter/module callbacks. See the workspace's
`dev-docs/GrevirPwmIntegration.md` for the complete example, resource identity rules,
startup preconditions and current limits. AVR compiler/hardware validation remains
on hold; native package installation does not establish MCU toolchain support.
