// $Id: Misc.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __MISC__
#define __MISC__

#include <sstream>

#define USE_INVERSE_LEDS               1

#define USE_FORCE_NO_GPS               1

// Masques pour la recopie des états des leds
#define STATE_NO_LEDS          0
#define STATE_LED_RED         (1 << 0)
#define STATE_LED_YELLOW      (1 << 1)
#define STATE_LED_GREEN       (1 << 2)
#define STATE_LED_SDCARD      (1 << 3)

#define LED_RED                       21      // @KiCad
#define LED_YELLOW                     0      // @KiCad
#define LED_GREEN                     15      // @KiCad
#define LED_SDCARD                    33      // @KiCad

#define USE_INCOMING_CMD              1

#if USE_INCOMING_CMD
#define USE_FORCE_CURRENT_COORD       1
#define USE_FORCE_TIME                1
#else
#define USE_FORCE_CURRENT_COORD       0
#define USE_FORCE_TIME                0
#endif

/*  Value of 'USE_REAL_PROMPTS_DURATION_FROM_TIME':
 *  0: Use 'ST_PROMPTS_DEF.duration_from_length' (real duration in mS)
 *  1: Use 'ST_PROMPTS_DEF.duration_from_time'   (estimated duration in Sec.)
 */
#define USE_REAL_PROMPTS_DURATION_FROM_TIME             0
#define USE_ATTRACTOR_TO_PLOT                           1
#define USE_POS_COMMUNES                                1

#ifndef USE_SIMULATION
#define USE_TIMER_ALL_DIFFUSIONS                        1
#define USE_TIMER_DIFFUSION                             1
#endif

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

// Années bissextiles
#define LEAP_YEAR(year) ((year % 4) == 0)

typedef enum {
	GMT,
	LOCALTIME
} DEF_GMT_LOCAL;

typedef enum {
  NO_SOMMER_WINTER = 0,
  SOMMER,
  WINTER
} ENUM_SOMMER_WINTER;

typedef struct {
  int tm_sec;    /* Seconds. [0-60] (1 leap second) */
  int tm_min;    /* Minutes. [0-59] */
  int tm_hour;   /* Hours. [0-23] */
  int tm_mday;   /* Day.   [1-31] */
  int tm_mon;    /* Month. [0-11] */
  int tm_year;   /* Year - 1900.  */
} ST_TM;

typedef struct {
  char                  seconds;              // Seconde [0..59]
  char                  minutes;
  char                  hours;
  char                  day;
  char                  month;
  char                  year;                 // Year same 20XX
  char                  day_of_week;
  byte                  nbr_days_before;      // Nombre du même jour avant et après dans le mois
  byte                  nbr_days_after;       // (nbr_days_before + 1 + nbr_days_after) = Nbr total du même jour
  ENUM_SOMMER_WINTER    sommer_winter;
  long                  epoch;
} ST_DATE_AND_TIME;

/* Définitions pour la détermination du changement d'heure été/hiver avec
   le numéro du jour dans la semaine (dernier dimance de mars et octobre)
*/
typedef enum {
  JEUDI = 0,
  VENDREDI,
  SAMEDI,
  DIMANCHE,
  LUNDI,
  MARDI,
  MERCREDI
} ENUM_DAYS_OF_WEEK;

typedef struct {
  ENUM_DAYS_OF_WEEK   num_day;              // 0: Jeudi, 1: Vendredi, ..., 6: Mercredi
  const char          *name;                // car le 01/01/1970 est un Jeudi ;-)
} ST_DAYS_IN_WEEK;

typedef struct {
  boolean             flg_available;        // Dates de basculement disponibles (true/false)
  ST_DATE_AND_TIME    st_date_sommer;       // Date de basculement heure d'été
  ST_DATE_AND_TIME    st_date_winter;       // Date de basculement heure d'hiver
  ST_DAYS_IN_WEEK     st_days_in_week[7];   // => Si nbr_days_after = 0, ce jour est le dernier du mois ;-)
} ST_FOR_SOMMER_TIME_CHANGE;
// Fin: Définitions pour la détermination du changement d'heure été/hiver

// Type de synthèse vocale à produire
typedef enum {
  TYPE_SYNTH_NO_MEAN = 0,
  TYPE_SYNTH_THE_PLOT_IS,             // #1: Le tracé est à xxx mètres au 'cap' [à 'cap_hours' @ cap du déplacement]
  TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1,   // #2: xxx > 50 M:         Vous êtes sur le tracé, continuez sur xxx mètres au 'cap' [à 'cap_hours' @ cap du déplacement]
  TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2,   // #3: 15 M < xxx <= 50 M: Vous êtes sur le tracé, préparez vous à bifurquer à yyy mètres au 'cap' [à 'cap_hours' @ cap du déplacement]
  TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3,   // #4: xxx <= 15 M:        Bifurquez maintenant au 'cap' [à 'cap_hours' @ cap du déplacement] et continuez sur xxx mètres
  TYPE_SYNTH_YOU_HAVE_ARRIVED         // #5: Vous êtes [presque] arrivé à la fin du tracé déterminé @ 'Plots::remaining_distance' (2 distances pour le [presque] ;-)
} ENUM_TYPE_SYNTH;

