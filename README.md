# Grevir Peripherals

First portable GPIO and timing extraction from Ardoinus. This local development
snapshot contains digital input/output pins, both open-drain variants, external
pin claims, pin interfaces, elapsed-time polling and cyclic/finite period sequences.
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

## Build and validation

Install Base, Time and Core into a prefix first, then:

```sh
cmake -S . -B build/native -DCMAKE_PREFIX_PATH=/your/grevir-install \
  -DGREVIR_BUILD_COMPILE_CHECKS=ON
cmake --build build/native
cmake --install build/native --prefix /your/grevir-install
```

Consumers use `find_package(grevir-peripherals CONFIG REQUIRED)` and link
`grevir::peripherals`. Six public headers compile independently, and concrete pin,
application and poller types compile without Catch2. Compiler probes accept
separate input/output pins and reject the same pin with the expected Core resource
conflict diagnostic. `tests/installed-consumer` builds and runs against installed
packages without sibling checkout includes.

For host behavior tests, also install Grevir Test Support and provision Catch2
3.8.1, then enable `GREVIR_BUILD_HOST_TESTS=ON` and run CTest. Ten cases execute the
production wrappers/pollers using a recorded GPIO backend and controlled 32-bit
clock: input modes, writes, both open-drain variants, external pin lifecycle,
strict expiry, catch-up, wraparound, reset, cyclic/finite sequences and a composed
blinking application. Mocks record software operations; they do not model pull-up
voltages, electrical behavior, interrupt timing or MCU registers.

Debounce, button events, the separate sequencer, PWM, timer selection and storage
regions remain planned. MCU adapters and board validation remain deferred.

Source: [owebeeone/ardoinus](https://github.com/owebeeone/ardoinus). The original
MIT license notice is copied unchanged in `LICENSE.txt`.
