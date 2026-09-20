/**
* Button click classification.
*/
#ifndef ARDO_BUTTON_EVENT__H
#define ARDO_BUTTON_EVENT__H

#include <grevir/core/module.hpp>
#include <grevir/time/time.hpp>

namespace ardo {

/**
 * Converts GPIO input from a button to events, Click, DoubleClick and LongClick
 * events. Input is active-low and should already be debounced. Clock supplies
 * TimeType and now(); thresholds remain 300 ms and 400 ms in clock units.
 */
template <typename w_InPin, typename Clock>
class ButtonEventModule : public ardo::ModuleBase <ardo::Parameters<w_InPin>> {
 protected:
  using InPin = w_InPin;
  static ButtonEventModule instance;

  using TimeType = typename Clock::TimeType;
  using PeriodType = typename TimeType::period_type;

  const PeriodType LONG_PRESS_TIME = PeriodType(setl::Period<unsigned long, setl::TimeUnit::MILLIS>(300));
  const PeriodType DOUBLE_CLICK_TIME = PeriodType(setl::Period<unsigned long, setl::TimeUnit::MILLIS>(400));

  enum InProcessingState : char {
    Off,
    WaitForDelay,
    WaitForRelease,
    WaitForPossibleDoubleClick
  };
 public:
  enum OutEventType : char {
    None,
    Click,
    DoubleClick,
    LongClick
  };

  static void runSetup() {
    instance.inState = Off;
    instance.outEvent = None;
    instance.last_pressed_time = Clock::now();
  }

  static void runLoop() {
    instance.instanceLoop();
  }

  void instanceLoop() {
    bool pinState = InPin::get();
    switch (inState) {
      case Off: {
        if (!pinState) {
          inState = WaitForDelay;
          last_pressed_time = Clock::now();
        }
        break;
      }

      case WaitForDelay: {
        if (pinState) {
          inState = WaitForPossibleDoubleClick;
          last_pressed_time = Clock::now();
          break;
        }

        TimeType now_pressed_time = Clock::now();
        if (now_pressed_time - last_pressed_time >= LONG_PRESS_TIME) {
          // This is a long press.
          outEvent = LongClick;
          inState = WaitForRelease;
        }
        break;
      }

      case WaitForPossibleDoubleClick: {

        if (!pinState) {
          outEvent = DoubleClick;
          inState = WaitForRelease;
          break;
        }

        TimeType now_pressed_time = Clock::now();
        if (now_pressed_time - last_pressed_time >= DOUBLE_CLICK_TIME) {
          // The double-click window has elapsed.
          outEvent = Click;
          inState = WaitForRelease;
        }
        break;
      }

      case WaitForRelease: {
        if (pinState) {
          inState = Off;
          break;
        }
        break;
      }
    }
  }

  /**
   * Retrieves an event or None if no event is pending.
   */
  static OutEventType get() {
    return instance.instanceGet();
  }

 protected:
  OutEventType instanceGet() {
    OutEventType result = outEvent;
    outEvent = None;
    return result;
  }

  TimeType last_pressed_time;
  InProcessingState inState = Off;
  OutEventType outEvent = None;
};


template <typename InPin, typename Clock>
ButtonEventModule<InPin, Clock> ButtonEventModule<InPin, Clock>::instance;

} // namespace ardo

#endif  // ARDO_BUTTON_EVENT__H
