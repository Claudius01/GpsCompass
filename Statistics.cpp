// $Id: Statistics.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

/* Statistiques avec QOS
 * - GPS @ ST_STATS_GPS: La QOS est calculée comme suit:
 * 
 *       (nbr_of_good_frames - nbr_of_warn_time_too_large) - nbr_of_error_frames
 * QOS = ----------------------------------------------------------------------- %
 *                 (nbr_of_good_frames - nbr_of_warn_time_too_large)
 */

#ifndef USE_SIMULATION
#include <Arduino.h>
#else
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#endif

#include <sstream>

#include "Misc.h"

#ifndef USE_SIMULATION
#include "SerialNMEA.h"
#include "SerialMP3Player.h"
#endif

#include "Statistics.h"
#include "Errors.h"

#include "WT2003S.h"

Statistics::Statistics()
{
  Serial.println("Statistics::Statistics()");

  memset(&general, '\0', sizeof(ST_STATS_GENERAL));
  memset(&gps, '\0', sizeof(ST_STATS_GPS));

  general.gest_counter_for_10ms.value_min =  100;
  general.gest_counter_for_10ms.value_max =    0;

  // Statistiques cinétiques
  clearCineticInfos();
}

Statistics::~Statistics()
{
  Serial.println("Statistics::~Statistics()");
}

void Statistics::updateGestionCounterFor10ms(byte i__value)
{
  ST_GEST_COUNTER_FOR_10MS *l__pst = &general.gest_counter_for_10ms;

  l__pst->nbr_of_samples++;
  l__pst->value_current = i__value;

  if (i__value < l__pst->value_min) {
    l__pst->value_min = i__value;
  }
  else if (i__value > l__pst->value_max) {
    l__pst->value_max = i__value;
  }

  /*  Protection if update from (unsigned int)65535 to 0
   *  => Ignore this sample
   */
  if (l__pst->nbr_of_samples != 0) {
    // Calcul of average value with a 'long' because possible overflow ;-)
    long l__intermediate_calc = (long)l__pst->value_average;
    l__intermediate_calc  = ((l__pst->nbr_of_samples - 1) * l__intermediate_calc) + i__value;
    l__pst->value_average = (byte)(l__intermediate_calc / l__pst->nbr_of_samples);
  }
}

void Statistics::setGenQos()
{
  if (general.duration_internal != 0 && general.duration_from_gps != 0 && general.duration_internal >= general.duration_not_connected) {
    general.qos = (100.0 * (general.duration_internal - general.duration_not_connected)) / general.duration_internal;
  }
  else {
    general.qos = 0.0;
  }
}

void Statistics::resetGpsCounter(ENUM_STATS_GPS i__enum)
{
  char l__buffer[80];

  switch (i__enum) {
  case ENUM_STATS_GPS_NBR_STARTUP_MODULE:
    gps.nbr_startup_module = 0;
  case ENUM_STATS_GPS_NBR_OF_GOOD_FRAMES:
    gps.nbr_of_good_frames = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_LOSS:
    gps.nbr_of_loss = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS:
    gps.nbr_of_establishments = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_RETRY:
    gps.nbr_of_retry = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT:
    gps.nbr_of_retry_advert = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_ERR_TIME_PROGRESSION:
    gps.nbr_of_err_time_progression = 0;
    break;

  // Warning
  case ENUM_STATS_GPS_NBR_OF_WARN_TIME_TOO_LARGE:
    gps.nbr_of_warn_time_too_large = 0;
    break;

  // Errors
  case ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES:
    gps.nbr_of_error_frames = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_WRONG_CKS:
    gps.nbr_of_wrong_cks = 0;
    break;
  case ENUM_STATS_GPS_NBR_OF_INVALID_INFOS:
    gps.nbr_of_invalid_infos = 0;
    break;

  default:
    sprintf(l__buffer, "resetGpsCounter(%d): Error: ENUM_STATS_GPS not supported\n", i__enum);
    Serial.print(l__buffer);
  }

  // Update the Gps QOS in the all the cases
  setGpsQos();
}

