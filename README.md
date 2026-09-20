# Grevir Peripherals

Portable GPIO, button handling and timing extracted from Ardoinus. This local development
snapshot contains digital input/output pins, both open-drain variants, external
pin claims, pin interfaces, debounce, button events, elapsed-time polling and
cyclic/finite period sequences.
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

## Build and validation

Install Base, Time and Core into a prefix first, then:

```sh
cmake -S . -B build/native -DCMAKE_PREFIX_PATH=/your/grevir-install \
  -DGREVIR_BUILD_COMPILE_CHECKS=ON
cmake --build build/native
cmake --install build/native --prefix /your/grevir-install
```

Consumers use `find_package(grevir-peripherals CONFIG REQUIRED)` and link
`grevir::peripherals`. Eight public headers compile independently, and concrete pin,
application, debounce/button and poller types compile without Catch2. Compiler
probes accept separate pins and reject same-pin conflicts for both raw and
debounced inputs (two positive and two negative cases). `tests/installed-consumer` builds and runs against installed
packages without sibling checkout includes.

For host behavior tests, also install Grevir Test Support and provision Catch2
3.8.1, then enable `GREVIR_BUILD_HOST_TESTS=ON` and run CTest. Nineteen cases execute the
production wrappers/pollers using a recorded GPIO backend and controlled 32-bit
clock: input modes, writes, both open-drain variants, external pin lifecycle,
strict expiry, catch-up, wraparound, reset, cyclic/finite sequences and a composed
blinking application. Additional cases cover bouncing presses/releases, startup,
repeat setup, single/double/long clicks and microsecond-clock thresholds across
wraparound. Mocks record software operations; they do not model pull-up
voltages, electrical behavior, interrupt timing or MCU registers.

The separate sequencer, PWM, timer selection and storage regions remain planned. MCU adapters and board validation remain deferred.

Source: [owebeeone/ardoinus](https://github.com/owebeeone/ardoinus). The original
MIT license notice is copied unchanged in `LICENSE.txt`.
