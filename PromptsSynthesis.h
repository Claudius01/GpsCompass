// $Id: PromptsSynthesis.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __PROMPTS_SYNTHESIS__
#define __PROMPTS_SYNTHESIS__

#ifdef USE_SIMULATION
#include "../ArduinoTypes.h"
#endif

#define USE_FLAT_STONE_THROW_INTO_WATER   1
#define USE_SYNTH_YOU_ARE_ON_THE_PLOT     0     // 0/1: Pas de / synhèse de "Vous êtes [toujours] sur le tracé"

#define SIZE_1_COMMAND        3       // 3 bytes for 1! play command

/* General synthesis
 *  - Synchro with the end playing of this prompts
 *    Remarque: #nnn prompts commun entre 'SELECT_SONG_IN_MP3_DIRECTORY' et 'INSERT_SONG_FROM_ADVERT_DIRECTORY' non supportés
 *              à cause du 'switch/case' dans l'automate
 */
#define NUM_PROMPT_HELLO_WORLD          0x0000
#define NUM_PROMPT_JE_SUIS__FEMALE      0x0001
#define NUM_PROMPT_JE_SUIS__MALE        0x0002
#define NUM_RETRY_PROMPTS_DIFFUSION     0x0003
#define NUM_LOSS_GPS                    0x0004
#define NUM_OUPS_MALE                   0x0005
#define NUM_OUPS_FEMALE                 0x0006
#define NUM_PLEASE_WAIT_GPS             0x0007
#define NUM_PLEASE_WAIT_RECOVERY_GPS    0x0008
#define NUM_GPS                         0x000a
#define NUM_RECOVERY_GPS                0x000b
#define NUM_PROMPT_JE_SUIS__FEMALE_BIS  0x000c
#define NUM_SORRY_WAIT_AGAIN            0x000d
#define NUM_INTERSTELLAR_THEME          0x0064
#define NUM_ADVERT_WAIT                 0x00c8      // Warning: <- 'advert'
#define NUM_TEST_MESSAGE                0x03e7

/* Message "15/1002_salamisound-3569093-flat-stone-thrown-into-water.mp3" en remplacement de 'NUM_TEST_MESSAGE' (répertoire 'mp3')
   => "Pierre plate jetée dans l'eau" dans le répertoire 15 synthétisé par 'buildCommandsPromptsGeneral()'
      qui ne prend pas en compte le numéro du répertoire ;-)
*/
#define NUM_SUDDEN_EVENT_DIFFERENT      0xf3e8        //                                          -> Not used
#define NUM_FLAT_STONE_THROW_INTO_WATER 0xf3ea        //                                          -> Suppression de la courte famine
#define NUM_DING_DONG_BELL_DOORBELL_2   0xf3ee        // Dong (2 sons)                            -> Enregistrement de 2 plots
#define NUM_SHOCK_IMPACT_METALLIC       0xf3f0        //                                          -> Suppression de la longue famine
#define NUM_SHUTTER_SLR_CAMERA_DIGITAL  0xf3f1        // Ouverture d'un objectif d'appareil photo -> Spare
#define NUM_SWITCH_TOGGLE_OR_ROTARY     0xf3f2        // Interrupteur à bascule ou rotatif        -> Forçage diffusion sur timeout de 'TIMER_DIFFUSION'
#define NUM_DING_DONG_BELL_DOORBELL_3   0xf3f3        // Dong (3 sons)                            -> Spare
#define NUM_UNLOCK_OLD_CASTEL_ON_AN     0xf3f4        // Ouverture d'un vieux château             -> Vitesse < 0.75 KmH 
#define NUM_DING_DONG_BELL_DOORBELL_1   0xf3f5        // Dong (1 son)                             -> Enregistrement d'un plot

#define NUM_BIG_RATCHET_HALF_TURN       0xf3f7        // Clic tic                                 -> Rotation du bouton

typedef enum {
  BASE_HELLO_WORLD = 0,
  BASE_JE_SUIS__FEMALE,
  BASE_JE_SUIS__MALE,
  BASE_RETRY_PROMPTS_DIFFUSION,
  BASE_PLEASE_WAIT_GPS,
  BASE_PLEASE_WAIT_RECOVERY_GPS,
  BASE_LOSS_GPS,
  BASE_OUPS_MALE,
  BASE_OUPS_FEMALE,
  BASE_GPS,
  BASE_RECOVERY_GPS,
  BASE_JE_SUIS__FEMALE_BIS,
  BASE_SORRY_WAIT_AGAIN,
  BASE_INTERSTELLAR_THEME,
  BASE_ADVERT_WAIT,
  BASE_TEST_MESSAGE,

  BASE_SUDDEN_EVENT_DIFFERENT,
  BASE_FLAT_STONE_THROW_INTO_WATER,
  BASE_DING_DONG_BELL_DOORBELL_2,
  BASE_SHOCK_IMPACT_METALLIC,
  BASE_SHUTTER_SLR_CAMERA_DIGITAL,
  BASE_SWITCH_TOGGLE_OR_ROTARY,
  BASE_DING_DONG_BELL_DOORBELL_3,
  BASE_UNLOCK_OLD_CASTEL_ON_AN,
  BASE_DING_DONG_BELL_DOORBELL_1,

  BASE_BIG_RATCHET_HALF_TURN,

  BASE_LAST_PROMPT_GENERAL            // Number [BASE_LAST_PROMPT_GENERAL + 1] and last of positions of array
} BASE_PROMPTS;

