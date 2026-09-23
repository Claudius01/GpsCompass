// $Id: Plots.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifdef USE_SIMULATION
// Include files for simulation
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"

#include "../SimuMove.h"
#else
#include <Arduino.h>
#endif

#include "Misc.h"
#include "PromptsSynthesis.h"
#include "GpsCompass.h"

#ifndef USE_SIMULATION
#include "Pilot.h"
#endif

#include "Plots.h"

#ifndef USE_SIMULATION
#include "SerialMP3Player.h"
#endif

#include "Timers.h"
#include "Statistics.h"

Plots::Plots() : flg_adjust_plots_avail(false), flg_detect_sens_inverse(false), optim_level(0), idx_of_adjust_plot(-1),
                 state(RECORDS_NOT_AVAILABLE), nbrOfRecords(0), checksum_calc((uint32_t)-1), size_datas_calc(0), checksum_read((uint32_t)-1), size_datas_read(0),
                 num_section(0), name_of_plots(""), nbr_of_plots(0), nbr_of_plots_adjust(0),
                 flg_make_synth_properties(false), total_distance(0), total_distance_2D(0), total_distance_3D(0),
                 remaining_distance(UINT_MAX), distance_on_plots(0), plots_direction(PLOTS_DIR_NO_MEAN),
                 m__idx(0), m__crc_tmp(0), m__size_cumul(0)
{
  char l__buffer[80];

  Serial.println("Plots::Plots()");

  sprintf(l__buffer, "\tsizeof 'PlotRecord': %d bytes (%d elements)\n", sizeof(plotRecord), NBR_PLOT_RECORDS);
  Serial.print(l__buffer);

  initToNaN(&m__nan);

#if USE_ATTRACTOR_TO_PLOT
  memset(t_dist_avg, '\0', sizeof(t_dist_avg));
  idx_dist_avg = 0;
#endif

  size_t n = 0;
  for (n = 0; n < (sizeof(plotRecord) / sizeof(plotRecord[0])); n++) {
    ST_PLOT_RECORD *l__p_record = &plotRecord[n];

    l__p_record->section = (byte)-1;
    l__p_record->idx = -1;

    initToNaN(&l__p_record->plot.lat);
    initToNaN(&l__p_record->plot.lon);
    initToNaN(&l__p_record->plot.ele);
    initToNaN(&l__p_record->plot.capToNextPlot);

    l__p_record->plot.flgValid = false;
    l__p_record->plot.capHoursToNextPlot = -1;
    l__p_record->plot.distance = 0;

    // Definitions de DESMOS
    // Coordonnees relatives de la jonction
    l__p_record->desmos.lat_y = INT_MAX;
    l__p_record->desmos.lon_x = INT_MAX;
    l__p_record->desmos.A     = INT_MAX;
    l__p_record->desmos.B     = INT_MAX;

    initToNaN(&l__p_record->desmos.X);
    initToNaN(&l__p_record->desmos.Y);
    l__p_record->desmos.Ds    = INT_MAX;
    l__p_record->desmos.Dj    = INT_MAX;
    l__p_record->desmos.Ps    = false;
    // Fin: Definitions de DESMOS
  }

  for (n = 0; n < (sizeof(plotRecordAdjust) / sizeof(plotRecordAdjust[0])); n++) {
    ST_PLOT_RECORD *l__p_record_adjust = &plotRecordAdjust[n];

    l__p_record_adjust->section = (byte)-1;
    l__p_record_adjust->idx = -1;

    initToNaN(&l__p_record_adjust->plot.lat);
    initToNaN(&l__p_record_adjust->plot.lon);
    initToNaN(&l__p_record_adjust->plot.ele);
    initToNaN(&l__p_record_adjust->plot.capToNextPlot);

    l__p_record_adjust->plot.flgValid = false;
    l__p_record_adjust->plot.capHoursToNextPlot = -1;
    l__p_record_adjust->plot.distance = 0;

    // Definitions de DESMOS
    // Coordonnees relatives de la jonction
    l__p_record_adjust->desmos.lat_y = INT_MAX;
    l__p_record_adjust->desmos.lon_x = INT_MAX;
    l__p_record_adjust->desmos.A     = INT_MAX;
    l__p_record_adjust->desmos.B     = INT_MAX;

    initToNaN(&l__p_record_adjust->desmos.X);
    initToNaN(&l__p_record_adjust->desmos.Y);
    l__p_record_adjust->desmos.Ds    = INT_MAX;
    l__p_record_adjust->desmos.Dj    = INT_MAX;
    l__p_record_adjust->desmos.Ps    = false;
    // Fin: Definitions de DESMOS
  }

  initToNaN(&plotStartPosition.lat);
  initToNaN(&plotStartPosition.lon);
  initToNaN(&plotStartPosition.ele);

  for (n = 0; n < DESMOS_COORD_NBR; n++) {
    initCoordToNoMean(&st_desmos_coord[n].coord);

    st_desmos_coord[n].lat_y = INT_MAX;
    st_desmos_coord[n].lon_x = INT_MAX;
  }

  plotResults.flg_avalaible = false;
  plotResultsSave.flg_avalaible = false;

  plotResults.flg_dist_to_track_too_large = true;
  plotResultsSave.flg_dist_to_track_too_large = true;

  // Init. 'ST_POSITION'
  memset(&positions, '\0', sizeof(ST_POSITION));

  positions.flg_available = false;

  initCoordToNoMean(&positions.current_pos);
  positions.idx_select = -1;
  positions.dist_min = UINT_MAX;

  positions.pos_select.idx = -1;
  positions.pos_select.distance = UINT_MAX;

  initToNaN(&positions.pos_select.lat);
  initToNaN(&positions.pos_select.lon);
  initToNaN(&positions.pos_select.ele);

  positions.pos_select.range = (uint8_t)-1;

  for (n = 0; n < NBR_POSITIONS; n++) {
    positions.pos_all[n].idx = -1;
    positions.pos_all[n].distance = UINT_MAX;

    initToNaN(&positions.pos_all[n].lat);
    initToNaN(&positions.pos_all[n].lon);
    initToNaN(&positions.pos_all[n].ele);

    positions.pos_all[n].range = (uint8_t)-1;
  }
  // End: Init. 'ST_POSITION'

#if USE_POS_COMMUNES
  // Init. 'ST_POS_COMMUNES'
  memset(&pos_communes, '\0', sizeof(ST_POS_COMMUNES));

  //pos_communes.flg_available = false;

  initCoordToNoMean(&pos_communes.current_pos);
  pos_communes.idx_select = -1;
  pos_communes.dist_min = UINT_MAX;

  pos_communes.pos_select.idx = -1;
  pos_communes.pos_select.distance = UINT_MAX;

  initToNaN(&pos_communes.pos_select.lat);
  initToNaN(&pos_communes.pos_select.lon);
  initToNaN(&pos_communes.pos_select.ele);

  pos_communes.pos_select.range = (uint8_t)-1;

  // Nombre de positions de communes définies dans 'PromptsSynthesis.cpp'
  pos_communes.nbr_positions = getPosCommNbrPositions();

  if (pos_communes.nbr_positions > NBR_POS_COMMUNES) {
    sprintf(l__buffer, "Warning: Too many Positions of Communes [%d max]\n", NBR_POS_COMMUNES);
    Serial.print(l__buffer);

    // Réduction à 'NBR_POS_COMMUNES'
    pos_communes.nbr_positions = NBR_POS_COMMUNES;
  }
  sprintf(l__buffer, "Nbr Positions of Communes [%d/%d]\n", getPosCommNbrPositions(), NBR_POS_COMMUNES);
  Serial.print(l__buffer);

  sprintf(l__buffer, "- Copy the %d Positions of Communes...\n", getPosCommNbrPositions());
  Serial.print(l__buffer);

  for (n = 0; n < getPosCommNbrPositions(); n++) {
    ST_COORD_POSITION l__coord_position;

    l__coord_position.idx  = n;
    l__coord_position.type = TYPE_POSITION_NO_MEAN;     // Non utilisée dans le cas de 'ST_POS_COMMUNES.pos_select'

    initToNaN(&l__coord_position.lat);
    initToNaN(&l__coord_position.lon);
    initToNaN(&l__coord_position.ele);
    bool l__flg_rtn = getPosCommPositionOfCommune(n, &l__coord_position.lat, &l__coord_position.lon, &l__coord_position.ele);

    if (l__flg_rtn == false) {
      sprintf(l__buffer, "\tError: #%d position\n", n);
      Serial.print(l__buffer);
    }

    l__coord_position.range = (uint8_t)-1;    // [0, 1, ... n]: Pour la synthèse de '(n+1)-ième'... (non utilisée dans le cas de 'ST_POS_COMMUNES.pos_select')

    l__coord_position.distance = UINT_MAX;

    memcpy(&pos_communes.pos_all[n], &l__coord_position, sizeof(ST_COORD_POSITION));
  }

  sprintf(l__buffer, "- End of copy the %d Positions of Communes\n", n);
  Serial.print(l__buffer);

  sprintf(l__buffer, "- Clear the %d Positions of Communes...\n", (NBR_POS_COMMUNES - getPosCommNbrPositions()));
  Serial.print(l__buffer);

  for ( ; n < NBR_POS_COMMUNES; n++) {
    pos_communes.pos_all[n].idx = -1;
    pos_communes.pos_all[n].distance = UINT_MAX;

    initToNaN(&pos_communes.pos_all[n].lat);
    initToNaN(&pos_communes.pos_all[n].lon);
    initToNaN(&pos_communes.pos_all[n].ele);

    pos_communes.pos_all[n].range = (uint8_t)-1;
  }

  sprintf(l__buffer, "- End of clear the %d Positions of Communes\n", (NBR_POS_COMMUNES - getPosCommNbrPositions()));
  Serial.print(l__buffer);

  // End: Init. 'ST_POS_COMMUNES'
#endif

  // Initialisation des éléments pour la simulation des déplacements
  st_simu_move.flg_in_progress = false;

  st_simu_move.num_plot_init    =
  st_simu_move.num_plot_current = 0;

  initToNaN(&st_simu_move.speed_kmh);

  st_simu_move.duration_previous = 0;
  // Fin: Initialisation des éléments pour la simulation des déplacements
}

Plots::~Plots()
{
  Serial.println("Plots::~Plots()");
}

void Plots::initCoordToNoMean(COORD *i__coord)
{
  i__coord->duration = 0L;
  i__coord->lat      = m__nan;
  i__coord->lon      = m__nan;
  i__coord->ele      = m__nan;
  i__coord->cap      = m__nan;
  i__coord->speedKmH = m__nan;
}

void Plots::setCoordCurrent(COORD *i__coord, boolean i__flg_force)
{
  // Recopie du timestamp
  st_desmos_coord[DESMOS_COORD_CURRENT].coord.duration = i__coord->duration;

  /* Recopie des coordonnées géographiques et cinétiques et
     calcul des coordonnées relatives à la 1st jonction si existe
  */
  st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat      = i__coord->lat;
  st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon      = i__coord->lon;
  st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele      = i__coord->ele;
  st_desmos_coord[DESMOS_COORD_CURRENT].coord.cap      = i__coord->cap;
  st_desmos_coord[DESMOS_COORD_CURRENT].coord.speedKmH = i__coord->speedKmH;

  if (flg_adjust_plots_avail == true) {
    st_desmos_coord[DESMOS_COORD_CURRENT].lat_y = desmosCalculOfRelativeLat(i__coord->lat);
    st_desmos_coord[DESMOS_COORD_CURRENT].lon_x = desmosCalculOfRelativeLon(i__coord->lon);
  }

  idx_of_adjust_plot = 0;

  Serial.print("Coord position:\n");
  Serial.print("\tTime  [");
  Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].coord.duration);
  Serial.print("]");

  // Formatage HH:MM:SS du timestamp de la position courante
  {
      char l__buffer[32];
      char l__buffer_bis[32];

      convertDurationToString(l__buffer_bis, st_desmos_coord[DESMOS_COORD_CURRENT].coord.duration, false);

      sprintf(l__buffer, " [%s]\n", l__buffer_bis);
      Serial.print(l__buffer);
  }
  // Fin: Formatage HH:MM:SS du timestamp de la position courante

  Serial.print("\tLat.  [");
  Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat, 6);
  Serial.print("] degrees");

  if (flg_adjust_plots_avail == true) {
    Serial.print(" [");
    Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].lat_y);
    Serial.print("] M\n");
  }
  else {
    Serial.print("\n");
  }
  Serial.print("\tLon.  [");
  Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon, 6);
  Serial.print("] degrees");

  if (flg_adjust_plots_avail == true) {
    Serial.print(" [");
    Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].lon_x);
    Serial.print("] M\n");
  }
  else {
    Serial.print("\n");
  }
  Serial.print("\tEle.  [");
  Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele, 0);
  Serial.print("] M\n");
  Serial.print("\tCap   [");
  Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].coord.cap, 1);
  Serial.print("] degrees\n");
  Serial.print("\tSpeed [");
  Serial.print(st_desmos_coord[DESMOS_COORD_CURRENT].coord.speedKmH, 1);
  Serial.print("] KmH\n");

  // Relance des calculs des distances aux positions 'enregistrées' et 'mémorisées'
  positions.idx_current = 0;
  setCurrentPosition(i__coord);
  // Fin: Relance des calculs des distances aux positions 'enregistrées' et 'mémorisées'

  // Relance des calculs des distances aux positions des communes
  pos_communes.idx_current = 0;
  setCurrentPosCommunes(i__coord);
  // Fin: Relance des calculs des distances aux positions des communes

  // Test si enregistrement des positions courantes dans les plots records
  if (g__gestion.flg_save_current_positions == true) {
    char l__buffer[48];
    uint32_t l__distance = INT_MAX;
    float l__cap;
    int l__nbr_records = 0;

    initToNaN(&l__cap);

    // Trace des éléments de 'st_desmos_coord[]'
    int n = 0;
    Serial.print("'st_desmos_coord[]' content:\n");
    for (n = 0; n < DESMOS_COORD_NBR; n++) {
      sprintf(l__buffer, "\t#%d: Lat [", n);
      Serial.print(l__buffer);
      Serial.print(st_desmos_coord[n].coord.lat, 6);
      Serial.print("] Lon [");
      Serial.print(st_desmos_coord[n].coord.lon, 6);
      Serial.print("] Ele [");
      Serial.print(st_desmos_coord[n].coord.ele, 0);
      Serial.print("] Cap [");
      Serial.print(st_desmos_coord[n].coord.cap, 1);
      Serial.print("]\n");
    }
    // Fin: Trace des éléments de 'st_desmos_coord[]'

    if (nbr_of_plots == 0 || isNewCurrentPosition(&l__distance, &l__cap, &l__nbr_records) == true) {
      if (nbr_of_plots == 0) {
        Serial.print("\t=> 1st New position (1 record)\n");

        // Réinitialisation du nom et des coordonnées de la position "#Start" des plots records
        name_of_plots = "Local";

        plotStartPosition.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
        plotStartPosition.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
        plotStartPosition.ele = st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele;

        // Enregistrement de la position courante
        plotRecord[nbr_of_plots].idx = nbr_of_plots;
        plotRecord[nbr_of_plots].plot.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
        plotRecord[nbr_of_plots].plot.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
        plotRecord[nbr_of_plots].plot.ele = st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele;
        nbr_of_plots++;

        l__nbr_records = 1;

        // Maj de la dernière position enregistrée depuis la position courante
        memcpy(&st_desmos_coord[DESMOS_COORD_LAST_RECORDED].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));
      }
      else {
        sprintf(l__buffer, "\t=> #%d: New position (%d record) [", nbr_of_plots, l__nbr_records);
        Serial.print(l__buffer);
        Serial.print(l__distance);
        Serial.print("] M");
        Serial.print(" Cap [");
        Serial.print(l__cap, 1);
        Serial.print("]\n");
      }

      // Synthèse d'un son indiquant le(s) enregistrement(s)
      byte      *l__commands;
      uint16_t  *l__durations = NULL;
      size_t    l__nbr_durations = 0;

#ifdef USE_SIMULATION   // 'l__size' not used
      buildCommandsPromptsRecordPlots(l__nbr_records, &l__commands, &l__durations, &l__nbr_durations);
#else
      size_t l__size = buildCommandsPromptsRecordPlots(l__nbr_records, &l__commands, &l__durations, &l__nbr_durations);
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#endif
    }
  }

