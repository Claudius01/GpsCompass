#ifndef __GESTION_EEPROM__
#define __GESTION_EEPROM__

#include <sstream>
#include <Arduino.h>

#include "EEPROMClass.h"

#define USE_WRITE_OPTIMIZATION      1

// Structure de l'eeprom
#define NBR_COORD_IN_EEPROM         5

#define EEPROM_OFFSET_NBR_WRITE     4
#define EEPROM_OFFSET_1ST_COORD     0x0010
#define EEPROM_OFFSET_CHECKSUM      0x007e

typedef struct {
  uint8_t             idx;        //  1
  float               lat;        // +4
  float               lon;        // +4
  float               ele;        // +4
  uint8_t             range;      // +1             // Range [0, 1, ..., N] de la position pour la synthèse
  uint16_t            checksum;   // +2 = 16 bytes
} __attribute__((packed)) ST_STRUCT_COORD;

typedef struct {
  char              version[4];            // 4 char "x.y.z" + '\0'
  uint32_t          nbr_write;             // 4 bytes [0, ..., UINT_MAX]
  uint8_t           spare_1[8];            // Complément à 8 bytes

  ST_STRUCT_COORD   coord[NBR_COORD_IN_EEPROM];   // NBR_COORD_IN_EEPROM x infos (Idx, Lat, Lon, Ele, Cks)

  uint8_t           spare_2[EEPROM_SIZE - (16 + NBR_COORD_IN_EEPROM * sizeof(ST_STRUCT_COORD) + sizeof(uint16_t))];

  // Total checksum
  uint16_t          checksum;                     // Checksum du contenu de l'eeprom
} __attribute__((packed)) ST_STRUCT_EEPROM;

class GestionEEPROM {
  private:
    const char*   _name;
    uint32_t      _size;      // Size at the creation with the constructor (equal to useful size for the call to 'EEPROMClass::begin' method)

    EEPROMClass   *_eeprom;

    ST_STRUCT_EEPROM    eeprom_image_pre;     // Image précédente (pour comparaison)
    ST_STRUCT_EEPROM    eeprom_image;         // et image courante de l'eeprom 

  public:
    GestionEEPROM(const char* i__name, uint32_t i__ize);
    ~GestionEEPROM(void);

    void initToNaN(float *o__value);

    uint16_t length() { return _eeprom->useful_length(); };
    bool init(const char *i__version);

    bool push(float i__lat, float i__lon, float i__ele, uint8_t i__range = 0);
    bool pop(int *o__idx, float *o__lat, float *o__lon, float *o__ele, uint8_t *o__range, int i__pos = 0);
    bool inc_idx(int i__inc_idx);

    // For test => To remove
    uint8_t readByte(int i__address) { return _eeprom->readByte(i__address); };
    bool    writeByte(int i__address, uint8_t i__value) { return _eeprom->writeByte(i__address, i__value); };
    // End: For test => To remove

    uint16_t checksum(char *i__from, size_t i__len);

    void    toDump(std::ostringstream &o__out);
    void    printValues();
};
#endif
