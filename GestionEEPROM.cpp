/* Gestion de l'EEPROM au dessus de la classe 'EEPROMClass'
 *
 * Organisation de l'EEPROM (--: not used)
   ---------------------------------------
 *  Address    0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F
    00000000  XX YY ZZ NN NN NN NN -- --  -- -- -- -- -- -- --   [XX YY ZZ]: Version x.y.z de l'eeprom ("0.0.0", ..., "9.9.9") [NN NN NN NN]: Nbr d'écritures (commit)
    00000010  I0 Latitude___ Longitude___ Elevation__ -- CK___   [I0]: Index (0x00, ..., 0x7F) Lat, Lon et Ele codés en 'float' (4 bytes) CK: Checksum sur 2 bytes
    00000020  I1 Latitude___ Longitude___ Elevation__ -- CK___   [I1]: Index (0x00, ..., 0x7F) Lat, Lon et Ele codés en 'float' (4 bytes) CK: Checksum sur 2 bytes
    00000030  I2 Latitude___ Longitude___ Elevation__ -- CK___   [I2]: Index (0x00, ..., 0x7F) Lat, Lon et Ele codés en 'float' (4 bytes) CK: Checksum sur 2 bytes
    00000040  I3 Latitude___ Longitude___ Elevation__ -- CK___   [I3]: Index (0x00, ..., 0x7F) Lat, Lon et Ele codés en 'float' (4 bytes) CK: Checksum sur 2 bytes
    00000050  -- -- -- -- -- -- -- --  -- -- -- -- -- -- -- --
  *
    00000070  -- -- -- -- -- -- -- --  -- -- -- -- -- -- CK___   CK: Total checksum
    00000080

    => Les informations sont écrites comme un FIFO dont l'index s'incrémente modul 128
    => La dernière information est celle dont l'index est le plus grand ou 0x00 si la précédente a un index égal à 0x7F
       => Un index égal à 0x80 indique que la position n'est pas attribuée
       
    Contenu au formatage de l'eeprom: (Version: 1.0.0 - Nbr écritures: 1 - Index: 0x80 - Valeurs NaN pour (Lat, Lon et Ele) - CK: 0xFF
    --------------------------------
    Address    0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F   0123456789ABCDEF
    00000000  31 30 30 00 01 00 00 00  ff ff ff ff ff ff ff ff  |100.............|
    00000010  80 ff ff ff ff ff ff ff  ff ff ff ff ff 00 8c f3  |................|
    00000020  81 ff ff ff ff ff ff ff  ff ff ff ff ff 00 8b f3  |................|
    00000030  82 ff ff ff ff ff ff ff  ff ff ff ff ff 00 8a f3  |................|
    00000040  83 ff ff ff ff ff ff ff  ff ff ff ff ff 00 89 f3  |................|
    00000050  84 ff ff ff ff ff ff ff  ff ff ff ff ff 00 88 f3  |................|
    00000060  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
    00000070  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff d5 93  |................|
    00000080

    Infos of EEPROM (128 bytes)
      Version [1.0.0]
      Nbr write [1]
      Positions:
        #0: Idx [0x80] Lat [nan] Lon [nan] Ele [nan] Cks Read [0xf38c] Calc. [0xf38c] (Ok)
        #1: Idx [0x81] Lat [nan] Lon [nan] Ele [nan] Cks Read [0xf38b] Calc. [0xf38b] (Ok)
        #2: Idx [0x82] Lat [nan] Lon [nan] Ele [nan] Cks Read [0xf38a] Calc. [0xf38a] (Ok)
        #3: Idx [0x83] Lat [nan] Lon [nan] Ele [nan] Cks Read [0xf389] Calc. [0xf389] (Ok)
        #4: Idx [0x84] Lat [nan] Lon [nan] Ele [nan] Cks Read [0xf388] Calc. [0xf388] (Ok)
      Total Cks Read [0x93d5] Calc. [0x93d5] (Ok)
 */

#include "GestionEEPROM.h"

extern void hexDump(std::ostringstream &o_out, const char *i__bytes, const size_t i__nbr_bytes);