#ifndef USE_SIMULATION
  // Maj des infos statistiques cinématiques
  g__stats->updateCineticInfosGps(st_desmos_coord[DESMOS_COORD_CURRENT].coord.speedKmH);

  if (isValidCoord(&st_desmos_coord[DESMOS_COORD_PREVIOUS_FOR_STATS].coord) == false) {
    memcpy(&st_desmos_coord[DESMOS_COORD_PREVIOUS_FOR_STATS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));
  }
#endif

#ifndef USE_SIMULATION
  long     l__delta_time     = (st_desmos_coord[DESMOS_COORD_CURRENT].coord.duration - st_desmos_coord[DESMOS_COORD_PREVIOUS_FOR_STATS].coord.duration);
  uint32_t l__delta_distance = calculateDistance(&st_desmos_coord[DESMOS_COORD_PREVIOUS_FOR_STATS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord);

  boolean l__flg_rtn = g__stats->updateCineticInfosCalc(l__delta_distance, l__delta_time);

  // Sauvegarde des coordonnées et de 'duration' si m.a.j. effective 
  if (l__flg_rtn == true) {
    memcpy(&st_desmos_coord[DESMOS_COORD_PREVIOUS_FOR_STATS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));
  }
#endif
  // Fin: Maj des infos statistiques cinématiques

#ifndef USE_SIMULATION
  // Synthèse d'un son indiquant un nouveau non déplacement
  /* Image de 'g__stats->isMovementInProgress()'
   * => Détermine le retour à un non déplacement pour la synthèse sur --\__
   * => Prompts au lancement (normalement aucun déplacement)
   */
  static boolean g__flg_movement_in_progress = true;

  if (g__stats->isMovementInProgress() == true) {   // // Vitesses GPS ou calculée > 0.75 KmH
    g__flg_movement_in_progress = true;
  }
  else {
    if (g__flg_movement_in_progress == true) {
      // Nouveau non déplacement
      byte      *l__commands;
      uint16_t  *l__durations = NULL;
      size_t    l__nbr_durations = 0;

      size_t l__size = buildCommandsPromptsNoMovementInProgress(&l__commands, &l__durations, &l__nbr_durations);
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
    }

    g__flg_movement_in_progress = false;
  }
  // Fin: Synthèse d'un son indiquant un nouveau non déplacement
#endif
}

void Plots::getCoordCurrent(COORD *o__coord) const
{
  o__coord->lat      = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
  o__coord->lon      = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
  o__coord->ele      = st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele;
  o__coord->cap      = st_desmos_coord[DESMOS_COORD_CURRENT].coord.cap;
  o__coord->speedKmH = st_desmos_coord[DESMOS_COORD_CURRENT].coord.speedKmH;
}

void Plots::copyPlotsRecordToAdjust()
{
  Serial.print("Copy ");
  Serial.print(nbr_of_plots);
  Serial.print(" 'PlotsRecord' to 'PlotsAdjust'...\n");

  int n = 0;
  for (n = 0; n < nbr_of_plots && n < NBR_PLOT_RECORDS; n++) {
    memcpy(&plotRecordAdjust[n], &plotRecord[n], sizeof(ST_PLOT_RECORD));
    plotRecordAdjust[n].plot.flgValid = true;
  }

  // Init. the nbr of 'PlotsAdjust' equal to 'nbr_of_plots'
  if (nbr_of_plots < NBR_PLOT_RECORDS) {
    nbr_of_plots_adjust = nbr_of_plots;
  }

  // Init. Caps and Distance of the last 'PlotsAdjust' to zero
  plotRecordAdjust[nbr_of_plots_adjust-1].plot.capToNextPlot      = 0.0;
  plotRecordAdjust[nbr_of_plots_adjust-1].plot.capHoursToNextPlot = 0;
  plotRecordAdjust[nbr_of_plots_adjust-1].plot.distance           = 0;

  Serial.print("\tEnd copy of ");
  Serial.print(nbr_of_plots_adjust);
  Serial.print(" 'PlotsAdjust'\n");
}

void Plots::getStartingPosition(COORD *o__coord)
{
  char l__buffer[80];

  /*  Starting Position:
   *  => Celle définie par les "#Start Latitude", "#Start Longitude" et "#Start Elevation" si correctes
   *  => Sinon 1st position dans la liste des records
   */
  float l__lat = m__nan;
  float l__lon = m__nan;
  float l__ele = m__nan;

  if (plotStartPosition.lat == plotStartPosition.lat
   && plotStartPosition.lon == plotStartPosition.lon
   && plotStartPosition.ele == plotStartPosition.ele) {
    l__lat = plotStartPosition.lat;
    l__lon = plotStartPosition.lon;
    l__ele = plotStartPosition.ele;
  }
  else {
    l__lat = plotRecord[0].plot.lat;
    l__lon = plotRecord[0].plot.lon;
    l__ele = plotRecord[0].plot.ele;
  }

  o__coord->lat = l__lat;
  o__coord->lon = l__lon;
  o__coord->ele = l__ele;

  sprintf(l__buffer, "Plots::getStartingPosition(): Lat [");
  Serial.print(l__buffer);
  Serial.print(l__lat, 6);
  sprintf(l__buffer, "] Lon [");
  Serial.print(l__buffer);
  Serial.print(l__lon, 6);
  sprintf(l__buffer, "] Ele [");
  Serial.print(l__buffer);
  Serial.print(l__ele, 0);
  sprintf(l__buffer, "]\n");
  Serial.print(l__buffer);
}

uint32_t Plots::calculateDistance(COORD *i__previous, COORD *i__current)
{
  /*  Distance en mètres des coordonnées courantes aux coordonnées passées en argument
   *  => Remarques: - L'élévation n'est pas prise en compte (pas significative: TBC) relative
   *                - La correction est appliquée a la moyenne des latitudes 'from' -> 'to'
   */
  float l__lat_from = i__previous->lat;
  float l__lon_from = i__previous->lon;
  float l__lat_to   = i__current->lat;
  float l__lon_to   = i__current->lon;

  float l__correction = cos(2.0 * M_PI * ((l__lat_from + l__lat_to) / 2.0) / 360.0);
  float l__distlat = 60.0 * 1852.0 * (l__lat_from - l__lat_to);
  float l__distlon = 60.0 * 1852.0 * (l__lon_from - l__lon_to) * l__correction;

  float l__distance = sqrt(l__distlat * l__distlat + l__distlon * l__distlon);

  return (uint32_t)l__distance;
}

float Plots::calculateCap(COORD *i__previous, COORD *i__current)
{
  char l__buffer[64];
  COORD l__wrk;

  /* Point a la même latitude que le poit 'previous' et situe a la même longitude que le point 'current'
   * -> Permet de calculer la distance sur le méridien (distance_lat
   */
  l__wrk.lat = i__previous->lat;
  l__wrk.lon = i__current->lon;
  uint32_t l__dist_lon = calculateDistance(i__previous, &l__wrk);

  /* Point a la même longitude que l point 'previous' et situe a la même latitude qe le point 'current'
   * -> Permet de calculer la distance sur la longitude (distance_lon)
   */
  l__wrk.lat = i__current->lat;
  l__wrk.lon = i__previous->lon;
  uint32_t l__dist_lat = calculateDistance(i__previous, &l__wrk);

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
    sprintf(l__buffer, "Error: Plots::calculateCap: Wrong 'l__alpha_deg' [");

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

    return m__nan;
  }

  float l__cap = l__alpha_deg;

  // Application de l'offset suivant les positions relatives dans les 4 cadrans
  if (i__previous->lat > i__current->lat && i__previous->lon >= i__current->lon) {
    l__cap = 270.0 - l__alpha_deg;
  }
  else if (i__previous->lat <= i__current->lat && i__previous->lon >= i__current->lon) {
    l__cap = 270.0 + l__alpha_deg;
  }
  else if (i__previous->lat <= i__current->lat && i__previous->lon < i__current->lon) {
    l__cap = 90.0 - l__alpha_deg;
  }
  else if (i__previous->lat > i__current->lat && i__previous->lon < i__current->lon) {
    l__cap = 90.0 + l__alpha_deg;
  }
  else {
    /* Ne doit jamais arriver
     * => Return 'nan'
     */
    sprintf(l__buffer, "Error: Plots::calculateCap(): No test found\n");
#ifndef USE_SIMULATION
    Serial.print(l__buffer);
#else
    printf(l__buffer);
#endif
  }

  return l__cap;
}

/* Arrondi du cap
   => [0;15[    -> NUM_CAP_MIDDAY (0 degrees)
   => [15;45[   -> NUM_CAP_13H    (30 degrees)
   => [45;75[   -> NUM_CAP_14H    (60 degrees)
   => [75;105[  -> NUM_CAP_15H    (90 degrees)
   => [105;135[ -> NUM_CAP_16H    (120 degrees)
   => [135;165[ -> NUM_CAP_17H    (150 degrees)
   => [165;195[ -> NUM_CAP_6H     (180 degrees)
   => [195;225[ -> NUM_CAP_7H     (210 degrees)
   => [225;255[ -> NUM_CAP_8H     (240 degrees)
   => [255;285[ -> NUM_CAP_9H     (270 degrees)
   => [285;345[ -> NUM_CAP_10H    (300 degrees)
   => [345;360[ -> NUM_CAP_11H    (330 degrees)
*/
int Plots::calculateCapHours(float i__cap)
{
  int l__cap = -1;

  if (i__cap < 0.0 || i__cap > CAP_MAX) {
#ifndef USE_SIMULATION
    Serial.print("Error: Plots::calculateCapHours(): Invalid cap value\n");
#else
    printf("Error: Plots::calculateCapHours(): Invalid cap value\n");
#endif
  }
  else {
    if (i__cap >= CAP_MIN && i__cap < THRESHOLD_CAP_13H) {
      l__cap = CAP_MIDDAY;
    }
    else if (i__cap >= THRESHOLD_CAP_13H && i__cap < THRESHOLD_CAP_14H) {
      l__cap = (CAP_MIDDAY + 1);
    }
    else if (i__cap >= THRESHOLD_CAP_14H  && i__cap < THRESHOLD_CAP_15H) {
      l__cap = (CAP_MIDDAY + 2);
    }
    else if (i__cap >= THRESHOLD_CAP_15H && i__cap < THRESHOLD_CAP_16H) {
      l__cap = (CAP_MIDDAY + 3);
    }
    else if (i__cap >= THRESHOLD_CAP_16H   && i__cap < THRESHOLD_CAP_17H) {
      l__cap = (CAP_MIDDAY + 4);
    }
    else if (i__cap >= THRESHOLD_CAP_17H && i__cap < THRESHOLD_CAP_6H) {
      l__cap = (CAP_MIDDAY + 5);
    }
    else if (i__cap >= THRESHOLD_CAP_6H  && i__cap < THRESHOLD_CAP_7H) {
      l__cap = (CAP_MIDDAY + 6 - 12);
    }
    else if (i__cap >= THRESHOLD_CAP_7H && i__cap < THRESHOLD_CAP_8H) {
      l__cap = (CAP_MIDDAY + 7 - 12);
    }
    else if (i__cap >= THRESHOLD_CAP_8H   && i__cap < THRESHOLD_CAP_9H) {
      l__cap = (CAP_MIDDAY + 8 - 12);
    }
    else if (i__cap >= THRESHOLD_CAP_9H && i__cap < THRESHOLD_CAP_10H) {
      l__cap = (CAP_MIDDAY + 9 - 12);
    }
    else if (i__cap >= THRESHOLD_CAP_10H  && i__cap < THRESHOLD_CAP_11H) {
      l__cap = (CAP_MIDDAY + 10 - 12);
    }
    else if (i__cap >= THRESHOLD_CAP_11H && i__cap < THRESHOLD_CAP_MIDDAY) {
      l__cap = (CAP_MIDDAY + 11 - 12);
    }
    else if (i__cap >= THRESHOLD_CAP_MIDDAY && i__cap <= CAP_MAX) {
      l__cap = CAP_MIDDAY;
    }
  }

  return l__cap;
}