void Statistics::incGpsCounter(ENUM_STATS_GPS i__enum)
{
  char l__buffer[80];

  switch (i__enum) {
  case ENUM_STATS_GPS_NBR_STARTUP_MODULE:
    gps.nbr_startup_module++;
  case ENUM_STATS_GPS_NBR_OF_GOOD_FRAMES:
    gps.nbr_of_good_frames++; 
    break;
  case ENUM_STATS_GPS_NBR_OF_LOSS:
    gps.nbr_of_loss++; 
    break;
  case ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS:
    gps.nbr_of_establishments++;
    break;
  case ENUM_STATS_GPS_NBR_OF_RETRY:
    gps.nbr_of_retry++;
    break;
  case ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT:
    gps.nbr_of_retry_advert++;
    break;
  case ENUM_STATS_GPS_NBR_OF_ERR_TIME_PROGRESSION:
    gps.nbr_of_err_time_progression++; 
    break;

  // Warning
  case ENUM_STATS_GPS_NBR_OF_WARN_TIME_TOO_LARGE:
    gps.nbr_of_warn_time_too_large++; 
    break;

  // Errors
  case ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES:
    gps.nbr_of_error_frames++; 
    break;
  case ENUM_STATS_GPS_NBR_OF_WRONG_CKS:
    gps.nbr_of_wrong_cks++; 
    break;
  case ENUM_STATS_GPS_NBR_OF_INVALID_INFOS:
    gps.nbr_of_invalid_infos++; 
    break;

  default:
    sprintf(l__buffer, "incGpsCounter(%d): Error: ENUM_STATS_GPS not supported\n", i__enum);
    Serial.print(l__buffer);
  }

  // Update the Gps QOS in the all the cases
  setGpsQos();
}

uint16_t Statistics::getGpsCounter(ENUM_STATS_GPS i__enum)
{
  char l__buffer[80];

  switch (i__enum) {
  case ENUM_STATS_GPS_NBR_STARTUP_MODULE:
    return gps.nbr_startup_module;
  case ENUM_STATS_GPS_NBR_OF_GOOD_FRAMES:
    return gps.nbr_of_good_frames; 
  case ENUM_STATS_GPS_NBR_OF_LOSS:
    return gps.nbr_of_loss;
  case ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS:
    return gps.nbr_of_establishments;
  case ENUM_STATS_GPS_NBR_OF_RETRY:
    return gps.nbr_of_retry;
  case ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT:
    return gps.nbr_of_retry_advert;

  // Warning
  case ENUM_STATS_GPS_NBR_OF_WARN_TIME_TOO_LARGE:
    return gps.nbr_of_warn_time_too_large; 

  // Errors
  case ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES:
    return gps.nbr_of_error_frames; 
  case ENUM_STATS_GPS_NBR_OF_WRONG_CKS:
    return gps.nbr_of_wrong_cks; 
  case ENUM_STATS_GPS_NBR_OF_INVALID_INFOS:
    return gps.nbr_of_invalid_infos; 
  case ENUM_STATS_GPS_NBR_OF_ERR_TIME_PROGRESSION:
    return gps.nbr_of_err_time_progression; 

  default:
    sprintf(l__buffer, "getGpsCounter(%d): Error: ENUM_STATS_GPS not supported\n", i__enum);
    Serial.print(l__buffer);
    return (uint16_t)-1;
  }
}

/* Calcul de:
 *
 *       (nbr_of_good_frames - nbr_of_warn_time_too_large + nbr_of_error_frames)     GOOD - (WARN + ERR)
 * QOS = ----------------------------------------------------------------------- % = ------------------- %
 *                                             nbr_of_good_frames                           GOOD
 *
 * Examples:
 *  - (WARN + ERR) == 0            => qos = 100%
 *  - WARN == 0; ERR == 0.5 * GOOD => qos =  50%
 *  - WARN == 0; ERR == 0.1 * GOOD => qos =  90%
 *  - WARN == 0; ERR == 0.9 * GOOD => qos =  10%
 *  - WARN == 0.5 * GOOD; ERR == 0 => qos =  50%
 *  - WARN == GOOD; ERR == 0       => qos =   0%
 */
