// $Id: Statistics.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __STATISTICS__
#define __STATISTICS__

typedef enum
{
  ENUM_STATS_GENERAL_NONE = 0,

  ENUM_STATS_GENERAL_OPERATING_TIME,    // Temps de fonctionnement à compter de la réception de la 1st trame GPS valide
  ENUM_STATS_GENERAL_WITHOUT_GPS        // Temps de fonctionnement sans signal GPS (cumul des durées calculées en interne et recalées sur les trames GPS)

} ENUM_STATS_GENERAL;

typedef enum
{
  ENUM_STATS_GPS_NONE = 0,

  ENUM_STATS_GPS_NBR_STARTUP_MODULE,
  ENUM_STATS_GPS_NBR_OF_GOOD_FRAMES,
  ENUM_STATS_GPS_NBR_OF_LOSS,
  ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS,
  ENUM_STATS_GPS_NBR_OF_RETRY,
  ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT,         // Gestion de la "non" terminaison de 'NUM_INTERSTELLAR_THEME'

  ENUM_STATS_GPS_NBR_OF_WARN_TIME_TOO_LARGE,

  ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES,
  ENUM_STATS_GPS_NBR_OF_WRONG_CKS,
  ENUM_STATS_GPS_NBR_OF_INVALID_INFOS,
  ENUM_STATS_GPS_NBR_OF_ERR_TIME_PROGRESSION

} ENUM_STATS_GPS;

typedef struct {
  unsigned int        nbr_of_samples;                 // Nbr of execution of 'internalTimeCalibration()' method
  byte                value_current;                  // Reprise de 'g__counter_for_10ms'
  byte                value_min;                      // Min
  byte                value_average;                  // Moyenne
  byte                value_max;                      // Max
} ST_GEST_COUNTER_FOR_10MS;

typedef struct {
  int                 offset_seconds;                 // Nbr of seconds to apply for change Sommer/Winter time (+/- xxx Sec.)
  boolean             gps_available;

  long                epoch_init;                     // Epoch de la 1st trame GPS valide
  long                epoch_current;                  // Epoch de la trame GPS valide courrante
  long                duration_from_gps;              // Durée calculée @ GPS (epoch_current - epoch_init)
  long                duration_internal;              // Durée interne calculée @ cadencement toutes les secondes (!= duration_from_gps)
  long                duration_pulse;                 // Durée calculée @ aux pulses du GPS
  long                duration_wait_1st_cnx;          // Recopie de 'duration_internal' lorsque 'duration_from_gps' est égal à zéro (attente 1st connexion GPS)
  long                duration_not_connected;         // Durée en mode "NOT CONNECTED"

  ST_GEST_COUNTER_FOR_10MS    gest_counter_for_10ms;

  float               qos;                            // QOS [0%, 1%, ..., 99%, 100%]
} ST_STATS_GENERAL;

/* Remarque: Avec 'uint16_t', le max de 'nbr_of_good_frames' est 65535; soit (65535 / 12) minutes à raison d'une trame reçue toutes les 5"
 *           => Durée de comptabilisation maximale: (65535 / 12) / 60 = 91 heures > autonomie estimée à 80 heures ;-)
 */
typedef struct {
  uint16_t            nbr_startup_module;             // Nombre de démarrage du module GPS (comptabilisation des chaines "[#UNO: GPS Recorder - Version x.y.z]")
  uint16_t            nbr_of_good_frames;             // Nombre de trames correctes (inclut le warning 'nbr_of_warn_time_too_large')
  uint16_t            nbr_of_loss;                    // Nombre de pertes du signal GPS
  uint16_t            nbr_of_establishments;          // Nombre d'établissements (et de rétablissements) du signal GPS
  uint16_t            nbr_of_retry;                   // Nombre de reprises avant [r]établissement du signal GPS
  byte                nbr_of_retry_advert;            // Nombre de reprises de 'NUM_ADVERT_WAIT' limité à 4

  uint16_t            nbr_of_warn_time_too_large;     // Nombre de trames avec "Time gap too large"

  uint16_t            nbr_of_error_frames;            // Nombre de trames en erreur (somme de 'nbr_of_wrong_cks' + 'nbr_of_invalid_infos')
  uint16_t            nbr_of_wrong_cks;               // Nombre de trames avec mauvaise checksum
  uint16_t            nbr_of_invalid_infos;           // Nombre de trames avec "Invalid infos"
  uint16_t            nbr_of_err_time_progression;    // Nombre de trames avec "Invalid Date/Time progression"

  float               qos;                            // QOS [0%, 1%, ..., 99%, 100%]
} ST_STATS_GPS;

typedef enum {
  SPEED_GPS = 0,
  SPEED_CALC,
  SPEED_NBR_ORIGIN
} SPEED_ORIGIN;

typedef struct {
  float               val_kmh_inst;                   // Vitesse instantannée
  float               val_kmh_avg;                    // Vitesse moyenne ( >= 0.75 KmH)
  float               val_kmh_max;                    // Vitesse maximale 
  uint32_t            nbr_updating;                   // Nombre de m.a.j. pour le calcul de la vitesses moyenne
} ST_SPEED_VALUES;