void Plots::adjustmentCapAndDistance()
{
  Serial.print("adjustmentCapAndDistance(): Entering...\n");

  char l__buffer[80];

  // Ajustement des distances
  COORD l__previous;
  l__previous.lat = plotRecord[0].plot.lat;
  l__previous.lon = plotRecord[0].plot.lon;

  // A priori 1st plot valide pour le suivi du tracé
  plotRecord[0].plot.flgValid = true;

  size_t n = 0;
  for (n = 1; n < (size_t)nbr_of_plots; n++) {
    COORD l__current;
    l__current.lat = plotRecord[n].plot.lat;
    l__current.lon = plotRecord[n].plot.lon;

    // Distance maj a la position du 'previous' pour indiquer "... sur xxx mètres"
    plotRecord[n-1].plot.distance = calculateDistance(&l__previous, &l__current);

    // A priori plot valide pour le suivi du trace sauf le dernier
    if (n < (size_t)nbr_of_plots) {
      plotRecord[n-1].plot.flgValid = true;
    }

    // Preparation prochain calcul
    l__previous.lat = l__current.lat;
    l__previous.lon = l__current.lon;
  }

  /* Application de la distance minimale prise en compte entre 2 plots
     => Report sur le plot precedent et 
  */
  boolean l__flg_break = false;
  int l__pass = 0;

  sprintf(l__buffer, "adjustmentCapAndDistance(): Apply minimal distance with [%d]\n", DISTANCE_MIN_FOR_ADJUST);
  Serial.print(l__buffer);

  do {
    l__flg_break = true;

#ifndef USE_SIMULATION
    sprintf(l__buffer, "adjustmentCapAndDistance(): Pass %d...\n", l__pass);
    Serial.print(l__buffer);
#else
    printf("adjustmentCapAndDistance(): Pass %d...\n", l__pass);
#endif

    for (n = 0; n < ((size_t)nbr_of_plots - 1); n++) {
      if (plotRecord[n].plot.flgValid == true) {
        if (plotRecord[n].plot.distance < DISTANCE_MIN_FOR_ADJUST) {
          size_t m = 0;
          for (m = (n+1); m < ((size_t)nbr_of_plots - 1); m++) {
            if (plotRecord[m].plot.flgValid == true) {
              plotRecord[n].plot.distance += plotRecord[m].plot.distance;

              plotRecord[m].plot.distance = 0;
              plotRecord[m].plot.flgValid = false;

              l__flg_break = false;

              break;
            }
          }
        }
      }
    }

    l__pass++;
  }
  while (l__flg_break == false);

  // Calculate the new distances after adjustement
  uint32_t l__total_distance = 0;

  l__previous.lat = plotRecord[0].plot.lat;
  l__previous.lon = plotRecord[0].plot.lon;

  for (n = 1; n < (size_t)nbr_of_plots; n++) {
    if (plotRecord[n].plot.flgValid == true) {
      COORD l__current;
      l__current.lat = plotRecord[n].plot.lat;
      l__current.lon = plotRecord[n].plot.lon;

      l__total_distance += calculateDistance(&l__previous, &l__current);

      // Preparation prochain calcul
      l__previous.lat = l__current.lat;
      l__previous.lon = l__current.lon;
    }
  }

#ifndef USE_SIMULATION
  sprintf(l__buffer, "Total distance after adjustment [%d]\n", l__total_distance);
  Serial.print(l__buffer);
#else
  printf("Total distance after adjustment [%d]\n", l__total_distance);
#endif
  // Fin: Ajustement des distances

  // Ajustement des caps
  l__previous.lat = plotRecord[0].plot.lat;
  l__previous.lon = plotRecord[0].plot.lon;

  size_t n__previous = 0;
  for (n = 1; n < (size_t)nbr_of_plots; n++) {
    if (plotRecord[n].plot.flgValid == true || n == ((size_t)nbr_of_plots) - 1) {
      COORD l__current;
      l__current.lat = plotRecord[n].plot.lat;
      l__current.lon = plotRecord[n].plot.lon;

      float l__cap = calculateCap(&l__previous, &l__current);

      plotRecord[n__previous].plot.capToNextPlot      = l__cap;
      plotRecord[n__previous].plot.capHoursToNextPlot = calculateCapHours(l__cap);

      // Preparation prochain calcul
      l__previous.lat = l__current.lat;
      l__previous.lon = l__current.lon;
      n__previous = n;
    }
  }
  // Fin: Ajustement des caps
}

bool Plots::adjustmentCapAndDistanceMore()
{
  Serial.print("adjustmentCapAndDistanceMore(): Entering...\n");

  char l__buffer[80];
  bool l__rtn = true;

  uint32_t l__total_dist = 0;
  int l__plot = 0;
  int l__num_plot = 0;
  for (l__num_plot = 0; l__num_plot < nbr_of_plots; l__num_plot++) {
    ST_PLOT_RECORD l__plot_record;
    memset(&l__plot_record, '\0', sizeof(ST_PLOT_RECORD));

    getPlotRecord(l__num_plot, &l__plot_record);

    if (l__plot_record.plot.flgValid == false) {
      continue;
    }

    float l__capToNextPlot = l__plot_record.plot.capToNextPlot;

    // Add 'CAP_MAX' = 360.0 si dans [CAP_MIN; THRESHOLD_CAP_13H[
    if (l__capToNextPlot >= CAP_MIN && l__capToNextPlot < THRESHOLD_CAP_13H) {
      l__capToNextPlot += CAP_MAX;
    }

    int      l__capHoursToNextPlot = l__plot_record.plot.capHoursToNextPlot;
    uint32_t l__distance           = l__plot_record.plot.distance;

    int l__num_plot_more = 0;
    int l__nbr_plot_more = 0;

    for (l__num_plot_more = (l__num_plot + 1); l__num_plot_more < nbr_of_plots; l__num_plot_more++) {
      ST_PLOT_RECORD l__plot_record_more;
      memset(&l__plot_record_more, '\0', sizeof(ST_PLOT_RECORD));

      getPlotRecord(l__num_plot_more, &l__plot_record_more);

      if (l__plot_record_more.plot.flgValid == true) {
        if (l__plot_record_more.plot.capHoursToNextPlot == l__capHoursToNextPlot) {
          // Cumul des distances
          l__distance += l__plot_record_more.plot.distance;

          // Moyenne du Cap reel
          l__nbr_plot_more++;

          // Add 'CAP_MAX' = 360.0 si dans [CAP_MIN; THRESHOLD_CAP_13H[
          float l__cap_offset = 0.0;
          if (l__plot_record_more.plot.capToNextPlot >= CAP_MIN && l__plot_record_more.plot.capToNextPlot < THRESHOLD_CAP_13H) {
            l__cap_offset = CAP_MAX;
          }

          l__capToNextPlot =
            (l__nbr_plot_more * l__capToNextPlot + (l__plot_record_more.plot.capToNextPlot + l__cap_offset)) / (l__nbr_plot_more + 1);

          l__num_plot = l__num_plot_more;   // Saut des plots pris en compte
        }
        else {
          break;
        }
      }
    }

    // Sub 'CAP_MAX' = 360.0 si > 'CAP_MAX'
    if (l__capToNextPlot >= CAP_MAX) {
      l__capToNextPlot -= CAP_MAX;
    }

    l__total_dist += l__distance;

#if 0
    printf("#%04d: Cap [%5.1f] [%d] Dist [%3d] Avg Cap [%5.1f] [%d] (%d samples) (%s)\n",
      l__plot,
      l__plot_record.plot.capToNextPlot, l__plot_record.plot.capHoursToNextPlot,
      l__distance,
      l__capToNextPlot, calculateCapHours(l__capToNextPlot), l__nbr_plot_more + 1,
      (l__plot_record.plot.capHoursToNextPlot == calculateCapHours(l__capToNextPlot)) ? "Ok" : "Ko");
#endif

    if (l__plot_record.plot.capHoursToNextPlot != calculateCapHours(l__capToNextPlot)) {
#ifndef USE_SIMULATION
      sprintf(l__buffer, "Error: #%d: Cap hours %d ! %d\n",
        l__plot, l__plot_record.plot.capHoursToNextPlot, calculateCapHours(l__capToNextPlot));
      Serial.print(l__buffer);
#else
      printf("Error: #%d: Cap hours %d ! %d\n",
        l__plot, l__plot_record.plot.capHoursToNextPlot, calculateCapHours(l__capToNextPlot));
#endif

      l__rtn = false;
    }
    else if (l__distance < DISTANCE_MIN_FOR_ADJUST) {
#ifndef USE_SIMULATION
      sprintf(l__buffer, "Error: #%d: Distance %d < %d\n", l__plot, l__distance, DISTANCE_MIN_FOR_ADJUST);
      Serial.print(l__buffer);
#else
      printf("Error: #%d: Distance %d < %d\n", l__plot, l__distance, DISTANCE_MIN_FOR_ADJUST);
#endif

      l__rtn = false;
    }
    else {
         ST_PLOT_RECORD *l__p_record_adjust = &plotRecordAdjust[l__plot];
         l__p_record_adjust->idx = l__plot;
         l__p_record_adjust->plot.lat = l__plot_record.plot.lat;
         l__p_record_adjust->plot.lon = l__plot_record.plot.lon;
         l__p_record_adjust->plot.ele = l__plot_record.plot.ele;
         l__p_record_adjust->plot.capToNextPlot = l__capToNextPlot;
         l__p_record_adjust->plot.flgValid = true;
         l__p_record_adjust->plot.capHoursToNextPlot = l__plot_record.plot.capHoursToNextPlot;
         l__p_record_adjust->plot.distance = l__distance;

         nbr_of_plots_adjust++;
    }

    l__plot++;
  }

  /* Forcage m.a.j du dernier 'plotRecordAdjust[]' avec le dernier 'plotRecord[]'
   * => Seuls les champs 'lat, 'lon' et 'ele' sont utilisés
   */
  ST_PLOT_RECORD l__plot_last_record;
  memset(&l__plot_last_record, '\0', sizeof(ST_PLOT_RECORD));

  getPlotRecord((nbr_of_plots - 1), &l__plot_last_record);

  plotRecordAdjust[nbr_of_plots_adjust].idx = l__plot;
  plotRecordAdjust[nbr_of_plots_adjust].plot.lat = l__plot_last_record.plot.lat;
  plotRecordAdjust[nbr_of_plots_adjust].plot.lon = l__plot_last_record.plot.lon;
  plotRecordAdjust[nbr_of_plots_adjust].plot.ele = l__plot_last_record.plot.ele;
  plotRecordAdjust[nbr_of_plots_adjust].plot.capToNextPlot = 0.0;             // Init "au Nord"
  plotRecordAdjust[nbr_of_plots_adjust].plot.flgValid = true;
  plotRecordAdjust[nbr_of_plots_adjust].plot.capHoursToNextPlot = 12;         // Init "à midi"
  plotRecordAdjust[nbr_of_plots_adjust].plot.distance = 0;

#ifndef USE_SIMULATION
  sprintf(l__buffer, "Total distance [%d]\n", l__total_dist);
  Serial.print(l__buffer);
#else
  printf("Total distance [%d]\n", l__total_dist);
#endif

#ifndef USE_SIMULATION
  // Add properties of the plots
  g__pilot->setLat(COORD_END_POS, plotRecordAdjust[nbr_of_plots_adjust].plot.lat);
  g__pilot->setLon(COORD_END_POS, plotRecordAdjust[nbr_of_plots_adjust].plot.lon);
  g__pilot->setEle(COORD_END_POS, plotRecordAdjust[nbr_of_plots_adjust].plot.ele);
#endif

  nbr_of_plots_adjust++;    // Update after the last use

  total_distance = l__total_dist;

#ifndef USE_SIMULATION
  Serial.print("Properties of plots:\n");
  sprintf(l__buffer, "\tEnd position of %d adjust plots:\n", nbr_of_plots_adjust);
  Serial.print(l__buffer);
  Serial.print("\t\tLat.  [");
  Serial.print(g__pilot->getLat(COORD_END_POS), 6);
  Serial.print("] degrees\n");
  Serial.print("\t\tLon.  [");
  Serial.print(g__pilot->getLon(COORD_END_POS), 6);
  Serial.print("] degrees\n");
  Serial.print("\t\tEle.  [");
  Serial.print(g__pilot->getEle(COORD_END_POS), 0);
  Serial.print("] M\n");
#endif

  Serial.print("\t\tTotal dist. of adjust plots: [");
  Serial.print(getTotalDistance());
  Serial.print("] M\n");

  const char *l__text = "Unknown";
  plots_direction = PLOTS_DIR_UNKNOWN;
  if (nbr_of_plots > 2) {
    if (plotRecord[0].idx < plotRecord[nbr_of_plots - 1].idx) {
      l__text = "Aller";
      plots_direction = PLOTS_DIR_GO;
    }
    else {
      l__text = "Retour";
      plots_direction = PLOTS_DIR_RETURN;
    }
  }
  sprintf(l__buffer, "\t\tSens [%s]\n", l__text);
  Serial.print(l__buffer);

  // Synthesis of properties ... if "Connected"
  flg_make_synth_properties = true;

  // End: Add properties of the plots

  // Fin: Forcage m.a.j du dernier 'plotRecordAdjust[]' avec le dernier 'plotRecord[]'

  sprintf(l__buffer, "adjustmentCapAndDistanceMore(): Leave: %s\n", (l__rtn == true) ? "Ok" : "Ko");
  Serial.print(l__buffer);

  return l__rtn;
}

void Plots::calculOfCapsAndDistance()
{
  uint32_t l__total_dist = 0;

  // Recalcul des Index, Caps et Distance sauf pour dernier 'PlotAdjust'
  int n = 0;
  for (n = 0; n < (nbr_of_plots_adjust -1); n++) {
    COORD l__from;
    l__from.lat = plotRecordAdjust[n].plot.lat;
    l__from.lon = plotRecordAdjust[n].plot.lon;

    COORD l__to;
    l__to.lat = plotRecordAdjust[n+1].plot.lat;
    l__to.lon = plotRecordAdjust[n+1].plot.lon;

    plotRecordAdjust[n].idx                     = n;
    plotRecordAdjust[n].plot.capToNextPlot      = calculateCap(&l__from, &l__to);
    plotRecordAdjust[n].plot.capHoursToNextPlot = calculateCapHours(plotRecordAdjust[n].plot.capToNextPlot);
    plotRecordAdjust[n].plot.distance           = calculateDistance(&l__from, &l__to);

    l__total_dist += plotRecordAdjust[n].plot.distance;
  }

  // Init. Caps and Distance of the last 'PlotsAdjust' to zero
  plotRecordAdjust[nbr_of_plots_adjust-1].plot.capToNextPlot      = 0.0;
  plotRecordAdjust[nbr_of_plots_adjust-1].plot.capHoursToNextPlot = 0;
  plotRecordAdjust[nbr_of_plots_adjust-1].plot.distance           = 0;

  total_distance = l__total_dist;

  // Sens du tracé @ index
  plots_direction = PLOTS_DIR_UNKNOWN;
  if (nbr_of_plots_adjust > 2) {
    if (plotRecordAdjust[0].idx < plotRecordAdjust[nbr_of_plots_adjust - 1].idx) {
      plots_direction = PLOTS_DIR_GO;
    }
    else {
      plots_direction = PLOTS_DIR_RETURN;
    }
  }
  // Fin: Sens du tracé @ index

  // Synthesis of properties ... if "Connected"
  flg_make_synth_properties = true;
}

// Methods for desmos calculation
void Plots::desmosCalculOfRelativeLatLon()
{
  Serial.print("desmosCalculOfRelativeLatLon(): Entering...\n");

#if 0
   if (plotRecordAdjust[0].plot.flgValid == true) {
     plotRecordAdjust[0].plot.lat_y = 0;
     plotRecordAdjust[0].plot.lon_x = 0;
   }
#endif

   size_t n = 0;
   for (n = 0; n < (size_t)nbr_of_plots_adjust; n++) {
      if (plotRecordAdjust[n].plot.flgValid == true) {
        COORD l__previous;
        COORD l__current;

        // Calcul of relative Latitude @ 1st jonction
        l__previous.lat = plotRecordAdjust[0].plot.lat;
        l__previous.lon = plotRecordAdjust[0].plot.lon;

        l__current.lat = plotRecordAdjust[n].plot.lat;
        l__current.lon = l__previous.lon;

        uint32_t l__y = calculateDistance(&l__previous, &l__current);
        plotRecordAdjust[n].desmos.lat_y = (l__current.lat >= l__previous.lat) ? l__y: -l__y;
        // End: Calcul of relative Latitude @ 1st jonction

        // Calcul of relative Longitude @ 1st jonction
        l__previous.lat = plotRecordAdjust[0].plot.lat;
        l__previous.lon = plotRecordAdjust[0].plot.lon;

        l__current.lat = l__previous.lat;
        l__current.lon = plotRecordAdjust[n].plot.lon;

        uint32_t l__x = calculateDistance(&l__previous, &l__current);
        plotRecordAdjust[n].desmos.lon_x = (l__current.lon >= l__previous.lon) ? l__x: -l__x;
        // End: Calcul of relative Longitude @ 1st jonction
     }
   }
}

