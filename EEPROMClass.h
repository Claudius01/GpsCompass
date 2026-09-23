/*
  EEPROM.h -ported by Paolo Becchi to Esp32 from esp8266 EEPROM
           -Modified by Elochukwu Ifediora <ifedioraelochukwuc@gmail.com>
           -Converted to nvs lbernstone@gmail.com

  Uses a nvs byte array to emulate EEPROM

  Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  CR 2021/10: Modification de signatures + Add 'useful_length()' method
*/

#ifndef EEPROM_h
#define EEPROM_h
#ifndef EEPROM_FLASH_PARTITION_NAME
#define EEPROM_FLASH_PARTITION_NAME "eeprom"
#endif
#include <Arduino.h>

#define USE_ESP_LOG             0     // Non utilisation de 'esp_log'

#define EEPROM_SIZE           128     // Use only 128 bytes

typedef uint32_t nvs_handle;

class EEPROMClass {
  public:
    EEPROMClass(uint32_t sector);
    EEPROMClass(const char* name, uint32_t user_defined_size);
    EEPROMClass(void);
    ~EEPROMClass(void);

    bool begin(size_t size);
    uint8_t read(int address);
    bool write(int address, uint8_t val);
    uint16_t length();
    uint16_t useful_length();
    bool commit();
    bool end();

    uint8_t * getDataPtr();
    uint16_t convert(bool clear, const char* EEPROMname = "eeprom", const char* nvsname = "eeprom");

    template<typename T>
    T &get(int address, T &t) {
      if (address < 0 || address + sizeof(T) > _size)
        return t;

      memcpy((uint8_t*) &t, _data + address, sizeof(T));
      return t;
    }

    template<typename T>
    const T &put(int address, const T &t) {
      if (address < 0 || address + sizeof(T) > _size)
        return t;

      memcpy(_data + address, (const uint8_t*) &t, sizeof(T));
      _dirty = true;
      return t;
    }

    uint8_t readByte(int address);
    int8_t readChar(int address);
    uint8_t readUChar(int address);
    int16_t readShort(int address);
    uint16_t readUShort(int address);
    int32_t readInt(int address);
    uint32_t readUInt(int address);
    int32_t readLong(int address);
    uint32_t readULong(int address);
    int64_t readLong64(int address);
    uint64_t readULong64(int address);
    float_t readFloat(int address);
    double_t readDouble(int address);
    bool readBool(int address);
    size_t readString(int address, char* value, size_t maxLen);
    String readString(int address);
    size_t readBytes(int address, void * value, size_t maxLen);
    template <class T> T readAll (int address, T &);

    bool writeByte(int address, uint8_t value);
    bool writeChar(int address, int8_t value);
    bool writeUChar(int address, uint8_t value);
    bool writeShort(int address, int16_t value);
    bool writeUShort(int address, uint16_t value);
    bool writeInt(int address, int32_t value);
    bool writeUInt(int address, uint32_t value);
    bool writeLong(int address, int32_t value);
    bool writeULong(int address, uint32_t value);
    bool writeLong64(int address, int64_t value);
    bool writeULong64(int address, uint64_t value);
    bool writeFloat(int address, float_t value);
    bool writeDouble(int address, double_t value);
    bool writeBool(int address, bool value);
    bool writeString(int address, const char* value);
    bool writeString(int address, String value);
    bool writeBytes(int address, const void* value, size_t len);

    template <class T> T writeAll (int address, const T &);

  protected:
    nvs_handle _handle;
    uint8_t* _data;
    size_t _size;
    bool _dirty;
    const char* _name;
    uint32_t _user_defined_size;      // Size at the creation with the constructor
    uint32_t _user_useful_size;       // Useful size after the call to 'begin' method (expand eor truncate @ '_user_defined_size')
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
extern EEPROMClass EEPROM;
#endif

#endif
