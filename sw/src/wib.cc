#include "wib.h"
#include "sensors.h"

#include <cstdio>

WIB::WIB() {
    io_reg_init(&this->regs,0xA0002000,32);
    i2c_init(&this->i2c,"/dev/i2c-0");
    for (int i = 0; i < 4; i++) {
        //FIXME: need FEMB devices in FW first
        //this->femb[i] = new FEMB(i);
    }
}

WIB::~WIB() {
    io_reg_free(&this->regs);
    i2c_free(&this->i2c);
    for (int i = 0; i < 4; i++) {
        //FIXME: need FEMB devices in FW first
        //delete this->femb[i];
    }
}

bool WIB::initialize() {
    //setup the FEMBs and COLDATA chips
}

bool WIB::read_sensors(wib::Sensors &sensors) {

   
   printf("Activating I2C_SENSOR bus\n");
   io_reg_write(&this->regs,1,0x00000005); //reg1 = 0x5
    
   printf("Enabling voltage sensors\n");
   uint8_t buf[1] = {0x7};
   i2c_write(&i2c,0x70,buf,1); //
   // 0x48 LTC2991
   // 0x4C 0x4E  LTC2990
   
   enable_ltc2990(&i2c,0x4E);
   for (uint8_t i = 1; i < 5; i++) {
       printf("LTC2990 0x4E ch%i -> %0.2f V\n",i,0.00030518*read_ltc2990_value(&i2c,0x4E,i));
   }
   printf("LTC2990 0x4E Vcc -> %0.2f V\n",0.00030518*read_ltc2990_value(&i2c,0x4E,5)+2.5);
   
   enable_ltc2990(&i2c,0x4C);
   for (uint8_t i = 1; i < 5; i++) {
       printf("LTC2990 0x4C ch%i -> %0.2f V\n",i,0.00030518*read_ltc2990_value(&i2c,0x4C,i));
   }
   printf("LTC2990 0x4C Vcc -> %0.2f V\n",0.00030518*read_ltc2990_value(&i2c,0x4C,5)+2.5);
   
   enable_ltc2991(&i2c,0x48);
   for (uint8_t i = 1; i < 8; i++) {
       printf("LTC2991 0x48 ch%i -> %0.2f V\n",i,0.00030518*read_ltc2991_value(&i2c,0x48,i));
   }
   printf("LTC2991 0x48 T -> %0.2f C\n",0.0625*read_ltc2991_value(&i2c,0x48,8));
   printf("LTC2991 0x48 Vcc -> %0.2f V\n",0.00030518*read_ltc2991_value(&i2c,0x48,9)+2.5);
   
   // 0x49 0x4D 0x4A supposedly are AD7414
   printf("AD7414 0x49 temp %i\n", read_ad7414_temp(&i2c,0x49));
   printf("AD7414 0x4D temp %i\n", read_ad7414_temp(&i2c,0x4D));
   printf("AD7414 0x4A temp %i\n", read_ad7414_temp(&i2c,0x4A));
   
   // 0x15 LTC2499
   printf("Reading temperature sensors\n");
   for (uint8_t i = 0; i < 7; i++) {
       printf("LTC2499 ch%i -> %0.14f\n",i,read_ltc2499_temp(&i2c,i));
   }
   
   return true;
}