typedef struct {
  uint32_t            distance_total;                 // Distance parcourue en mètres
  long                duration_total;                 // Durée totale = ('duration_in_movement' + 'duration_pause')
  long                duration_in_movement;           // Durée sans les pauses (Vinst >= 0.75 KmH ou  >= 20.0 KmH)
  long                duration_pause;                 // Durée des pauses (Vinst < 0.75 KmH)
  ST_SPEED_VALUES     speeds[SPEED_NBR_ORIGIN];
} ST_STATS_CINETICS;

class Statistics {
  private:
    ST_STATS_GENERAL  general;
    ST_STATS_GPS      gps;
    ST_STATS_CINETICS cinetic;

  public:
    Statistics();
    ~Statistics();

    // General @ 'ST_STATS_GENERAL'
    void              setOffsetSeconds(int i__offset_seconds) { general.offset_seconds = i__offset_seconds; };
    void              setGpsAvailable(boolean i__flg) { general.gps_available = i__flg; };

    void              setGenEpochInit(long i__epoch) { general.epoch_init = i__epoch; };
    long              getGenEpochInit() { return general.epoch_init; };
    void              setGenEpochCurrent(long i__epoch) { general.epoch_current = i__epoch; general.duration_from_gps = (general.epoch_current - general.epoch_init); };
    long              getGenEpochCurrent() { return general.epoch_current; };
    void              copyGenEpochCurrentToDurationInternal() { general.duration_internal = general.duration_from_gps; };

    void              incGenDurationInternal() { general.duration_internal++; };
    long              getGenDurationInternal() { return general.duration_internal; };

    void              setGpsPulse() { general.duration_pulse++; };

    void              incGenDurationNotConnected() { general.duration_not_connected++; };
    void              addGenDurationNotConnected(long i__duration) { general.duration_not_connected += i__duration; };
    long              getGenDurationFromGps() { return general.duration_from_gps; };
    long              getDiffDurationInternalToGps() { return (general.duration_internal - general.duration_from_gps); };
    long              getDiffDurationInternalToPulse() { return (general.duration_internal - general.duration_pulse); };

    void              updateGestionCounterFor10ms(byte i__value);

    void              setGenQos();
    float             getGenQos() { return general.qos; };

    // GPS @ 'ST_STATS_GPS'
    void              resetGpsCounter(ENUM_STATS_GPS i__enum);
    void              incGpsCounter(ENUM_STATS_GPS i__enum);
    uint16_t          getGpsCounter(ENUM_STATS_GPS i__enum);
    void              setGpsQos();
    float             getGpsQos() { return gps.qos; };

    // Cinématique @ 'ST_STATS_CINETICS'
    void              clearCineticInfos();
    boolean           getCineticInfos(ST_STATS_CINETICS *o__infos);
    void              updateCineticInfosGps(float i__speed_kmh_inst);
    boolean           updateCineticInfosCalc(uint32_t i__distance, long i__delta_time);

    boolean           isMovementInProgressPedestrian(SPEED_ORIGIN i__type) {
      return (cinetic.speeds[i__type].val_kmh_inst > 0.75 && cinetic.speeds[i__type].val_kmh_inst <= 10.0) ? true : false;
    };

    boolean           isMovementInProgressPedestrian() {
      return (cinetic.speeds[SPEED_GPS].val_kmh_inst > 0.75  && cinetic.speeds[SPEED_CALC].val_kmh_inst > 0.75
           && cinetic.speeds[SPEED_GPS].val_kmh_inst <= 10.0 && cinetic.speeds[SPEED_CALC].val_kmh_inst <= 10.0) ? true : false;
    };

    boolean           isMovementInProgressBike(SPEED_ORIGIN i__type) {
      return (cinetic.speeds[i__type].val_kmh_inst > 10.0 && cinetic.speeds[i__type].val_kmh_inst <= 20.0) ? true : false;
    };

    boolean           isMovementInProgressBike() {
      return (cinetic.speeds[SPEED_GPS].val_kmh_inst > 10.0  && cinetic.speeds[SPEED_CALC].val_kmh_inst > 10.0
           && cinetic.speeds[SPEED_GPS].val_kmh_inst <= 20.0 && cinetic.speeds[SPEED_CALC].val_kmh_inst <= 20.0) ? true : false;
    };

    boolean           isMovementInProgressCar(SPEED_ORIGIN i__type) {
      return (cinetic.speeds[i__type].val_kmh_inst > 20.0) ? true : false;
    };

    boolean           isMovementInProgressCar() {
      return (cinetic.speeds[SPEED_GPS].val_kmh_inst > 20.0 && cinetic.speeds[SPEED_CALC].val_kmh_inst > 20.0) ? true : false;
    };

    boolean           isMovementInProgress(SPEED_ORIGIN i__type) {
      return (isMovementInProgressPedestrian(i__type) || isMovementInProgressBike(i__type) || isMovementInProgressCar(i__type));
    };

    boolean           isMovementInProgress() {
      return (isMovementInProgressPedestrian() || isMovementInProgressBike() || isMovementInProgressCar());
    };

    // Impression des statistisques
    void              printAll();
};

extern Statistics                  *g__stats;

#endif