extern size_t buildCommandsPromptsGeneral(int i__base_prompt, byte **o__command);     // Commands Play into FIFO/Tx Play
// End: General synthesis

// Synthesis of date and time
typedef enum {
  SYNTH_DATE = 0,
  SYNTH_TIME,
  SYNTH_DATE_TIME_ALL,
  SYNTH_DATE_TIMES_NBR_MODES
} ENUM_SYNTH_DATE_TIME_MODES;

extern size_t buildCommandsPromptsDateTime(ST_DATE_AND_TIME *i__st_date_time, byte **o__command, ENUM_SYNTH_DATE_TIME_MODES l__mode, uint16_t **o__durations, size_t *o__nbr_durations);
// End: Synthesis of date and time

// Synthesis of distance and cap synthesis
#define DISTANCE_VERY_NEAR            10L     // 10 m
#define DISTANCE_15M                  15L     // 15 m
#define DISTANCE_20M                  20L     // 20 m
#define DISTANCE_25M                  25L     // 25 m
#define DISTANCE_30M                  30L     // 30 m

#define DISTANCE_NEAR        DISTANCE_30M

#define DISTANCE_50M                  50L     // 50 m
#define DISTANCE_100M                100L     // 100 m
#define DISTANCE_300M                300L     // 300 m
#define DISTANCE_500M                500L     // 500 m
#define DISTANCE_1KM                1000L     // 1 Km
#define DISTANCE_2_5KM              2500L     // 2.5 Km
#define DISTANCE_5KM                5000L     // 5 Km
#define DISTANCE_10KM              10000L     // 10 Km
#define DISTANCE_30KM              30000L     // 30 Km
#define DISTANCE_100KM            100000L     // 100 Km
#define DISTANCE_1000KM          1000000L     // 1000 Km

#define DISTANCE_FOR_ON_THE_PLOT      DISTANCE_30M

typedef enum {
  SYNTH_DIST_NO_MEAN = 0,
  SYNTH_DIST_FROM_STARTING_POS,     // #1: Par rapport à la position enregistrée dans le programme
  SYNTH_DIST_FROM_POS_SAVE          // #2: Par rapport à la position acquise sur le terrain
} ENUM_SYNTH_DISTANCE_MODES;

typedef enum {
  SYNTH_DIST_UNKNOWN_TYPE = 0,
  SYNTH_DIST_VERY_NEAR,             // 1: Distance très proche
  SYNTH_DIST_NEAR,                  // 2: Distance proche
  SYNTH_DIST_DISTANT,               // 3: Distance éloignée
  SYNTH_DIST_VERY_DISTANT           // 4: Distance très éloignée
} ENUM_SYNTH_DISTANCE_TYPES;

typedef enum {
  SYNTH_NO_POSITION = 0,            // #0: Pas de synthèse de la position
  SYNTH_POSITION,                   // #1: Synthèse de la position
  SYNTH_POSITION_NBR_MODES
} ENUM_SYNTH_POSITION_MODES;

// Range into [0.0;360.0]
#define CAP_MIN                         0.0
#define CAP_MAX                       360.0

// Threshold: increment of (360.0 / 16) = 22.5
#define THRESHOLD_CAP_NNE             11.25
#define THRESHOLD_CAP_NE              32.75
#define THRESHOLD_CAP_ENE             56.25
#define THRESHOLD_CAP_E               78.75
#define THRESHOLD_CAP_ESE            101.25
#define THRESHOLD_CAP_SE             123.75
#define THRESHOLD_CAP_SSE            146.25
#define THRESHOLD_CAP_S              168.75
#define THRESHOLD_CAP_SSW            191.25
#define THRESHOLD_CAP_SW             213.75
#define THRESHOLD_CAP_WSW            236.25
#define THRESHOLD_CAP_W              258.75
#define THRESHOLD_CAP_WNW            281.25
#define THRESHOLD_CAP_NW             303.75
#define THRESHOLD_CAP_NNW            326.25
#define THRESHOLD_CAP_N              348.75

