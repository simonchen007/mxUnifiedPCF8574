#include "mxUnifiedPCF8574.h"

#ifdef ESP8266
mxUnifiedPCF8574::mxUnifiedPCF8574(uint8_t uI2C_address, uint8_t nPinSDA, uint8_t nPinSCL)		// defaults:  uint8_t nPinSDA = SDA, uint8_t nPinSCL = SCL);
#else
mxUnifiedPCF8574::mxUnifiedPCF8574(uint8_t uI2C_address)
#endif
{
	// Call without parameters assumes using the default pins.
	// the unified PCF8574 I2C library assumes default SDA/SCL pins unless specified otherwise
	// ATmega 328: A4, A5
	// ESP8266: 4, 5 (alternative pins for ESP-01 are 1 (RX) and 3 (TX)

	// This standard expanded pin definition allows the following devices connected as example
	// ATmega328, PCD8544+Max7219 panel:
	//  P3;  D13 - led
	//  P2:  D3 - RST
	//  P4:  D4 - CE
	//  P5:  D5 - DC
	//  P6:  D6 - DIN
	//  P7:  D7 - CLK	
	
	//  P0:  D10 - CS  (SS)
	//  P1:  D11 - DIN (MOSI)
	//       D12 - not connected (MISO)
	//  P3:  D13 - CLK (SCK)
	
	_nConstr++;		// Constructor counter only for debug purposes
	_i2c_address=uI2C_address;
	_i2c_speed=0L;		// will be set to proper value in begin()
#ifdef ESP8266
  _i2c_sda=nPinSDA;
  _i2c_scl=nPinSCL;
#else
  _i2c_sda=0;
  _i2c_scl=0;
#endif
}

void mxUnifiedPCF8574::begin(uint32_t i2c_speed)		// default ESP8266: uint32_t i2c_speed=1000000L, others: uint32_t i2c_speed=400000L
{	// there is an alternative begin() for ESP using higher default I2C speed
	_i2c_error = 0;
	_i2c_speed=i2c_speed;
	
#ifdef ESP8266
		Wire.begin(_i2c_sda, _i2c_scl);			// esp8266 can set alternative I2C pins 
#else
		Wire.begin();
#endif
		Wire.setClock(_i2c_speed);		// set clock a bit faster than the default 100 kHz
																	// 400000L (400kHz) seems fastest speed on Atmega328 8Mhz, faster (800kHz) may be possible on 16Mhz
																	// 400000L (400kHz) seems stable speed on ESP8266@80Mzh, faster (1MHz) is possible, but perhaps less stable

#if(_MXUNIFIEDIO_DEBUG)
  Serial.print(F("mxUnifiedPCF8574::begin("));
  Serial.print(_i2c_address);
  Serial.print(F(") constr:"));
  Serial.print(_nConstr);
  Serial.println(F("."));
#endif
}

void mxUnifiedPCF8574::send8Bits(bool fClosedTransmission)		// default: fClosedTransmission=true
{    // Overwrite in subclasses if desired!
	if(fClosedTransmission)
		startTransmission();
  Wire.write(_dataOut);
	if(fClosedTransmission)
	  endTransmission();		// this call will do the actual sending of the bits
}

void mxUnifiedPCF8574::sendBits()
{
	send8Bits();
}

void mxUnifiedPCF8574::shiftOut(uint8_t nDataPin, uint8_t nClockPin, uint8_t bitOrder, uint8_t nValue)
{	// shiftOut the bits of one byto to the stated exended data and clock pins
	// TODO: bitOrder can be MSBFIRST or LSBFIRST, not implemented yet!
	// TODO: currently shiftOut will not start/end transmission. Would be nice if automatic?
	uint8_t aI2C_bytestream[16];
	uint8_t uCnt=0;
	//uint8_t dataOut=_pUniOut->getBits();
//	Serial.print(F("C"));
  for(uint8_t bit = 0x80; bit; bit >>= 1)
  {
		// Using the PCF8574 library for bulk writes is WAY TOO SLOW! (1400 ms to display the screen)
 		// Each SPI DIN databyte transfer requires 16 CLK toggles, i.e. 16 I2C bytes to write.
 		// Therefor bundle the writing of each byte into one Wire.write() call and minimize the begin/end calls.
 		// This saves writing the address for each bit and speeds screen display up to 380 ms.
 		// Still slow, but acceptable when display() function is not called after drawing each primitive. 
		// BTW. Setting bits is more readable using the i2cSetBit function and doesn't cost much performance.
  	setBit(nClockPin, LOW);
  	setBit(nDataPin, (nValue & bit));
    aI2C_bytestream[uCnt++]=_dataOut;
    //Wire.write(_i2c_dataOut);
  	setBit(nClockPin, HIGH);
    aI2C_bytestream[uCnt++]=_dataOut;
    //Wire.write(_i2c_dataOut);
  }
//	Serial.print(F("W"));
  Wire.write(aI2C_bytestream, sizeof(aI2C_bytestream));		// writing the bytes of all 16 bits in one go (still takes 302-480ms)
//	Serial.print(F("e"));
}


void mxUnifiedPCF8574::startTransmission(void)
{    // Overwrite in subclasses if desired!
	Wire.beginTransmission(_i2c_address);
}

void mxUnifiedPCF8574::endTransmission(void)
{    // Overwrite in subclasses if desired!
//	Serial.print(F("T"));
	Wire.endTransmission();		// this call will do the actual sending of the bits
//	Serial.print(F("n"));
}

 	
void mxUnifiedPCF8574::digitalWrite(uint8_t nPin, uint8_t nVal)
{ // Overwrite in subclasses if desired!
#if(_MXUNIFIEDIO_DEBUG)
  Serial.print(F("mxUnifiedPCF8574::digitalWrite("));
  Serial.print(nPin);
  Serial.print(F(","));
  Serial.print(nVal);
  Serial.println(F(")"));
#endif

	if (nPin > 7) return;
	setBit(nPin, nVal);
//	::digitalWrite(_aPins[nPin], nVal);		// call the regular digitalWrite using the global scope resolution operator
	send8Bits();
}	