typedef struct {
  long        duration;     // Temps interne recopié de 'ST_STATS_GENERAL::duration_internal'
  float       lat;
  float       lon;
  float       ele;
  float       cap;
  float       speedKmH;
} COORD;

typedef enum {
  TYPE_SEG_JONC_UNKNOWN = 0,
  TYPE_SEGMENT,
  TYPE_JONCTION
} ENUM_TYPE_SEG_JONC;

typedef struct {         // Synthese des resultats
  int           Ds_idx;  // Index dans la liste 'plotRecordAdjust' (donne le numero du segment [0..N-2] -> [1..N-1])
  int           Dj_idx;  // Index dans la liste 'plotRecordAdjust' (donne le numero de la jonction [0..N-1])
  int32_t       Ds_min;  // Distance minimale non signee au segment ou INT_MAX
  int32_t       Dj_min;  // Distance minimale non signee a la jonction ou INT_MAX

  /* Synthese des résultats
     => L'index dans la liste 'plotRecordAdjust' donne le numero du segment [0..N-2] -> [1..N-1]
        si 'flg_segment == true'; sinon, c'est le numero de la jonction [0..N-1]
  */
  boolean              flg_avalaible;
  ENUM_TYPE_SEG_JONC   flg_seg_jonc;   // Unknown/Segment/Jonction
  int                  idx;            // Index dans la liste 'plotRecordAdjust'
  int32_t              D_min;          // Distance minimale non signee au segment ou INT_MAX
  int32_t              X;              // Abscisse et
  int32_t              Y;              // Ordonnee du point d'intersection sur le segment ou coordonnee de la jonction
  float                lat;            // Latitude et
  float                lon;            // Longitude du point d'intersection sur le segment ou de la jonction

  // Informations pour la synthese vocale
  ENUM_TYPE_SYNTH      type_synth;     // Type de la synthèse #n

#if USE_ATTRACTOR_TO_PLOT
  uint32_t             D_min_avg;      // Moyenne des 'NBR_DIST_AVG' précédentes 'D_min' (distances au tracé)
#endif

  uint32_t             distance;       // Distance sur le tracé à synthétiser (#1/#2: xxx, #3: yyy ou #4: INT_MAX si pas à synthétiser)
  float                cap;            // Cap à synthétiser pour rester sur le tracé (#1/#2/#3/#4 ou nan si pas à synthétiser dans le cas d'une vitesee < 1 Km)

  boolean              flg_dist_to_track_too_large;   // Distance au tracé trop grande
} ST_RESULTS;

typedef enum {
  PLOTS_DIR_NO_MEAN = 0,          // Aucune direction déterminée (état à l'initialisation)
  PLOTS_DIR_UNKNOWN,              // Erreur dans la détermination
  PLOTS_DIR_GO,                   // Sens "Aller"
  PLOTS_DIR_RETURN                // Sens "Retour"
} ENUM_PLOTS_DIR;

/* Définitions pour la synthèse de la position 'enregistrée' ou 'mémorisée' la plus proche de la position actuelle
 * => La sélection de ladite position n'est pas fonction de son type 'enregistrée' ou 'mémorisée' mais uniquement
 *    de sa distance à la position actuelle...
 */
#define NBR_POSITIONS   20

typedef enum {
  TYPE_POSITION_NO_MEAN = 0,
  TYPE_POSITION_RECORDED,
  TYPE_POSITION_MEMORIZED
} ENUM_TYPE_POSITION;

typedef struct {
  int                   idx;
  ENUM_TYPE_POSITION    type;     // Non utilisée dans le cas de 'ST_POS_COMMUNES.pos_select'
  float                 lat;
  float                 lon;
  float                 ele;
  uint8_t               range;    // [0, 1, ... n]: Pour la synthèse de '(n+1)-ième'... (non utilisée dans le cas de 'ST_POS_COMMUNES.pos_select')

  uint32_t              distance;
} ST_COORD_POSITION;