GestionEEPROM::GestionEEPROM(const char* i__name, uint32_t i__size)
  : _name(i__name)
  , _size(i__size)
  , _eeprom(NULL)
{
  char l__buffer[80];
  sprintf(l__buffer, "GestionEEPROM::GestionEEPROM([%s], %d)\n", _name, _size);
  Serial.print(l__buffer);

  memset(&eeprom_image,     0xff, sizeof(ST_STRUCT_EEPROM));    // Image de l'eeprom
  memset(&eeprom_image_pre, 0xff, sizeof(ST_STRUCT_EEPROM));    // Image de l'eeprom pour une optimisation des écritures

  _eeprom = new EEPROMClass(_name, _size);
  _eeprom->begin(_size);

  sprintf(l__buffer, "\tSize of EEPROM        [%d] bytes\n", _eeprom->length());  
  Serial.print(l__buffer);
  sprintf(l__buffer, "\tUseful size of EEPROM [%d] bytes\n", _eeprom->useful_length());  
  Serial.print(l__buffer);
}

GestionEEPROM::~GestionEEPROM() {
  Serial.println("GestionEEPROM::~GestionEEPROM()");

  if (_eeprom != NULL) {
    delete _eeprom;
  }
}

void GestionEEPROM::initToNaN(float *o__value)
{
  memset(o__value, 0xff, sizeof(float));
}

bool GestionEEPROM::init(const char *i__version)
{
  char l__buffer[80];
  bool l__rtn = false;

  sprintf(l__buffer, "GestionEEPROM::init(%s)...\n", i__version);  
  Serial.print(l__buffer);

  // Size of 'ST_STRUCT_COORD' and 'ST_STRUCT_EEPROM'
  sprintf(l__buffer, "\t-> sizeof(ST_STRUCT_COORD) [%d] bytes\n", sizeof(ST_STRUCT_COORD));
  Serial.print(l__buffer);
  sprintf(l__buffer, "\t-> sizeof(ST_STRUCT_EEPROM)[%d] bytes\n", sizeof(ST_STRUCT_EEPROM));
  Serial.print(l__buffer);

  // Read the all eeprom
  size_t l__size = _eeprom->readBytes(0, (void *)&eeprom_image, EEPROM_SIZE);

  // Dump of eeprom before init.
  {
    sprintf(l__buffer, "Dump before init... (%d bytes)\n", l__size);  
    Serial.print(l__buffer);

    std::ostringstream l__out;
    toDump(l__out);
    Serial.print(l__out.str().c_str());
    Serial.print("\n");  
  }

  // Raz the image
  memset(&eeprom_image, 0xff, sizeof(eeprom_image));

  // Set the version field "x.y.z" -> 'x' + 'y' + 'z' without '.' separator
  char *l__version = strdup(i__version);
  char *l__pattern = strtok(l__version, ".");
  if (l__pattern != NULL) {
    eeprom_image.version[0] = *l__pattern;

    l__pattern = strtok(NULL, ".");
    if (l__pattern != NULL) {
      eeprom_image.version[1] = *l__pattern;

      l__pattern = strtok(NULL, ".");
      if (l__pattern != NULL) {
        eeprom_image.version[2] = *l__pattern;
      }
    }

    eeprom_image.version[3] = '\0';
  }
  free(l__version);
  // End: Set the version field "x.y.z" -> 'x' + 'y' + 'z' without '.' separator

  // Nbr of write
  eeprom_image.nbr_write = 1;
  // End: Nbr of write

  // Replace the float values by a 'NaN' and calcul of checksum from fields 'coord.idx' to 'coord.spare' included
  int n = 0;
  for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
    eeprom_image.coord[n].idx = (0x80 + n);

    initToNaN(&eeprom_image.coord[n].lat);
    initToNaN(&eeprom_image.coord[n].lon);
    initToNaN(&eeprom_image.coord[n].ele);

    eeprom_image.coord[n].range = 0x00;

    eeprom_image.coord[n].checksum = checksum((char *)&eeprom_image.coord[n].idx, (sizeof(ST_STRUCT_COORD) - sizeof(uint16_t)));
  }
  // End: Replace the float values by a 'NaN' and calcul of checksum from fields 'coord.idx' to 'coord.range' included

  // Total checksum
  uint16_t l__total_checksum = checksum((char *)&eeprom_image, (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)));

  sprintf(l__buffer, "\t-> Total checksum [0x%04x]\n", l__total_checksum);  
  Serial.print(l__buffer);

  eeprom_image.checksum = l__total_checksum;
  // End: Total checksum

  // Dump of image
  {
    sprintf(l__buffer, "Dump of image... (%d bytes)\n", sizeof(eeprom_image));  
    Serial.print(l__buffer);
    std::ostringstream l__out;
    hexDump(l__out, (const char *)&eeprom_image, sizeof(eeprom_image));
    Serial.print(l__out.str().c_str());
    Serial.print("\n");
  }

  // Write the all eeprom
  l__rtn = _eeprom->writeBytes(0, (void *)&eeprom_image, l__size);

  if (l__rtn == true) {
    l__rtn = _eeprom->commit();

    if (l__rtn == true) {
      // Dump of eeprom after init.
      sprintf(l__buffer, "Dump after init... (%d bytes)\n", l__size);  
      Serial.print(l__buffer);
      std::ostringstream l__out;
      toDump(l__out);
      Serial.print(l__out.str().c_str());
      Serial.print("\n");
    }
    else {
      Serial.print("Error: init(): commit()\n");
    }
  }
  else {
    Serial.print("Error: init(): writeBytes()\n");
  }

  return l__rtn;
}

