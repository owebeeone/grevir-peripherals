#pragma once

namespace ardo::gpio {
// Logical modes; backend adapters translate these to their platform constants.
enum class InputPinMode { PullUp, Untied };
enum class OutputPinMode { Output, OpenDrainLowOutput, OpenDrainHighOutput };
} // namespace ardo::gpio
