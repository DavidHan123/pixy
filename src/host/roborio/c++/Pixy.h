//
// begin license header
//
// This file is part of Pixy CMUcam5 or "Pixy" for short
//
// All Pixy source code is provided under the terms of the
// GNU General Public License v2 (http://www.gnu.org/licenses/gpl-2.0.html).
// Those wishing to use Pixy source code, software and/or
// technologies under different licensing terms should contact us at
// cmucam@cs.cmu.edu. Such licensing terms are available for
// all portions of the Pixy codebase presented here.
//
// end license header
//
// This file is for defining the SPI-related classes.  It's called Pixy.h instead
// of Pixy_SPI.h because it's the default/recommended communication method
// with Arduino.  This class assumes you are using the ICSP connector to talk to 
// Pixy from your Arduino.  For more information go to:
//
//http://cmucam.org/projects/cmucam5/wiki/Hooking_up_Pixy_to_a_Microcontroller_(like_an_Arduino)
//

#ifndef PIXY_H
#define PIXY_H

#include "TPixy.h"
#include "SPI.h"


#define PIXY_SYNC_BYTE              0x5a
#define PIXY_SYNC_BYTE_DATA         0x5b
#define PIXY_BUF_SIZE               16

template <class BufType> struct CircularQ
{
  CircularQ()
  {
    len = 0;
  writeIndex = 0;
  readIndex = 0;
  }
  
  bool Read(BufType *c)
  {
    if (len)
  {
      *c = buf[readIndex++];
      len--;
      if (readIndex==PIXY_BUF_SIZE)
        readIndex = 0;
    return true;
  }
  else
    return false;
  }
  
  uint8_t FreeLen()
  {
    return PIXY_BUF_SIZE-len;
  } 
  
  bool Write(BufType c)
  {
    if (FreeLen()==0)
      return false;

    buf[writeIndex++] = c;
  len++;
    if (writeIndex==PIXY_BUF_SIZE)
      writeIndex = 0;
    return true;
  }

    BufType buf[PIXY_BUF_SIZE];
    uint8_t len;
    uint8_t writeIndex;
    uint8_t readIndex;
};

class LinkSPI
{
  public:
  LinkSPI(SPI::Port SPIport):
    SPI(SPIport)
  {

  }
  uint16_t getWord()
    {
      // ordering is different (big endian) because Pixy is sending 16 bits through SPI 
      // instead of 2 bytes in a 16-bit word as with I2C
      uint16_t w;

    if (inQ.Read(&w))
      return w;
    
    return GetWordHw();
    }

    uint8_t getByte()
    {
      uint8_t c[];
      SPI.Transaction(0x00, c, 1);
      return c;
    }
    
    int8_t send(uint8_t *data, uint8_t len)
    {
      int i;

      // check to see if we have enough space in our circular queue
      if (outQ.FreeLen()<len)
        return -1;

      for (i=0; i<len; i++)
    outQ.Write(data[i]);
    flushSend();
      return len;
    }

    void setArg(uint16_t arg)
    {
    }

  private:
    SPI SPI;
    uint16_t GetWordHw()
    {
      // ordering is different (big endian) because Pixy is sending 16 bits through SPI 
      // instead of 2 bytes in a 16-bit word as with I2C
      uint8_t c[], cout = 0, d[];
    
    if (outQ.Read(&cout))
       SPI.Transaction(&uint8_t(PIXY_SYNC_BYTE_DATA), d, 1);
      else
        SPI.Transaction(&uint8_t(PIXY_SYNC_BYTE), d, 1);
    
      SPI.Transaction(&uint8_t(cout), c, 1);

      uint16_t w = (c[0] << 8) + d[0];

      return w;
    }

  void flushSend()
    {
      uint16_t w;
      while(outQ.len)
      {
        w = GetWordHw();
    inQ.Write(w);
    }
  }
  
  // we need a little circular queues for both directions
  CircularQ<uint8_t> outQ;
  CircularQ<uint16_t> inQ;
};


typedef TPixy<LinkSPI> Pixy;

#endif
