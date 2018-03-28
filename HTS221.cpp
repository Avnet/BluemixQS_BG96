/* ===================================================================
Copyright © 2016, AVNET Inc.  

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, 
software distributed under the License is distributed on an 
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
either express or implied. See the License for the specific 
language governing permissions and limitations under the License.

======================================================================== */

#include "HTS221.h"

#include <string>
#include "mbed.h"

I2C i2c(PTC11, PTC10);         //SDA, SCL -- define the I2C pins being used

// Read a single unsigned char from addressToRead and return it as a unsigned char
unsigned char HTS221::readRegister(unsigned char slaveAddress, unsigned char ToRead)
{
    char data = ToRead;

    //i2c.write(slaveAddress, &data, 1, 0);
    i2c.write(slaveAddress, &data, 1, 1); //by Stefan
    i2c.read(slaveAddress, &data, 1, 0);
    return data;
}

// Writes a single unsigned char (dataToWrite) into regToWrite
int HTS221::writeRegister(unsigned char slaveAddress, unsigned char regToWrite, unsigned char dataToWrite)
{
    const char data[] = {regToWrite, dataToWrite};

    return i2c.write(slaveAddress,data,2,0);
}


//jmf end
// ------------------------------------------------------------------------------

HTS221::HTS221(void) : _address(HTS221_ADDRESS) {
    _temperature = 0;
    _humidity = 0;
}


int HTS221::init(void) {
    uint8_t data;

    data = readRegister(_address, WHO_AM_I);
    if (data == WHO_AM_I_RETURN){
        if (activate()){
            storeCalibration();
            return data;
        }
    }

    return 0;
}

int HTS221::storeCalibration(void) {
    uint8_t data;
    uint16_t tmp;

    for (int reg=CALIB_START; reg<=CALIB_END; reg++) {
        if ((reg!=CALIB_START+8) && (reg!=CALIB_START+9) && (reg!=CALIB_START+4)) {

            data = readRegister(HTS221_ADDRESS, reg);

            switch (reg) {
            case CALIB_START:
                _h0_rH = data;
                break;
            case CALIB_START+1:
            _h1_rH = data;
            break;
            case CALIB_START+2:
            _T0_degC = data;
            break;
            case CALIB_START+3:
            _T1_degC = data;
            break;

            case CALIB_START+5:
            tmp = _T0_degC;
            _T0_degC = (data&0x3)<<8;
            _T0_degC |= tmp;

            tmp = _T1_degC;
            _T1_degC = ((data&0xC)>>2)<<8;
            _T1_degC |= tmp;
            break;
            case CALIB_START+6:
            _H0_T0 = data;
            break;
            case CALIB_START+7:
            _H0_T0 |= data<<8;
            break;
            case CALIB_START+0xA:
            _H1_T0 = data;
            break;
            case CALIB_START+0xB:
            _H1_T0 |= data <<8;
            break;
            case CALIB_START+0xC:
            _T0_OUT = data;
            break;
            case CALIB_START+0xD:
            _T0_OUT |= data << 8;
            break;
            case CALIB_START+0xE:
            _T1_OUT = data;
            break;
            case CALIB_START+0xF:
            _T1_OUT |= data << 8;
            break;


            case CALIB_START+8:
            case CALIB_START+9:
            case CALIB_START+4:
            //DO NOTHING
            break;

            // to cover any possible error
            default:
                return false;
            } /* switch */
        } /* if */
    }  /* for */
    return true;
}


int HTS221::activate(void) {
    uint8_t data;

    data = readRegister(_address, CTRL_REG1);
    data |= POWER_UP;
    data |= ODR0_SET;
    writeRegister(_address, CTRL_REG1, data);

    return true;
}


int HTS221::deactivate(void) {
    uint8_t data;

    data = readRegister(_address, CTRL_REG1);
    data &= ~POWER_UP;
    writeRegister(_address, CTRL_REG1, data);
    return true;
}


int HTS221::bduActivate(void) {
    uint8_t data;

    data = readRegister(_address, CTRL_REG1);
    data |= BDU_SET;
    writeRegister(_address, CTRL_REG1, data);

    return true;
}


int HTS221::bduDeactivate(void) {
    uint8_t data;

    data = readRegister(_address, CTRL_REG1);
    data &= ~BDU_SET;
    writeRegister(_address, CTRL_REG1, data);
    return true;
}


int HTS221::readHumidity(void) {
    uint8_t data   = 0;
    uint16_t h_out = 0;
    double h_temp  = 0.0;
    double hum     = 0.0;

    data = readRegister(_address, STATUS_REG);

    if (data & HUMIDITY_READY) {
        data = readRegister(_address, HUMIDITY_H_REG);
        h_out = data << 8;  // MSB
        data = readRegister(_address, HUMIDITY_L_REG);
        h_out |= data;      // LSB

        // Decode Humidity
        hum = ((int16_t)(_h1_rH) - (int16_t)(_h0_rH))/2.0;  // remove x2 multiple

        // Calculate humidity in decimal of grade centigrades i.e. 15.0 = 150.
        h_temp = (((int16_t)h_out - (int16_t)_H0_T0) * hum) / ((int16_t)_H1_T0 - (int16_t)_H0_T0);
        hum    = ((int16_t)_h0_rH) / 2.0; // remove x2 multiple
        _humidity = (int16_t)((hum + h_temp)); // provide signed % measurement unit
    }
    return _humidity;
}



double HTS221::readTemperature(void) {
    uint8_t data   = 0;
    uint16_t t_out = 0;
    double t_temp  = 0.0;
    double deg     = 0.0;

    data = readRegister(_address, STATUS_REG);

    if (data & TEMPERATURE_READY) {

        data= readRegister(_address, TEMP_H_REG);
        t_out = data  << 8; // MSB
        data = readRegister(_address, TEMP_L_REG);
        t_out |= data;      // LSB

        // Decode Temperature
        deg    = ((int16_t)(_T1_degC) - (int16_t)(_T0_degC))/8.0; // remove x8 multiple

        // Calculate Temperature in decimal of grade centigrades i.e. 15.0 = 150.
        t_temp = (((int16_t)t_out - (int16_t)_T0_OUT) * deg) / ((int16_t)_T1_OUT - (int16_t)_T0_OUT);
        deg    = ((int16_t)_T0_degC) / 8.0;     // remove x8 multiple
        _temperature = deg + t_temp;   // provide signed celsius measurement unit
    }

    return _temperature;
}
