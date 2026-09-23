// $Id: SerialNMEA.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifdef USE_SIMULATION
#include <stdio.h>
#include <string.h>

// Include files for simulation
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#else
#include <Arduino.h>
#include <HardwareSerial.h>
#endif

#include "SerialNMEA.h"
#include "Misc.h"
#include "Statistics.h"
#include "Pilot.h"

#include "SDCard.h"

#include "Timers.h"

#ifndef USE_SIMULATION
HardwareSerial              g__serialNMEA(1);
#endif

#if USE_FORCE_NO_GPS
SerialNMEA::SerialNMEA(int i__speed_uart) : flg_force_no_gps(false), idx_write(0), idx_read(0), frame_nmea_available(false), frame_tlv_available(false), frame_nmea_str(""), frame_tlv_str("")
#else
SerialNMEA::SerialNMEA(int i__speed_uart) : idx_write(0), idx_read(0), frame_nmea_available(false), frame_tlv_available(false), frame_nmea_str(""), frame_tlv_str("")
#endif
{
  Serial.printf("SerialNMEA::SerialNMEA(%d)\n", i__speed_uart);

#ifndef USE_SIMULATION
  pinMode(RXD1, INPUT_PULLUP);
  g__serialNMEA.begin(i__speed_uart, SERIAL_8N1, RXD1, TXD1);
#endif

  memset(fifo, '\0', sizeof(fifo));
  memset(&gps_nmea_infos, '\0', sizeof(ST_GPS_INFOS));
  memset(&gps_tlv_infos, '\0', sizeof(ST_GPS_INFOS));

  gps_reception_state = GPS_NO_RECEPTION;

#if USE_STATS_SERIAL_NMEA
  nbr_frames_nmea = 0;
  nbr_frames = 0;
  err_fifo_0 = 0;
  err_fifo_1 = 0;
  spare_min = INT_MAX;
  err_cks = 0;
  err_nmea = 0;
  err_missing_field = 0;
#endif
}

SerialNMEA::~SerialNMEA()
{
  Serial.println("SerialNMEA::~SerialNMEA()");
}

#if USE_STATS_SERIAL_NMEA
unsigned int SerialNMEA::getErrFifo(int i__num_err) const
{
  switch(i__num_err) {
  case 0: return err_fifo_0;
  case 1: return err_fifo_1;
  default: return (unsigned int)-1;
  }
}
#endif

/*  Mise à jour de la FIFO de réception sous interruption
 */
void SerialNMEA::update()
{
#ifndef USE_SIMULATION
#if USE_STATS_SERIAL_NMEA
  if (frame_nmea_available == true) {
    // La trame précédente n'a pas été extraite ;-(
    err_fifo_0++;
  }
#endif
  
  if (g__serialNMEA.available() > 0) {
    char l__char = char(g__serialNMEA.read());

    /*  Conservation des seuls caractères "imprimables"
     *  => Warning: Les trames comprises entre '1>Begin' et '1>End' (informations lues de la clé USB de l'Arduino)
     *              => Trames qui peuvent être terminées par "\r\n" dans cet ordre et qui participent au calcul
     *                 de la checksum 'cksum' UNIX
     *                 => '\n' est le dernier caractère qui termine une trame
     */
    if ((l__char >= 0x20 && l__char <= 0x7F) || l__char == '\r' || l__char == '\n') {
      fifo[idx_write] = l__char;

      idx_write = (idx_write + 1) % NBR_OF_CHAR_IN_FIFO;

#if USE_STATS_SERIAL_NMEA
      // Test de l'état de la FIFO circulaire: Si 'idx_write' atteint 'idx_read' => Saturation FIFO
      int l__diff = (idx_write - idx_read);

      if (l__diff == 0) {
        err_fifo_1++;

        spare_min = INT_MAX;    // Reinit du minimum
      }
      else if (l__diff > 0) {
        int l__spare = (NBR_OF_CHAR_IN_FIFO - l__diff);
        spare_min = l__spare < spare_min ? l__spare : spare_min;
      }
      else {
        int l__spare = -l__diff;
        spare_min = l__spare < spare_min ? l__spare : spare_min;
      }
      // End: Test de l'état de la FIFO circulaire
#endif
    }

    // Trame NMEA disponible ?
    if (l__char == '\n') {
#if USE_STATS_SERIAL_NMEA
      nbr_frames_nmea++;
#endif

      frame_nmea_available = true;
    }
  }
#endif
}

boolean SerialNMEA::isFrameNMEAAvailable()
{
#ifdef USE_SIMULATION
  if (frame_nmea_available == true) {
    Serial.println("SerialNMEA::isFrameNMEAAvailable(): true");
  }

  return frame_nmea_available;
#else
  // TBC: Suppress 'noInterrupts()' and 'interrupts()' (size(boolean) = 1)
  noInterrupts();
  boolean l__frame_nmea_available = frame_nmea_available;
  interrupts();

  // Ignore si trame non disponible
  if (l__frame_nmea_available == false) {
    return l__frame_nmea_available;
  }

  // Read of the FIFO
  noInterrupts();
  int l__idx_write = idx_write;     // Atomic read
  interrupts();

  frame_nmea_str = "";

  while (idx_read != (unsigned int)l__idx_write) {
    char l__value = fifo[idx_read];
    idx_read = (idx_read + 1) % NBR_OF_CHAR_IN_FIFO;

    // Non recopie du '\r' et du '\n'
    if (l__value != '\r' && l__value != '\n') {
      frame_nmea_str += l__value;
    }

    /*  Break if '\n' found (more datas is possibly)
     *  => Added for acquisition of USB frames ;-)
     */
    if (l__value == '\n') {
      break;
    }
  }
  // End: Read of the FIFO

  frame_nmea_available = false;

  // Return available state
  return l__frame_nmea_available;
#endif
}

boolean SerialNMEA::isFrameTLVAvailable()
{
#ifdef USE_SIMULATION
  if (frame_tlv_available == true) {
    Serial.println("SerialNMEA::isFrameAvailable(): true");
  }

  return frame_tlv_available;
#else
  boolean l__frame_available = frame_tlv_available;

  // Ignore si trame non disponible
  if (l__frame_available == false) {
    return l__frame_available;
  }

  // TLV frame already available in 'frame_tlv_str'
  frame_tlv_available = false;

  // Return available state
  return l__frame_available;
#endif
}