// Calcul of relative Latitude @ 1st jonction
int32_t Plots::desmosCalculOfRelativeLat(float i__lat)
{
   COORD l__previous;
   COORD l__current;

   l__previous.lat = plotRecordAdjust[0].plot.lat;
   l__previous.lon = plotRecordAdjust[0].plot.lon;

   l__current.lat = i__lat;
   l__current.lon = l__previous.lon;

   uint32_t l__y = calculateDistance(&l__previous, &l__current);
   return (l__current.lat >= l__previous.lat) ? l__y: -l__y;
}

// Calcul of relative Longitude @ 1st jonction
int32_t Plots::desmosCalculOfRelativeLon(float i__lon)
{
   COORD l__previous;
   COORD l__current;

   l__previous.lat = plotRecordAdjust[0].plot.lat;
   l__previous.lon = plotRecordAdjust[0].plot.lon;

   l__current.lat = l__previous.lat;
   l__current.lon = i__lon;

   uint32_t l__x = calculateDistance(&l__previous, &l__current);
   return (l__current.lon >= l__previous.lon) ? l__x: -l__x;
}

// Calcul of latitude from 'lat_y'
void Plots::convertLonXLatYToLatLon(COORD *o__coord, int32_t i__lat_y, int32_t i__lon_x)
{
   COORD l__coord;

   /* Inverse of: float l__distlat = 60.0 * 1852.0 * (l__lat_from - l__lat_to);
      with 'l__lat_from' the latitude of 1st jonction and 'l__lat_to' the latitude to calculated
   */
   l__coord.lat = plotRecordAdjust[0].plot.lat + (i__lat_y / (60.0 * 1852.0));

   /* Inverse of:
      float l__correction = cos(2.0 * M_PI * ((l__lat_from + l__lat_to) / 2.0) / 360.0);
      float l__distlon = 60.0 * 1852.0 * (l__lon_from - l__lon_to) * l__correction;

      with 'l__lon_from' the longitude of 1st jonction and 'l__lon_to' the longitude to calculated
   */
   float l__correction = cos(2.0 * M_PI * ((plotRecordAdjust[0].plot.lat + l__coord.lat) / 2.0) / 360.0); 
   l__coord.lon = plotRecordAdjust[0].plot.lon + (i__lon_x / (60.0 * 1852.0 * l__correction));

   memcpy(o__coord, &l__coord, sizeof(COORD));
}

void Plots::desmosCalculOfA()
{
    Serial.print("desmosCalculOfA(): Entering...\n");

   size_t n = 0;
   for (n = 0; n < ((size_t)nbr_of_plots_adjust - 1); n++) {
      if (plotRecordAdjust[n].plot.flgValid == true && plotRecordAdjust[n+1].plot.flgValid == true) {
         plotRecordAdjust[n].desmos.A = 
            (plotRecordAdjust[n+1].desmos.lon_x * plotRecordAdjust[n].desmos.lat_y
           - plotRecordAdjust[n].desmos.lon_x * plotRecordAdjust[n+1].desmos.lat_y);
      }
   }
}

void Plots::desmosCalculOfB(int i__idx, int32_t l__x, int32_t l__y)
{
   if (i__idx < (nbr_of_plots_adjust - 1)
    && plotRecordAdjust[i__idx].plot.flgValid == true && plotRecordAdjust[i__idx+1].plot.flgValid == true) {

      if (l__x != INT_MAX && l__y != INT_MAX) {
         plotRecordAdjust[i__idx].desmos.B =
            (plotRecordAdjust[i__idx+1].desmos.lon_x - plotRecordAdjust[i__idx].desmos.lon_x) * l__x
          + (plotRecordAdjust[i__idx+1].desmos.lat_y - plotRecordAdjust[i__idx].desmos.lat_y) * l__y;
      }
   }
}

void Plots::desmosCalculOfDs(int i__idx, int32_t l__x, int32_t l__y)
{
   if (i__idx < (nbr_of_plots_adjust - 1)
    && plotRecordAdjust[i__idx].plot.flgValid == true && plotRecordAdjust[i__idx+1].plot.flgValid == true) {

      if (l__x != INT_MAX && l__y != INT_MAX) {
         float l__D =
            (plotRecordAdjust[i__idx+1].desmos.lat_y - plotRecordAdjust[i__idx].desmos.lat_y) * l__x
          - (plotRecordAdjust[i__idx+1].desmos.lon_x - plotRecordAdjust[i__idx].desmos.lon_x) * l__y
          +  plotRecordAdjust[i__idx].desmos.A;

         float l__x2 = (plotRecordAdjust[i__idx+1].desmos.lon_x - plotRecordAdjust[i__idx].desmos.lon_x);
         l__x2 *= l__x2;

         float l__y2 = (plotRecordAdjust[i__idx+1].desmos.lat_y - plotRecordAdjust[i__idx].desmos.lat_y);
         l__y2 *= l__y2;

         l__D /= sqrt(l__x2 + l__y2);
         l__D += (l__D < 0.0 ? -0.5 : 0.5);   // Arrondi

         plotRecordAdjust[i__idx].desmos.Ds = (int32_t)(l__D);
      }
   }
}

/* - Distance a la jonction
   - Distance minimale a la jonction
*/
void Plots::desmosCalculOfDj(int i__idx, int32_t l__x, int32_t l__y)
{
   if (i__idx < nbr_of_plots_adjust && plotRecordAdjust[i__idx].plot.flgValid == true) {
      if (l__x != INT_MAX && l__y != INT_MAX) {
         float l__x2 = (plotRecordAdjust[i__idx].desmos.lon_x - l__x);
         l__x2 *= l__x2;

         float l__y2 = (plotRecordAdjust[i__idx].desmos.lat_y - l__y);
         l__y2 *= l__y2;

         float l__D = sqrt(l__x2 + l__y2);
         l__D += (l__D < 0.0 ? -0.5 : 0.5);   // Arrondi

         plotRecordAdjust[i__idx].desmos.Dj = (int32_t)(l__D);

         // Distance minimale a la jonction
         if (plotRecordAdjust[i__idx].desmos.Dj < plotResults.Dj_min) {
            plotResults.Dj_idx = i__idx;
            plotResults.Dj_min = plotRecordAdjust[i__idx].desmos.Dj;
         }
      }
   }
}

// Abscisse et ordonnee du point d'intersection sur le segment
void Plots::desmosCalculOfXY(int i__idx)
{
   if (i__idx < (nbr_of_plots_adjust - 1)
    && plotRecordAdjust[i__idx].plot.flgValid == true && plotRecordAdjust[i__idx+1].plot.flgValid == true) {

      float l__x =
         (plotRecordAdjust[i__idx+1].desmos.lon_x - plotRecordAdjust[i__idx].desmos.lon_x) * (float)plotRecordAdjust[i__idx].desmos.B
       - (plotRecordAdjust[i__idx+1].desmos.lat_y - plotRecordAdjust[i__idx].desmos.lat_y) * (float)plotRecordAdjust[i__idx].desmos.A;

      float l__y =
         (plotRecordAdjust[i__idx+1].desmos.lon_x - plotRecordAdjust[i__idx].desmos.lon_x) * (float)plotRecordAdjust[i__idx].desmos.A
       + (plotRecordAdjust[i__idx+1].desmos.lat_y - plotRecordAdjust[i__idx].desmos.lat_y) * (float)plotRecordAdjust[i__idx].desmos.B;

      float l__x2 = (plotRecordAdjust[i__idx+1].desmos.lon_x - plotRecordAdjust[i__idx].desmos.lon_x);
      l__x2 *= l__x2;

      float l__y2 = (plotRecordAdjust[i__idx+1].desmos.lat_y - plotRecordAdjust[i__idx].desmos.lat_y);
      l__y2 *= l__y2;

      plotRecordAdjust[i__idx].desmos.X = l__x / (l__x2 + l__y2);
      plotRecordAdjust[i__idx].desmos.Y = l__y / (l__x2 + l__y2);
   }
}

/* - Produit scalaire pour une distance signee et valide au point d'intersection sur le segment
   - Distance minimale au segment si point "a l'interieur" du segment
*/
void Plots::desmosCalculOfPs(int i__idx, int32_t l__x, int32_t l__y)
{
   if (i__idx < (nbr_of_plots_adjust - 1)
    && plotRecordAdjust[i__idx].plot.flgValid == true && plotRecordAdjust[i__idx+1].plot.flgValid == true) {

      plotRecordAdjust[i__idx].desmos.Ps = false;

      float l__ps_x =
         (plotRecordAdjust[i__idx].desmos.X - plotRecordAdjust[i__idx].desmos.lon_x)
       * (plotRecordAdjust[i__idx].desmos.X - plotRecordAdjust[i__idx + 1].desmos.lon_x);

      if (l__ps_x < 0.0) {
         plotRecordAdjust[i__idx].desmos.Ps = true;
      }

      float l__ps_y =
         (plotRecordAdjust[i__idx].desmos.Y - plotRecordAdjust[i__idx].desmos.lat_y)
       * (plotRecordAdjust[i__idx].desmos.Y - plotRecordAdjust[i__idx + 1].desmos.lat_y);

      if (l__ps_y < 0.0) {
         plotRecordAdjust[i__idx].desmos.Ps = true;
      }

      // Distance minimale au segment si point "a l'interieur" du segment
      int32_t l__Ds = plotRecordAdjust[i__idx].desmos.Ds;
      if (l__Ds < 0) {
         l__Ds = -l__Ds;
      }

      if (plotRecordAdjust[i__idx].desmos.Ps == true && l__Ds < plotResults.Ds_min) {
         plotResults.Ds_idx = i__idx;
         plotResults.Ds_min = l__Ds;
      }
   }
}

void Plots::clearResults()
{
   plotResults.flg_avalaible = false;

   plotResults.Ds_idx = -1;       // Index dans la liste 'plotRecordAdjust' (donne le numero du segment [0..N-2] -> [1..N-1])
   plotResults.Dj_idx = -1;       // Index dans la liste 'plotRecordAdjust' (donne le numero de la jonction [0..N-1]
   plotResults.Ds_min = INT_MAX;  // Distance minimale non signee au segment ou INT_MAX
   plotResults.Dj_min = INT_MAX;  // Distance minimale non signee a la jonction ou INT_MAX

   plotResults.flg_seg_jonc = TYPE_SEG_JONC_UNKNOWN;
   plotResults.idx          = -1;
   plotResults.D_min        = INT_MAX;  
   plotResults.X            = INT_MAX;  // Abscisse et
   plotResults.Y            = INT_MAX;  // Ordonnee du point d'intersection sur le segment ou coordonnee de la jonction

   initToNaN(&plotResults.lat);
   initToNaN(&plotResults.lon);
}

