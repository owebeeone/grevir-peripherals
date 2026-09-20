#pragma once


namespace ardo {

/** A base class for output pins where a base class is needed. */
class OutputPinIF {
public:
  virtual ~OutputPinIF() {}
  virtual void setPin(bool) const = 0;
  virtual unsigned pinNo() const = 0;
};

/** A base class for input pins where a base class is needed. */
class InputPinIF {
public:
  virtual ~InputPinIF() {}
  virtual bool getPin() const = 0;
  virtual unsigned pinNo() const = 0;
};

/** A base class for pins that support both input and output where a 
 * base class is needed.
 * @deprecated - use OutputPinIF, InputPinIF as Base
 */
class InputOutputPinIF : public OutputPinIF, public InputPinIF {
};

} // namespace ardo
