// $Id: SerialNMEA.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __SERIAL_NMEA__
#define __SERIAL_NMEA__

#include "Misc.h"

#define USE_STATS_SERIAL_NMEA       1
#define USE_PRINT_GPS_VALUES        0

#define RXD1            4           // @KiCad: Reception NMEA a 9600 bauds
#define TXD1            2           // @KiCad: Not used

#define NBR_OF_CHAR_IN_FIFO                512
#define NBR_OF_CHAR_IN_NMEA_FRAME          128

typedef struct {
  time_t epoch;
  char t_date_time[132];    // Date/Time formated (ie. 2020/03/12 07:45:56)
} ST_UNIX_TIME;

#define MAX_BUFFER        512

#define INFO_5_SEC_ELAPSED        (1 << 12)
#define INFO_GPS_STATUS           (1 << 11)
#define INFO_GPS_TIME             (1 << 10)
#define INFO_GPS_DATE             (1 <<  9)
#define INFO_GPS_LAT              (1 <<  8)
#define INFO_GPS_LAT_DIR          (1 <<  7)
#define INFO_GPS_LON              (1 <<  6)
#define INFO_GPS_LON_DIR          (1 <<  5)
#define INFO_GPS_SPEED_KNOTS      (1 <<  4)
#define INFO_GPS_SPEED_KMH        (1 <<  3)
#define INFO_GPS_CAP              (1 <<  2)
#define INFO_GPS_ELEVATION        (1 <<  1)
#define INFO_GPS_NBR_SATELLITES   (1 <<  0)

#define INFO_GPS_ALL  (INFO_5_SEC_ELAPSED | INFO_GPS_STATUS | INFO_GPS_TIME | INFO_GPS_DATE | INFO_GPS_LAT | INFO_GPS_LAT_DIR | INFO_GPS_LON | INFO_GPS_LON_DIR | INFO_GPS_SPEED_KNOTS | INFO_GPS_SPEED_KMH | INFO_GPS_CAP | INFO_GPS_ELEVATION | INFO_GPS_NBR_SATELLITES)

typedef enum {
  GPS_NO_RECEPTION = 0,             // Aucune reception du module GPS
  GPS_RECEPTION_0_SATELLITE,        // Reception ou non des trames 'GPGSV' avec 0 satellite
  GPS_RECEPTION_1_3_SATELLITES,     // Reception des trames 'GPGSV' avec 1, 2 ou 3 satellites
  GPS_RECEPTION_4_MORE_SATELLITES   // Reception des trames 'GPGSV' avec 4 satellites ou plus
} ENUM_GPS_RECEPTION_STATE;

typedef enum {
  GPS_UNKNOWN = 0,
  GPS_NORTH,
  GPS_EAST,
  GPS_SOUTH,
  GPS_WEST
} ENUM_GPS_DIRECTION;

typedef struct {
  char                frameNMEA[NBR_OF_CHAR_IN_NMEA_FRAME];

  unsigned int        flg_infos;
  char                status;

  char                time[6 + 1];            // Time: HHMMSS (ie. 155403 for 15:54:03)
  char                date[6 + 1];            // Date: DDMMYY (ie. 110320 for 11/03/2020)
  char                date_save[6 + 1];       // Pour la detection "New traces" (TODO: Change the text)
  long                dateAndTimeGPS;         // Time GPS as UNIX (GMT)
  long                delta_time;             // Delta time with previous good frame
  char                t_latitude[4 + 1 + 4 + 1];    // Valeur en ASCII recopiee de GPRMC (ex @ Home: "4847.0711")
  float               latitude;                     // Valeur en +/- degres décimal calculee (ex @ Home: 48.784576)
  ENUM_GPS_DIRECTION  north_south;
  char                t_longitude[5 + 1 + 4 + 1];   // Valeur en ASCII recopiee de GPRMC (ex @ Home: "00157.5171")
  float               longitude;                    // Valeur en +/- degrés decimal calculee (ex @ Home:  1.958657)
  ENUM_GPS_DIRECTION  east_west;

  float               speedKnots;
  float               speedKmH;
  float               cap;
  float               elevation;
  int                 nbr_satellites;
  char                status_antenna[32];

#ifdef USE_SIMULATION    // [USE_SIMULATION...
  char                t_speedKnots[8];
  char                t_speedKmH[8];
  char                t_cap[8];
  char                t_elevation[8];
#endif              // ..USE_SIMULATION]

  unsigned char       checksum;

  unsigned int        dateAndTimeValue;
  ST_DATE_AND_TIME    dateAndTime;
  ST_UNIX_TIME        dateAndTimeUNIX_GMT;

  boolean             flg_valid;
} ST_GPS_INFOS;

class SerialNMEA {
  private:

#if USE_FORCE_NO_GPS
  boolean             flg_force_no_gps;
#endif