/* Finalise les resultats:
   - Si 'Dj' (qui est toujours disponible) est < a 'Ds'
     => La distance 'Dj' est choisie avec son index et les coordonnees de la jonction
   - Sinon la distance 'Ds' (qui doit exister normalement) est choisie avec son index
     et les coordonnees du point d'intersection avec le segment
   - Renseignement des informations 'type_synth', 'distance' et 'cap' pour la synthèse
*/
void Plots::finalyzeResults()
{
   plotResults.flg_seg_jonc = TYPE_SEG_JONC_UNKNOWN;

   if (plotResults.Dj_idx >= 0 && plotResults.Dj_idx < nbr_of_plots_adjust
    && plotResults.Dj_min <= plotResults.Ds_min) {

      plotResults.flg_seg_jonc = TYPE_JONCTION;
      plotResults.idx          = plotResults.Dj_idx;
      plotResults.D_min        = plotResults.Dj_min;

      plotResults.X            = plotRecordAdjust[plotResults.idx].desmos.lon_x;
      plotResults.Y            = plotRecordAdjust[plotResults.idx].desmos.lat_y;

      COORD l__coord;
      convertLonXLatYToLatLon(&l__coord, plotResults.Y, plotResults.X);
      plotResults.lat = l__coord.lat;
      plotResults.lon = l__coord.lon;
   }
   else if (plotResults.Ds_idx >= 0 && plotResults.Ds_idx < (nbr_of_plots_adjust - 1)
         && plotRecordAdjust[plotResults.Ds_idx].desmos.Ps == true) {

      plotResults.flg_seg_jonc = TYPE_SEGMENT; 
      plotResults.idx          = plotResults.Ds_idx;
      plotResults.D_min        = plotResults.Ds_min;

      plotResults.X            = (int32_t)plotRecordAdjust[plotResults.idx].desmos.X;
      plotResults.Y            = (int32_t)plotRecordAdjust[plotResults.idx].desmos.Y;

      COORD l__coord;
      convertLonXLatYToLatLon(&l__coord, plotResults.Y, plotResults.X);
      plotResults.lat = l__coord.lat;
      plotResults.lon = l__coord.lon;
   }

   // Informations pour la synthèse
   plotResults.type_synth = TYPE_SYNTH_NO_MEAN;

#if USE_ATTRACTOR_TO_PLOT
   /* Algorithme de lissage des distances et attraction sur le tracé:
      - Calcul de la moyenne des 'NBR_DIST_RESULT' dernieres distances
      - Si la distance est <= 15 M
        => Forcage a 0 comme une sorte d'attracteur de remise sur le trace
      - Permet de supprimer de courtes periodes "à coté" mais proches du tracé :-)
   */
   uint32_t l__dist_average = 0;     // A priori, sur le tracé

   if (plotResults.D_min <= DISTANCE_15M) {
     // Effacement des précédentes distances => Attraction sur le tracé
     memset(t_dist_avg, '\0', sizeof(t_dist_avg));
     idx_dist_avg = 0;
   }
   else {
     /* Ajout de cette distance à la précédente moyenne
      * => Lissage de l'éloignement
      *    => Moyenne limitée aux 'NBR_DIST_AVG' dernières distances
      */
     t_dist_avg[idx_dist_avg] = plotResults.D_min;
     idx_dist_avg = (idx_dist_avg + 1) % NBR_DIST_AVG;

     l__dist_average = t_dist_avg[0];
     size_t l__idx_dist_avg = 0;
     for (l__idx_dist_avg = 1; l__idx_dist_avg < NBR_DIST_AVG; l__idx_dist_avg++) {
       l__dist_average = ((l__dist_average * l__idx_dist_avg) + t_dist_avg[l__idx_dist_avg]) / (l__idx_dist_avg + 1);
     }
   }
   // Fin: Algorithme de lissage des distances et attraction sur le tracé
#endif

#if USE_ATTRACTOR_TO_PLOT
   uint32_t l__D_min = l__dist_average;

   plotResults.D_min_avg = l__D_min;        // Pour la synthèse "Vous êtes sur le tracé..." si proche du tracé ;-) 
#else
   uint32_t l__D_min = plotResults.D_min;
#endif

   uint32_t l__dist_rounded = distanceRoundedForThePlot(l__D_min);    // Distance au tracé approximée

   if (plotResults.idx >= nbr_of_plots_adjust) {
     Serial.print("Error: Plots::finalyzeResults: Index out of range (max)\n");
   }
   else if (plotResults.idx >= 0) {
     if (l__dist_rounded <= DISTANCE_FOR_ON_THE_PLOT) {
       // Cas si distance "proche" à un segment ou une jonction
       COORD l__from;
       COORD l__to;

       // Coordonnées réelle
       l__from.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
       l__from.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;

       l__to.lat   = plotRecordAdjust[plotResults.idx + 1].plot.lat;
       l__to.lon   = plotRecordAdjust[plotResults.idx + 1].plot.lon;

       plotResults.distance = calculateDistance(&l__from, &l__to);    // Distance à la jonction suivante

       if (plotResults.flg_seg_jonc == TYPE_SEGMENT) {
         if (plotResults.distance > DISTANCE_50M) {
           // Synthèse de "Vous êtes sur le tracé, continuez sur xxx mètres au 'cap' [à 'cap_hours' @ cap du déplacement]"
           plotResults.type_synth = TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1;
         }
         else if (plotResults.distance > DISTANCE_15M) {
           // Synthèse de "Vous êtes sur le tracé, préparez vous à bifurquer à yyy mètres au 'cap' [à 'cap_hours' @ cap du déplacement]"
           plotResults.type_synth = TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2;
         }
         else {
           /*  Point sur un segment et "proche" de la jonction suivante
            *  - Distance et cap du segment suivant
            *  - Synthèse de "Bifurquez maintenant au 'cap' [à 'cap_hours' @ cap du déplacement] et continuez sur xxx mètres"
            */
            plotResults.distance   = plotRecordAdjust[plotResults.idx + 1].plot.distance;
            plotResults.cap        = plotRecordAdjust[plotResults.idx + 1].plot.capToNextPlot;
            plotResults.type_synth = TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3;
         }
       }
       else if (plotResults.flg_seg_jonc == TYPE_JONCTION) {
          /*  Point "proche" de la jonction
           *  - Distance et cap du segment
           *  - Synthèse de "Bifurquez maintenant au 'cap' [à 'cap_hours' @ cap du déplacement] et continuez sur xxx mètres"
           */
          plotResults.distance   = plotRecordAdjust[plotResults.idx].plot.distance;
          plotResults.cap        = plotRecordAdjust[plotResults.idx].plot.capToNextPlot;
          plotResults.type_synth = TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3;
       }

       if (plotResults.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1 || plotResults.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2) {
          // Cap à suivre réellement déterminé par les positions géographiques
          COORD l__coord_from;
          COORD l__coord_to;

          l__coord_from.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
          l__coord_from.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
          l__coord_to.lat   = plotRecordAdjust[plotResults.idx + 1].plot.lat;
          l__coord_to.lon   = plotRecordAdjust[plotResults.idx + 1].plot.lon;

          /* Amélioration du Cap dans le cas 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2'
           * => Evite par exemple: "Préparez vous à bifurquer à 11 H" suivi d'un "Bifurquez maintenant à 13H"
           *    => Prendre le même cap que dans le cas 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3' c.a.d. le cap à suivre du prochain segment 'plotResults.idx + 1'
           */
          plotResults.cap = (plotResults.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1) ?
                                calculateCap(&l__coord_from, &l__coord_to) : plotRecordAdjust[plotResults.idx + 1].plot.capToNextPlot;
          // Fin: Amélioration du Cap dans le cas 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2'
       }
     }
     else {
       // Cas si distance "éloignée" à un segment ou une jonction
       plotResults.distance = l__D_min;    // Distance au segment ou à la jonction

       // Synthèse de "Le tracé est à xxx mètres au 'cap' [à 'cap_hours' @ cap du déplacement]"
       plotResults.type_synth = TYPE_SYNTH_THE_PLOT_IS;

       /* Cap à suivre réellement déterminé par les positions géographiques
          (coordonnées sur le segement (ou de la jonction) - coordonnées courantes)
       */
       COORD l__coord_from;
       COORD l__coord_to;
       l__coord_from.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
       l__coord_from.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
       l__coord_to.lat   = plotResults.lat;
       l__coord_to.lon   = plotResults.lon;

       plotResults.cap = calculateCap(&l__coord_from, &l__coord_to);
     }
   }
   else {
      Serial.print("Error: Plots::finalyzeResults: Index out of range (< 0)\n");
   }
   // Fin: Informations pour la synthèse

   /* Détermination des 2 distances restante et parcourue sur le tracé
      => "Bifurquez maintenant" ignoré dans le calcul mais pas dans la détermination)
    */
#define USE_DEBUG_CALCUL_DIST    0

   if (plotResults.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1
    || plotResults.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2
    || plotResults.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3)
   {
#if USE_DEBUG_CALCUL_DIST
      char l__buffer[64];
      Serial.print("Dist. on plots:\n");
#endif
      uint32_t l__distance_on_plots = 0;
      int l__idx = 0;
      for (l__idx = 0; l__idx < plotResults.idx && l__idx < nbr_of_plots_adjust; l__idx++) {
#if USE_DEBUG_CALCUL_DIST
        sprintf(l__buffer, "\t#%d: [%d] + [%d] = [%d] M\n", l__idx,
          l__distance_on_plots, plotRecordAdjust[l__idx].plot.distance,
          (l__distance_on_plots + plotRecordAdjust[l__idx].plot.distance));
        Serial.print(l__buffer);
#endif
        l__distance_on_plots += plotRecordAdjust[l__idx].plot.distance;
      }

      if (l__idx < nbr_of_plots_adjust) {
#if USE_DEBUG_CALCUL_DIST
        sprintf(l__buffer, "\t#%d: [%d] + [%d] - [%d] = [%d] M\n", l__idx,
          l__distance_on_plots, plotRecordAdjust[l__idx].plot.distance, plotResults.distance,
          (l__distance_on_plots + plotRecordAdjust[l__idx].plot.distance - plotResults.distance));
        Serial.print(l__buffer);
#endif
        l__distance_on_plots += (plotRecordAdjust[l__idx].plot.distance - plotResults.distance);
      }

      distance_on_plots = l__distance_on_plots;

#if USE_DEBUG_CALCUL_DIST
      sprintf(l__buffer, "\t\t=> [%d]\n", distance_on_plots);
      Serial.print(l__buffer);
   
      Serial.print("Dist. remaining:\n");
#endif

      uint32_t l__remaining_distance = plotResults.distance;
      
#if USE_DEBUG_CALCUL_DIST
      sprintf(l__buffer, "\tInit [%d]\n", l__remaining_distance);
      Serial.print(l__buffer);
#endif
      for (l__idx = plotResults.idx + 1; l__idx < nbr_of_plots_adjust; l__idx++) {
#if USE_DEBUG_CALCUL_DIST
        sprintf(l__buffer, "\t#%d: [%d] + [%d] = [%d] M\n", l__idx,
          l__remaining_distance, plotRecordAdjust[l__idx].plot.distance,
          (l__remaining_distance + plotRecordAdjust[l__idx].plot.distance));
        Serial.print(l__buffer);
#endif
        l__remaining_distance += plotRecordAdjust[l__idx].plot.distance;
      }

      remaining_distance = l__remaining_distance;

#if USE_DEBUG_CALCUL_DIST
      sprintf(l__buffer, "\t\t=> [%d] M (on the route)\n", remaining_distance);
      Serial.print(l__buffer);
#endif

      // Effacement et remplacement des états sur le tracé si arrivée à la fin du tracé
      if (remaining_distance <= DISTANCE_50M) {
        plotResults.type_synth = TYPE_SYNTH_YOU_HAVE_ARRIVED;
      }

      // Pour la synthèse toutes les minutes des distances aux villes
      plotResults.flg_dist_to_track_too_large = false;
   }
   else if (plotResults.idx >= 0 && plotResults.type_synth == TYPE_SYNTH_THE_PLOT_IS) {
#if USE_DEBUG_CALCUL_DIST
      char l__buffer[64];
#endif

    if (plotResults.distance <= DISTANCE_2_5KM) {
      /* Calcul de la distance restante comme la somme des distances des segments
       * à partir de l'index (jonction ou segment) jusqu'au dernier index
       */
      uint32_t l__remaining_distance = 0;
      int l__idx = plotResults.idx;

#if USE_DEBUG_CALCUL_DIST
      sprintf(l__buffer, "Dist. remaining from #%d to #%d:\n", plotResults.idx, (nbr_of_plots_adjust - 1));
      Serial.print(l__buffer);
#endif

      for (l__idx = plotResults.idx; l__idx < nbr_of_plots_adjust; l__idx++) {
#if USE_DEBUG_CALCUL_DIST
        sprintf(l__buffer, "\t#%d: [%d] + [%d] = [%d] M\n", l__idx,
          l__remaining_distance, plotRecordAdjust[l__idx].plot.distance,
          (l__remaining_distance + plotRecordAdjust[l__idx].plot.distance));
        Serial.print(l__buffer);
#endif
        l__remaining_distance += plotRecordAdjust[l__idx].plot.distance;
      }

      remaining_distance = l__remaining_distance;

      // Pour la synthèse toutes les minutes des distances aux villes
      plotResults.flg_dist_to_track_too_large = false;
    }
    else {
      // Distance au tracé trop éloignée => Synthèse de la distance totale du tracé
      remaining_distance = (UINT_MAX);

      // Pour la synthèse toutes les minutes des distances aux villes
      plotResults.flg_dist_to_track_too_large = true;
    }
   
#if USE_DEBUG_CALCUL_DIST
      sprintf(l__buffer, "\t\t=> [%d] M (out of the route)\n", remaining_distance);
      Serial.print(l__buffer);
#endif
   }
   // Fin: Détermination des 2 distances restante et parcourue sur le tracé
   
   // Mise à disposition des résultats à tout moment
   plotResults.flg_avalaible = true;
   memcpy(&plotResultsSave, &plotResults, sizeof(ST_RESULTS));
}

/* Inversion des 'nbr_of_plots' plots records
   From  <=> To
   ----      --
   (N-1) <=> 0
   (N-2) <=> 1
   ...
   Arret si 'To' >= 'From'

   Remarque: 'idx_of_adjust_plot' est à zéro pour le lancement du calcul
             des 'adjust plots' inversés par la méthode 'calculOfAdjustPlotLine'
             appelée dans 'GpsPilot.ino'
*/
void Plots::inversePlotsRecord()
{
  char l__buffer[80];

  sprintf(l__buffer, "Inversion of the %d plots records...\n", nbr_of_plots);
  Serial.print(l__buffer);

  int n = 0;
  for (n = (nbr_of_plots -1); n >= 0; n--) {
    int n__from = n;
    int n__to = (nbr_of_plots - 1) - n;

    if (n__to >= n__from) {
      break;
    }

    ST_PLOT_RECORD l__st_plot_from;
    ST_PLOT_RECORD l__st_plot_to;

    getPlotRecord(n__from, &l__st_plot_from);
    getPlotRecord(n__to, &l__st_plot_to);

    setPlotRecord(n__from, &l__st_plot_to);
    setPlotRecord(n__to, &l__st_plot_from);
  }

  int l__optim_level = getOptimLevel();
  Serial.print("Optimization level [");
  Serial.print(l__optim_level);
  Serial.print("]\n");

  if (l__optim_level == 1) {
    /* Optimisation #1 configurée
     * => Recopie des 'PlotRecords' dans 'PlotAdjust' + calcul des coefficients
    */
    copyPlotsRecordToAdjust();

    // Calcul of Caps and Distance
    calculOfCapsAndDistance();

    // Calcul of relative Latitude [Longitude] in meters @ 1st jonction
    desmosCalculOfRelativeLatLon();

    // Calcul of constant A as for Desmos treatments
    desmosCalculOfA();            
  }
  else {
    Serial.print("No optimization\n");

    // "Recalcul" of constantes...
    nbr_of_plots_adjust = 0;

    // Calculate the 'Cap' and 'Distance' between each plot
    adjustmentCapAndDistance();

    /* Adjustement more of 'Cap' and 'Distance'
     * et détermination du sens du tracé 
    */
    adjustmentCapAndDistanceMore();
          
    // Calcul of relative Latitude [Longitude] in meters @ 1st jonction
    desmosCalculOfRelativeLatLon();

    // Calcul of constant A as for Desmos treatments
    desmosCalculOfA();
  }

  // Force the "recalcul" of 'adjust records' in background
  idx_of_adjust_plot = 0;
}

void Plots::printResults(char *o__buffer, ST_RESULTS *i__results)
{
   const char *l__text = "Unknown";
   if (i__results->flg_seg_jonc == TYPE_SEGMENT) {
      l__text = "Segment";
   }
   else if (i__results->flg_seg_jonc == TYPE_JONCTION) {
      l__text = "Jonction";
   }

#if 0
    printf("Plot results: Ds_idx [%d] Ds_min [%d] Dj_idx [%d] Dj_min [%d] => %s: idx [%d] D [%d] X:Y [%d]:[%d] Lat [%.6f] Lon [%.6f] Cap [%.1f]\n",
      i__results->Ds_idx, i__results->Ds_min,
      i__results->Dj_idx, i__results->Dj_min,
      l__text, i__results->idx, i__results->D_min, i__results->X, i__results->Y,
      i__results->lat, i__results->lon, plotRecordAdjust[i__results->idx].plot.capToNextPlot);
#endif

    Serial.print("Plot results: ");
    sprintf(o__buffer, "Ds_idx [%d] Ds_min [%d] Dj_idx [%d] Dj_min [%d]\n",
      i__results->Ds_idx, i__results->Ds_min,
      i__results->Dj_idx, i__results->Dj_min);
    Serial.print(o__buffer);

    sprintf(o__buffer, "\t=> %s: idx [%d] D [%d] X:Y [%d]:[%d] ",
      l__text,
      i__results->idx, i__results->D_min, i__results->X, i__results->Y);
    Serial.print(o__buffer);

    Serial.print("Lat [");
    Serial.print(i__results->lat, 6);
    Serial.print("] Lon [");
    Serial.print(i__results->lon, 6);
    Serial.print("] Cap [");
    Serial.print(plotRecordAdjust[i__results->idx].plot.capToNextPlot, 1);
    Serial.print("]\n");

#if USE_ATTRACTOR_TO_PLOT
    sprintf(o__buffer, "\t=> %d Dist.", NBR_DIST_AVG);
    Serial.print(o__buffer);

    int n = 0;
    for (n = 0; n < NBR_DIST_AVG; n++) {
      sprintf(o__buffer, " [%d]", t_dist_avg[n]);
      Serial.print(o__buffer);
    }
    sprintf(o__buffer, " Avg [%d]\n", i__results->D_min_avg);
    Serial.print(o__buffer);
#endif

    // Type de synthèse à produire
    switch (i__results->type_synth) {
    case TYPE_SYNTH_THE_PLOT_IS:
      sprintf(o__buffer, "\t=> #1: Le tracé est à [%d] M à [", i__results->distance);
      Serial.print(o__buffer);
      Serial.print(i__results->cap, 1);
      Serial.print("] deg.\n");
      break;

    case TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1:
#if USE_SYNTH_YOU_ARE_ON_THE_PLOT
      sprintf(o__buffer, "\t=> #2: Vous êtes sur le tracé, continuez sur [%d] M à [", i__results->distance);
#else
      sprintf(o__buffer, "\t=> #2: Continuez sur [%d] M à [", i__results->distance);
#endif
      Serial.print(o__buffer);
      Serial.print(i__results->cap, 1);
      Serial.print("] deg.\n");
      break;

    case TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2:
#if USE_SYNTH_YOU_ARE_ON_THE_PLOT
      sprintf(o__buffer, "\t=> #3: Vous êtes sur le tracé, préparez vous à bifurquer à [%d] M à [", i__results->distance);
#else
      sprintf(o__buffer, "\t=> #3: Préparez vous à bifurquer à [%d] M à [", i__results->distance);
#endif
      Serial.print(o__buffer);
      Serial.print(i__results->cap, 1);
      Serial.print("] deg.\n");
      break;

    case TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3:
      sprintf(o__buffer, "\t=> #4: Bifurquez maintenant à [");
      Serial.print(o__buffer);
      Serial.print(i__results->cap, 1);
      Serial.print("] deg.");
      sprintf(o__buffer, " et continuez sur [%d] M\n", i__results->distance);
      Serial.print(o__buffer);
      break;

    case TYPE_SYNTH_YOU_HAVE_ARRIVED:
      //sprintf(o__buffer, "\t=> #5: Vous êtes%s arrivé...", (remaining_distance <= DISTANCE_50M) ? "presque" : " ");
      sprintf(o__buffer, "\t=> #5: Vous êtes arrivé...\n");
      Serial.print(o__buffer);
      break;

    default:
      sprintf(o__buffer, "\t=> Unknown synth. type [%d]\n", i__results->type_synth);
      Serial.print(o__buffer);
      break;
    }
    
    l__text = "Unknown";
    switch (plots_direction) {
    case PLOTS_DIR_GO:
      l__text = "Aller";
      break;
    case PLOTS_DIR_RETURN:
      l__text = "Retour";
      break;
    default:
      break;
    }

    sprintf(o__buffer, "\t=> Type synth [#%d] Idx [#%d] Dist [%d] M\n", i__results->type_synth, i__results->idx, i__results->distance);
    Serial.print(o__buffer);
    sprintf(o__buffer, "\t=> Dist: remaining [%d] on plots [%d] total [%d] M Sens [%s]\n", remaining_distance, distance_on_plots, total_distance, l__text);
    Serial.print(o__buffer);

  // Valeurs des timers de diffusion
  boolean l__flg_timers = false;

  sprintf(o__buffer, "\t=> Timers:");
  Serial.print(o__buffer);

  if (g__timers->isInUse(TIMER_TYPE_SYNTH_THE_PLOT_IS) == true) {
    l__flg_timers = true;
    sprintf(o__buffer, " #1: [%ld]", g__timers->getDuration(TIMER_TYPE_SYNTH_THE_PLOT_IS) * 10L);
    Serial.print(o__buffer);
  }
  if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1) == true) {
    l__flg_timers = true;
    sprintf(o__buffer, " #2: [%ld]", g__timers->getDuration(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1) * 10L);
    Serial.print(o__buffer);
  }
  if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2) == true) {
    l__flg_timers = true;
    sprintf(o__buffer, " #3: [%ld]", g__timers->getDuration(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2) * 10L);
    Serial.print(o__buffer);
  }
  if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3) == true) {
    l__flg_timers = true;
    sprintf(o__buffer, " #4: [%ld]", g__timers->getDuration(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3) * 10L);
    Serial.print(o__buffer);
  }
  if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED) == true) {
    l__flg_timers = true;
    sprintf(o__buffer, " #5: [%ld]", g__timers->getDuration(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED) * 10L);
    Serial.print(o__buffer);
  }

  if (l__flg_timers == false) {
    Serial.print(" Not in use\n");
  }
  else {
    Serial.print(" mS\n");
  }
  // Fin: Valeurs des timers de diffusion
}