// Threshold: increment of (360.0 / 12) = 30.0
#define THRESHOLD_CAP_13H             15.0
#define THRESHOLD_CAP_14H             45.0
#define THRESHOLD_CAP_15H             75.0
#define THRESHOLD_CAP_16H            105.0
#define THRESHOLD_CAP_17H            135.0
#define THRESHOLD_CAP_6H             165.0
#define THRESHOLD_CAP_7H             195.0
#define THRESHOLD_CAP_8H             225.0
#define THRESHOLD_CAP_9H             255.0
#define THRESHOLD_CAP_10H            285.0
#define THRESHOLD_CAP_11H            315.0
#define THRESHOLD_CAP_MIDDAY         345.0

// Elevation in the range [-4999;+4999]
#define ELEVATION_ROUND_FLOAT         10.0      // Altitudes identiques si |diff| < 10 mètres
#define ELEVATION_ROUND_FIX              5      // Arrondi aux 5 mètres les plus proches (fix calcul)
#define ELEVATION_MAX                 4999

typedef enum {
  SYNTH_ELEVATION_NONE = 0,           // #0: Non définie
  SYNTH_ELEVATION_ZERO,               // #1: Altitude du niveau de la mer (0 mètres)
  SYNTH_ELE_BELOW,                    // #2: Altitude négative (au dessous)
  SYNTH_ELE_ABOVE,                    // #3: Altitude positive (au dessus)
  SYNTH_ELE_LOWER,                    // #4: Altitude négative (plus bas)
  SYNTH_ELE_UPPER                     // #5: Altitude positive (plus haut)
} ENUM_SYNTH_ELE_SIGN;

typedef enum {
  SYNTH_NO_ELEVATION = 0,             // #0: Pas de synthèse de l'altitude
  SYNTH_ELE_ABSOLUTE,                 // #1: Synthèse de l'altitude absolue
  SYNTH_ELE_RELATIVE                  // #2: Synthèse de l'altitude relative ('au dessous' eor 'au dessus')
} ENUM_SYNTH_ELE_MODES;

// Structure commune à toutes les synthèses
typedef struct {
  int           idx;                        // Index record
  byte          commands[SIZE_1_COMMAND];   // 3 bytes of command
  const char    *text;                      // Name of file for traces
  boolean       flg_duration;               // true or false: Prise ou non de la durée du prompt
  uint16_t      duration_from_time;         // Duration in Sec. of the prompt extracted from the time (HH:MM:SS read by explorer)
  long          duration_from_length;       // Duration in mS of the prompt extracted from the length (xxx KBytes of the file)
} ST_PROMPTS_DEF;

extern const ST_PROMPTS_DEF     g__prompts_general[BASE_LAST_PROMPT_GENERAL + 1];

extern boolean                  g__flg_inh_synth_dist_to_plot;

extern size_t buildCommandsPromptsDistance(uint32_t i__distance, ENUM_SYNTH_DISTANCE_TYPES *o__dist_type, byte **o__command, ENUM_SYNTH_POSITION_MODES i__pos_mode, ENUM_SYNTH_DISTANCE_MODES i__dist_mode, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsCap(float i__cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsCapHours(float i__cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsElevation(float i__ele_current, float i__ele_ref, byte **o__command, ENUM_SYNTH_ELE_MODES i__ele_mode, uint16_t **o__durations, size_t *o__nbr_durations);
// End: Synthesis of distance and cap synthesis

extern uint32_t distanceRoundedForThePlot(uint32_t i__distance);

extern size_t buildCommandsPromptsDistToPlots(ST_RESULTS *i__results, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations, boolean *o__flg_more_synth);
extern size_t buildCommandsPromptsBranchOfNow(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsContinueOn(uint32_t i__distance, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsAndContinueOn(uint32_t i__distance, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsPrepareYourselves(uint32_t i__distance, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);

extern size_t buildCommandsPromptsBuildingOfPlots(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsChangeSensOfPlots(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);

extern size_t buildCommandsPromptsRecordPlots(int i__nbr_plots, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
extern size_t buildCommandsPromptsNoMovementInProgress(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);

extern size_t buildCommandsPromptsPlotsProperties(uint32_t i__total_distance, boolean i__flg_about, ENUM_PLOTS_DIR i__plots_direction, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);

extern size_t buildCommandsPromptsYouHaveArrived(boolean i__flg_almost, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);

extern size_t buildCommandsPromptsPosition(ST_COORD_POSITION *i__st_position, int i__nbr_positions, boolean *o__flg_synth_cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);

// Méthodes de récupération des informations de la structure 'ST_POS_COMMUNES_COORD g__pos_communes_coord[]'
extern size_t       getPosCommNbrPositions();
extern bool         getPosCommPositionOfCommune(size_t i__idx, float *o__lat, float *o__lon, float *o__ele);
extern const char  *getPosCommNameOfCommune(size_t i__idx);
extern int          getPosCommRule1(size_t i__idx);
extern int          getPosCommRule2(size_t i__idx);

extern size_t buildCommandsPromptsPosCommune(ST_COORD_POSITION *i__st_position, boolean *o__flg_synth_cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations);
#endif
