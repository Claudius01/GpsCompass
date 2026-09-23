// $Id: Pilot.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef USE_SIMULATION
#include <Arduino.h>
#else
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#endif

#include <math.h>     // Cf. '/Users/.../AppData/Local/Arduino15/packages/esp32/hardware/esp32/1.0.4/tools/sdk/include/newlib/math.h'

#include "Misc.h"
#include "Pilot.h"

Pilot::Pilot()
{
  Serial.println("Pilot::Pilot()");

  initToNaN(&nan);

  int n = 0;
  for (n = 0; n < COORD_NUMBER; n++) {
    initCoordToNoMean(&coord[n]);
  }

  // In progres: 'SYNTH_DIST_FROM_STARTING_POS' or 'SYNTH_DIST_FROM_POS_SAVE'
  dist_mode = SYNTH_DIST_NO_MEAN;
}

Pilot::~Pilot()
{
  Serial.println("Pilot::~Pilot()");
}

void Pilot::initCoordToNoMean(COORD *i__coord)
{
  i__coord->duration = 0L;
  i__coord->lat      = nan;
  i__coord->lon      = nan;
  i__coord->ele      = nan;
  i__coord->cap      = nan;
  i__coord->speedKmH = nan;
}

void Pilot::initCoordStartingPosition(const COORD *i__coord)
{
  coord[COORD_STARTING_POS].duration = i__coord->duration;
  coord[COORD_STARTING_POS].lat      = i__coord->lat;
  coord[COORD_STARTING_POS].lon      = i__coord->lon;
  coord[COORD_STARTING_POS].ele      = i__coord->ele;
  coord[COORD_STARTING_POS].cap      = i__coord->cap;
  coord[COORD_STARTING_POS].speedKmH = nan;              // No initialize the 'speedKmH' field
}

void Pilot::initCoordLastPosSave(COORD *i__coord)
{
  coord[COORD_LAST_POS_SAVE].duration = i__coord->duration;
  coord[COORD_LAST_POS_SAVE].lat      = i__coord->lat;
  coord[COORD_LAST_POS_SAVE].lon      = i__coord->lon;
  coord[COORD_LAST_POS_SAVE].ele      = i__coord->ele;
  coord[COORD_LAST_POS_SAVE].cap      = i__coord->cap;
  coord[COORD_LAST_POS_SAVE].speedKmH = nan;              // No initialize the 'speedKmH' field
}

/* Initialisation des coordonnées courantes à partir de la trame GPS
 * ou forçage en cas de simulation en labo si 'g__flg_force_coord_current == true'
 */
void Pilot::initCoordToCurrent(COORD *i__coord)
{
#if USE_FORCE_CURRENT_COORD
  if (g__flg_force_coord_current == true) {
    // Test si valeurs initialisées à != 'NaN'
    if (g__force_coord_current.lat      == g__force_coord_current.lat
     && g__force_coord_current.lon      == g__force_coord_current.lon
     && g__force_coord_current.ele      == g__force_coord_current.ele
     && g__force_coord_current.cap      == g__force_coord_current.cap
     && g__force_coord_current.speedKmH == g__force_coord_current.speedKmH)
    {
      Serial.print("initCoordToCurrent(): Force the current coord\n");
      memcpy(&coord[COORD_CURRENT], &g__force_coord_current, sizeof(COORD));
      return;
    }
    else {
      Serial.print("initCoordToCurrent(): Err: 1! or more Invalid forced values\n");

      // L'initialisation se fera à partir des valeurs réelles issues de la trame GPS ;-)
    }
  }
#endif

  coord[COORD_CURRENT].duration = i__coord->duration;
  coord[COORD_CURRENT].lat      = i__coord->lat;
  coord[COORD_CURRENT].lon      = i__coord->lon;
  coord[COORD_CURRENT].ele      = i__coord->ele;
  coord[COORD_CURRENT].cap      = i__coord->cap;
  coord[COORD_CURRENT].speedKmH = i__coord->speedKmH;
}

#if 0
void Pilot::setCoordCurrent(COORD *i__coord)
{
  // Recopie des coordonnées géographiques et cinétiques
  coord[COORD_CURRENT].lat      = i__coord->lat;
  coord[COORD_CURRENT].lon      = i__coord->lon;
  coord[COORD_CURRENT].ele      = i__coord->ele;
  coord[COORD_CURRENT].cap      = i__coord->cap;
  coord[COORD_CURRENT].speedKmH = i__coord->speedKmH;

  Serial.print("Coord position:\n");
  Serial.print("\tLat.  [");
  Serial.print(coord[COORD_CURRENT].lat, 6);
  Serial.print("] degrees\n");
  Serial.print("\tLon.  [");
  Serial.print(coord[COORD_CURRENT].lon, 6);
  Serial.print("] degrees\n");
  Serial.print("\tEle.  [");
  Serial.print(coord[COORD_CURRENT].ele, 0);
  Serial.print("] M\n");
  Serial.print("\tCap   [");
  Serial.print(coord[COORD_CURRENT].cap, 1);
  Serial.print("] degrees\n");
  Serial.print("\tSpeed [");
  Serial.print(coord[COORD_CURRENT].speedKmH, 1);
  Serial.print("] KmH\n");
}
#endif