void Plots::getResults(ST_RESULTS *o__st_results)
{
  memcpy(o__st_results, &plotResults, sizeof(ST_RESULTS));
}

boolean Plots::getResultsSave(ST_RESULTS *o__results)
{
  boolean l__flg_rtn = plotResultsSave.flg_avalaible;

  if (l__flg_rtn == true) {
    memcpy(o__results, &plotResultsSave, sizeof(ST_RESULTS));
  }
  else {
    /* Forçage 'Distance au tracé très grande' pour une synthèse
     * des distances aux villes toutes les minutes
     */
    o__results->flg_dist_to_track_too_large = true;
  }

  return l__flg_rtn;
}

boolean Plots::calculOfAdjustPlotLine()
{
  char l__buffer[80];

  if (idx_of_adjust_plot >= 0 && idx_of_adjust_plot < nbr_of_plots_adjust) {
    if (idx_of_adjust_plot == 0) {
      sprintf(l__buffer, "Start of 'calculOfAdjustPlotLine()' (%d/%d)\n", idx_of_adjust_plot, nbr_of_plots_adjust);
      Serial.print(l__buffer);

      flg_adjust_plots_avail = false;

      // Raz resultats
      clearResults();
    }

    if (idx_of_adjust_plot >= 0 && idx_of_adjust_plot < (nbr_of_plots_adjust - 1)) {
      // Calcul of B coefficients, distances signed, (X;Y)-Intersection sur le segment, distances non signees aux jonctions
      desmosCalculOfB (idx_of_adjust_plot, st_desmos_coord[DESMOS_COORD_CURRENT].lon_x, st_desmos_coord[DESMOS_COORD_CURRENT].lat_y);
      desmosCalculOfDs(idx_of_adjust_plot, st_desmos_coord[DESMOS_COORD_CURRENT].lon_x, st_desmos_coord[DESMOS_COORD_CURRENT].lat_y);
      desmosCalculOfDj(idx_of_adjust_plot, st_desmos_coord[DESMOS_COORD_CURRENT].lon_x, st_desmos_coord[DESMOS_COORD_CURRENT].lat_y);
      desmosCalculOfXY(idx_of_adjust_plot);
      desmosCalculOfPs(idx_of_adjust_plot, st_desmos_coord[DESMOS_COORD_CURRENT].lon_x, st_desmos_coord[DESMOS_COORD_CURRENT].lat_y);
    }
    else if (idx_of_adjust_plot == (nbr_of_plots_adjust - 1)) {
      // Complements (proximite avec la derniere jonction)
      desmosCalculOfDj(idx_of_adjust_plot, st_desmos_coord[DESMOS_COORD_CURRENT].lon_x, st_desmos_coord[DESMOS_COORD_CURRENT].lat_y);

      // Finalise les resultats
      finalyzeResults();
    }

    // Next line
    idx_of_adjust_plot++;

    if (idx_of_adjust_plot >= nbr_of_plots_adjust) {
      sprintf(l__buffer, "End of 'calculOfAdjustPlotLine()' (%d/%d)\n", idx_of_adjust_plot, nbr_of_plots_adjust);
      Serial.print(l__buffer);

      flg_adjust_plots_avail = true;

      printResults(l__buffer, &plotResults);

      return true;
    }
  }

  return false;
}
// End: Methods for desmos calculation

// Méthodes pour le calcul des distances aux positions 'enregistrées' et 'mémorisées'
boolean Plots::getPositionSelected(ST_COORD_POSITION *o__st_coord_position)
{
  if (positions.pos_select.idx != -1 && (positions.pos_select.type == TYPE_POSITION_RECORDED || positions.pos_select.type == TYPE_POSITION_MEMORIZED)) {
    memcpy(o__st_coord_position, &positions.pos_select, sizeof(ST_COORD_POSITION));

    return true;
  }
  else {
    return false;
  }
}

void Plots::setCurrentPosition(COORD *i__coord)
{
  memcpy(&positions.current_pos, i__coord, sizeof(COORD));
}

boolean Plots::calculOfPositions()
{
  char l__buffer[80];

  if (positions.idx_current >= 0 && positions.idx_current < positions.nbr_positions) {
    if (positions.idx_current == 0) {
      sprintf(l__buffer, "Start of 'calculOfPositions()' (%d/%d)\n", positions.idx_current, positions.nbr_positions);
      Serial.print(l__buffer);

      positions.dist_min = UINT_MAX;
      positions.idx_select = -1;
      
      positions.flg_available = false;
    }

    // Calcul of distance to current position
    COORD l__coord;
    initCoordToNoMean(&l__coord);
    l__coord.lat = positions.pos_all[positions.idx_current].lat;
    l__coord.lon = positions.pos_all[positions.idx_current].lon;
    l__coord.ele = positions.pos_all[positions.idx_current].ele;    // Provision for the 3D calcul

    positions.pos_all[positions.idx_current].distance = calculateDistance(&l__coord, &positions.current_pos);
    if (positions.pos_all[positions.idx_current].distance < positions.dist_min) {
      positions.dist_min = positions.pos_all[positions.idx_current].distance;
      positions.idx_select = positions.idx_current;
    }

    if (positions.idx_current == (positions.nbr_positions - 1) && (positions.idx_select >= 0 && positions.idx_select < positions.nbr_positions)) {
      // Mise à disposition de la distance à la position la plus proche
      memcpy(&positions.pos_select, &positions.pos_all[positions.idx_select], sizeof(ST_COORD_POSITION));
    }

    // Next position
    positions.idx_current++;

    if (positions.idx_current >= positions.nbr_positions) {
      sprintf(l__buffer, "End of 'calculOfPositions()' (%d/%d)\n", positions.idx_current, positions.nbr_positions);
      Serial.print(l__buffer);

      positions.flg_available = true;

      toStringPositions(false, true, false);

      return true;
    }
  }

  return false;
}
// Fin: Méthodes pour le calcul des distances aux positions 'enregistrées' et 'mémorisées'

#if USE_POS_COMMUNES
// Méthodes pour le calcul des distances aux positions des communes
boolean Plots::getPosCommunesSelected(ST_COORD_POSITION *o__st_coord_position)
{
  if (pos_communes.pos_select.idx != -1) {
    memcpy(o__st_coord_position, &pos_communes.pos_select, sizeof(ST_COORD_POSITION));

    return true;
  }
  else {
    return false;
  }
}

void Plots::setCurrentPosCommunes(COORD *i__coord)
{
  memcpy(&pos_communes.current_pos, i__coord, sizeof(COORD));
}

boolean Plots::calculOfPosCommunes()
{
  char l__buffer[80];

  if (pos_communes.idx_current >= 0 && pos_communes.idx_current < pos_communes.nbr_positions) {
    if (pos_communes.idx_current == 0) {
      sprintf(l__buffer, "Start of 'calculOfPosCommunes()' (%d/%d)\n", pos_communes.idx_current, pos_communes.nbr_positions);
      Serial.print(l__buffer);

      pos_communes.dist_min = UINT_MAX;
      pos_communes.idx_select = -1;
      
      //pos_communes.flg_available = false;
    }

    // Calcul of distance to current position
    COORD l__coord;
    initCoordToNoMean(&l__coord);
    l__coord.lat = pos_communes.pos_all[pos_communes.idx_current].lat;
    l__coord.lon = pos_communes.pos_all[pos_communes.idx_current].lon;
    l__coord.ele = pos_communes.pos_all[pos_communes.idx_current].ele;    // Provision for the 3D calcul

    pos_communes.pos_all[pos_communes.idx_current].distance = calculateDistance(&l__coord, &pos_communes.current_pos);

    /* Les chefs-lieux sont définis après les communes par secteur
     * => 'Versailles' sera sélectionné avant '78_Versailles' qui comporte la synthèse du département ;-)
     */
    if (pos_communes.pos_all[pos_communes.idx_current].distance < pos_communes.dist_min) {
      pos_communes.dist_min = pos_communes.pos_all[pos_communes.idx_current].distance;
      pos_communes.idx_select = pos_communes.idx_current;
    }

    if (pos_communes.idx_current == (pos_communes.nbr_positions - 1) && (pos_communes.idx_select >= 0 && pos_communes.idx_select < pos_communes.nbr_positions)) {
      /* Mise à disposition de la distance à la position la plus proche
       * => Remarque: Si le parcours de toutes les positions dure plus de 5" (timestamp d'acquisition GPS)
       *              => Pas gênant car m.a.j. à la fin du parcours des positions, donc position valide mais pas forçément la plus proche durant ces 5" ;-)
       */
      memcpy(&pos_communes.pos_select, &pos_communes.pos_all[pos_communes.idx_select], sizeof(ST_COORD_POSITION));
    }

    // Next position
    pos_communes.idx_current++;

    if (pos_communes.idx_current >= pos_communes.nbr_positions) {
      sprintf(l__buffer, "End of 'calculOfPosCommunes()' (%d/%d)\n", pos_communes.idx_current, pos_communes.nbr_positions);
      Serial.print(l__buffer);

      //pos_communes.flg_available = true;

      toStringPosCommunes(false, true);

      return true;
    }
  }

  return false;
}
// Fin: Méthodes pour le calcul des distances aux positions des communes
#endif

// Méthodes pour la simulation des déplacements
void Plots::setSimuMoveParameters(int i__num_plot, float i__speed_kmh)
{
  Serial.print("setSimuMoveParameters([");
  Serial.print(i__num_plot);
  Serial.print("], [");
  Serial.print(i__speed_kmh, 1);
  Serial.print("] KmH): Entering...\n");

  st_simu_move.num_plot_init    =
  st_simu_move.num_plot_current = i__num_plot;
  st_simu_move.num_plot_max     = (nbr_of_plots_adjust - 1);   // Simulation jusqu'à l'avant dernier adjust plot

  st_simu_move.speed_kmh = i__speed_kmh;

  /* Calcul du nombre de samples à simuler et max sur le segment 'num_plot_init' à la vitesse 'speed_kmh'
   * => Delta duration fixée à 5" (durée d'échantillonage de l'acquisition GPS)
   */
  st_simu_move.num_sample_current = 0;
  calcSimuMoveSamples(5);

  st_simu_move.flg_in_progress = true;

#ifndef USE_SIMULATION
  // Raz des statistiques sur simulation d'un nouveau segment
  g__stats->clearCineticInfos();
#endif
}