void Statistics::setGpsQos()
{
  uint16_t l__good_frames_warn_and_err = (gps.nbr_of_warn_time_too_large + gps.nbr_of_error_frames);
  uint16_t l__good_frames_without_w_e  = (gps.nbr_of_good_frames - l__good_frames_warn_and_err);

  gps.qos = (gps.nbr_of_good_frames != 0 && l__good_frames_without_w_e < gps.nbr_of_good_frames) ? ((100.0 *  l__good_frames_without_w_e) / gps.nbr_of_good_frames) : 0.0;
}

void Statistics::clearCineticInfos()
{
  cinetic.distance_total = 0;

  cinetic.duration_total   =
  cinetic.duration_in_movement =
  cinetic.duration_pause   = 0L;

  size_t n = 0;
  for (n = 0; n < SPEED_NBR_ORIGIN; n++) {
    cinetic.speeds[n].val_kmh_inst =
    cinetic.speeds[n].val_kmh_avg  =
    cinetic.speeds[n].val_kmh_max  = 0.0;

    cinetic.speeds[n].nbr_updating = 0;
  }
}

boolean Statistics::getCineticInfos(ST_STATS_CINETICS *o__infos)
{
  if (o__infos != NULL) {
    memcpy(o__infos, &cinetic, sizeof(ST_STATS_CINETICS));

    return true;
  }
  else {
    return false;
  }
}

void Statistics::updateCineticInfosGps(float i__speed_kmh_inst)
{
  Serial.print("updateCineticInfosGps([");
  Serial.print(i__speed_kmh_inst, 1);
  Serial.print("] KmH)\n");

  // Calcul des vitesses moyenne et maximale après l'incrémentation du nombre de m.a.j.
  cinetic.speeds[SPEED_GPS].nbr_updating++;

  if (cinetic.speeds[SPEED_GPS].nbr_updating > 5) {    // 5 échantillons pour m.a.j. (protection retour à 0 de 'nbr_updating' + écart important)
    cinetic.speeds[SPEED_GPS].val_kmh_inst = i__speed_kmh_inst;

    cinetic.speeds[SPEED_GPS].val_kmh_avg =
      (cinetic.speeds[SPEED_GPS].val_kmh_inst + ((cinetic.speeds[SPEED_GPS].nbr_updating - 1) * cinetic.speeds[SPEED_GPS].val_kmh_avg)) / cinetic.speeds[SPEED_GPS].nbr_updating;

    if (cinetic.speeds[SPEED_GPS].val_kmh_inst > cinetic.speeds[SPEED_GPS].val_kmh_max) {
      cinetic.speeds[SPEED_GPS].val_kmh_max = cinetic.speeds[SPEED_GPS].val_kmh_inst;
    }
  }
}

