#ifndef __PLOTS__
#define __PLOTS__

#define NBR_PLOT_RECORDS      512

#define DISTANCE_MIN_FOR_ADJUST   50
#define CAP_MIDDAY                12

typedef enum {
  RECORDS_NOT_AVAILABLE = 0,
  RECORDS_IN_BUILD,
  RECORDS_IN_ERROR,
  RECORDS_AVAILABLE
} ENUM_STATE_RECORDS;

// Definitions a l'image de DESMOS (graphique 'GpsPilot') pour un calcul a l'identique
typedef struct {
  COORD         coord;
  int32_t       lat_y;   // Ordonnee courante relative a la 1st jonction
  int32_t       lon_x;   // Abscisse courante relative a la 1st jonction
} ST_DESMOS_COORD;

typedef struct {
  int32_t       lat_y;   // Ordonnee de la jonction relative a la 1st jonction
  int32_t       lon_x;   // Abscisse de la jonction relative a la 1st jonction
  int32_t       A;       // Constantes pour les equations parametriques des segments
  int32_t       B;       // et des droites perpendiculaires
  float         X;       // Abscisse et
  float         Y;       // Ordonnee du point d'intersection sur le segment
  int32_t       Ds;      // Distance signee au point d'intersection sur le segment
  int32_t       Dj;      // Distance non signee aux jonctions
  boolean       Ps;      // Produit scalaire pour la distance signee au point d'intersection sur le segment
} ST_DESMOS;
// Fin: Definitions a l'image de DESMOS (graphique 'GpsPilot') pour un calcul a l'identique

typedef struct {
  float         lat;
  float         lon;
  float         ele;
  boolean       flgValid;
  float         capToNextPlot;
  int           capHoursToNextPlot;
  uint32_t      distance;
} ST_PLOT;

typedef struct {
  byte          section;
  int           idx;
  ST_PLOT       plot;
  ST_DESMOS     desmos;
} ST_PLOT_RECORD;

typedef enum {
  DESMOS_COORD_CURRENT = 0,         // Position courante
  DESMOS_COORD_PREVIOUS,            // Position précédente
  DESMOS_COORD_LAST_RECORDED,       // Dernière position enregistrée
  DESMOS_COORD_PREVIOUS_FOR_STATS,  // Position précédente pour les statistiques cinématiques calculées
  DESMOS_COORD_NBR                  // Nombre d'éléments
} ENUM_DESMOS_COORD;

typedef enum {
  TYPE_DATAS_PLOTS_NO_MEAN = 0,
  TYPE_DATAS_PLOTS_RESULTS,
  TYPE_DATAS_POS_RECORDED
} ENUM_TYPE_COORD;

#define SPEED_EPSILON         (float)0.01

typedef struct {
  boolean         flg_in_progress;

  int             num_plot_init;
  int             num_plot_current;
  int             num_plot_max;
  float           speed_kmh;
  long            duration_previous;

  // Pour le calcul du nombre de samples
  int             num_sample_current;
  int             num_sample_max;
  int             nbr_samples;
} ST_SIMU_MOVE;

class Plots {
  private:
    boolean             flg_adjust_plots_avail;
    boolean             flg_detect_sens_inverse;    // Deplacement sur le trace en sens inverse detecte
    int                 optim_level;                // Niveau de l'optimisation du 'GpsPilot.txt' [0, 1, ...]
    int                 idx_of_adjust_plot;
    ST_DESMOS_COORD     st_desmos_coord[DESMOS_COORD_NBR];

    ENUM_STATE_RECORDS  state;
    int                 nbrOfRecords;
    uint32_t            checksum_calc;
    size_t              size_datas_calc;
    uint32_t            checksum_read;
    size_t              size_datas_read;

    byte                num_section;                  // Numero de la section [0, 1, ...] en cours (configuration et utilisation)

#ifdef USE_SIMULATION
    std::string         name_of_plots;
#else
    String              name_of_plots;
#endif

    int                 nbr_of_plots;
    int                 nbr_of_plots_adjust;

    /* Properties of current adjust plots and current position
     * => TODO: Creation of a structure...
     */
    boolean             flg_make_synth_properties;    // 0/1: La synthèse des propriétés [n'est pas / est] à faire
    uint32_t            total_distance;
    uint32_t            total_distance_2D;
    uint32_t            total_distance_3D;
    uint32_t            remaining_distance;           // Distance restante @sens du tracé et déterminée dès que sur le tracé ;-)
    uint32_t            distance_on_plots;            // Distance parcourue sur le tracé @sens du tracé et déterminée dès que sur le tracé ;-)
                                                      // => Vérification si (distance_on_plots + remaining_distance ~= total_distance)
    ENUM_PLOTS_DIR      plots_direction;
    // End: Properties of current adjust plots and current position
    
