// $Id: GpsCompass.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __GPS_COMPASS__
#define __GPS_COMPASS__

// Prompt au lancement
#define PROMPT                "\n#ESP32: GpsCompass - Date: 2023/06/24 - Version: 1.4.3c\n"

#define PROMPT_GPS_MODULE               "#UNO: GPS Recorder - Version x.y.z"    // -> 'GESTION_SUB_STATE3_IDENT'
#define SIZE_STR_GPS_MODULE             18    // Comparaison sur "#UNO: GPS Recorder" seulement (abandon de la version)

#define INDEX_FOR_BASCULE     3

typedef enum {
  GESTION_STATE_UNKNOWN = 0,
  GESTION_STATE_NOT_CONNECTED,
  GESTION_STATE_CONNECTED
} ENUM_GESTION_STATES;

typedef enum {
  GESTION_SUB_STATE_UNKNOWN = 0,
  GESTION_SUB_STATE_INIT_IN_PROGRESS,
  GESTION_SUB_STATE_INIT_DONE,
  GESTION_SUB_STATE_INIT_RETRY,
  GESTION_SUB_STATE_INIT_DONE2
} ENUM_GESTION_SUB_STATES;

typedef enum {
  GESTION_SUB_STATE2_UNKNOWN = 0,
  GESTION_SUB_STATE2_CURRENT_POSITION_SAVED
} ENUM_GESTION_SUB_STATES2;

typedef struct {
  ENUM_GESTION_STATES       state_pre;                    // Pour detection le passage "connecté" -> "non connecté"
  ENUM_GESTION_STATES       state;
  ENUM_GESTION_SUB_STATES   sub_state;
  ENUM_GESTION_SUB_STATES2  sub_state2;
  int                       duration_connect;             // Comptabilise les durées pour les états "connecté" / "non connecté"
  boolean                   flg_detect_module_gps;        // TODO: To remove
  boolean                   flg_retry_last_sequence;
  boolean                   flg_force_last_pos_saved;     // Forcage de la position enregistrée
  boolean                   flg_save_current_positions;   // Enregistrement des positions courantes dans les plots records
  boolean                   flg_err_missing_usb_end;      // TODO: To remove: Pour la présentation de l'erreur 1! fois
} ST_GESTION;

extern byte                     g__state_leds;

extern void execTreatmentsWithSynthesisPrompts();

extern void synthesisDateTime(ENUM_SYNTH_DATE_TIME_MODES i__mode);
extern ENUM_SYNTH_DISTANCE_TYPES synthesisDistance(uint32_t i__distance, ENUM_SYNTH_POSITION_MODES i__pos_mode, ENUM_SYNTH_DISTANCE_MODES i__dist_mode);

extern void callback_activity_error_1();

extern ST_GESTION                  g__gestion;

extern void IRAM_ATTR doIsrRisingInputWT2003SBusy();
extern void IRAM_ATTR doIsrFallingInputWT2003SBusy();

#if USE_INCOMING_CMD
extern void gestionOfCommands();
#endif
#endif