void Plots::getSimuMovePosition(COORD *io__coord_current, long i__duration)
{
  char l__buffer[80];

  sprintf(l__buffer, "getSimuMovePosition(%ld): Entering...\n", i__duration);
  Serial.print(l__buffer);

  sprintf(l__buffer, "\tSimu. Move [%s]\n", st_simu_move.flg_in_progress == true ? "start" : "stop");
  Serial.print(l__buffer);

  Serial.print("\tAdjust Plot: Init #");
  Serial.print(st_simu_move.num_plot_init);
  Serial.print(" Current #");
  Serial.print(st_simu_move.num_plot_current);
  Serial.print(" Max #");
  Serial.print(st_simu_move.num_plot_max);
  Serial.print("\n");
  Serial.print("\tSpeed [");
  Serial.print(st_simu_move.speed_kmh, 1);
  Serial.print("] KmH\n");
  Serial.print("\tDelta Duration [");
  Serial.print(i__duration - st_simu_move.duration_previous);
  Serial.print("]\n");

  Serial.print("\tSample: Current #");
  Serial.print(st_simu_move.num_sample_current);
  Serial.print(" Max #");
  Serial.print(st_simu_move.num_sample_max);
  Serial.print(" Nbr [");
  Serial.print(st_simu_move.nbr_samples);
  Serial.print("]\n");

  // Détermination des coordonnées simulées au sample 'num_sample_current' @ 'nbr_samples'
  if (st_simu_move.num_sample_current >= 0 && st_simu_move.num_sample_current < st_simu_move.num_sample_max && st_simu_move.nbr_samples != 0) {
    uint32_t l__distance = plotRecordAdjust[st_simu_move.num_plot_current].plot.distance;
    float l__cap         = plotRecordAdjust[st_simu_move.num_plot_current].plot.capToNextPlot;

    Serial.print("\tDistance and Cap on the segment [");
    Serial.print(l__distance);
    Serial.print("] M [");
    Serial.print(l__cap, 1);
    Serial.print("] degrees\n");

    float l__new_lat = 0.0;
    float l__new_lon = 0.0;
    float l__new_ele = 0.0;

    if (st_simu_move.num_plot_current < st_simu_move.num_plot_max) {
      float l__ratio = ((float)st_simu_move.num_sample_current / (float)st_simu_move.nbr_samples);

      l__new_lat  = (plotRecordAdjust[st_simu_move.num_plot_current + 1].plot.lat - plotRecordAdjust[st_simu_move.num_plot_current].plot.lat);
      l__new_lat *= l__ratio;
      l__new_lat += plotRecordAdjust[st_simu_move.num_plot_current].plot.lat;

      l__new_lon  = (plotRecordAdjust[st_simu_move.num_plot_current + 1].plot.lon - plotRecordAdjust[st_simu_move.num_plot_current].plot.lon);
      l__new_lon *= l__ratio;
      l__new_lon += plotRecordAdjust[st_simu_move.num_plot_current].plot.lon;

      l__new_ele  = (plotRecordAdjust[st_simu_move.num_plot_current + 1].plot.ele - plotRecordAdjust[st_simu_move.num_plot_current].plot.ele);
      l__new_ele *= l__ratio;
      l__new_ele += plotRecordAdjust[st_simu_move.num_plot_current].plot.ele;
    }
    else {
      Serial.print("\tEnd of the simulation\n");    // 'st_simu_move.num_plot_current + 1' not exists

      l__new_lat = plotRecordAdjust[st_simu_move.num_plot_current].plot.lat;
      l__new_lon = plotRecordAdjust[st_simu_move.num_plot_current].plot.lon;
      l__new_ele = plotRecordAdjust[st_simu_move.num_plot_current].plot.ele;
    }

    // Coordonnates of simulation position ;-)
    io__coord_current->lat      = l__new_lat;
    io__coord_current->lon      = l__new_lon;
    io__coord_current->ele      = l__new_ele;
    io__coord_current->cap      = l__cap;
    io__coord_current->speedKmH = (st_simu_move.flg_in_progress == true) ? st_simu_move.speed_kmh : 0.0;
  }
  else {
    sprintf(l__buffer, "\tErr: Invalid current #%d (< 0 or >= %d)\n", st_simu_move.num_sample_current, st_simu_move.num_sample_max);
    Serial.print(l__buffer);
    sprintf(l__buffer, "\t  or Invalid nbr samples #%d (== 0)\n", st_simu_move.nbr_samples);
    Serial.print(l__buffer);
  }

  if (st_simu_move.flg_in_progress == true) {
    // Préparation prochain segment
    st_simu_move.num_sample_current++;

    if (st_simu_move.num_sample_current >= st_simu_move.num_sample_max) {
      st_simu_move.num_plot_current++;

      st_simu_move.num_sample_current = 0;    // Retour à l'origine du segment

      // Arrêt de la simulation si dernier échantillon du dernier segment
      if (st_simu_move.num_plot_current >= st_simu_move.num_plot_max) {
        Serial.print("\tStop the simulation\n");

        stopSimuMove();
      }
      else {
        /* Calcul du nombre de samples à simuler et max sur le segment 'num_plot_init' à la vitesse 'speed_kmh'
         * => Delta duration fixée à 5" (durée d'échantillonage de l'acquisition GPS)
        */
        Serial.print("\tNew segment\n");

        calcSimuMoveSamples(5);
      }
    }

    /* Pour les traces de chaque appel à 'getSimuMovePosition()'
     * => Non utilisé pour les calculs de la simulation
     *    => Utilisé pour la progression dans le temps @ 5" de l'acquisition GPS ;-)
     */
    st_simu_move.duration_previous = i__duration;
  }
}

// Détermination des samples de déplacement sur le segment 'num_plot_current'
boolean Plots::calcSimuMoveSamples(long i__duration)
{
  char l__buffer[80];
  boolean l__rtn = false;

  if (st_simu_move.num_plot_current < nbr_of_plots_adjust) {
    // Vitesse jamais nulle
    float l__speed_kmh = (st_simu_move.speed_kmh <= SPEED_EPSILON) ? SPEED_EPSILON : st_simu_move.speed_kmh;

    // Durée en secondes pour parcourir ce segment à 'speed_kmh'
    long l__duration = (long)((float)plotRecordAdjust[st_simu_move.num_plot_current].plot.distance / (l__speed_kmh / 3.6));

    if (i__duration != 0) {
      st_simu_move.nbr_samples    = (l__duration / i__duration);
      st_simu_move.num_sample_max = (st_simu_move.nbr_samples - 1);

      l__rtn = true;
    }
    else {
      sprintf(l__buffer, "calcSimuMoveSamples(%ld): Err: Duration == 0\n", i__duration);
      Serial.print(l__buffer);      
    }
  }

  return l__rtn;
}
// Fin: Méthodes pour la simulation des déplacements

// Private methods
void Plots::m__clearIndex()
{
  nbrOfRecords = 0;
  m__idx = 0;

  checksum_calc = (uint32_t)-1;
  size_datas_calc = 0;
}

int Plots::m__nextIndex()
{
  if (m__idx < (NBR_PLOT_RECORDS - 1)) {
    nbrOfRecords++;
    m__idx++;

    return m__idx;
  }
  else {
    return -1;
  }
}

void Plots::m__calculChecksumInc(uint8_t *i__datas, size_t i__size, boolean i__flg_end)
{
  if (i__flg_end == false) {
    m__size_cumul += i__size;
    m__crc_tmp = cksum_inc(i__datas, i__size, m__crc_tmp, 0);
  }
  else {
    checksum_calc = cksum_inc(NULL, 0, m__crc_tmp, m__size_cumul);
    size_datas_calc = m__size_cumul;

    // Prepare for next call
    m__size_cumul = 0;
    m__crc_tmp = 0;
  }
}

boolean Plots::addPosition(ST_COORD_POSITION *i__pos)
{
  boolean l__rtn = false;
  char l__buffer[128];

  sprintf(l__buffer, "addPosition(Idx [0x%x] Type [%d] Lat [%.6f] Lon [%.6f] Ele [%.1f] Range [%d])\n",
    i__pos->idx, i__pos->type, i__pos->lat, i__pos->lon, i__pos->ele, i__pos->range);
  Serial.print(l__buffer);

  // Détermination de la 1st position disponible
  boolean l__flg_found = false;
  int n =0;
  for (n = 0; n < NBR_POSITIONS; n++) {
    if (positions.pos_all[n].idx == -1) {
      switch(i__pos->type) {
      case TYPE_POSITION_RECORDED:
        l__flg_found = true;
        positions.nbr_pos_recorded++;
        break;

      case TYPE_POSITION_MEMORIZED:
        l__flg_found = true;
        positions.nbr_pos_memorized++;
        break;

      default:
        break;
      }

      if (l__flg_found == true) {
        positions.nbr_positions++;

        positions.pos_all[n].idx   = i__pos->idx;
        positions.pos_all[n].type  = i__pos->type;
        positions.pos_all[n].lat   = i__pos->lat;
        positions.pos_all[n].lon   = i__pos->lon;
        positions.pos_all[n].ele   = i__pos->ele;
        positions.pos_all[n].range = i__pos->range;

        sprintf(l__buffer, "\tSuccessful: New idx [%d] at position #%d\n", positions.pos_all[n].idx, n);
        Serial.print(l__buffer);

        l__rtn = true;

        break;
      }
    }
  }

  if (l__rtn == false) {
    sprintf(l__buffer, "\tErr: Too many positions (>= %d) or invalid type (%d)\n", NBR_POSITIONS, i__pos->type);
    Serial.print(l__buffer);
  }

  return l__rtn;
}

void Plots::toString()
{
  if (g__gestion.flg_save_current_positions == true) {
    Serial.print("Plots in building...\n");
  }
  else if (isAvailable() == false) {
    Serial.print("Plots not available\n");
    return;
  }

  Serial.print("List of ");
  Serial.print(nbr_of_plots);
  Serial.print(" plots of [");
  Serial.print(name_of_plots);
  Serial.print("]\n");

  Serial.print("\t#Start Lat [");
  Serial.print(plotStartPosition.lat, 6);
  Serial.print("] Lon [");
  Serial.print(plotStartPosition.lon, 6);
  Serial.print("] Ele [");
  Serial.print(plotStartPosition.ele, 0);
  Serial.print("]\n");

  int n = 0;
  for (n = 0; n < nbr_of_plots; n++) {
    char l__buffer[32];
    sprintf(l__buffer, "\t#%04d: Idx [%d]", n, plotRecord[n].idx);
    Serial.print(l__buffer);
    Serial.print(" Lat [");
    Serial.print(plotRecord[n].plot.lat, 6);
    Serial.print("] Lon [");
    Serial.print(plotRecord[n].plot.lon, 6);
    Serial.print("] Ele [");
    Serial.print(plotRecord[n].plot.ele, 0);
    Serial.print("] Cap [");
    Serial.print(plotRecord[n].plot.capToNextPlot, 1);
    Serial.print("] [");
    Serial.print(plotRecord[n].plot.capHoursToNextPlot);
    Serial.print("H] Dist [");
    Serial.print(plotRecord[n].plot.distance);
    Serial.print("]\n");
  }
}

void Plots::toStringAdjust(boolean i__flg_all)
{
  char l__buffer[80];

  if (isAvailable() == false) {
    Serial.print("Adjust Plots not available\n");
    return;
  }

  Serial.print("List of ");
  Serial.print(nbr_of_plots_adjust);
  Serial.print(" adjust plots of [");
  Serial.print(name_of_plots);
  Serial.print("]\n");

  Serial.print("\t#Start Lat [");
  Serial.print(plotStartPosition.lat, 6);
  Serial.print("] Lon [");
  Serial.print(plotStartPosition.lon, 6);
  Serial.print("] Ele [");
  Serial.print(plotStartPosition.ele, 0);
  Serial.print("]\n");

  uint32_t l__total_distance = 0;

  size_t n = 0;
  for (n = 0; n < (size_t)nbr_of_plots_adjust; n++) {
    sprintf(l__buffer, "\t#%04d: Section [%d] Idx [%d]", n, plotRecordAdjust[n].section, plotRecordAdjust[n].idx);
    Serial.print(l__buffer);
    Serial.print(" Lat [");
    Serial.print(plotRecordAdjust[n].plot.lat, 6);
    Serial.print("] Lon [");
    Serial.print(plotRecordAdjust[n].plot.lon, 6);
    Serial.print("] Ele [");
    Serial.print(plotRecordAdjust[n].plot.ele, 0);

    if (plotRecordAdjust[n].plot.flgValid != true) {
      Serial.print("] Record not valid\n");
      return;
    }

    Serial.print("] Cap [");
    Serial.print(plotRecordAdjust[n].plot.capToNextPlot, 1);
    Serial.print("] [");
    Serial.print(plotRecordAdjust[n].plot.capHoursToNextPlot);
    Serial.print("H] D [");
    Serial.print(plotRecordAdjust[n].plot.distance);

    if (i__flg_all == true) {
      Serial.print("] X:Y [");
      Serial.print(plotRecordAdjust[n].desmos.lon_x);
      Serial.print("]:[");
      Serial.print(plotRecordAdjust[n].desmos.lat_y);
      Serial.print("] A:B [");
      Serial.print(plotRecordAdjust[n].desmos.A);
      Serial.print("]:[");
      Serial.print(plotRecordAdjust[n].desmos.B);
      Serial.print("] Ps [");
      Serial.print(plotRecordAdjust[n].desmos.Ps);
    }

    Serial.print("]\n");

    l__total_distance += plotRecordAdjust[n].plot.distance;
  }

  sprintf(l__buffer, "#2: Total distance [%d]\n", l__total_distance);
  Serial.print(l__buffer);
}

void Plots::toStringWaypoints()
{
  char l__buffer[80];

  if (isAvailable() == false) {
    Serial.print("Waypoints not available\n");
    return;
  }

  uint32_t l__total_distance = 0;

  sprintf(l__buffer, "\nList of %d waypoints plots of [", nbr_of_plots_adjust);
  Serial.print(l__buffer);
  Serial.print(name_of_plots);
  Serial.print("]\n");

  size_t n = 0;
  for (n = 0; n < (size_t)nbr_of_plots_adjust; n++) {
    sprintf(l__buffer, "  <wpt lat=\"%.6f\" lon=\"%.6f\">\n",
      plotRecordAdjust[n].plot.lat, plotRecordAdjust[n].plot.lon);
    Serial.print(l__buffer);

    sprintf(l__buffer, "    <name>#%d: Cap %.1f (%dH) sur %d metres</name>\n",
      n,
      plotRecordAdjust[n].plot.capToNextPlot, plotRecordAdjust[n].plot.capHoursToNextPlot,
      plotRecordAdjust[n].plot.distance);
    Serial.print(l__buffer);

    sprintf(l__buffer, "    <type>sommet</type>\n");
    Serial.print(l__buffer);

    sprintf(l__buffer, "  </wpt>\n");
    Serial.print(l__buffer);

    l__total_distance += plotRecordAdjust[n].plot.distance;
  }

  sprintf(l__buffer, "#3: Total distance [%d]\n", l__total_distance);
  Serial.print(l__buffer);
}

void Plots::toStringSegments()
{
  char l__buffer[80];

  if (isAvailable() == false) {
    printf("Segments not available\n");
    Serial.print("Segments not available\n");
    return;
  }

  uint32_t l__total_distance = 0;

  sprintf(l__buffer, "\nList of %d segments plots of [", nbr_of_plots_adjust);
  Serial.print(l__buffer);
  Serial.print(name_of_plots);
  Serial.print("]\n");

  sprintf(l__buffer, "  <trk>\n");
  Serial.print(l__buffer);

  sprintf(l__buffer, "    <name>Trace a suivre</name>\n");
  Serial.print(l__buffer);
  
  sprintf(l__buffer, "    <extensions>\n");
  Serial.print(l__buffer);
  sprintf(l__buffer, "      <line xmlns=\"http://www.topografix.com/GPX/gpx_style/0/2\">\n");
  Serial.print(l__buffer);
  sprintf(l__buffer, "        <color>000000</color>\n");
  Serial.print(l__buffer);
  sprintf(l__buffer, "        <width>4</width>\n");
  Serial.print(l__buffer);
  sprintf(l__buffer, "      </line>\n");
  Serial.print(l__buffer);
  sprintf(l__buffer, "    </extensions>\n");
  Serial.print(l__buffer);

  sprintf(l__buffer, "    <trkseg>\n");
  Serial.print(l__buffer);

  size_t n = 0;
  for (n = 0; n < (size_t)nbr_of_plots_adjust; n++) {
    sprintf(l__buffer, "    <trkpt lat=\"%.6f\" lon=\"%.6f\"/>\n",
      plotRecordAdjust[n].plot.lat, plotRecordAdjust[n].plot.lon);
    Serial.print(l__buffer);

    l__total_distance += plotRecordAdjust[n].plot.distance;
  }

  sprintf(l__buffer, "    </trkseg>\n");
  Serial.print(l__buffer);
  sprintf(l__buffer, "  </trk>\n");
  Serial.print(l__buffer);

  sprintf(l__buffer, "#4: Total distance [%d]\n", l__total_distance);
  Serial.print(l__buffer);
}