    ST_PLOT_RECORD      plotRecord[NBR_PLOT_RECORDS];         // TBC: Test si les 'plot.lat' et 'plot.lon' restent inchangés
    ST_PLOT_RECORD      plotRecordAdjust[NBR_PLOT_RECORDS];
    ST_PLOT             plotStartPosition;
    ST_RESULTS          plotResults;
    ST_RESULTS          plotResultsSave;     // Mise à disposition des résultats à tout moment

    ST_POSITION         positions;           // Positions 'enregistrées' et 'mémorisées' à synthétiser @ à la position courante

#if USE_POS_COMMUNES
    ST_POS_COMMUNES     pos_communes;        // Positions de la commune à synthétiser @ à la position courante
#endif

    ST_SIMU_MOVE        st_simu_move;

    // Méthodes internes
    int                 m__idx;
    uint32_t            m__crc_tmp;
    size_t              m__size_cumul;
    float               m__nan;

#if USE_ATTRACTOR_TO_PLOT
#define NBR_DIST_AVG    12
    uint32_t            t_dist_avg[NBR_DIST_AVG];
    size_t              idx_dist_avg;
#endif

    void                m__clearIndex();
    int                 m__nextIndex();
    void                m__calculChecksumInc(uint8_t *i__datas, size_t i__size, boolean i__flg_end = false);

#ifdef USE_SIMULATION
    void                m__extractStringValue(std::string &o__str, char *i__buff);
#else
    void                m__extractStringValue(String &o__str, char *i__buff);
#endif

    int                 m__extractIntValue(char *i__buff);
    float               m__extractFloatValue(char *i__buff, int i__nbr_dec = 0);

    void m__set_section(byte i__value) { if (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) plotRecord[m__idx].section = i__value; };
    void m__set_idx(int i__value)      { if (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) plotRecord[m__idx].idx = i__value; };
    void m__set_lat(float i__value)    { if (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) plotRecord[m__idx].plot.lat = i__value; };
    void m__set_lon(float i__value)    { if (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) plotRecord[m__idx].plot.lon = i__value; };
    void m__set_ele(float i__value)    { if (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) plotRecord[m__idx].plot.ele = i__value; };

    int   m__get_section() const { return (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) ? plotRecord[m__idx].section : -1; };
    int   m__get_idx()     const { return (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) ? plotRecord[m__idx].idx : -1; };
    float m__get_lat()     const { return (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) ? plotRecord[m__idx].plot.lat : m__nan; };
    float m__get_lon()     const { return (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) ? plotRecord[m__idx].plot.lon : m__nan; };
    float m__get_ele()     const { return (m__idx >= 0 && m__idx < NBR_PLOT_RECORDS) ? plotRecord[m__idx].plot.ele : m__nan; };

    boolean isNewCurrentPosition(uint32_t *o__distance, float *o__cap, int *o__nbr_records);

    boolean calcSimuMoveSamples(long i__duration);
    // Fin: Méthodes internes

  public:
    Plots();
    ~Plots();

    int      getOptimLevel() const { return optim_level; };

    void     initCoordToNoMean(COORD *i__coord);
    void     setCoordCurrent(COORD *i__coord, boolean i__flg_force = false);
    void     getCoordCurrent(COORD *o__coord) const;
    boolean  isValidCoord(COORD *i__coord) const { return (i__coord->lat == i__coord->lat && i__coord->lon == i__coord->lon && i__coord->ele == i__coord->ele); };

    int      getNbrOfPlots() const { return nbr_of_plots; };
    void     resetNbrOfPlots() { nbr_of_plots = 0; state = RECORDS_NOT_AVAILABLE; };
    int      getNbrOfPlotsAdjust() const { return nbr_of_plots_adjust; };
    //void     setNbrOfPlotsAdjust(int i__value) { nbr_of_plots_adjust = i__value; };
    void     copyPlotsRecordToAdjust();

    void     getPlotRecord(int i__num_plot, ST_PLOT_RECORD *o__plot_record) {
      if (i__num_plot >= 0 && i__num_plot < nbr_of_plots && o__plot_record != NULL) {
        memcpy(o__plot_record, &plotRecord[i__num_plot], sizeof(ST_PLOT_RECORD));
      }
    };

    void     setPlotRecord(int i__num_plot, ST_PLOT_RECORD *i__plot_record) {
      if (i__num_plot >= 0 && i__num_plot < nbr_of_plots && i__plot_record != NULL) {
        memcpy(&plotRecord[i__num_plot], i__plot_record, sizeof(ST_PLOT_RECORD));
      }
    };

    void     getPlotAdjust(int i__num_plot_adjust, ST_PLOT_RECORD *o__plot_adjust) {
      if (i__num_plot_adjust >= 0 && i__num_plot_adjust < nbr_of_plots_adjust && o__plot_adjust != NULL) {
        memcpy(o__plot_adjust, &plotRecordAdjust[i__num_plot_adjust], sizeof(ST_PLOT_RECORD));
      }
    };