  // FIFO circulaire des caracteères recus
  unsigned int        idx_write;
  unsigned int        idx_read;
  char                fifo[NBR_OF_CHAR_IN_FIFO];
  // Fin: FIFO circulaire des caractères recus

  boolean             frame_nmea_available;
  boolean             frame_tlv_available;

#ifndef USE_SIMULATION
  String              frame_nmea_str;
  String              frame_tlv_str;
#else
  std::string         frame_nmea_str;
  std::string         frame_tlv_str;
#endif

  ST_GPS_INFOS        gps_nmea_infos;
  ST_GPS_INFOS        gps_tlv_infos;

#if USE_STATS_SERIAL_NMEA
  unsigned int        nbr_frames_nmea;
  unsigned int        nbr_frames;

  unsigned int        err_fifo_0;       // La trame precedente n'a pas ete decodee (normal si != 0)
  unsigned int        err_fifo_1;       // Saturation FIFO => Reinit 'spare_min' (arrive si demande de 'Stats' et/ou 'Serial NMEA stats')
  int                 spare_min;

  unsigned int        err_cks;
  unsigned int        err_nmea;
  unsigned int        err_missing_field;
#endif

  ENUM_GPS_RECEPTION_STATE  gps_reception_state;

  void extractInfosGPRMC(const char *i__frame);
  void extractInfosGPGGA(const char *i__frame);
  void extractInfosGPGSV(const char *i__frame, boolean i__flg_trace = false);
  void extractInfosGPTXT(const char *i__frame, boolean i__flg_trace = false);

  void convertFloatToString(char *o__buffer, char *i__buff_cmp, float i__value, int i__nbr_dec);
  void buildStrForExternal(String &o__str);

  public:
    SerialNMEA(int i__speed_uart);
    ~SerialNMEA();

    void update();

    boolean isFrameNMEAAvailable();
    boolean isFrameTLVAvailable();

#ifndef USE_SIMULATION
    String & getFrameTLV();
    String & getFrameNMEA();
#else
    std::string & getFrameTLV();
    std::string & getFrameNMEA();
#endif

    boolean calculChecksumNMEA(char *i__frame, unsigned char *o__cks_calculated, unsigned char *o__cks_expected = NULL);
    boolean calculChecksum(char *i__frame, unsigned char *o__cks_calculated);

#ifndef USE_SIMULATION
    boolean extractGpsTLVInfos(String &o__message, boolean i__simu_move_flg = false);
#else
    boolean extractGpsTLVInfos(std::string &o__message, boolean i__simu_move_flg = false, char *i__date = NULL, char *i__time = NULL);
#endif
    void convertFloatToString(char *o__buffer, float i__value, int i__nbr_dec);

    long getUnixTimeGMT(char *o__t_date_time, ST_DATE_AND_TIME *o__dateAndTime);

    boolean isGpsTLVInfosValid() const { return gps_tlv_infos.flg_valid; };
    void setGpsTLVInfosValid(boolean i__flg) { gps_tlv_infos.flg_valid = i__flg; };

    boolean isMulipleOfHours();
    boolean isMulipleOf10Minutes();
    boolean isMulipleOf5Minutes();
    boolean isNewMinute();

    float  getLat() const { return gps_tlv_infos.latitude; };
    float  getLon() const { return gps_tlv_infos.longitude; };
    float  getCap() const { return gps_tlv_infos.cap; };

#if USE_STATS_SERIAL_NMEA
    unsigned int getNbrFramesNMEA() const { return nbr_frames_nmea; };
    unsigned int getNbrFrames() const { return nbr_frames; };
    unsigned int getErrFifo(int i__num_err) const;
    int getSpareMin() const { return spare_min; };
    unsigned int getErrCks() const { return err_cks; };
    unsigned int getErrNMEA() const { return err_nmea; };
    unsigned int getErrMissingField() const { return err_missing_field; };
    void hexDumpFifoRx();
#endif

#ifdef USE_SIMULATION
    void setFrameTLVAvailable(boolean i__value) { frame_tlv_available = i__value; };
#endif

    void extractInfosNMEA(const char *i__frame);

    void setGpsReceptionState(ENUM_GPS_RECEPTION_STATE i__state) { gps_reception_state = i__state; };
    ENUM_GPS_RECEPTION_STATE getGpsReceptionState() const { return gps_reception_state; };

    const char *getStatusAntenna() const { return gps_nmea_infos.status_antenna; };

#if USE_FORCE_NO_GPS
    void setFlgForceNoGps(boolean i__flg) { flg_force_no_gps = i__flg; };
#endif
};

extern void callback_exec_gps_reception_state();

extern SerialNMEA    *g__serial_nmea;

#ifdef USE_SIMULATION
extern char				g__force_date[];
extern char				g__force_time[];
extern unsigned int	g__force_duration;
#endif

#endif