void GestionEEPROM::toDump(std::ostringstream &o__out)
{
  char l__buffer[80];

  o__out << "GestionEEPROM::hexDump()...\n";

  size_t l__useful_length = _eeprom->useful_length();

  if (l__useful_length != 0) {
    sprintf(l__buffer, "Content of EEPROM (%d useful bytes)\n", l__useful_length);
    o__out << l__buffer;

    // TODO: Use 'EEPROMClass::readBytes(int address, void* value, size_t maxLen)' method
    uint8_t *l__values = (uint8_t*)malloc(l__useful_length);
    size_t n = 0;
    for (n = 0; n < l__useful_length; n++) {
      *(l__values + n) = _eeprom->readByte((int)n);
    }

    hexDump(o__out, (const char *)l__values, l__useful_length);

    free(l__values);
  }
  else {
    o__out << "No acces to EEPROM\n";
  }
}

void GestionEEPROM::printValues()
{
  char l__buffer[128];

  size_t l__useful_length = length();

  if (l__useful_length != 0 && l__useful_length == sizeof(eeprom_image)) {
    size_t l__size = _eeprom->readBytes(0, (void *)&eeprom_image, l__useful_length);

    sprintf(l__buffer, "Infos of EEPROM (%d bytes)\n", l__size);
    Serial.print(l__buffer);

    sprintf(l__buffer, "\tVersion [%c.%c.%c]\n", eeprom_image.version[0], eeprom_image.version[1], eeprom_image.version[2]);
    Serial.print(l__buffer);

    sprintf(l__buffer, "\tNbr write [%d]\n", eeprom_image.nbr_write);
    Serial.print(l__buffer);

    sprintf(l__buffer, "\tPositions:\n");
    Serial.print(l__buffer);

    int n = 0;
    for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
      sprintf(l__buffer, "\t\t#%d: Idx [0x%02x]", n, eeprom_image.coord[n].idx);
      Serial.print(l__buffer);

      Serial.print(" Lat [");
      Serial.print(eeprom_image.coord[n].lat, 6);
      Serial.print("] Lon [");
      Serial.print(eeprom_image.coord[n].lon, 6);
      Serial.print("] Ele [");
      Serial.print(eeprom_image.coord[n].ele, 1);
      Serial.print("] Range [");
      Serial.print(eeprom_image.coord[n].range);

      uint16_t l__cks = checksum((char *)&eeprom_image.coord[n].idx, (sizeof(ST_STRUCT_COORD) - sizeof(uint16_t)));

      sprintf(l__buffer, "] Cks Read [0x%04x] Calc. [0x%04x] (%s)\n",
        eeprom_image.coord[n].checksum,
        l__cks,
        (eeprom_image.coord[n].checksum == l__cks) ? "Ok" : "Ko");

      Serial.print(l__buffer);
    }

    uint16_t l__total_cks = checksum((char *)&eeprom_image, (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)));

    sprintf(l__buffer, "\tTotal Cks Read [0x%04x] Calc. [0x%04x] (%s)\n",
      eeprom_image.checksum,
      l__total_cks,
      (eeprom_image.checksum == l__total_cks) ? "Ok" : "Ko");

    Serial.print(l__buffer);
  }
  else {
    Serial.print("No acces to EEPROM\n");
  }
}

