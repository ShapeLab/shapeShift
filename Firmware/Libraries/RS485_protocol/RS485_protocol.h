#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WConstants.h"
#endif

typedef void (*WriteCallback)  (const byte what);    // send a byte to serial port
typedef int  (*AvailableCallback)  ();    // return number of bytes available
typedef int  (*ReadCallback)  ();    // read a byte from serial port

void sendMsg (WriteCallback fSend, 
              const byte * data, const byte length);
byte recvMsg (AvailableCallback fAvailable, ReadCallback fRead, 
              byte * data, const byte length, 
              unsigned long timeout = 10);