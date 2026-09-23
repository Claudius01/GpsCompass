#ifndef __FILE_GPS_PILOT__
#define __FILE_GPS_PILOT__

#define NAME_OF_FILE_GPS_PILOT       "/PILOT/GpsPilot.txt"

#define PROMPT_FRAME_SEPARATOR          "#---"
#define PROMPT_FRAME_SECTION            "#Section"
#define PROMPT_FRAME_DESCRIPTION        "#Description"
#define PROMPT_FRAME_NAME               "#Name"
#define PROMPT_FRAME_DATE               "#Date"
#define PROMPT_FRAME_DURATION           "#Duration"
#define PROMPT_FRAME_START_LATITUDE     "#Start Latitude"
#define PROMPT_FRAME_START_LONGITUDE    "#Start Longitude"
#define PROMPT_FRAME_START_ELEVATION    "#Start Elevation"
#define PROMPT_FRAME_CUMUL_ELE_POSITIVE "#Cumul Elevation Positive"
#define PROMPT_FRAME_CUMUL_ELE_NEGATIVE "#Cumul Elevation Negative"
#define PROMPT_FRAME_MEANING            "#Sens"
#define PROMPT_FRAME_DISTANCE_2D        "#Distance 2D"
#define PROMPT_FRAME_DISTANCE_3D        "#Distance 3D"
#define PROMPT_FRAME_SPEED_AVERAGE      "#Speed average"
#define PROMPT_FRAME_TRACK_POINTS       "#Track points"
#define PROMPT_FRAME_OPTIM_LEVEL        "#Optim level"
#define PROMPT_FRAME_DISTANCE_SAMPLES   "#Distance samples"
#define PROMPT_FRAME_INTERPOLATION      "#Interpolation"
#define PROMPT_FRAME_POS_TO_RECORD      "#Positions to record"
#define PROMPT_FRAME_CHECKSUM           "#Checksum"

class FileGpsPilot {
  private:
    int                 nbrOfRecords;

    // Attributs pour le calcul de la checksum incrementale
    uint32_t            crc_tmp;
    size_t              size_cumul;
    uint32_t            checksum_calc;
    size_t              size_datas_calc;
    uint32_t            checksum_read;
    size_t              size_datas_read;
    // Fin: Attributs pour le calcul de la checksum incrementale

    void calculChecksumIncClearValues();
    void calculChecksumInc(uint8_t *i__datas, size_t i__size, boolean i__flg_end = false);

  public:
    FileGpsPilot();
    ~FileGpsPilot();

    bool getFileLines(char *i__file_name, boolean i__flg_trace = false);
};

extern SDCard     *g__sdcard;
#endif
