#ifndef __I2CCommun_H__
#define __I2CCommun_H__

#define WAIT 0x80

enum {
      CNone,CASCReq,
      CPauseMs,CInitI2CCCoul,CPosCol,CTime,CStop,
     };

#define R_Word(t) (((unsigned char *)t)[0]+((unsigned)(((unsigned char *)t)[1])<<8))

#define I2C_AD_CCOUL    0x24

#endif