#ifndef USE_SIMULATION
String & SerialNMEA::getFrameNMEA()
#else
std::string & SerialNMEA::getFrameNMEA()
#endif
{
  return frame_nmea_str;
}

#ifndef USE_SIMULATION
String & SerialNMEA::getFrameTLV()
#else
std::string & SerialNMEA::getFrameTLV()
#endif
{
  return frame_tlv_str;
}

/*  Calcul de la checksum des trames NMEA: value ^= pre_value
 *     - Si 'o__cks_expected' == NULL, calcul sur tous les carateres
 *     - Sinon, calcul du 2nd caractère a celui qui precede '*' (trame GPS)
 */
boolean SerialNMEA::calculChecksumNMEA(char *i__frame, unsigned char *o__cks_calculated, unsigned char *o__cks_expected) {
  boolean l__cks_end = false;
  size_t l__size = strlen(i__frame);

  // If 'o__cks_expected != NULL'; Start calcul after '$' character
  // Else; Start calcul with all characters
  size_t n = (o__cks_expected != NULL) ? 1 : 0;

  for (; n < l__size; n++) {
    char l__byte = *(i__frame + n);
    if (l__byte == '*') {
      l__cks_end = true;
    }
    else if (l__cks_end) {
      // Get and convert in hexa the 2 last characters after '*' character
      if (n == (l__size - 2) && o__cks_expected != NULL) {
        *o__cks_expected = (unsigned char)strtol(&i__frame[n], NULL, 16);
      }
    }
    else {
      // Calculate the checksum between {'$', ..., '*'}
      *o__cks_calculated ^= (unsigned char)l__byte;
    }
  }

  if (o__cks_expected != NULL) {
    if (*o__cks_calculated != *o__cks_expected) {
#if USE_STATS_SERIAL_NMEA
      err_cks++;
#endif
      return false;
    }
    else {
      return true;
    }
  }
  else {
    return true;
  }
}

/*  Calcul et comparaison de la checksum attendue des trames TLV
 *       ex: [AA150225.000B1AC94847.0719D1NEA00157.5194F1EG40.00s40.00H5274.2I6110720K5132.1L1M*301F]
 *            |_______________________________________________________________________________|                                                                               ^
 */
boolean SerialNMEA::calculChecksum(char *i__frame, unsigned char *o__cks_calculated)
{
  *o__cks_calculated = 0;
  size_t n = 0;
  for (; n < strlen(i__frame); n++) {
    char l__byte = *(i__frame + n);

    // Stop if '*' reached ("*30XX")
    if (l__byte == '*') {
       break;
    }
    *o__cks_calculated ^= (unsigned char)l__byte;
  }

  // Calcul of 'XX'
  unsigned char l__checksum_read = (unsigned char)strtol(i__frame + n + 3, NULL, 16);

#if USE_STATS_SERIAL_NMEA
  if (*o__cks_calculated != l__checksum_read) {
    err_cks++;
  }
#endif

  return (*o__cks_calculated == l__checksum_read) ? true : false;
}