/*  Checksum sur 2 bytes
 */
uint16_t GestionEEPROM::checksum(char *i__from, size_t i__len)
{
  int16_t l__cks = 0;
  for (size_t n = 0; n < i__len; n++) {
    l__cks += *(i__from + n);
  }

  return -l__cks;
}

/* Mémorisation d'une position à la manière d'une FIFO
 *  - Au 1st emplacement dont l'index est supérieur ou égal à 0x80
 *  - Sinon, à l'emplacement dont l'index est le plus petit dans la plage [0x00; ...0x7f] (le plus vieux)
 *
 *  => Le 'nbr_write' est incémenté et les checksum sont m.a.j.
 */
bool GestionEEPROM::push(float i__lat, float i__lon, float i__ele, uint8_t i__range)
{
  char l__buffer[80];

  Serial.print("GestionEEPROM::push([");
  Serial.print(i__lat, 6);
  Serial.print("], [");
  Serial.print(i__lon, 6);
  Serial.print("], [");
  Serial.print(i__ele, 1);
  Serial.print("], [");
  Serial.print(i__range);
  Serial.print("])\n");

  bool l__rtn = false;
  size_t l__useful_length = length();

  if (l__useful_length != 0 && l__useful_length == sizeof(eeprom_image)) {
    _eeprom->readBytes(0, (void *)&eeprom_image, l__useful_length);

    // Recopie pour une comparaison et optimisation des écritures
    memcpy(&eeprom_image_pre, &eeprom_image, sizeof(ST_STRUCT_EEPROM));

    int n = 0;

    // Recherche de la position à renseigner avec incrémentation de l'index le plus grand dans la plage [0x00, ..., 0x7F]
    // Prise de la valeur du plus grand index (si dans la plage [0x00, ..., 0x7f])
    uint8_t l__idx_max = 0x00;    
    int l__pos_to_write = -1;

    for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
      uint8_t l__idx = eeprom_image.coord[n].idx & 0x80;
      if (l__idx != 0x80 && eeprom_image.coord[n].idx >= l__idx_max) {
        l__idx_max = eeprom_image.coord[n].idx;
        l__pos_to_write = (n + 1) % NBR_COORD_IN_EEPROM;   // Ecriture après cette position de cet index le plus grand
      }
    }

    bool l__flg_inc_idx = true;           // Incrémentation de l'index à priori

    for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
      if ((eeprom_image.coord[n].idx & 0x80) == 0x80) {
        // Position à écrire au 1st index trouvé non attribué ([0x80, ..., 0xff])
        l__pos_to_write = n;    // Remplacement de l'emplacement non attribué

        // Pas d'incrémentation si position #0 (1st push sur une eeprom initialisée ;-)
        if (n == 0) {
          l__flg_inc_idx = false;
        }

        break;
      }
    }

    bool l__flg_write_all_idx = false;

    if (l__pos_to_write >= 0 && l__pos_to_write < NBR_COORD_IN_EEPROM) {
      if (l__flg_inc_idx == true) {
        if (l__idx_max == 0x7f) {
          // Renumérotation des 'NBR_COORD_IN_EEPROM' position pour éviter une rupture de l'incrémentation 0x7f -> 0x00
          l__flg_write_all_idx = true;

          sprintf(l__buffer, "\tRenumbering of index [%d] Idx max [0x%02x] -> new [0x%02x]\n", l__pos_to_write, l__idx_max, (l__idx_max + 1) & 0x7f);
          Serial.print(l__buffer);
        }
        else {
          sprintf(l__buffer, "\tPos. to write [%d] Idx max [0x%02x] -> new [0x%02x]\n", l__pos_to_write, l__idx_max, (l__idx_max + 1) & 0x7f);
          Serial.print(l__buffer);
        }
      }
    }
    else {
      sprintf(l__buffer, "Internal error with pos. write (%d) out of range [0..%d]\n", l__pos_to_write, (NBR_COORD_IN_EEPROM - 1));
      Serial.print(l__buffer);

      return false;
    }