boolean Statistics::updateCineticInfosCalc(uint32_t i__distance, long i__delta_time)
{
  boolean l__rtn = false;

  Serial.print("updateCineticInfosCalc([");
  Serial.print(i__distance);
  Serial.print("] M, [");
  Serial.print(i__delta_time);
  Serial.print("] Sec)\n");

  /* Calcul des nouvelles informations si 'i__delta_time' est > 12"
   * => Temps pour obtenir un déplacement significatif @ à la précision du GPS
   * => Dans ce cas, return 'true' pour une recopie de la position courante dans la position précédente
   */
  if (i__delta_time > 12L) {
    // Calcul des distances, vitesses moyenne et maximale après l'incrémentation du nombre de m.a.j.
    cinetic.speeds[SPEED_CALC].nbr_updating++;

    if (cinetic.speeds[SPEED_CALC].nbr_updating > 4) {    // 5 échantillons pour m.a.j. (protection retour à 0 de 'nbr_updating' + écart important)
      // Cumul de la distance
      cinetic.distance_total += i__distance;

      /* Calcul de la vitesse instantannée à partir de la distance parcourue
       *                          3600 * m
       *  Conversion m/s -> KmH = -------- = 3.6 * m/s
       *                          1000 * s
      */
      cinetic.speeds[SPEED_CALC].val_kmh_inst = 3.6 * ((float)i__distance / (float)i__delta_time);

      // Cumul des distances parcourues totale, en déplacement et en pause
      cinetic.duration_total += i__delta_time;

      if (cinetic.speeds[SPEED_CALC].val_kmh_inst >= 0.75) {
        cinetic.duration_in_movement += i__delta_time;
      }
      else {
        cinetic.duration_pause += i__delta_time;
      }

      cinetic.speeds[SPEED_CALC].val_kmh_avg =
        (cinetic.speeds[SPEED_CALC].val_kmh_inst + ((cinetic.speeds[SPEED_CALC].nbr_updating - 1) * cinetic.speeds[SPEED_CALC].val_kmh_avg)) / cinetic.speeds[SPEED_CALC].nbr_updating;

      if (cinetic.speeds[SPEED_CALC].val_kmh_inst > cinetic.speeds[SPEED_CALC].val_kmh_max) {
        cinetic.speeds[SPEED_CALC].val_kmh_max = cinetic.speeds[SPEED_CALC].val_kmh_inst;
      }
    }

    // Recopie d'une position "stabilisée" ;-)
    l__rtn = true;
  }

  Serial.print("\t-> Rtn [");
  Serial.print(l__rtn);
  Serial.print("]\n");

  return l__rtn;
}