#ifndef USE_SIMULATION
boolean SerialNMEA::extractGpsTLVInfos(String &o__message, boolean i__simu_move_flg)
#else
boolean SerialNMEA::extractGpsTLVInfos(std::string &o__message, boolean i__simu_move_flg, char *i__date, char *i__time)
#endif
{
#ifndef USE_SIMULATION
  char l__buffer[80];
  bool l__rtn = true;
  char *l__frame = (char *)frame_tlv_str.c_str();
#endif

  // Prepare update the current coordinates
  COORD l__coord_current;
  g__pilot->initCoordToNoMean(&l__coord_current);

#ifdef USE_SIMULATION
  bool l__rtn = true;
  Serial.println("Force GPS infos...");
#else
  // A priori, infos GPS invalides
  gps_tlv_infos.flg_valid = false;

  /* Extraction from NMEA format as:
     [AA110201.000B1AC94847.0711D1NEA00157.5218F1EG40.00s40.00H5355.8I6110320K5124.3L1M*301B]
      ^           ^  ^          ^              ^        ^     ^      ^       ^      ^
  */
  size_t l__idx = 0;
  char l__TLV_T = 0;        // First field
  char l__t_TLV_V[32+1];    // Field NMEA extracted
  int l__TLV_L = -1;        // Length of TLV extracted

  //int l__num_field = 0;
  bool l__flg_break = false;
  while (l__flg_break == false && l__idx < frame_tlv_str.length()) {
    char l__length[1+1];
    l__TLV_T = *(l__frame + l__idx);

    l__length[0] = *(l__frame + l__idx + 1);
    l__length[1] = '\0';

    l__TLV_L = (int)strtol(l__length, NULL, 16);
    if (l__TLV_L <= 0 || l__TLV_L >= (int)sizeof(l__t_TLV_V)) {
      o__message  = "Error: NMEA extraction: Invalid length in [";
      o__message += (l__frame + l__idx);
      o__message += "]";
      return false;
    }

    memset(l__t_TLV_V, '\0', sizeof(l__t_TLV_V));
    strncpy(l__t_TLV_V, l__frame + l__idx + 2, l__TLV_L);

    sprintf(l__buffer, "\tT:[%c] L:[%2d] V:[%s]", l__TLV_T, l__TLV_L, l__t_TLV_V);
    Serial.print(l__buffer);

    switch (l__TLV_T) {
    case 'A':  // Time: "hhmmss\0"
      if (l__TLV_L == 10) {    // 10 because "HHMMSS.000"
        memset(gps_tlv_infos.time, '\0', sizeof(gps_tlv_infos.time));
        strncpy(gps_tlv_infos.time, l__t_TLV_V, 6);
        gps_tlv_infos.flg_infos |= INFO_GPS_TIME;

        // Verification des secondes [0, 5, 10, ..., 55]
        if (gps_tlv_infos.time[5] == '0' || gps_tlv_infos.time[5] == '5') {
          gps_tlv_infos.flg_infos |= INFO_5_SEC_ELAPSED;
        }
      }
      else {
        goto error_length;
      }
      break;

    case 'B':  // Status
      if (*l__t_TLV_V == 'V' || *l__t_TLV_V == 'A') {
        gps_tlv_infos.status = *l__t_TLV_V;
        gps_tlv_infos.flg_infos |= INFO_GPS_STATUS;
      }
      else {
        goto error_status;
      }
      break;

    case 'C':  // Latitude
      if (l__TLV_L == (sizeof(gps_tlv_infos.t_latitude) - 1)) {
        memset(gps_tlv_infos.t_latitude, '\0', sizeof(gps_tlv_infos.t_latitude));
        strncpy(gps_tlv_infos.t_latitude, l__t_TLV_V, l__TLV_L);

        // Convert to degrees decimaux
        gps_tlv_infos.latitude = convertToDecimalDegres(gps_tlv_infos.t_latitude);

        gps_tlv_infos.flg_infos |= INFO_GPS_LAT;
      }
      else {
        goto error_length;
      }
      break;

    case 'D':  // Hemisphere North/South
      if (*l__t_TLV_V == 'N') {
        gps_tlv_infos.north_south = GPS_NORTH;
        gps_tlv_infos.flg_infos |= INFO_GPS_LAT_DIR;
      }
      else if (*l__t_TLV_V == 'S') {
        gps_tlv_infos.north_south = GPS_SOUTH;
        gps_tlv_infos.flg_infos |= INFO_GPS_LAT_DIR;

        // Change the sign
        gps_tlv_infos.latitude = -gps_tlv_infos.latitude;
      }
      else {
        goto error_lat;
      }

      l__coord_current.lat = gps_tlv_infos.latitude;

      Serial.print("\t\tLat.  [");
      Serial.print(gps_tlv_infos.latitude, 6);
      Serial.print("] degrees");
      break;

    case 'E':  // Longitude
      if (l__TLV_L == (sizeof(gps_tlv_infos.t_longitude) - 1)) {
        memset(gps_tlv_infos.t_longitude, '\0', sizeof(gps_tlv_infos.t_longitude));
        strncpy(gps_tlv_infos.t_longitude, l__t_TLV_V, l__TLV_L);

        // Convert to degrees decimaux
        gps_tlv_infos.longitude = convertToDecimalDegres(gps_tlv_infos.t_longitude);

        gps_tlv_infos.flg_infos |= INFO_GPS_LON;
      }
      else {
        goto error_length;
      }
      break;

    case 'F':
      if (*l__t_TLV_V == 'E') {
        gps_tlv_infos.east_west = GPS_EAST;
        gps_tlv_infos.flg_infos |= INFO_GPS_LON_DIR;
      }
      else if (*l__t_TLV_V == 'W') {
        gps_tlv_infos.east_west = GPS_WEST;
        gps_tlv_infos.flg_infos |= INFO_GPS_LON_DIR;

        // Change the sign
        gps_tlv_infos.longitude = -gps_tlv_infos.longitude;
      }
      else {
        goto error_lon;
      }

      l__coord_current.lon = gps_tlv_infos.longitude;

      Serial.print("\t\tLon.  [");
      Serial.print(gps_tlv_infos.longitude, 6);
      Serial.print("] degrees");
      break;

    case 'G':  // Speed in Knots
      gps_tlv_infos.speedKnots = (float)strtod(l__t_TLV_V, NULL);
      gps_tlv_infos.flg_infos |= INFO_GPS_SPEED_KNOTS;

      Serial.print("\t\tSpeed [");
      Serial.print(gps_tlv_infos.speedKnots, 1);
      Serial.print("] Knots");
      break;

    case 'H':  // Cap
      gps_tlv_infos.cap = (float)strtod(l__t_TLV_V, NULL);
      gps_tlv_infos.flg_infos |= INFO_GPS_CAP;

      l__coord_current.cap = gps_tlv_infos.cap;

      Serial.print("\t\tCap   [");
      Serial.print(gps_tlv_infos.cap, 1);
      Serial.print("] degrees");
      break;

    case 'I':  // Date: "mmddyy\0"
      if (l__TLV_L == 6) {
        memset(gps_tlv_infos.date, '\0', sizeof(gps_tlv_infos.date));
        strncpy(gps_tlv_infos.date, l__t_TLV_V, l__TLV_L);
        gps_tlv_infos.flg_infos |= INFO_GPS_DATE;
      }
      else {
        goto error_length;
      }
      break;

    case 'K':  // Elevation
      gps_tlv_infos.elevation = (float)strtod(l__t_TLV_V, NULL);
      gps_tlv_infos.flg_infos |= INFO_GPS_ELEVATION;
      break;

    case 'L':  // Unit of elevation (Meter)
      if (*l__t_TLV_V != 'M') {
        goto error_meter;
      }

      l__coord_current.ele = gps_tlv_infos.elevation;

      Serial.print("\t\tEle.  [");
      Serial.print(gps_tlv_infos.elevation, 0);
      Serial.print("] M");
      break;

    case 's':  // Speed in KmH
      gps_tlv_infos.speedKmH = (float)strtod(l__t_TLV_V, NULL);
      gps_tlv_infos.flg_infos |= INFO_GPS_SPEED_KMH;

      l__coord_current.speedKmH = gps_tlv_infos.speedKmH;

      Serial.print("\t\tSpeed [");
      Serial.print(gps_tlv_infos.speedKmH, 1);
      Serial.print("] KmH");
      break;

    case 'a':  // Nbr of satellites
      gps_tlv_infos.nbr_satellites = (int)strtol(l__t_TLV_V, NULL, 10);
      gps_tlv_infos.flg_infos |= INFO_GPS_NBR_SATELLITES;

      Serial.print("\t\t");
      Serial.print(gps_tlv_infos.nbr_satellites);
      Serial.print(" satellites");
      break;

    case '*':
      l__flg_break = true;
      break;

    default:
      goto default_more;

error_length:
      o__message = "Error: NMEA extraction: Invalid length";
      goto rtn_false;

error_status:
      o__message = "Error: NMEA extraction: Invalid status";
      goto rtn_false;

error_lat:
      o__message = "Error: NMEA extraction: Invalid latitude";
      goto rtn_false;

error_lon:
      o__message = "Error: NMEA extraction: Invalid longitude";
      goto rtn_false;

error_meter:
      o__message = "Error: NMEA extraction: Invalid meter unit";
      goto rtn_false;

default_more:
      o__message  = "Error: NMEA extraction: Type [";
      o__message += l__TLV_T;
      o__message += "]";

rtn_false:
#if USE_STATS_SERIAL_NMEA
      err_nmea++;
#endif
      return false;
    }

    // Print '\n'
    Serial.print("\n");

    l__idx += 1 + 1 + l__TLV_L;  // Next field 
  }

  if (gps_tlv_infos.flg_infos != INFO_GPS_ALL) {
#if USE_STATS_SERIAL_NMEA
    err_missing_field++;
#endif
    o__message = "Error: NMEA extraction: Missing field in the frame";
    return false;
  }
#endif

  // Update the current coordinates
  if (i__simu_move_flg == true) {
    // Prise d'une nouvelle position simulée en remplacement de la position GPS réelle
    getSimuMovePosition(&l__coord_current);
  }

  g__pilot->initCoordToCurrent(&l__coord_current);

#ifdef USE_SIMULATION
  if (i__date != NULL) {
    strncpy(gps_tlv_infos.date, i__date, sizeof(gps_tlv_infos.date));
    gps_infos.flg_infos |= INFO_GPS_DATE;
  }
  if (i__time != NULL) {
    strncpy(gps_tlv_infos.time, i__time, sizeof(gps_tlv_infos.time));
    gps_tlv_infos.flg_infos |= INFO_GPS_TIME;
  }
#endif

  if ((gps_tlv_infos.flg_infos &INFO_GPS_DATE) && (gps_tlv_infos.flg_infos & INFO_GPS_TIME)) {

    // Calcul de la date GPS as UNIX
    long l__dateAndTimeGPS = buildGpsDateTime(gps_tlv_infos.date, gps_tlv_infos.time, gps_tlv_infos.dateAndTimeUNIX_GMT.t_date_time, &gps_tlv_infos.dateAndTime);

#if 0
    char l__buffer[80];
    //sprintf(l__buffer, "Time GPS as UNIX [%lu] compare to [%lu]\n", l__dateAndTimeGPS, gps_tlv_infos.dateAndTimeGPS, &gps_tlv_infos.dateAndTime);
    sprintf(l__buffer, "Time GPS as UNIX [%lu] compare to [%lu]\n", l__dateAndTimeGPS, gps_tlv_infos.dateAndTimeGPS);
    Serial.print(l__buffer);
#endif

    // Test of Date/Time progression
    gps_tlv_infos.delta_time = (l__dateAndTimeGPS - gps_tlv_infos.dateAndTimeGPS);
    if (gps_tlv_infos.delta_time <= 0) {
      // Fix Bug-20201102: Change to error
      o__message = "Error: Invalid Date/Time progression";

      // Update statistics
      g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_ERR_TIME_PROGRESSION);
      g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES);
      // End: Fix Bug-20201102: Change to error

      return false;
    }
    else if (gps_tlv_infos.delta_time > 5) {
      o__message = "Warning: Time gap too large";

      // Update statistics (no error)
      g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_WARN_TIME_TOO_LARGE);
    }

    // Update in the all warning cases -> 'gps_tlv_infos.flg_valid' to true
    gps_tlv_infos.dateAndTimeGPS = l__dateAndTimeGPS;
    gps_tlv_infos.dateAndTimeUNIX_GMT.epoch = l__dateAndTimeGPS;

    gps_tlv_infos.flg_valid = true;
 }

  return l__rtn;
}