#if !USE_WRITE_OPTIMIZATION
    bool l__flg_write = true;
#endif

    int l__nbr_byte_write = 0;    // Nbr de m.a.j. sans compter les 8 bytes de 'nbr_write' et 'total checksum'

    for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
      if (n == l__pos_to_write) {
        eeprom_image.coord[n].idx = (l__flg_inc_idx == true) ? ((l__idx_max + 1) & 0x7f) : 0x00;
        eeprom_image.coord[n].lat = i__lat;
        eeprom_image.coord[n].lon = i__lon;
        eeprom_image.coord[n].ele = i__ele;

        eeprom_image.coord[n].range = i__range;

        eeprom_image.coord[n].checksum = checksum((char *)&eeprom_image.coord[n].idx, (sizeof(ST_STRUCT_COORD) - sizeof(uint16_t)));

#if !USE_WRITE_OPTIMIZATION
        l__flg_write = (l__flg_write & _eeprom->writeBytes(EEPROM_OFFSET_1ST_COORD + n * sizeof(ST_STRUCT_COORD),
                                                          (void *)&eeprom_image.coord[n], sizeof(ST_STRUCT_COORD)));
        l__nbr_byte_write += sizeof(ST_STRUCT_COORD);
#endif
      }

      if (l__flg_write_all_idx == true) {
        eeprom_image.coord[n].idx = ((eeprom_image.coord[n].idx + (NBR_COORD_IN_EEPROM - 1)) & 0x7f);
        eeprom_image.coord[n].checksum = checksum((char *)&eeprom_image.coord[n].idx, (sizeof(ST_STRUCT_COORD) - sizeof(uint16_t)));

#if !USE_WRITE_OPTIMIZATION
        l__flg_write = (l__flg_write & _eeprom->writeBytes(EEPROM_OFFSET_1ST_COORD + n * sizeof(ST_STRUCT_COORD),
                                                          (void *)&eeprom_image.coord[n], sizeof(ST_STRUCT_COORD)));
        l__nbr_byte_write += sizeof(ST_STRUCT_COORD);
#endif
      }
    }

    if (l__nbr_byte_write > 0) {
      eeprom_image.nbr_write++;    // Incrémentation nbr d'écritures et recalcul de la checksum totale
    }

#if !USE_WRITE_OPTIMIZATION
    eeprom_image.checksum = checksum((char *)&eeprom_image, (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)));

    l__flg_write = (l__flg_write & _eeprom->writeUInt(EEPROM_OFFSET_NBR_WRITE, eeprom_image.nbr_write));
    l__flg_write = (l__flg_write & _eeprom->writeUShort(EEPROM_OFFSET_CHECKSUM, eeprom_image.checksum));
#endif

#if USE_WRITE_OPTIMIZATION
    // Parcours de chaque 'byte' pour son écriture
    uint8_t *l__image_pre = (uint8_t *)&eeprom_image_pre;
    uint8_t *l__image_new = (uint8_t *)&eeprom_image;
    int l__address = 0;
    bool l__flg_write = true;

    for (l__address = 0; l__address < (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)); l__address++) {
      if (*(l__image_new + l__address) != *(l__image_pre + l__address)) {
        l__flg_write = (l__flg_write & _eeprom->writeByte(l__address, *(l__image_new + l__address)));

        l__nbr_byte_write++;

#if 0
        sprintf(l__buffer, "\tAddress [0x%x] Value [0x%02x] -> [0x%02x] (write: %d)\n",
          l__address, *(l__image_pre + l__address), *(l__image_new + l__address), l__nbr_byte_write);

        Serial.print(l__buffer);
#endif
      }
    }

    sprintf(l__buffer, "\tNbr byte write [%d/%d]\n", l__nbr_byte_write, sizeof(ST_STRUCT_EEPROM));
    Serial.print(l__buffer);