void Statistics::printAll()
{
  char                  l__buffer[80];
  char                  l__buffer_bis[80];
  std::ostringstream    l__out;

  memset(l__buffer_bis, '\0', sizeof(l__buffer_bis));

  // --------
  l__out << "Errors:\n";

  // Print into 'l__out' reference
  g__errors->printAll(l__out);

  // --------
  l__out << "\n";     // Line feed
  l__out << "Statistics:\n";

  // GENERAL
  convertFloatToString(l__buffer_bis, getGenQos(), 2);     // 2 chiffres après la virgule
  sprintf(l__buffer, "\tGeneral: QOS [%s%%]\n", l__buffer_bis); l__out << l__buffer;

  sprintf(l__buffer, "\t\tepoch_init             [%lu]\n", general.epoch_init); l__out << l__buffer;
  sprintf(l__buffer, "\t\tepoch_current          [%lu]\n", general.epoch_current); l__out << l__buffer;

  convertDurationToString(l__buffer_bis, general.duration_from_gps, false);
  sprintf(l__buffer, "\t\tduration_from_gps      [%lu] [%s]\n", general.duration_from_gps, l__buffer_bis); l__out << l__buffer;

  convertDurationToString(l__buffer_bis, general.duration_internal, false);
  sprintf(l__buffer, "\t\tduration_internal      [%lu] [%s]\n", general.duration_internal, l__buffer_bis); l__out << l__buffer;

  convertDurationToString(l__buffer_bis, general.duration_pulse, false);
  sprintf(l__buffer, "\t\tduration_pulse         [%lu] [%s]\n", general.duration_pulse, l__buffer_bis); l__out << l__buffer;

  // Fix: 'duration_wait_1st_cnx' est trouve anormalement a 0 (non systematique ;-)
  if (gps.nbr_of_establishments == 0) {
    general.duration_wait_1st_cnx = general.duration_internal;
  }
  convertDurationToString(l__buffer_bis, general.duration_wait_1st_cnx, false);
  sprintf(l__buffer, "\t\tduration_wait_1st_cnx  [%lu] [%s]\n", general.duration_wait_1st_cnx, l__buffer_bis); l__out << l__buffer;

  convertDurationToString(l__buffer_bis, general.duration_not_connected, false);
  sprintf(l__buffer, "\t\tduration_not_connected [%lu] [%s]\n", general.duration_not_connected, l__buffer_bis); l__out << l__buffer;

  long l__diff = getDiffDurationInternalToGps();
  convertDurationToString(l__buffer_bis, l__diff, true);
  sprintf(l__buffer, "\t\t\tdiff internal -> GPS   [%ld] [%s]\n", l__diff, l__buffer_bis); l__out << l__buffer;

  l__diff = getDiffDurationInternalToPulse();
  convertDurationToString(l__buffer_bis, l__diff, true);
  sprintf(l__buffer, "\t\t\tdiff internal -> Pulse [%ld] [%s]\n", l__diff, l__buffer_bis); l__out << l__buffer;

  ST_GEST_COUNTER_FOR_10MS *l__pst = &general.gest_counter_for_10ms;
  sprintf(l__buffer, "\t\t\tcalibration: Samples [%u] Current [%d] Min [%d] Avg [%d] Max [%d]\n",
    l__pst->nbr_of_samples, l__pst->value_current, l__pst->value_min, l__pst->value_average, l__pst->value_max); l__out << l__buffer;

  // HH:MM:SS interne ('connected' and 'not connected'
  long l__time = (general.epoch_init + general.duration_internal) % 86400L;

  // Application du changement d'heure été/hivber déterminé en mode 'connected'
  l__time += general.offset_seconds;

  int l__hh = (l__time / 3600);
  int l__mm = (l__time - 3600 * l__hh) / 60;
  int l__ss = (l__time - 3600 * l__hh) % 60;
  sprintf(l__buffer, "\t\t\tInternal time: %02d:%02d:%02d (GMT %c%02d:%02d) (%s)\n",
    l__hh, l__mm, l__ss,
    (general.offset_seconds >= 0) ? '+' : '-',
    (general.offset_seconds >= 0) ? general.offset_seconds / 3600 : (-general.offset_seconds) / 3600,
    (general.offset_seconds >= 0) ? general.offset_seconds % 60   : (-general.offset_seconds) % 60,
    general.gps_available ? "GPS" : "GPS out of order");  l__out << l__buffer;

  l__out << "\n";     // Line feed

  // GPS
  convertFloatToString(l__buffer_bis, getGpsQos(), 2);     // 2 chiffres après la virgule
  sprintf(l__buffer, "\tGPS: QOS [%s%%]\n", l__buffer_bis); l__out << l__buffer;

  sprintf(l__buffer, "\t\tnbr_startup_module           [%u]\n", gps.nbr_startup_module); l__out << l__buffer;
  sprintf(l__buffer, "\t\tnbr_of_good_frames           [%u]\n", gps.nbr_of_good_frames); l__out << l__buffer;
  sprintf(l__buffer, "\t\tnbr_of_loss                  [%u]\n", gps.nbr_of_loss); l__out << l__buffer;
  sprintf(l__buffer, "\t\tnbr_of_establishments        [%u]\n", gps.nbr_of_establishments); l__out << l__buffer;
  sprintf(l__buffer, "\t\tnbr_of_retry                 [%u]\n", gps.nbr_of_retry); l__out << l__buffer;
  sprintf(l__buffer, "\t\tnbr_of_retry_advert          [%u]\n", gps.nbr_of_retry_advert); l__out << l__buffer;

  sprintf(l__buffer, "\t\tnbr_of_warn                  [%u]\n", (gps.nbr_of_warn_time_too_large)); l__out << l__buffer;
  sprintf(l__buffer, "\t\t\tnbr_of_warn_time_too_large  [%u]\n", gps.nbr_of_warn_time_too_large); l__out << l__buffer;

  sprintf(l__buffer, "\t\tnbr_of_error_frames          [%u]\n", gps.nbr_of_error_frames); l__out << l__buffer;
  sprintf(l__buffer, "\t\t\tnbr_of_wrong_cks            [%u]\n", gps.nbr_of_wrong_cks); l__out << l__buffer;
  sprintf(l__buffer, "\t\t\tnbr_of_invalid_infos        [%u]\n", gps.nbr_of_invalid_infos); l__out << l__buffer;
  sprintf(l__buffer, "\t\t\tnbr_of_err_time_progression [%u]\n", gps.nbr_of_err_time_progression); l__out << l__buffer;

  l__out << "\n";     // Line feed
  sprintf(l__buffer, "\t\tStatus Antenna               [%s]\n",
    (strlen(g__serial_nmea->getStatusAntenna()) != 0) ? g__serial_nmea->getStatusAntenna() : "Unknown");    l__out << l__buffer;

  l__out << "\n";     // Line feed

  sprintf(l__buffer, "\tCinetic:\n"); l__out << l__buffer;

  // Arrondi à 50 mètres de la distance totale
  uint32_t l__dist_km = ((cinetic.distance_total + 50) / 1000);
  uint32_t l__dist_hm = ((cinetic.distance_total + 50) % 1000) / 100;
  sprintf(l__buffer, "\t\tDistance total      [%d] M [%d.%d] Km\n", cinetic.distance_total, l__dist_km, l__dist_hm); l__out << l__buffer;

  convertDurationToString(l__buffer_bis, cinetic.duration_total, false);
  sprintf(l__buffer, "\t\tDuration total      [%ld] [%s]\n", cinetic.duration_total, l__buffer_bis); l__out << l__buffer;

  convertDurationToString(l__buffer_bis, cinetic.duration_in_movement, false);
  sprintf(l__buffer, "\t\t         movement   [%ld] [%s]", cinetic.duration_in_movement, l__buffer_bis); l__out << l__buffer;

  if (cinetic.duration_in_movement > 0) {
    // Vitesse moyenne de déplacement ;-)
    float l__speed_kmh = 3.6 * ((float)cinetic.distance_total / cinetic.duration_in_movement);
    convertFloatToString(l__buffer_bis, l__speed_kmh, 1);     // 1 chiffre après la virgule
    sprintf(l__buffer, " (%s KmH)", l__buffer_bis); l__out << l__buffer;
  }
  l__out << "\n";     // Line feed

  convertDurationToString(l__buffer_bis, cinetic.duration_pause, false);
  sprintf(l__buffer, "\t\t         pause      [%ld] [%s]\n", cinetic.duration_pause, l__buffer_bis); l__out << l__buffer;

  size_t n = 0;
  for (n = 0; n < SPEED_NBR_ORIGIN; n++) {
    sprintf(l__buffer, (n == SPEED_GPS) ? "\t\tInfos from GPS\n" : "\t\tInfos calculated\n");
    l__out << l__buffer;

    convertFloatToString(l__buffer_bis, cinetic.speeds[n].val_kmh_inst, 1);     // 1 chiffre après la virgule
    sprintf(l__buffer, "\t\t\tInstantaneous   [%s] KmH (%s)\n", l__buffer_bis, isMovementInProgressPedestrian((SPEED_ORIGIN)n) ? "Movement in progress" : "No movement");
    l__out << l__buffer;

    convertFloatToString(l__buffer_bis, cinetic.speeds[n].val_kmh_avg, 1);     // 1 chiffre après la virgule
    sprintf(l__buffer, "\t\t\tAverage         [%s] KmH\n", l__buffer_bis); l__out << l__buffer;
    convertFloatToString(l__buffer_bis, cinetic.speeds[n].val_kmh_max, 1);     // 1 chiffre après la virgule
    sprintf(l__buffer, "\t\t\tMaximal         [%s] KmH\n", l__buffer_bis); l__out << l__buffer;

    sprintf(l__buffer, "\t\t\tNbr of updating [%d]\n", cinetic.speeds[n].nbr_updating); l__out << l__buffer;
  }

  l__out << "\n";     // Line feed

  // Statistics of Audio
  sprintf(l__buffer, "\tAudio (KT403A):\n"); l__out << l__buffer;

#ifndef USE_SIMULATION
  sprintf(l__buffer, "\t\tEqualizer Value    [%d] (0x%02x)\n", g__serial_mp3_player->getValueEqualizer(), g__serial_mp3_player->getValueEqualizer());
  l__out << l__buffer;
  sprintf(l__buffer, "\t\tQuery Volume Level [%d] (0x%02x)\n", g__serial_mp3_player->getValueVolumeLevel(), g__serial_mp3_player->getValueVolumeLevel());
  l__out << l__buffer;

  l__out << "\n";     // Line feed
  sprintf(l__buffer, "\tAudio (WT2003S): [%s]\n", (g__wt2003s->isInService() == true) ? "In service" : "Out of order"); l__out << l__buffer;

  sprintf(l__buffer, "\t\tQuery Volume Level [%d] (0x%02x)\n", g__wt2003s->getValueVolumeLevel(), g__wt2003s->getValueVolumeLevel());
  l__out << l__buffer;
  l__out << "\n";     // Line feed

  const char *l__state_previous_label = g__wt2003s->getLabelState(g__wt2003s->getStatePrevious());
  const char *l__state_current_label  = g__wt2003s->getLabelState(g__wt2003s->getStateCurrent());

  sprintf(l__buffer, "\t\tState Previous [%s]\n", l__state_previous_label != NULL ? l__state_previous_label : "UNKNOWN");   l__out << l__buffer;
  sprintf(l__buffer, "\t\tState Current  [%s]\n", l__state_current_label  != NULL ? l__state_current_label  : "UNKNOWN");   l__out << l__buffer;
  l__out << "\n";     // Line feed

  // Verification comptabilisation
  sprintf(l__buffer, "\t\tPlay Start          [%d]\n", g__wt2003s->getCptPlayStart());      l__out << l__buffer;
  sprintf(l__buffer, "\t\tPlay End            [%d] (%s)\n",
    g__wt2003s->getCptPlayEnd(), (g__wt2003s->getCptPlayEnd() == g__wt2003s->getCptPlayStart()) ? "Ok" : "Ko");   l__out << l__buffer;

  sprintf(l__buffer, "\t\tPlay In Progress    [%d] (%s)\n",
    g__wt2003s->getCptPlayInProgress(), (g__wt2003s->getCptPlayInProgress() == g__wt2003s->getCptPlayStart()) ? "Ok" : "Ko"); l__out << l__buffer;

  l__out << "\n";     // Line feed

  // Verification comptabilisation
  uint32_t l__response_total = g__wt2003s->getRespReceivedAndValid()
    + g__wt2003s->getRespReceivedNotSupported()
    + g__wt2003s->getRespReceivedNotExpected()
    + g__wt2003s->getRespReceivedUnknown()
    + g__wt2003s->getRespReceivedInternalErr()
    + g__wt2003s->getNbrSaturationsFifoRx();

  sprintf(l__buffer, "\t\tCommands   Total          [%d]\n", g__wt2003s->getCmdSentTotal());               l__out << l__buffer;
  sprintf(l__buffer, "\t\tResponses  Total          [%d] (%s)\n",
    g__wt2003s->getRespReceivedTotal(), (g__wt2003s->getRespReceivedTotal() == l__response_total) ? "Ok" : "Ko");   l__out << l__buffer;

  sprintf(l__buffer, "\t\tResponses  Valid          [%d]\n", g__wt2003s->getRespReceivedAndValid());       l__out << l__buffer;
  sprintf(l__buffer, "\t\tErr: Resp. Not Supported  [%d]\n", g__wt2003s->getRespReceivedNotSupported());   l__out << l__buffer;
  sprintf(l__buffer, "\t\tErr: Resp. Not Expected   [%d]\n", g__wt2003s->getRespReceivedNotExpected());    l__out << l__buffer;
  sprintf(l__buffer, "\t\tErr: Resp. Unknown        [%d]\n", g__wt2003s->getRespReceivedUnknown());        l__out << l__buffer;
  sprintf(l__buffer, "\t\tErr: Resp. Internal Error [%d]\n", g__wt2003s->getRespReceivedInternalErr());    l__out << l__buffer;
  sprintf(l__buffer, "\t\tErr: FIFO/Rx full         [%d]\n", g__wt2003s->getNbrSaturationsFifoRx());       l__out << l__buffer;
#endif

  l__out << "\n";     // Line feed
  // End: Statistics of Audio

  sprintf(l__buffer, "\tMisc:\n"); l__out << l__buffer;

#ifndef USE_SIMULATION
  /* Get the max duration between prompts (calcul with 'STATUS_FILE_END' event received in mode connected ;-)
   * => La durée max ne doit pas être supérieure à 'DURATION_TIMER_LONG_FAMINE' de détection de la longue famine
   */
  long l__duration_max = g__serial_mp3_player->getMaxDurationBetweenPrompts();    // Duration in Sec.
  convertDurationToString(l__buffer_bis, l__duration_max, false);
  sprintf(l__buffer, "\t\tMax duration between prompts    [%ld] [%s]\n", g__serial_mp3_player->getMaxDurationBetweenPrompts(), l__buffer_bis);
  l__out << l__buffer;
  // End: Get the max duration between prompts (calcul with 'STATUS_FILE_END' event received in mode connected ;-)

  /* Get the number call and exec to 'purgeFIFOTxPlay()' method
   * => Le nbr d'exécutions doit être égal au nbr d'appels ;-)
   */
  sprintf(l__buffer, "\t\tNbr call to 'purgeFIFOTxPlay()' [%u]\n", g__serial_mp3_player->getNbrCallPurgeFIFOTxPlay());
  l__out << l__buffer;
  sprintf(l__buffer, "\t\tNbr exec of 'purgeFIFOTxPlay()' [%u] (%s)\n",
    g__serial_mp3_player->getNbrExecPurgeFIFOTxPlay(),
    (g__serial_mp3_player->getNbrExecPurgeFIFOTxPlay() == g__serial_mp3_player->getNbrCallPurgeFIFOTxPlay()) ? "Ok" : "Ko");

  l__out << l__buffer;
  // End: Get the number call and exec to 'purgeFIFOTxPlay()' method

  // Durée min de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' en multiple de 100 mS => / 100 => x Sec.
  if (g__errors->getMinDurationOfVeryLongFamine() != LONG_MAX) {
    l__duration_max = ((g__errors->getMinDurationOfVeryLongFamine() + 50L) / 100L);    // Duration in Sec. with round ;-)

    convertDurationToString(l__buffer_bis, l__duration_max, false);
    sprintf(l__buffer, "\t\tMin Dur. of Very Long Famine    [%lu] [%s]\n", g__errors->getMinDurationOfVeryLongFamine(), l__buffer_bis);
  }
  else {
    sprintf(l__buffer, "\t\tMin Dur. of Very Long Famine    [n.a.]\n");
  }
  l__out << l__buffer;
  // Fin: Durée min de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' en multiple de 100 mS => / 100 => x Sec.

  // Durée max de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' en multiple de 100 mS => / 100 => x Sec.
  if (g__errors->getMaxDurationOfVeryLongFamine() != LONG_MAX) {
    long l__duration_max = ((g__errors->getMaxDurationOfVeryLongFamine() + 50L) / 100L);    // Duration in Sec. with round ;-)

    convertDurationToString(l__buffer_bis, l__duration_max, false);
    sprintf(l__buffer, "\t\tMax Dur. of Very Long Famine    [%lu] [%s]\n", g__errors->getMaxDurationOfVeryLongFamine(), l__buffer_bis);
  }
  else {
    sprintf(l__buffer, "\t\tMax Dur. of Very Long Famine    [n.a.]\n");
  }
  l__out << l__buffer;
  // Fin: Durée max de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' en multiple de 100 mS => / 100 => x Sec.
#endif

  l__out << "\n";     // Line feed

  Serial.print(l__out.str().c_str());
}