long SerialNMEA::getUnixTimeGMT(char *o__t_date_time, ST_DATE_AND_TIME *o__dateAndTime)
{
  strcpy(o__t_date_time, gps_tlv_infos.dateAndTimeUNIX_GMT.t_date_time);
  memcpy(o__dateAndTime, &gps_tlv_infos.dateAndTime, sizeof(ST_DATE_AND_TIME));

  return gps_tlv_infos.dateAndTimeUNIX_GMT.epoch;
}

/* Détermination des multiples des heures
 */
boolean SerialNMEA::isMulipleOfHours()
{
  static int g__cpt_hours_resolved = 0;

  if (gps_tlv_infos.dateAndTime.minutes == 0) {
    return ++g__cpt_hours_resolved == 1 ? true : false;    // Evite le bégaiement
  }
  else {
    // Minute pleine terminée (permet de "survivre" à un pb de réception fugitif)
    g__cpt_hours_resolved = 0;
  }

  return false;
}

/*  Détermination des multiples de 10 mn sauf les minutes à zéro
 *  qui correspondent aux passages aux heures déterminés par 'isMulipleOfHours()'
 */
boolean SerialNMEA::isMulipleOf10Minutes()
{
  static int g__cpt_10mn_resolved = 0;

  if (gps_tlv_infos.dateAndTime.minutes != 0 && (gps_tlv_infos.dateAndTime.minutes % 10) == 0) {
    return ++g__cpt_10mn_resolved == 1 ? true : false;    // Evite le bégaiement
  }
  else {
    // Minute pleine terminée (permet de "survivre" à un pb de réception fugitif)
    g__cpt_10mn_resolved = 0;
  }

  return false;
}

/*  Détermination des multiples de 5 mn sauf les minutes à zéro
 *  qui correspondent aux passages aux heures déterminés par 'isMulipleOfHours()'
 */
boolean SerialNMEA::isMulipleOf5Minutes()
{
  static int g__cpt_5mn_resolved = 0;

  if (gps_tlv_infos.dateAndTime.minutes != 0 && (gps_tlv_infos.dateAndTime.minutes % 5) == 0) {
    return ++g__cpt_5mn_resolved == 1 ? true : false;    // Evite le bégaiement
  }
  else {
    // Minute pleine terminée (permet de "survivre" à un pb de réception fugitif)
    g__cpt_5mn_resolved = 0;
  }

  return false;
}

/*  Détermination du changement de minutes
 */