    void     setPlotAdjust(int i__num_plot_adjust, ST_PLOT_RECORD *i__plot_adjust) {
      if (i__num_plot_adjust >= 0 && i__num_plot_adjust < nbr_of_plots_adjust && i__plot_adjust != NULL) {
        memcpy(&plotRecordAdjust[i__num_plot_adjust], i__plot_adjust, sizeof(ST_PLOT_RECORD));
      }
    };

    uint32_t getChecksum() const { return checksum_calc; };
    size_t   getSizeDatas() const { return size_datas_calc; };

    boolean  isAvailable() const { return (state == RECORDS_AVAILABLE) ? true : false; };
    void     setAvailable() { state = RECORDS_AVAILABLE; };

    void     getStartingPosition(COORD *o__coord);

    uint32_t calculateDistance(COORD *i__previous, COORD *i__current);
    float    calculateCap(COORD *i__previous, COORD *i__current);
    int      calculateCapHours(float i__cap);

    void     adjustmentCapAndDistance();
    bool     adjustmentCapAndDistanceMore();

    void     calculOfCapsAndDistance();

    // Methods for desmos calculation
    boolean  calculOfAdjustPlotLine();
    void     desmosCalculOfRelativeLatLon();
    int32_t  desmosCalculOfRelativeLat(float i__lat);
    int32_t  desmosCalculOfRelativeLon(float i__lon);
    void     desmosCalculOfA();
    void     desmosCalculOfB(int i__idx, int32_t l__x, int32_t l__y);
    void     desmosCalculOfXY(int i__idx);
    void     desmosCalculOfDs(int i__idx, int32_t l__x, int32_t l__y);
    void     desmosCalculOfDj(int i__idx, int32_t l__x, int32_t l__y);
    void     desmosCalculOfPs(int i__idx, int32_t l__x, int32_t l__y);
    // End: Methods for desmos calculation

    void     convertLonXLatYToLatLon(COORD *o__coord, int32_t i__lat_y, int32_t i__lon_x);

    void     clearResults();
    void     finalyzeResults();
    void     printResults(char *o__buffer, ST_RESULTS *i__results);
    void     getResults(ST_RESULTS *o__st_results);

    void     inversePlotsRecord();

    boolean  getResultsSave(ST_RESULTS *o__results);

    void     toString();
    void     toStringAdjust(boolean i__flg_all = false);
    void     toStringWaypoints();
    void     toStringSegments();

    void     toStringPositions(boolean i__flg_all = false, boolean i__flg_selected = false, boolean i__flg_waypoints = false);

#if USE_POS_COMMUNES
    void     toStringPosCommunes(boolean i__flg_all = false, boolean i__flg_selected = false);
#endif

    // Methods for properties of adjust plots
    boolean         isMakeSynthProperties() const { return flg_make_synth_properties; };
    void            setMakeSynthProperties(boolean i__value) { flg_make_synth_properties = i__value; };
    uint32_t        getTotalDistance() const { return total_distance; };
    uint32_t        getRemainingDistance() const { return remaining_distance; };
    uint32_t        getDistanceOnPlots() const { return distance_on_plots; };
    ENUM_PLOTS_DIR  getPlotsDirection() const { return plots_direction; };

    // Méthodes pour le calcul des distances aux positions 'enregistrées' et 'mémorisées'
    int             getNbrPositions()            const { return positions.nbr_positions; };
    int             getNbrPositionsRecorded()    const { return positions.nbr_pos_recorded; };
    int             getNbrPositionsMemorized()   const { return positions.nbr_pos_memorized; };
    boolean         isPositionsAvailable()       const { return positions.flg_available; };
    void            setCurrentPosition(COORD *i__coord);
    boolean         calculOfPositions();
    boolean         getPositionSelected(ST_COORD_POSITION *o__st_coord_position);   
    boolean         addPosition(ST_COORD_POSITION *i__pos);

#if USE_POS_COMMUNES
    // Méthodes pour le calcul des distances aux positions des communes
    int             getNbrPosCommunes()           const { return pos_communes.nbr_positions; };
    // boolean         isPosCommunesAvailable()      const { return pos_communes.flg_available; };
    void            setCurrentPosCommunes(COORD *i__coord);
    boolean         calculOfPosCommunes();
    boolean         getPosCommunesSelected(ST_COORD_POSITION *o__st_coord_position);
#endif

    void            stopSimuMove() { st_simu_move.flg_in_progress = false; };
    void            startSimuMove() { st_simu_move.flg_in_progress = true; };
    void            setSimuMoveParameters(int i__num_plot, float i__speed_kmh);
    void            getSimuMovePosition(COORD *io__coord_current, long i__duration);

    void            setDetectSensInverse(boolean i__flg) { flg_detect_sens_inverse = i__flg; };
    boolean         getDetectSensInverse() const { return flg_detect_sens_inverse; };

#ifdef USE_SIMULATION   // For 'SimuMove.cpp'
    boolean         isSimuMoveInProgress() { return st_simu_move.flg_in_progress; };
#endif
};

extern Plots                  *g__plots;
#endif
