// $Id: Timers.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __TIMERS__
#define __TIMERS__

#define DURATION_TIMER_CONNECT                (30 * 100L)               // 30" before state 'NOT_CONNECTED'

#define DURATION_TIMER_EXEC_SYNTH             ((7 * 60 + 30) * 100L)    // 7'30"
#define DURATION_TIMER_ACTIVITY_EXEC_SYNTH    (100L)                    // 1"
#define DURATION_TIMER_ADVERT                 ((4 * 60 + 30) * 100L)    // 4'30"
#define DURATION_TIMER_FIFO_TX_PLAY           (10L)                     // 100 mS

#if USE_REAL_PROMPTS_DURATION_FROM_TIME
#define DURATION_TIMER_SHORT_FAMINE           (30 * 100L)               // 30"
#endif

#define DURATION_TIMER_LONG_FAMINE            (60 * 100L)               //  1'
#define DURATION_TIMER_VERY_LONG_FAMINE       (60 * 100L)               //  1' après l'expiration de 'TIMER_LONG_FAMINE_FIFO_TX_PLAY'

#define DURATION_TIMER_WAIT_FIFO_TX_PLAY      33L                       //  330 mS (TODO: Non utilisée à terme)
#define DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN  50                        //  500 mS

#define DURATION_TIMER_WAIT_FIFO_TX_OTHER     25L                       //  250 mS

#define DURATION_TIMER_FOR_ERROR_1            (30 * 100L)               // 30"
#define DURATION_TIMER_FOR_ERROR_2            (100L)                    //  1"
#define DURATION_TIMER_FOR_ERROR_2_PAUSE      (150L)                    //  1"5
#define DURATION_TIMER_FOR_ERROR_3_NO         (5L)                      //  50 mS __/-\_____________________________
#define DURATION_TIMER_FOR_ERROR_3_NBR        (50L)                     // 500 mS __/----------------------\________
#define DURATION_TIMER_FOR_ERROR_3_0          (10L)                     // 100 mS __/----\__________________________
#define DURATION_TIMER_FOR_ERROR_3_1          (60L)                     // 600 mS __/----------------------------\__

#define DURATION_TIMER_TYPE_SYNTH_THE_PLOT_IS               (30 * 100L)               // 30" (Le tracé est distant de xxx mètres...)
#define DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1     (30 * 100L)               // 30"
#define DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2     (30 * 100L)               // 30"
#define DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3     (30 * 100L)               // 30"
#define DURATION_TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED          (30 * 100L)               // 30"

#define DURATION_TIMER_TYPE_SYNTH_POSITION                  (30 * 100L)               // 30"
#define DURATION_TIMER_TYPE_SYNTH_POS_COMMUNE               (60 * 100L)               // 1'

#define DURATION_TIMER_BUTTON                 (100L)                     // 1"

#define DURATION_TIMER_SDCARD_ACCES           (100L)                     // 1"
#define DURATION_TIMER_SDCARD_ERROR           (100L)                     // 1"
#define DURATION_TIMER_SDCARD_RETRY_INIT      (200L)                     // 2" 
#define DURATION_TIMER_SDCARD_INIT_ERROR      (100L)                     // 1"

#define DURATION_TIMER_GPS_RECEPTION_STATE    (20 * 100L)                // 20"

#define DURATION_TIMER_WT2003S_RESP_ERROR     (10L)                      // 100 mS

#ifdef USE_SIMULATION
#define DURATION_TIMER_SIMU_GPS               (500L)                     // 5" (simulation reception trame GPS)
#endif