boolean SerialNMEA::isNewMinute()
{
  static char g__minute_previous = (char)-1;
  static char g__minute_current  = (char)-1;
  boolean l__flg_rtn = false;

  g__minute_current = gps_tlv_infos.dateAndTime.minutes;

  if (g__minute_previous != (char)-1 && g__minute_current != g__minute_previous) {
    l__flg_rtn = true;
  }

  g__minute_previous = g__minute_current;

  return l__flg_rtn;
}

#if USE_STATS_SERIAL_NMEA
void SerialNMEA::hexDumpFifoRx()
{
  char l__buffer[80];
  std::ostringstream l__out;

  sprintf(l__buffer, "FIFO Rx Write/Read #0x%02X/#0x%02X\n", idx_write, idx_read);
  Serial.print(l__buffer);

  hexDump(l__out, fifo, NBR_OF_CHAR_IN_FIFO);
  Serial.print(l__out.str().c_str());
  Serial.print("\n");
}
#endif

// Extraction des informations des trames NMEA et construction de la trame TLV
void SerialNMEA::extractInfosNMEA(const char *i__frame)
{
  // All the frame must be equal to "$...."
  if (*i__frame == '$') {
    // Filter on "GPRMC", "GPGGA", "GPGSV" and "GPTXT"
    if (!strncmp(i__frame + 1, "GPRMC", 5)) {
      /* Forcage effacement pour extraire dans la meme seconde
         les infos complementaires de 'GPGGA' et 'GPGSV' qui suivent 'GPRMC'
      */
      gps_nmea_infos.flg_infos = 0;

      extractInfosGPRMC(i__frame);
    }
    else if (!strncmp(i__frame + 1, "GPGGA", 5)) {
      extractInfosGPGGA(i__frame);
    }
    else if (!strncmp(i__frame + 1, "GPGSV", 5)) {
      extractInfosGPGSV(i__frame);
    }
    else if (!strncmp(i__frame + 1, "GPTXT", 5)) {
      extractInfosGPTXT(i__frame);
    }

    /* Acceptation de l'absence du nombre de satellites
     * => Dans le cas de l'utilisation du module GPS EM406A, bien que le nombre de satellites soient
     *    correctement extraits, 'gps_nmea_infos.flg_infos' ne l'indique jamais ?!.
     *    => TBC: Sequencement des trames 'GPGSV' @ au module GPS L80 ;-)
     *            => Les xxx trames 'GPGSV' sont recues toutes les 5 secondes avec le module EM406A
     */
    if (gps_nmea_infos.flg_infos == INFO_GPS_ALL
     || gps_nmea_infos.flg_infos == (INFO_GPS_ALL & ~INFO_GPS_NBR_SATELLITES))
      {
      Serial.print("All infos GPS available...\n");

#if USE_FORCE_NO_GPS
      if (flg_force_no_gps == true) {
        return;
      }
#endif

      // Build string for "enregistreur de traces" in TLV format
      frame_tlv_str = "";
      buildStrForExternal(frame_tlv_str);

#if USE_PRINT_GPS_VALUES
      Serial.print("TLV builded [");
      Serial.print(frame_tlv_str);
      Serial.print("]\n");
#endif

      frame_tlv_str.concat('\n');
      g__sdcard->appendGpsFrame(frame_tlv_str.c_str());
      // End: Build string for "enregistreur de traces" in TLV format

      gps_nmea_infos.flg_infos = 0;     // Effacement de toutes les infos disponibles

      frame_tlv_available = true;
    }
    else if (!strncmp(i__frame + 1, "GPGSV", 5)) {
      // Trace du nombre de satellites extrait de la 1st trame tant que 'INFO_GPS_ALL' n'est pas disponible
      extractInfosGPGSV(i__frame, true);
    }
#if USE_PRINT_GPS_VALUES
    else {
      // Trace de 'gps_nmea_infos.flg_infos' pour connaitre ce qui manque
      Serial.printf("Missing infos in [0x%04x] @ [0x%04x]\n", gps_nmea_infos.flg_infos, INFO_GPS_ALL);
    }
#endif
  }
}