typedef struct {
  boolean               flg_available;
  int                   nbr_positions;            // Nbr de positions définies <= 'NBR_POSITIONS' et == 'nbr_pos_recorded' + 'nbr_pos_memorized'
  int                   nbr_pos_recorded;         // - Nbr de positions 'enregistrées'
  int                   nbr_pos_memorized;        // - Nbr de positions 'mémorisées'

  int                   idx_current;              // Index de la position à calculer (1! position toutes les 10 mS ;-)
                                                  // => Si == 'nbr_positions' => calcul de toutes les positions terminé
  COORD                 current_pos;              // Coordonnées de la position courante
  int                   idx_select;               // Index [0, 1, ..., 'nbr_positions'[ de la position 'enregistrée' eor 'mémorisée' sélectionnée 
  uint32_t              dist_min;                 // Distance min de la position actuelle à toutes les positions définies
                                                  // => Position recopiée dans 'pos_select' pour une synthèse de cette position si < xxx mètres
  ST_COORD_POSITION     pos_select;               // Coordonnées de la postion sélectionnée avec 'dist_min' la plus petite
  ST_COORD_POSITION     pos_all[NBR_POSITIONS];   // Coordonnées de toutes les positions 'enregistrées' et 'mémorisées'
} ST_POSITION;
// Fin: Définitions pour la synthèse de la position 'enregistrée' ou 'mémorisée' la plus proche de la position actuelle

#if USE_POS_COMMUNES
#define NBR_POS_COMMUNES           512            // Réservation pour 512 positions de communes (cf. 'ST_POS_COMMUNES_COORD g__pos_communes_coord[]')

// Définitions pour la synthèse de la position de la commune la plus proche @ à la position actuelle
typedef struct {
  //boolean               flg_available;
  int                   nbr_positions;            // Nbr de positions définies <= 'NBR_POS_COMMUNES'

  int                   idx_current;              // Index de la position à calculer (1! position toutes les 10 mS ;-)
                                                  // => Si == 'nbr_positions' => calcul de toutes les positions terminé
  COORD                 current_pos;              // Coordonnées de la position courante
  int                   idx_select;               // Index [0, 1, ..., 'nbr_positions'[ de la position de la commune sélectionnée 
  uint32_t              dist_min;                 // Distance min de la position actuelle à toutes les positions définies
                                                  // => Position recopiée dans 'pos_select' pour une synthèse de cette position si < xxx mètres
  ST_COORD_POSITION     pos_select;               // Coordonnées de la position sélectionnée avec 'dist_min' la plus petite maj à la fin du parcours de toutes les positions
  ST_COORD_POSITION     pos_all[NBR_POS_COMMUNES];   // Coordonnées de toutes les positions des communes recopiées à partir de 'ST_POS_COMMUNES_COORD g__pos_communes_coord[]'
} ST_POS_COMMUNES;
// Fin: Définitions pour la synthèse de la position de la commune la plus proche @ à la position actuelle
#endif

// Structure Input/Output pour la gestion des menus
typedef enum {
  MENU_ACTION_NONE = 0,
  MENU_ACTION_NEW_MENU,
  MENU_ACTION_BUTTON_1,
  MENU_ACTION_BUTTON_2
} MENU_ACTION_TYPE;

typedef struct {
  boolean                       flg_sens;       // false: Sens anti-horaire (decrement) true: Sens horaire (increment)
  MENU_ACTION_TYPE              menu_action;
  int                           menu_value;
  byte                          volume_level;   // Volume Level (input/output)
} ST_GESTION_MENUS;

extern ST_FOR_SOMMER_TIME_CHANGE            g__st_for_sommer_time_change;

#if USE_FORCE_CURRENT_COORD
extern boolean                              g__flg_force_coord_current;
extern COORD                                g__force_coord_current;
#endif

extern void initToNaN(float *o__value);

extern void convertFloatToString(char *o__buffer, float i__value, int i__nbr_dec);
extern void hexDump(std::ostringstream &o_out, const char *i__bytes, const size_t i__nbr_bytes);
extern long buildGpsDateTime(const char i__date[], const char i__time[], char *o_date_time, ST_DATE_AND_TIME *o__dateAndTime);
extern boolean setSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime);
extern void calcSommerTimeChange(ST_DATE_AND_TIME *io__dateAndTime);
extern ENUM_SOMMER_WINTER getSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime);
extern void applySommerWinterHour(ST_DATE_AND_TIME *io__dateAndTime_presentation);

// Methodes de conversion
extern int     convAscHexa2Int(char i__val);
extern char    convHexa2Ascii(char i__value);
extern boolean convAscii2Hexa(char i__char, byte *io__hexa);
extern void    convertDurationToString(char *o__result, long i__duration, boolean i__flg_with_sign);
extern float   convertToDecimalDegres(char *i__t_value);

extern size_t translatePromptsKT403AToWT2003S(byte *i__command, size_t i__size, byte *o__command, boolean i__flg_trace = false);

extern uint32_t cksum_inc(uint8_t *i__datas, size_t i__size, uint32_t i__crc_tmp, size_t i__size_cumul);

extern void getSimuMovePosition(COORD *io__coord_current);

extern byte                     g__state_leds;

#endif