/*  Distance en mètres des coordonnées courantes aux coordonnées passées en argument
 *  => Remarques: - L'élévation n'est pas prise en compte (pas significative: TBC)
 *                - La correction est appliquée a la moyenne des latitudes 'from' -> 'to'
 */
uint32_t Pilot::getDistanceTo(COORD *i__coord)
{
  float l__lat_from = coord[COORD_CURRENT].lat;
  float l__lon_from = coord[COORD_CURRENT].lon;
  float l__lat_to   = i__coord->lat;
  float l__lon_to   = i__coord->lon;

  float l__correction = cos(2.0 * M_PI * ((l__lat_from + l__lat_to) / 2.0) / 360.0);
  float l__distlat = 60.0 * 1852.0 * (l__lat_from - l__lat_to);
  float l__distlon = 60.0 * 1852.0 * (l__lon_from - l__lon_to) * l__correction;

  float l__distance = sqrt(l__distlat * l__distlat + l__distlon * l__distlon);

  return (uint32_t)l__distance;
}

#if 0
uint32_t Pilot::getDistanceFromTo(COORD *i__coord_from, COORD *i__coord_to)
{
  float l__correction = cos(2.0 * M_PI * ((i__coord_from->lat + i__coord_to->lat) / 2.0) / 360.0);
  float l__distlat = 60.0 * 1852.0 * (i__coord_from->lat - i__coord_to->lat);
  float l__distlon = 60.0 * 1852.0 * (i__coord_from->lon - i__coord_to->lon) * l__correction;

  float l__distance = sqrt(l__distlat * l__distlat + l__distlon * l__distlon);

  return (uint32_t)l__distance;
}
#endif

/*  Distance en mètres des coordonnées courantes à la position de départ
 */
uint32_t Pilot::getDistanceToStartingPos()
{
  return getDistanceTo(&coord[COORD_STARTING_POS]);
}

/*  Distance en mètres des coordonnées courantes à la dernière position enregistrée
 */
uint32_t Pilot::getDistanceToLastPosSave()
{
  return getDistanceTo(&coord[COORD_LAST_POS_SAVE]);
}

/* Cap en degrés [0;360] des coordonnées courantes aux coordonnées passées en argument
 * => Warning: Pas à exécuter si distance très proche (cf. 'SYNTH_DIST_VERY_NEAR @ ENUM_SYNTH_DISTANCE_TYPES')
 *             car dans ce cas, la distance n'est pas synthétisée au profit du prompt "vous ête à la position..." ;-)
 *
 * Principe du calcul du cap:
 *                             o: Position passée en argument (lat_0; lon_0)
 *                            #0: (lat_current >  lat_0 et lon_current >= lon_0) -> Cap [180.0;270.0[
 *           N                #1: (lat_current <= lat_0 et lon_current >= lon_0) -> Cap [270.0;360.0[
 *     #3    |    #0          #2: (lat_current <= lat_0 et lon_current <  lon_0) -> Cap ]0.0;90.0]
 *           |                #3: (lat_current >  lat_0 et lon_current <  lon_0) -> Cap ]90.0;180.0[]
 *           |
 *  W -------o------- E       Calcul de l'arc tangente (cap calculé tjs positif dans la plage [0.0;90.0[: alpha) du rapport:
 *           |                (distance_lat / distance_lon) et application suivant la position de 'o'; |  savoir:
 *           |                #0: Cap = 270.0 - alpha (direction de #0 vers 'o')
 *     #2    |    #1          #1: Cap = 270.0 + alpha (direction de #1 vers 'o')
 *           S                #2: Cap =  90.0 - alpha (direction de #2 vers 'o')
 *                            #3: Cap =  90.0 + alpha (direction de #3 vers 'o')
 */