void SerialNMEA::extractInfosGPRMC(const char *i__frame)
{
  char *l__pattern = NULL;

  // Copie de la trame car utilisation de 'strtok()' pour l'extraction des infos
  strcpy(gps_nmea_infos.frameNMEA, i__frame);

  int l__idx = 0;
  l__pattern = strtok(gps_nmea_infos.frameNMEA, ",");   // TBC: Test avec champs "absent" entre ",,"
  while (l__pattern != NULL) {
    switch (l__idx) {
    case 1: // Time: "hhmmss\0"
      strncpy(gps_nmea_infos.time, l__pattern, 6);
      gps_nmea_infos.time[6] = '\0';
      gps_nmea_infos.flg_infos |= INFO_GPS_TIME;

      // Detection des secondes [0, 5, 10, ..., 55] pour la construction de la trame TLV
      if (gps_nmea_infos.time[5] == '0' || gps_nmea_infos.time[5] == '5') {
        gps_nmea_infos.flg_infos |= INFO_5_SEC_ELAPSED;

#if USE_PRINT_GPS_VALUES
      Serial.print("5 seconds elapsed...\n");
#endif        
      }
      break;
    case 2: // status
      gps_nmea_infos.status = *l__pattern;
      gps_nmea_infos.flg_infos |= INFO_GPS_STATUS;
      break;
    case 3: // Latitude
      strncpy(gps_nmea_infos.t_latitude, l__pattern, sizeof(gps_nmea_infos.t_latitude));
#if USE_PRINT_GPS_VALUES
      gps_nmea_infos.latitude = (float)strtod(l__pattern, NULL) / 100.0;
#endif
      gps_nmea_infos.flg_infos |= INFO_GPS_LAT;
      break;
    case 4: // North/South
      if (*l__pattern == 'N') {
        gps_nmea_infos.north_south = GPS_NORTH;
        gps_nmea_infos.flg_infos |= INFO_GPS_LAT_DIR;
      }
      else if (*l__pattern == 'S') {
        gps_nmea_infos.north_south = GPS_SOUTH;
        gps_nmea_infos.flg_infos |= INFO_GPS_LAT_DIR;
      }
      break;
    case 5: // Longitude
    strncpy(gps_nmea_infos.t_longitude, l__pattern, sizeof(gps_nmea_infos.t_longitude));
#if USE_PRINT_GPS_VALUES
      gps_nmea_infos.longitude = (float)strtod(l__pattern, NULL) / 100.0;
#endif
      gps_nmea_infos.flg_infos |= INFO_GPS_LON;
      break;
    case 6: // East/West
      if (*l__pattern == 'E') {
        gps_nmea_infos.east_west = GPS_EAST;
        gps_nmea_infos.flg_infos |= INFO_GPS_LON_DIR;
      }
      else if (*l__pattern == 'W') {
        gps_nmea_infos.east_west = GPS_WEST;
        gps_nmea_infos.flg_infos |= INFO_GPS_LON_DIR;
      }
      break;
    case 7: // Speed in Knots and KmH
      gps_nmea_infos.speedKnots = (float)strtod(l__pattern, NULL);
      gps_nmea_infos.speedKmH   = 1.852 * gps_nmea_infos.speedKnots;
      gps_nmea_infos.flg_infos |= INFO_GPS_SPEED_KNOTS;
      gps_nmea_infos.flg_infos |= INFO_GPS_SPEED_KMH;

#ifdef USE_SIMULATION    // [USE_SIMULATION...
      strcpy(gps_nmea_infos.t_speedKnots, l__pattern);
#endif              // ..USE_SIMULATION]
      break;
    case 8: // Cap
      gps_nmea_infos.cap = (float)strtod(l__pattern, NULL);
      gps_nmea_infos.flg_infos |= INFO_GPS_CAP;

#ifdef USE_SIMULATION   // [USE_SIMULATION...
      strcpy(gps_nmea_infos.t_cap, l__pattern);
#endif              // ..USE_SIMULATION]
      break;
    case 9: // Date: "mmddyy\0"
      strncpy(gps_nmea_infos.date, l__pattern, 6);
      gps_nmea_infos.date[6] = '\0';
      gps_nmea_infos.flg_infos |= INFO_GPS_DATE;
      break;
    default:
      break;
    }
    
    l__pattern = strtok(NULL, ",");
    l__idx++;
  }

#ifndef USE_SIMULATION      // ...USE_SIMULATION]
  /* Determination de l'état 'CONNECTED' suite à:
   *    - l'info. 'Availaible' de la trame GPRMC
   *    - l'incrémentation de +1 du champ 'time' de la trame GPRMC
   *      Remarque: Seule les HH:MM:SS sont prises en compte; le passage à la minute suivante (ex: 07:23:59 => 07:24:00)
   *                fera que l'état 'CONNECTED' ne sera pas m.a.j. pendant 1 Sec. (< au 'time-out' de 30 Sec.
   *                avant de passer en 'NOT_CONNECTED')
   */
  if ((gps_nmea_infos.flg_infos & INFO_GPS_DATE) && (gps_nmea_infos.flg_infos & INFO_GPS_TIME)) {
    unsigned int l__dateAndTimeValue = (unsigned int)strtol(gps_nmea_infos.time, NULL, 10);

#if 0
    char l__buffer[16];
    sprintf(l__buffer, "[%u] => ", gps_nmea_infos.dateAndTime);
    Serial.print(l__buffer);
    sprintf(l__buffer, "[%u]\n", l__dateAndTime);
    Serial.print(l__buffer);
#endif
  
    if (gps_nmea_infos.status == 'A' && l__dateAndTimeValue == (gps_nmea_infos.dateAndTimeValue + 1)) {
#if USE_PRINT_GPS_VALUES
      Serial.print("Infos GPS valid...\n");
#endif
    }

  /* Nouvelles traces si l'heure courante est inferieure à celle sauvegardee ou 
   * si la date courante est differente de celle sauvegardée
   * => But: Permet de discriminer les nouvelles traces anterieures aux precedentes
   *         mais pas celle posterieures à la meme date et qui seront "attachees"
   *         à ces dernieres (pas genant en soit)
   *         => "New traces" au passage a minuit ;-)
   *
   * => Remarques:
   *    - TBC: Sur le terrain, ne doit jamais arriver sauf si le module GPS fournit
   *           une mauvaise Date/heure par rapport a la précédente
   */
    if (l__dateAndTimeValue < gps_nmea_infos.dateAndTimeValue
     // Filtrage pour les passages à la minute suivante (ie. HH:57:56 -> HH:58:00)
     || memcmp(gps_nmea_infos.date_save, gps_nmea_infos.date, sizeof(gps_nmea_infos.date_save))
    ) {
// #if USE_PRINT_GPS_VALUES
      Serial.print("New infos...\n");
// #endif

      g__sdcard->appendGpsFrame("#New infos...\n");
    }

    // Sauvegardes meme si erreur pour le test avec la trame suivante
    gps_nmea_infos.dateAndTimeValue = l__dateAndTimeValue;

    memcpy(gps_nmea_infos.date_save, gps_nmea_infos.date, sizeof(gps_nmea_infos.date_save));
  }
#endif    // [USE_SIMULATION...
}

void SerialNMEA::extractInfosGPGGA(const char *i__frame)
{
  char *l__pattern = NULL;

  // Copie de la trame car utilisation de 'strtok()' pour l'extraction des infos
  strcpy(gps_nmea_infos.frameNMEA, i__frame);

  int l__idx = 0;
  l__pattern = strtok(gps_nmea_infos.frameNMEA, ",");
  while (l__pattern != NULL) {
    switch (l__idx) {
    case 9: // Elevation
      gps_nmea_infos.elevation = strtod(l__pattern, NULL);
      gps_nmea_infos.flg_infos |= INFO_GPS_ELEVATION;

#ifdef USE_SIMULATION    // [USE_SIMULATION...
      strcpy(gps_nmea_infos.t_elevation, l__pattern);
#endif              // ..USE_SIMULATION]
      break;
    default:
      break;
    }

    l__pattern = strtok(NULL, ",");
    l__idx++;
  }
}