#endif

    // Commit des informations si écritures Ok et au moins un byte nouvellement m.a.j.
    if (l__flg_write == true && l__nbr_byte_write > 0) {
      // Incrémentation du nbr d'écritures de l'eeprom
      eeprom_image.nbr_write++;    // Incrémentation nbr d'écritures
      l__flg_write = (l__flg_write & _eeprom->writeUInt(EEPROM_OFFSET_NBR_WRITE, eeprom_image.nbr_write));

      eeprom_image.checksum = checksum((char *)&eeprom_image, (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)));
      l__flg_write = (l__flg_write & _eeprom->writeUShort(EEPROM_OFFSET_CHECKSUM, eeprom_image.checksum));

      if (l__flg_write == true) {
        l__flg_write = _eeprom->commit();
      }
      else {
        Serial.print("Warning: No commit because error in write (#2)\n");
      }
    }
    else {
      Serial.print("Warning: No commit because error in write (#1)\n");
    }

    l__rtn = l__flg_write;
  }
  else {
    Serial.print("No acces to EEPROM\n");
  }

  return l__rtn;
}

/* Prise des coordonnées @ 'l__pos'
 *  - l__pos:  0 => Index le plus grand dans la plage [0x00; ...0x7f]
 *  - l__pos: -1 => Précédentes coordonnées @ l'index le plus grand
 *  - l__pos: -2 => Précédentes coordonnées @ l'index correspond à 'l__pos' = -1
 *  - l__pos: -3 => Précédentes coordonnées @ l'index correspond à 'l__pos' = -2
 *    ...
 *  - l__pos: -n => Précédentes coordonnées @ l'index correspond à 'l__pos' = -(n-1)
 *
 *  => Les checksum des coordonnées ET de l'eeprom sont vérifiées
 */
bool GestionEEPROM::pop(int *o__idx, float *o__lat, float *o__lon, float *o__ele, uint8_t *o__range, int i__pos)
{
  char l__buffer[80];
  bool l__rtn = false;

  sprintf(l__buffer, "GestionEEPROM::pop([0x%x])\n", i__pos);
  Serial.print(l__buffer);

  if (i__pos > 0 || i__pos <= -NBR_COORD_IN_EEPROM) {
    sprintf(l__buffer, "\tInvalid Pos [%d > 0 || %d <= %d]\n", i__pos, i__pos, -NBR_COORD_IN_EEPROM);
    Serial.print(l__buffer);

    return l__rtn;
  }

  size_t l__useful_length = length();

  if (l__useful_length != 0 && l__useful_length == sizeof(eeprom_image)) {
    _eeprom->readBytes(0, (void *)&eeprom_image, l__useful_length);

    // Test de la checksum totale
    uint16_t l__cks_total = checksum((char *)&eeprom_image, (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)));
    if (eeprom_image.checksum != l__cks_total) {
      sprintf(l__buffer, "\tErr: Total Cks Read [0x%04x] != Calc. [0x%04x]\n", eeprom_image.checksum, l__cks_total);
      Serial.print(l__buffer);

      return l__rtn;
    }

    // Prise des valeurs du plus grand index dans la plage [0x00, ..., 0x7f]
    uint8_t l__idx_max = 0x00;
    int l__pos_to_read = -1;
    int n = 0;

    for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
      uint8_t l__idx = eeprom_image.coord[n].idx & 0x80;
      if (l__idx != 0x80 && eeprom_image.coord[n].idx >= l__idx_max) {   // eeprom_image.coord[n].idx dans la plage [0x00..0x7f]
        l__pos_to_read = n;                                              // Position suivante
        l__idx_max = eeprom_image.coord[n].idx;
      }
    }

    if (l__pos_to_read >= 0) {
      // Application de 'i__pos' (invariant si 'i__pos' = 0 ;-)
      l__pos_to_read = (l__pos_to_read + (NBR_COORD_IN_EEPROM + i__pos)) % NBR_COORD_IN_EEPROM;

      sprintf(l__buffer, "\tPos to read #%d -> #%d (Idx max [0x%02x])\n", i__pos, l__pos_to_read, l__idx_max);
      Serial.print(l__buffer);

      // Test de la checksum des coordonnées
      uint16_t l__cks = checksum((char *)&eeprom_image.coord[l__pos_to_read].idx, (sizeof(ST_STRUCT_COORD) - sizeof(uint16_t)));
      if (eeprom_image.coord[l__pos_to_read].checksum != l__cks) {
        sprintf(l__buffer, "\tErr: Cks Read [0x%04x] != Calc. [0x%04x]\n", eeprom_image.coord[l__pos_to_read].checksum, l__cks);
        Serial.print(l__buffer);

        return l__rtn;
      }

      if (o__idx != NULL) {
        *o__idx = eeprom_image.coord[l__pos_to_read].idx;

        if (o__lat != NULL) {
          *o__lat = eeprom_image.coord[l__pos_to_read].lat;

          if (o__lon != NULL) {
            *o__lon = eeprom_image.coord[l__pos_to_read].lon;

            if (o__ele != NULL) {
              *o__ele = eeprom_image.coord[l__pos_to_read].ele;

              if (o__range != NULL) {
                *o__range = eeprom_image.coord[l__pos_to_read].range;

                // Test si 'idx' valide et valeurs != NaN
                if (((*o__idx & 0x80) != 0x80) && (*o__lat == *o__lat) && (*o__lon == *o__lon) && (*o__ele == *o__ele)) {
                  l__rtn = true;
                }
              }
            }
          }
        }
      }
    }
    else {
      Serial.print("\tNo Pos to read\n");
    }
  }
  else {
    Serial.print("No acces to EEPROM\n");
  }

  return l__rtn;
}

