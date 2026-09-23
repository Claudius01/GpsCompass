#ifndef __SDCARD__
#define __SDCARD__

#include "src/SD.h"

#define SD_CS   5     // @KiCad: Select SDCard

// @KiCad: MISO   19
// @KiCad: SCK    18
// @KiCad: MOSI   23

#define NAME_OF_FILE_GPS_FRAMES       "/FRAME/GpsFrames.txt"

class SDCard {
  private:
    bool        flg_init;
    int         nbr_of_init_retry;
    uint8_t     cardType;
    uint64_t    cardSize;

    bool        flg_sdcard_in_use;
    bool        flg_inh_append_gps_frame;

    bool preparing();

  public:
    SDCard();
    ~SDCard();

    bool init();
    void init(int i__nbr_retry);
    void end();

    void startActivity();
    void stopActivity(boolean i__flg_no_error = true);

    void callback_sdcard_retry_init_more();

    bool isInit()  { return flg_init; };
    bool isInUse() { return flg_sdcard_in_use; };

    // Methods for '/FRAME/GpsFrames.txt' gestion
    void setInhAppendGpsFrame(bool i__flg) { flg_inh_append_gps_frame = i__flg; };
    bool appendGpsFrame(const char *i__frame, boolean i__flg_force_append = false);

    // Methods for tests
    bool printInfos();   
    bool listDir(const char *i__dir);
    bool exists(const char *i__path);
    bool readFile(const char *i__file);
    bool getFileLine(const char *i__file, String &o__line, bool i__flg_close = false);
    bool appendFile(const char *i__file, const char *i__line);
    bool renameFile(const char *i__path_from, const char *i__path_to);
    bool deleteFile(const char *i__path);
    // End: Methods for tests
};

extern SDCard     *g__sdcard;
extern boolean    g__flg_inh_sdcard_ope;

extern void disableTimer();
extern void enableTimer();

extern void callback_end_sdcard_acces();
extern void callback_end_sdcard_error();
extern void callback_sdcard_retry_init();
extern void callback_sdcard_init_error();
#endif
