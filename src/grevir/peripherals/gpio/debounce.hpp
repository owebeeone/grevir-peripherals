#pragma once

#include <grevir/peripherals/gpio/input.hpp>

namespace ardo {

/**
 * Debounced input type. debounceTime is measured in Clock ticks (default 300).
 * Call runSetup() before reading; setup accepts the current raw level.
 * Poll get() regularly: only observed samples contribute to debounce state.
 */
template <typename InputPin, typename Clock, unsigned long debounceTime = 300>
class DebounceInput : public InputPin {
protected:
  DebounceInput() {}
public:
  using TimeType = typename Clock::TimeType;

  static bool get() {
    return debounceState.debounce(InputPin::get());
  }

  // May override base class.
  bool getPin() const {
    return get();
  }

  inline static void runSetup() {
    InputPin::runSetup();
    debounceState.runSetup(); // Initialize the debounce state.
  }

  static const DebounceInput pin;

private:
  // The debounce state 
  struct DebounceState {
    bool debounce(bool input) {
      const bool changedNow = input != currentLevel;
      if (!changedNow) {
        // Returning to the accepted level cancels the pending transition.
        isChanged = false;
      } else if (isChanged) {
        TimeType now = Clock::now();
        auto elapsed = now - lastChangeTime;
        if (elapsed.get() > debounceTime) {
          currentLevel = input;
          isChanged = false;
        }
      } else {
        isChanged = true;
        lastChangeTime = Clock::now();
      }

      return currentLevel;
    }

    void runSetup() {
      currentLevel = InputPin::get();
      isChanged = false;
      lastChangeTime = Clock::now();
    }

    TimeType lastChangeTime;
    bool currentLevel = false;
    bool isChanged = false;
  };

  static DebounceState debounceState;
};

template <typename InputPin, typename Clock, unsigned long debounceTime>
typename DebounceInput<InputPin, Clock, debounceTime>::DebounceState
    DebounceInput<InputPin, Clock, debounceTime>::debounceState;

template <typename InputPin, typename Clock, unsigned long debounceTime>
const DebounceInput<InputPin, Clock, debounceTime>
    DebounceInput<InputPin, Clock, debounceTime>::pin;

} // namespace ardo