bool GestionEEPROM::inc_idx(int i__inc_idx)
{
  char l__buffer[80];

  sprintf(l__buffer, "GestionEEPROM::inc_idx([0%02x])\n", i__inc_idx);
  Serial.print(l__buffer);

  bool l__rtn = false;
  size_t l__useful_length = length();
  bool l__flg_write = true;

  if (l__useful_length != 0 && l__useful_length == sizeof(eeprom_image)) {
    _eeprom->readBytes(0, (void *)&eeprom_image, l__useful_length);

    int n = 0;
    for (n = 0; n < NBR_COORD_IN_EEPROM; n++) {
      sprintf(l__buffer, "\t#%d: Idx [0x%02x] -> [0x%02x]\n", n, eeprom_image.coord[n].idx, (eeprom_image.coord[n].idx + i__inc_idx) & 0x7f);
      Serial.print(l__buffer);

      eeprom_image.coord[n].idx = (eeprom_image.coord[n].idx + i__inc_idx) & 0x7f;
      eeprom_image.coord[n].checksum = checksum((char *)&eeprom_image.coord[n].idx, (sizeof(ST_STRUCT_COORD) - sizeof(uint16_t)));

      l__flg_write = (l__flg_write & _eeprom->writeBytes(EEPROM_OFFSET_1ST_COORD + n * sizeof(ST_STRUCT_COORD),
                                                        (void *)&eeprom_image.coord[n], sizeof(ST_STRUCT_COORD)));
    }

    eeprom_image.nbr_write++;
    l__flg_write = (l__flg_write & _eeprom->writeUInt(EEPROM_OFFSET_NBR_WRITE, eeprom_image.nbr_write));

    eeprom_image.checksum = checksum((char *)&eeprom_image, (sizeof(ST_STRUCT_EEPROM) - sizeof(uint16_t)));
    l__flg_write = (l__flg_write & _eeprom->writeUShort(EEPROM_OFFSET_CHECKSUM, eeprom_image.checksum));

    if (l__flg_write == true) {
      l__flg_write = _eeprom->commit();
    }

    l__rtn = l__flg_write;
  }
  else {
    Serial.print("No acces to EEPROM\n");
  }

  return l__rtn;
}