float Pilot::getCapTo(COORD *i__coord)
{
  char l__buffer[40];
  COORD l__wrk;

  /* Point à la même latitude que le point courant et situé à la même longitude que celui passé en argument
   * -> Permet de calculer la distance sur le méridien (distance_lat)
   */
  initCoordToNoMean(&l__wrk);
  l__wrk.lat = coord[COORD_CURRENT].lat;
  l__wrk.lon = i__coord->lon;
  uint32_t l__dist_lon = getDistanceTo(&l__wrk);

  /* Point à la même longitude que le point courant et situé à la même latitude que celui passé en argument
   * -> Permet de calculer la distance sur la longitude (distance_lon)
   */
  initCoordToNoMean(&l__wrk);
  l__wrk.lat = i__coord->lat;
  l__wrk.lon = coord[COORD_CURRENT].lon;
  uint32_t l__dist_lat = getDistanceTo(&l__wrk);

#if 0
  printf("### l__dist_lat [%d] m\n", l__dist_lat);
  printf("### l__dist_lon [%d] m\n", l__dist_lon);

  // Verif
  printf("###  => l__dist [%d] m\n", (uint32_t)sqrt(l__dist_lat * l__dist_lat + l__dist_lon * l__dist_lon));
#endif

  // TODO: 'l__dist_lon' est un 'uint32_t' comparé à EPSILON qui est un 'float' !
  float l__alpha_rad = (l__dist_lon > EPSILON) ? atan((float)l__dist_lat / (float)l__dist_lon) : M_PI_2;

  // Conversion en degrés
  float l__alpha_deg = (180.0 * l__alpha_rad) / M_PI;

#if 0
  printf("###  => l__alpha_deg [%.1f] degrees\n", l__alpha_deg);
#endif

  if (l__alpha_deg < 0.0 || l__alpha_deg > 90.0) {
    /* Ne doit jamais arriver
     * => Return 'nan'
     */
    sprintf(l__buffer, "Error: Pilot::getCapTo(): Wrong 'l__alpha_deg' [");

#ifndef USE_SIMULATION
    Serial.print(l__buffer);
    Serial.print(l__alpha_deg, 1);
#else
    printf(l__buffer);
    printf("%f", l__alpha_deg);
#endif
    sprintf(l__buffer, "] degrees ([0.0;90.0] expected)\n");
#ifndef USE_SIMULATION
    Serial.print(l__buffer);
#else
    printf(l__buffer);
#endif

    return nan;
  }

  // Application de l'offset suivant les positions relatives dans les 4 cadrans
  float l__cap = nan;
  if (coord[COORD_CURRENT].lat > i__coord->lat && coord[COORD_CURRENT].lon >= i__coord->lon) {
    l__cap = 270.0 - l__alpha_deg;
  }
  else if (coord[COORD_CURRENT].lat <= i__coord->lat && coord[COORD_CURRENT].lon >= i__coord->lon) {
    l__cap = 270.0 + l__alpha_deg;
  }
  else if (coord[COORD_CURRENT].lat <= i__coord->lat && coord[COORD_CURRENT].lon < i__coord->lon) {
    l__cap = 90.0 - l__alpha_deg;
  }
  else if (coord[COORD_CURRENT].lat > i__coord->lat && coord[COORD_CURRENT].lon < i__coord->lon) {
    l__cap = 90.0 + l__alpha_deg;
  }
  else {
    /* Ne doit jamais arriver
     * => Return 'nan'
     */
    sprintf(l__buffer, "Error: Pilot::getCapTo(): No test found\n");
#ifndef USE_SIMULATION
    Serial.print(l__buffer);
#else
    printf(l__buffer);
#endif
  }

  return l__cap;
}

/*  Cap en degrés [0;360] des coordonnées courantes aux coordonnées de la position de départ
 */
float Pilot::getCapToStartingPos()
{
  return getCapTo(&coord[COORD_STARTING_POS]);
}

/*  Cap en degrés [0;360] des coordonnées courantes aux coordonnées de la dernière position enregistrée
 */
float Pilot::getCapToLastPosSave()
{
  return getCapTo(&coord[COORD_LAST_POS_SAVE]);
}

#if 0
/*  Cap en degrés [0;360] des coordonnées de 2 points passées en argument
 *  Remarque: Méthode étendue de 'getCapTo()' et identique à 'Plots::calculateCap()'
 *            => TODO: Correction commentaires suivants
 *
 * Principe du calcul du cap:
 *                             o: Position passée en argument (lat_0; lon_0)
 *                            #0: (lat_current >  lat_0 et lon_current >= lon_0) -> Cap [180.0;270.0[
 *           N                #1: (lat_current <= lat_0 et lon_current >= lon_0) -> Cap [270.0;360.0[
 *     #3    |    #0          #2: (lat_current <= lat_0 et lon_current <  lon_0) -> Cap ]0.0;90.0]
 *           |                #3: (lat_current >  lat_0 et lon_current <  lon_0) -> Cap ]90.0;180.0[]
 *           |
 *  W -------o------- E       Calcul de l'arc tangente (cap calculé tjs positif dans la plage [0.0;90.0[: alpha) du rapport:
 *           |                (distance_lat / distance_lon) et application suivant la position de 'o'; |  savoir:
 *           |                #0: Cap = 270.0 - alpha (direction de #0 vers 'o')
 *     #2    |    #1          #1: Cap = 270.0 + alpha (direction de #1 vers 'o')
 *           S                #2: Cap =  90.0 - alpha (direction de #2 vers 'o')
 *                            #3: Cap =  90.0 + alpha (direction de #3 vers 'o')
 */