void Plots::toStringPositions(boolean i__flg_all, boolean i__flg_selected, boolean i__flg_waypoints)
{
  char l__buffer[128];

  if (i__flg_all == true || i__flg_waypoints == true) {
    sprintf(l__buffer, "List of %d positions of [%s]\n", positions.nbr_positions, name_of_plots.c_str());
    Serial.print(l__buffer);
    sprintf(l__buffer, "\t- %d positions recorded\n", positions.nbr_pos_recorded);
    Serial.print(l__buffer);
    sprintf(l__buffer, "\t- %d positions memorized\n", positions.nbr_pos_memorized);
    Serial.print(l__buffer);
    sprintf(l__buffer, "\t- Available [%s]\n", (positions.flg_available == true) ? "Yes" : "No");
    Serial.print(l__buffer);
  }

  ST_COORD_POSITION *l__pos = NULL;

  if (i__flg_all == true) {
    int n = 0;
    for (n = 0; n < positions.nbr_positions; n++) {
      l__pos = &positions.pos_all[n];

      sprintf(l__buffer, "\t#%d: Idx [0x%x] Type [%d] Lat [%.6f] Lon [%.6f] Ele [%.1f] Range [%d] Distance [%u]\n",
        n, l__pos->idx, l__pos->type, l__pos->lat, l__pos->lon, l__pos->ele, l__pos->range, l__pos->distance);
      Serial.print(l__buffer);
    }
  }

  // Position selected
  if (i__flg_all == true || i__flg_selected == true) {
    Serial.print("Position Recorded/Memorized selected\n");
    sprintf(l__buffer, "\tIdx select #%d Dist Min [%d]\n", positions.idx_select, positions.dist_min);
    Serial.print(l__buffer);

    l__pos = &positions.pos_select;

    sprintf(l__buffer, "\tIdx [0x%x] Type [%d] Lat [%.6f] Lon [%.6f] Ele [%.1f] Range [%d] Distance [%u]\n",
      l__pos->idx, l__pos->type, l__pos->lat, l__pos->lon, l__pos->ele, l__pos->range, l__pos->distance);
    Serial.print(l__buffer);
  }
  // End: Position selected

  // Print the 'waypoints' for verifications with 'VisuGPX'
  if (i__flg_all == true || i__flg_waypoints == true) {
    sprintf(l__buffer, "List of %d waypoints\n", positions.nbr_positions);
    Serial.print(l__buffer);

    int n = 0;
    for (n = 0; n < positions.nbr_positions; n++) {
      l__pos = &positions.pos_all[n];

      sprintf(l__buffer, "\t<wpt lat=\"%.6f\" lon=\"%.6f\">\n", l__pos->lat, l__pos->lon);
      Serial.print(l__buffer);

      sprintf(l__buffer, "\t  <ele>%.1f</ele>\n", l__pos->ele);
      Serial.print(l__buffer);

      sprintf(l__buffer, "\t  <name>Idx [0x%02x]</name>\n", l__pos->idx);
      Serial.print(l__buffer);

      sprintf(l__buffer, "\t  <type>water</type>\n");
      Serial.print(l__buffer);

      const char *l__text = "Unknown";
      if (l__pos->type == TYPE_POSITION_RECORDED) {
        l__text = "Recorded";
      }
      else if (l__pos->type == TYPE_POSITION_MEMORIZED) {
        l__text = "Memorized";
      }
      sprintf(l__buffer, "\t  <desc>#%d/#%d: Type [%s] Range [%d]</desc>\n", n, (positions.nbr_positions - 1), l__text, l__pos->range);
      Serial.print(l__buffer);

      sprintf(l__buffer, "\t</wpt>\n");
      Serial.print(l__buffer);
    }
  }
  // End: Print the 'waypoints' for verifications with 'VisuGPX'
}

#if USE_POS_COMMUNES
void Plots::toStringPosCommunes(boolean i__flg_all, boolean i__flg_selected)
{
  char l__buffer[128];

  if (i__flg_all == true) {
    sprintf(l__buffer, "List of %d positions of communes\n", pos_communes.nbr_positions);
    Serial.print(l__buffer);
  }

  ST_COORD_POSITION *l__pos = NULL;

  if (i__flg_all == true) {
    int n = 0;
    for (n = 0; n < pos_communes.nbr_positions; n++) {
      l__pos = &pos_communes.pos_all[n];

      sprintf(l__buffer, "\t#%d: Idx [%d] Lat [%.6f] Lon [%.6f] Ele [%.1f] Distance [%u] (%s)\n",
        n, l__pos->idx, l__pos->lat, l__pos->lon, l__pos->ele, l__pos->distance, getPosCommNameOfCommune(l__pos->idx));
      Serial.print(l__buffer);
    }
  }

  // TODO: Print the xxx positions of the communes the more nears of current position

  // Position selected
  if (i__flg_all == true || i__flg_selected == true) {
    Serial.print("Position of Commune selected\n");
    sprintf(l__buffer, "\tIdx select #%d Dist Min [%d]\n", pos_communes.idx_select, pos_communes.dist_min);
    Serial.print(l__buffer);

    l__pos = &pos_communes.pos_select;

    sprintf(l__buffer, "\tIdx [%d] Lat [%.6f] Lon [%.6f] Ele [%.1f] Distance [%u] (%s)\n",
      l__pos->idx, l__pos->lat, l__pos->lon, l__pos->ele, l__pos->distance, getPosCommNameOfCommune(l__pos->idx));
    Serial.print(l__buffer);
  }
  // End: Position selected
}
#endif

#ifdef USE_SIMULATION
void Plots::m__extractStringValue(std::string &o__str, char *i__buff)
#else
void Plots::m__extractStringValue(String &o__str, char *i__buff)
#endif
{
  // ie. #Name [Maurepas - Chennevieres]
  char *l__pattern = strtok(i__buff, "[");
  if (l__pattern != NULL) {
    l__pattern = strtok(NULL, "]");
  }

  // Si la longueur de 'i__buff' est "trop grande" (absence de ']') => "Unknown" ;-)
  Serial.print("\tValue [");
  Serial.print((l__pattern != NULL) ? l__pattern : "Unknown");
  Serial.print("]\n");

#ifdef USE_SIMULATION
  o__str.append((l__pattern != NULL) ? l__pattern : "Unknown");
#else
  o__str.concat((l__pattern != NULL) ? l__pattern : "Unknown");
#endif
}

int Plots::m__extractIntValue(char *i__buff)
{
  // Test strict of the value @ string conversion
  char *l__endPtr = NULL;
  int l__int_value = -1;

  // ie. #Track points [103]
  char *l__pattern = strtok(i__buff, "[");
  if (l__pattern != NULL) {
    l__pattern = strtok(NULL, "]");
    if (l__pattern != NULL) {
      l__int_value = (int)strtol(l__pattern, &l__endPtr, 10);

      if (*l__endPtr != '\0' || *l__pattern == '\0') {
        l__int_value = -1;
      }
    }
  }

  Serial.print("\tValue [");
  Serial.print(l__int_value);
  Serial.print("]\n");

  return l__int_value;
}

float Plots::m__extractFloatValue(char *i__buff, int i__nbr_dec)
{
  // Test strict of the value @ string conversion
  char *l__endPtr = NULL;
  float l__float_value = m__nan;

  // ie. #Start Latitude [48.77300]
  char *l__pattern = strtok(i__buff, "[");
  if (l__pattern != NULL) {
    l__pattern = strtok(NULL, "]");
    if (l__pattern != NULL) {
      l__float_value = (float)strtod(l__pattern, &l__endPtr);

      if (*l__endPtr != '\0' || *l__pattern == '\0') {
        l__float_value = m__nan;
      }
    }
  }

  Serial.print("\tValue [");
  Serial.print(l__float_value, i__nbr_dec);
  Serial.print("]\n");

  return l__float_value;
}

/* Détermination si 'current' position est:
 * - Distante de plus de 100 M de la dernière position enregistrée
 * - Vers un cap différent de plus de 22.5 degrés de celui de la 'previous' position
 * 
 *   => Auquel cas, les positions 'DESMOS_COORD_CURRENT' [et 'DESMOS_COORD_PREVIOUS'] seront enregistrées dans les 'plot records'
 */
boolean Plots::isNewCurrentPosition(uint32_t *o__distance, float *o__cap, int *o__nbr_records)
{
  char l__buffer[64];
  boolean l__flg_rtn = false;

  // A priori, pas d'enregistrement
  *o__nbr_records = 0;

  // Distance depuis la dernière position enregistrée et la position courante
  uint32_t l__distance_last_recorded_to_current = calculateDistance(&st_desmos_coord[DESMOS_COORD_LAST_RECORDED].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord);
  uint32_t l__distance_previous_to_current      = calculateDistance(&st_desmos_coord[DESMOS_COORD_PREVIOUS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord);

  // Caps du déplacement depuis la dernière position enregistrée et depuis la précédente position
  float    l__cap_last_recorded_to_current      = calculateCap(&st_desmos_coord[DESMOS_COORD_LAST_RECORDED].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord);
  float    l__cap_previous_to_current           = calculateCap(&st_desmos_coord[DESMOS_COORD_PREVIOUS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord);

  /* Différences des caps à la dernière position enregistrée et à la précédente position
   * => Permet de déterminer si changement de cap horaire ou non
   */
  float    l__cap_diff_last_recorded_to_current = (l__cap_last_recorded_to_current - st_desmos_coord[DESMOS_COORD_LAST_RECORDED].coord.cap);
  float    l__cap_diff_previous_to_current      = (l__cap_previous_to_current - st_desmos_coord[DESMOS_COORD_PREVIOUS].coord.cap);

  // Caps ramenés entre [0;360[
  if (l__cap_diff_last_recorded_to_current < 0.0) {
    l__cap_diff_last_recorded_to_current += 360.0;
  }
  if (l__cap_diff_previous_to_current < 0.0) {
    l__cap_diff_previous_to_current += 360.0;
  }

  /*  Nouvelle(s) position(s) à enregistrer si:
   *  - Distance ('DESMOS_COORD_CURRENT' - 'DESMOS_COORD_LAST_RECORDED') >= 100 M && <= 22.5 degres
   *    => Le déplacement est rectiligne (sans changement de Cap horaire) sur une longue distance (100 M)
   *       => Enregistrement de la seule position courante
   *          => La dernière position enregistrée sera la position courante
   *
   *  - Distance ('DESMOS_COORD_CURRENT' - 'DESMOS_COORD_PREVIOUS') >= 30 M && > 22.5 degres
   *    => Changement de Cap horaire sur une courte distance (30 M)
   *       => Enregistrements des positions précédente (pivot de la bifurcation) et courante
   *          => La dernière position enregistrée sera la position courante
   *
   *  Remarque: Le test '> 22.5 degres' (<= 22.5 degres) est à compléter de '< (360.0 - 22.5)' (>= (360.0 - 22.5))
   *            car le Cap est ramené entre [0;360[
   */
  if (l__cap_diff_last_recorded_to_current <= 22.5 || l__cap_diff_last_recorded_to_current >= (360.0 - 22.5)) {
    // Cap <= |22.5 degres|
    if (l__distance_last_recorded_to_current >= DISTANCE_100M) {
      // Déplacement rectiligne sans changement de Cap horaire sur une longue distance (100 M)
      *o__distance = l__distance_last_recorded_to_current;
      *o__cap = l__cap_last_recorded_to_current;

      if (nbr_of_plots >= 0 && nbr_of_plots < NBR_PLOT_RECORDS) {
        // Enregistrement de la position courante
        plotRecord[nbr_of_plots].idx = nbr_of_plots;
        plotRecord[nbr_of_plots].plot.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
        plotRecord[nbr_of_plots].plot.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
        plotRecord[nbr_of_plots].plot.ele = st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele;
        nbr_of_plots++;

        *o__nbr_records = 1;
        l__flg_rtn = true;
      }
      else {
        sprintf(l__buffer, "\t\t=> Too many positions (%d >= %d)\n", nbr_of_plots, NBR_PLOT_RECORDS);
        Serial.print(l__buffer);
      }

      // La dernière position enregistrée est la position courante
      memcpy(&st_desmos_coord[DESMOS_COORD_LAST_RECORDED].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));
    }

    // Recopie de la position courante dans la position précédente si déplacement rectiligne sans changement de Cap horaire
    memcpy(&st_desmos_coord[DESMOS_COORD_PREVIOUS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));
  }
  else {
    // Cap > |22.5 degres|
    if (l__distance_previous_to_current >= DISTANCE_30M) {
      // Changement de Cap horaire sur une courte distance (30 M)
      *o__distance = l__distance_previous_to_current;
      *o__cap = l__cap_previous_to_current;

      // Test si place pour 2 enregistrements
      if (nbr_of_plots >= 0 && nbr_of_plots < (NBR_PLOT_RECORDS - 1)) {
        /* Enregistrement de la position précédente
         * => TODO: Marquage 'Pivot' qui ne sera pas à supprimer car indique un changement de Cap horaire
         */
        plotRecord[nbr_of_plots].idx = nbr_of_plots;
        plotRecord[nbr_of_plots].plot.lat = st_desmos_coord[DESMOS_COORD_PREVIOUS].coord.lat;
        plotRecord[nbr_of_plots].plot.lon = st_desmos_coord[DESMOS_COORD_PREVIOUS].coord.lon;
        plotRecord[nbr_of_plots].plot.ele = st_desmos_coord[DESMOS_COORD_PREVIOUS].coord.ele;
        nbr_of_plots++;

        /* Enregistrement de la position courante
         * => REmarque: La position courante est enregistrée à une distance <= 30 M de la position précédente
         *    => Permet de "montrer les changements de Cap horaire
         *       => Sera supprimée si déplacement rectiligne > 100 M sans changement de Cap horaire
         */
        plotRecord[nbr_of_plots].idx = nbr_of_plots;
        plotRecord[nbr_of_plots].plot.lat = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lat;
        plotRecord[nbr_of_plots].plot.lon = st_desmos_coord[DESMOS_COORD_CURRENT].coord.lon;
        plotRecord[nbr_of_plots].plot.ele = st_desmos_coord[DESMOS_COORD_CURRENT].coord.ele;
        nbr_of_plots++;

        *o__nbr_records = 2;
        l__flg_rtn = true;
      }
      else {
        sprintf(l__buffer, "\t\t=> Too many positions (%d >= %d)\n", nbr_of_plots, NBR_PLOT_RECORDS - 1);
        Serial.print(l__buffer);
      }

      // La dernière position enregistrée est la position courante
      memcpy(&st_desmos_coord[DESMOS_COORD_LAST_RECORDED].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));

      // Recopie de la position courante dans la position précédente si déplacement >= 30 M avec changement de Cap horaire
      memcpy(&st_desmos_coord[DESMOS_COORD_PREVIOUS].coord, &st_desmos_coord[DESMOS_COORD_CURRENT].coord, sizeof(COORD));
    }
  }

  return l__flg_rtn;
}
// End: Private methods