/* Extraction du nombre de satellites
 * => TODO: Si au moins 1 satellite est reconnu, la date et heure de la trame 'GPRMC' est valide
 *          avec par experience les Sec.mS != "ss.000"
 *          => Si ensuite aucun satellite n'est reconnu, cette date et heure reste valide
 *          => Les infos de localisation sont quant a elles invalides car etat 'V' dans la trame ;-) 
 *
 *          => Synthese de la date et heure en insertion du message d'attente en cours de diffusion
 *             => Utilisation de la commande 'WT2003S_SET_CUTIN_MODE'
 */
void SerialNMEA::extractInfosGPGSV(const char *i__frame, boolean i__flg_trace)
{
  char *l__pattern = NULL;

  // Copie de la trame car utilisation de 'strtok()' pour l'extraction des infos
  strcpy(gps_nmea_infos.frameNMEA, i__frame);

  int l__nbr_frames = 0;    // Nombre de trames 'GPGSV'
  int l__num_frame  = 0;    // Numero de la trame 'GPGSV'

  int l__idx = 0;
  l__pattern = strtok(gps_nmea_infos.frameNMEA, ",");
  while (l__pattern != NULL) {
    switch (l__idx) {
    case 1:
      l__nbr_frames = (int)strtol(l__pattern, NULL, 10);
      break;
    case 2:
      l__num_frame = (int)strtol(l__pattern, NULL, 10);
      break;
    case 3: // Nbr of satellites
      // Remarque: Pas de test de 'l__num_frame' pour extraire le nombre de satellites et positionner 'INFO_GPS_NBR_SATELLITES'
      gps_nmea_infos.nbr_satellites = (int)strtol(l__pattern, NULL, 10);
      gps_nmea_infos.flg_infos |= INFO_GPS_NBR_SATELLITES;

      if (l__num_frame == 1 && i__flg_trace) {
        Serial.printf("extractInfosGPGSV(): #%d/%d: %d satellites\n", l__num_frame, l__nbr_frames, gps_nmea_infos.nbr_satellites);
      }

      // Gestion des etats de reception
      switch (gps_nmea_infos.nbr_satellites) {
      case 0:
        gps_reception_state = GPS_RECEPTION_0_SATELLITE;
        break;
      case 1:
      case 2:
      case 3:
        gps_reception_state = GPS_RECEPTION_1_3_SATELLITES;
        break;
      default:
        if (gps_nmea_infos.nbr_satellites >= 4) {
          gps_reception_state = GPS_RECEPTION_4_MORE_SATELLITES;
        }
        else {
          gps_reception_state = GPS_RECEPTION_0_SATELLITE;
          Serial.printf("extractInfosGPGSV(): Invalid nbr satellites (%d)\n", gps_nmea_infos.nbr_satellites);
        }
        break;
      }

      // Armement systematique du timer 'TIMER_GPS_RECEPTION'
      if (g__timers->isInUse(TIMER_GPS_RECEPTION_STATE)) {
        g__timers->restart(TIMER_GPS_RECEPTION_STATE, DURATION_TIMER_GPS_RECEPTION_STATE);
      }
      else {
        g__timers->start(TIMER_GPS_RECEPTION_STATE, DURATION_TIMER_GPS_RECEPTION_STATE, &callback_exec_gps_reception_state);
      }
      // Fin: Gestion des etats de reception

      break;
    default:
      break;
    }

    l__pattern = strtok(NULL, ",");
    l__idx++;
  }
}

void SerialNMEA::extractInfosGPTXT(const char *i__frame, boolean i__flg_trace)
{
  char *l__pattern = NULL;

  // Copie de la trame car utilisation de 'strtok()' pour l'extraction des infos
  strcpy(gps_nmea_infos.frameNMEA, i__frame);

  int l__idx = 0;
  l__pattern = strtok(gps_nmea_infos.frameNMEA, ",");
  while (l__pattern != NULL) {
    switch (l__idx) {
    case 4:
      {
        // Recopie du champ complet 'ANTSTATUS=OPEN*CKs' tronque a l'eventuel '*' qui precede la checksuum sur 2 caracteres
        memset(gps_nmea_infos.status_antenna, '\0', sizeof(gps_nmea_infos.status_antenna));
        char *l__ptr_star = strrchr(l__pattern, '*');
        if (l__ptr_star != NULL) {
          *l__ptr_star = '\0';
        }
        strncpy(gps_nmea_infos.status_antenna, l__pattern, min(strlen(l__pattern), sizeof(gps_nmea_infos.status_antenna) - 1));
      }
      break;

    default:
        break;
    }

    l__pattern = strtok(NULL, ",");
    l__idx++;
  }
}

void SerialNMEA::convertFloatToString(char *o__buffer, char *i__buff_cmp, float i__value, int i__nbr_dec)
{
#ifdef USE_SIMULATION    // [USE_SIMULATION...
  printf("convertFloatToString([%s], [%f], [%d])\n", i__buff_cmp, i__value, i__nbr_dec);
#endif              // ...USE_SIMULATION]

  boolean l__flg_neg = false;
  if (i__value < 0.0) {
    i__value = -i__value;
    l__flg_neg = true;
  }

  size_t n = 0;
  byte l__digit;
  char l__digits[8];
  memset(l__digits, '\0', sizeof(l__digits));

  // Add epsilon for round
  i__value += 0.0005;

  if (l__flg_neg == true) {
    l__digits[n++] = '-';
  }

  boolean l__flg_zero = false;
  l__digit = (byte)(((uint16_t)i__value % 1000) / 100);
  if (l__digit) {
    l__digits[n++] = '0' + l__digit;
    l__flg_zero = false;
  }
  else {
    l__flg_zero = true;
  }
  l__digit = (byte)(((uint16_t)i__value % 100) / 10);
  if (l__flg_zero == false || l__digit) {
    l__digits[n++] = '0' + l__digit;
    l__flg_zero = false;
  }
  else {
    l__flg_zero = true;
  }
  l__digit = (byte)(((uint16_t)i__value % 10)); l__digits[n++] = '0' + l__digit;
  l__digits[n++] = '.';

  if (i__nbr_dec > 0) {
    l__digit = (byte)(((uint16_t)(i__value * 10.0)) % 10); l__digits[n++] = '0' + l__digit;
  }
  if (i__nbr_dec > 1) {
    l__digit = (byte)(((uint16_t)(i__value * 100.0)) % 10); l__digits[n++] = '0' + l__digit;
  }

  strcpy(o__buffer, l__digits);

#ifdef USE_SIMULATION    // [USE_SIMULATION...
  boolean l__verif_equal = !strcmp(o__buffer, i__buff_cmp) ? true : false;

  printf("Result [%s] (%s)\n", o__buffer, l__verif_equal == true ? "Ok" :
    (strlen(i__buff_cmp) > 0 ? "Ko" : "No compare"));
#endif              // ...USE_SIMULATION]
}