float Pilot::getCapFromTo(COORD *i__coord_from, COORD *i__coord_to)
{
  char l__buffer[40];
  COORD l__wrk_from;

  /* Point à la même latitude que le point destination et situé à la même longitude que le point origine
   * -> Permet de calculer la distance sur le méridien (distance_lat)
   */
  initCoordToNoMean(&l__wrk_from);
  l__wrk_from.lat = i__coord_to->lat;
  l__wrk_from.lon = i__coord_from->lon;
  uint32_t l__dist_lon = getDistanceFromTo(&l__wrk_from, i__coord_to);

  /* Point à la même latitude que le point origine et situé à la même longitude que le point destination
   * -> Permet de calculer la distance sur la longitude (distance_lon)
   */
  initCoordToNoMean(&l__wrk_from);
  l__wrk_from.lat = i__coord_from->lat;
  l__wrk_from.lon = i__coord_to->lon;
  uint32_t l__dist_lat = getDistanceFromTo(&l__wrk_from, i__coord_to);

#if 0
  printf("### l__dist_lat [%d] m\n", l__dist_lat);
  printf("### l__dist_lon [%d] m\n", l__dist_lon);

  // Verif
  printf("###  => l__dist [%d] m\n", (uint32_t)sqrt(l__dist_lat * l__dist_lat + l__dist_lon * l__dist_lon));
#endif

  float l__alpha_rad = (l__dist_lon > EPSILON) ? atan((float)l__dist_lat / (float)l__dist_lon) : M_PI_2;

  // Conversion en degrés
  float l__alpha_deg = (180.0 * l__alpha_rad) / M_PI;

#if 0
  printf("###  => l__alpha_deg [%.1f] degrees\n", l__alpha_deg);
#endif

  if (l__alpha_deg < 0.0 || l__alpha_deg > 90.0) {
    /* Ne doit jamais arriver
     * => Return 'nan'
     */
    sprintf(l__buffer, "Error: Pilot::getCapFromTo(): Wrong 'l__alpha_deg' [");

#ifndef USE_SIMULATION
    Serial.print(l__buffer);
    Serial.print(l__alpha_deg, 1);
#else
    printf(l__buffer);
    printf("%f", l__alpha_deg);
#endif
    sprintf(l__buffer, "] degrees ([0.0;90.0] expected)\n");
#ifndef USE_SIMULATION
    Serial.print(l__buffer);
#else
    printf(l__buffer);
#endif

    return nan;
  }

  // Application de l'offset suivant les positions relatives dans les 4 cadrans
  float l__cap = nan;
  if (i__coord_from->lat > i__coord_to->lat && i__coord_from->lon >= i__coord_to->lon) {
    l__cap = 270.0 - l__alpha_deg;
  }
  else if (i__coord_from->lat <= i__coord_to->lat && i__coord_from->lon >= i__coord_to->lon) {
    l__cap = 270.0 + l__alpha_deg;
  }
  else if (i__coord_from->lat <= i__coord_to->lat && i__coord_from->lon < i__coord_to->lon) {
    l__cap = 90.0 - l__alpha_deg;
  }
  else if (i__coord_from->lat > i__coord_to->lat && i__coord_from->lon < i__coord_to->lon) {
    l__cap = 90.0 + l__alpha_deg;
  }
  else {
    /* Ne doit jamais arriver
     * => Return 'nan'
     */
    sprintf(l__buffer, "Error: Pilot::getCapFromTo(): No test found\n");
#ifndef USE_SIMULATION
    Serial.print(l__buffer);
#else
    printf(l__buffer);
#endif
  }

  return l__cap;
}
#endif

/*  Altitude des coordonnées de la position de départ
 */
float Pilot::getEleToStartingPos()
{
  return coord[COORD_STARTING_POS].ele;
}

/*  Altitude des coordonnées de la dernière position enregistrée
 */
float Pilot::getEleToLastPosSave()
{
  return coord[COORD_LAST_POS_SAVE].ele;
}