typedef enum {
  TIMER_CONNECT = 0,                        // Gestion états "connecté/non connecté"
  TIMER_EXEC_SYNTH,                         // Time-out passage dans 'execTreatmentsWithSynthesisPrompts()'
  TIMER_ACTIVITY_EXEC_SYNTH,                // Activité de 'execTreatmentsWithSynthesisPrompts()' reportée sur la Led Yellow
  TIMER_ADVERT,                             // Time-out d'insertion de 'advert' "veuillez toujours patienter" dans la diffusion de 'NUM_INTERSTELLAR_THEME'
  TIMER_ACTIVITY_FIFO_TX_PLAY,              // Activity if 'isAllPromptsPlayed()' method return false reportée sur la Led Yellow  TIMER_ACTIVITY_FIFO_TX_PLAY,              // Activity if 'isAllPromptsPlayed()' method return false reportée sur la Led Yellow
  TIMER_SHORT_FAMINE_FIFO_TX_PLAY,          // Famine de "courte" durée de la FIDO/Tx Play
  TIMER_LONG_FAMINE_FIFO_TX_PLAY,           // Famine de "longue" durée de la FIDO/Tx Play
  TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY,      // Famine de "très longue" durée de la FIDO/Tx Play

  TIMER_WAIT_FIFO_TX_PLAY,                  // Timer d'attente pour la consommation de la FIFO/Tx Play initialisé avec la valeur lue de la FIFO/Tx Play
  TIMER_WAIT_FIFO_TX_OTHER,                 // Timer d'attente pour la consommation de la FIFO/Tx Other initialisé avec la valeur lue de la FIFO/Tx Other

  TIMER_FOR_ERROR_1,
  TIMER_FOR_ERROR_2,
  TIMER_FOR_ERROR_3,

  TIMER_TYPE_SYNTH_THE_PLOT_IS,             // Timer pour différer la synyhèse associée à 'TYPE_SYNTH_THE_PLOT_IS'
  TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1,   // Timer pour différer la synyhèse associée à 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1'
  TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2,   // Timer pour différer la synyhèse associée à 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2'
  TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3,   // Timer pour différer la synyhèse associée à 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3'
  TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED,        // Timer pour différer la synyhèse associée à 'TYPE_SYNTH_YOU_HAVE_ARRIVED'
  TIMER_TYPE_SYNTH_POSITION,                // Timer pour différer la synyhèse de la position ("La n-ième position 'enregistrée/mémorisée' est à ..."
  TIMER_TYPE_SYNTH_POS_COMMUNE,             // Timer pour différer la synyhèse de la position à une commune

  TIMER_BUTTON,                             // Timer pour la détermination des actions "Button" (armé au 1st appui, action à son expiration)
  TIMER_NEW_MENU,                           // Timer pour determiner le positionnement du bouton rotatif sur un nouveau menu
  TIMER_WAIT_BUTTON,                        // Timer d'attente de l'appui bouton

#if USE_TIMER_DIFFUSION
  TIMER_DIFFUSION,                          // Timer pour le temps de diffusion d'un message (analyse @ 'fin de sa diffusion')
#endif
#if USE_TIMER_ALL_DIFFUSIONS
  TIMER_ALL_DIFFUSIONS,                     // Timer pour le temps de diffusion total des messages (analyse @ 'fin de toutes les diffusions')
#endif

  TIMER_SDCARD_ACCES,                       // Timer pour l'acces a la SD Card (allumage d'une duree minimun de 'DURATION_TIMER_SDCARD_ACCES')
  TIMER_SDCARD_ERROR,                       // Timer pour la presentation de l'erreur d'acces a la SD Card (allumage d'une duree minimun de 'DURATION_TIMER_SDCARD_ERROR')
  TIMER_SDCARD_RETRY_INIT,                  // Timer pour les tentatives d'initialisation
  TIMER_SDCARD_INIT_ERROR,                  // Timer pour la presentation de l'erreur d'initialisation

  TIMER_GPS_RECEPTION_STATE,                // Timer pour la detection absence de reception des trames GPS

  TIMER_WT2003S_RESP_ERROR,                 // Timer pour les reponses du WT2003S (cf. 'WT2003S::update()')

#ifdef USE_SIMULATION
  TIMER_SIMU_GPS,                           // Timer de simulation reception trame GPS
#endif

  NBR_OF_TIMERS                             // Numbers of timers defined
} DEF_TIMERS;

typedef struct {
  DEF_TIMERS      idx;
  boolean         flg_trace;                // Trace Yes/No: 0/1
  const char      *text;
} ST_TIMERS;

typedef struct {
  boolean         in_use;
  long            duration_init;            // Value of duration at the initialization (allows to calculate the current duration) 
  long            duration;                 // Duration in 10 mS and -1L for not armed
  void            (*fct_callback)(void);    // Callback function without argument and no return
} ST_TIMER;

class Timers {
  private:
    ST_TIMER      timer[NBR_OF_TIMERS];
    long          prompts_duration;         // Duration of prompts for 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
    size_t        num_prompt_played;        // Numéro du prompt joué sous la contrainte de 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
    long          prompts_duration_total;   // Total duration of prompts for 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'

  public:
    Timers();
    ~Timers();

    void      update();
    void      test();

    boolean isInUse(DEF_TIMERS i__def_timer);
    boolean start(DEF_TIMERS i__def_timer, long i__duration, void (*i__fct_callback)(void));
    boolean restart(DEF_TIMERS i__def_timer, long i__duration);
    long    getDuration(DEF_TIMERS i__def_timer);
    void    addDuration(DEF_TIMERS i__def_timer, long i__duration);

    long    getPromptsDuration() { return prompts_duration; };
    long    addPromptsDuration(long i__duration);
    long    getPromptsDurationTotal() { return prompts_duration_total; };
    long    addPromptsDurationTotal(long i__duration);

    void    clrPromptsDuration();   // Raz de la durée en cours et totale

    size_t  getNumPromptPlayed() { return num_prompt_played; };
    size_t  getAncIncNumPromptPlayed() { size_t l__num_prompt_played = num_prompt_played++; return l__num_prompt_played; };
    void    clrNumPromptPlayed() { num_prompt_played = 0; };

    const char *getText(DEF_TIMERS i__def_timer);

    boolean   stop(DEF_TIMERS i__def_timer);
};

extern Timers                  *g__timers;

#endif