/* Trame TLV avec checksum en fin de trame
 * # 1: Type [A] Len [10] Value [063032.000] (*) GPS time HHMMSS.000
   # 2: Type [B] Len [ 1] Value [A]          (*) Valid datas
   # 3: Type [C] Len [ 9] Value [4847.0701]  (*) Lattitude (DDMM.mmmm - minutes decimales)
   # 4: Type [D] Len [ 1] Value [N]          (*) Hemisphere (N ou S)
   # 5: Type [E] Len [10] Value [00157.5163] (*) Longitude (DDMM.mmmm - minutes decimales)
   # 6: Type [F] Len [ 1] Value [E]          (*) Position (E ou W)
   # 7: Type [G] Len [ 4] Value [0.19]           Vitesse en noeuds
   # 8: Type [s] Len [ 3] Value [0.3]            Vitesse en km/h (UUU.D ou ---.-)
   # 9: Type [H] Len [ 6] Value [116.55]         Cap
   #10: Type [I] Len [ 6] Value [150112]     (*) Date DDMMYY
   #11: Type [K] Len [ 5] Value [117.8]      (*) Altitude en metres (UUUU.D ou ----.)
   #12: Type [L] Len [ 1] Value [M]          (*) Unite [M]etre
   #13: Type [a] Len [ 2] Value [12]         (*) Nbr de satellites
 */
void SerialNMEA::buildStrForExternal(String &o__str)
{
  char l__buffer[16];
  char l__buffer2[16];

  o__str = "";

  sprintf(l__buffer, "A%X", strlen(gps_nmea_infos.time) + strlen(".000"));
  o__str += l__buffer;
  o__str += gps_nmea_infos.time;
  o__str += ".000";

  sprintf(l__buffer, "B1%c", gps_nmea_infos.status);
  o__str += l__buffer;

  sprintf(l__buffer, "C%X", strlen(gps_nmea_infos.t_latitude));
  o__str += l__buffer;
  o__str += gps_nmea_infos.t_latitude;

  sprintf(l__buffer, "D1%c", gps_nmea_infos.north_south == GPS_NORTH ? 'N' : 'S');
  o__str += l__buffer;

  sprintf(l__buffer, "E%X", strlen(gps_nmea_infos.t_longitude));
  o__str += l__buffer;
  o__str += gps_nmea_infos.t_longitude;

  sprintf(l__buffer, "F1%c", gps_nmea_infos.east_west == GPS_EAST ? 'E' : 'W');
  o__str += l__buffer;

#ifdef USE_SIMULATION    // [USE_SIMULATION...
  convertFloatToString(l__buffer, gps_nmea_infos.t_speedKnots, gps_nmea_infos.speedKnots, 2); // %0.2f
#else
  convertFloatToString(l__buffer, NULL, gps_nmea_infos.speedKnots, 2);                      // %0.2f
#endif              // ...USE_SIMULATION]

  sprintf(l__buffer, "%s", l__buffer);
  sprintf(l__buffer2, "G%X%s", strlen(l__buffer), l__buffer);
  o__str += l__buffer2;

#ifdef USE_SIMULATION    // [USE_SIMULATION...
  convertFloatToString(l__buffer, gps_nmea_infos.t_speedKmH, gps_nmea_infos.speedKmH, 2); // %0.2f
#else
  convertFloatToString(l__buffer, NULL, gps_nmea_infos.speedKmH, 2);                    // %0.2f
#endif              // ...USE_SIMULATION]

  sprintf(l__buffer, "%s", l__buffer);
  sprintf(l__buffer2, "s%X%s", strlen(l__buffer), l__buffer);
  o__str += l__buffer2;

#ifdef USE_SIMULATION    // [USE_SIMULATION...
  convertFloatToString(l__buffer, gps_nmea_infos.t_cap, gps_nmea_infos.cap, 1); // %0.1f
#else
  convertFloatToString(l__buffer, NULL, gps_nmea_infos.cap, 1);               // %0.1f
#endif              // ...USE_SIMULATION]
  sprintf(l__buffer, "%s", l__buffer);
  sprintf(l__buffer2, "H%X%s", strlen(l__buffer), l__buffer);
  o__str += l__buffer2;

  sprintf(l__buffer, "I%X", strlen(gps_nmea_infos.date));
  o__str += l__buffer;
  o__str += gps_nmea_infos.date;

#ifdef USE_SIMULATION    // [USE_SIMULATION...
  convertFloatToString(l__buffer, gps_nmea_infos.t_elevation, gps_nmea_infos.elevation, 1); // %0.1f
#else
  convertFloatToString(l__buffer, NULL, gps_nmea_infos.elevation, 1);                     // %0.1f
#endif              // ...USE_SIMULATION]
  sprintf(l__buffer, "%s", l__buffer);
  sprintf(l__buffer2, "K%X%s", strlen(l__buffer), l__buffer);
  o__str += l__buffer2;

  // TBC: To remove because implicit
  o__str += "L1M";

  sprintf(l__buffer, "a2%02d", gps_nmea_infos.nbr_satellites);
  o__str += l__buffer;

  // Calcul de la checksum
  unsigned char l__cks_calculated = 0x00;
  calculChecksumNMEA((char *)o__str.c_str(), &l__cks_calculated, NULL);
  sprintf(l__buffer, "*30%02X", l__cks_calculated);
  o__str += l__buffer;
}
// Fin: Extraction des informations des trames NMEA et construction de la trame TLV
    
