// $Id: Timers.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifdef USE_SIMULATION
#include <stdio.h>
#include <string.h>
#include <string>

#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#else
#include <Arduino.h>
#include "Misc.h"
#endif

#include "Timers.h"

static ST_TIMERS        g__st_timers[NBR_OF_TIMERS] =
{
  { TIMER_CONNECT,                   true,  "Connect" },
  { TIMER_EXEC_SYNTH,                true,  "Exec Synth" },
  { TIMER_ACTIVITY_EXEC_SYNTH,       true,  "Activity Exec Synth" },
  { TIMER_ADVERT,                    true,  "Advert" },
  { TIMER_ACTIVITY_FIFO_TX_PLAY,     false, "Activity FIFO/Tx Play" },

  { TIMER_SHORT_FAMINE_FIFO_TX_PLAY,      true, "Short famine FIFO/Tx Play" },
  { TIMER_LONG_FAMINE_FIFO_TX_PLAY,       true, "Long famine FIFO/Tx Play" },
  { TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY,  true, "Very long famine FIFO/Tx Play" },

  { TIMER_WAIT_FIFO_TX_PLAY,         true,  "Wait FIFO/Tx Play" },
  { TIMER_WAIT_FIFO_TX_OTHER,        true,  "Wait FIFO/Tx Other" },
  { TIMER_FOR_ERROR_1,               true,  "Timer for error #1" },
  { TIMER_FOR_ERROR_2,               false, "Timer for error #2" },
  { TIMER_FOR_ERROR_3,               false, "Timer for error #3" },

  { TIMER_TYPE_SYNTH_THE_PLOT_IS,           true,  "The plot is..." },
  { TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1, true,  "The plot #1" },
  { TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2, true,  "The plot #2" },
  { TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3, true,  "The plot #3" },
  { TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED,      true,  "You have arrived" },

  { TIMER_TYPE_SYNTH_POSITION,       true,  "The position is at..." },
  { TIMER_TYPE_SYNTH_POS_COMMUNE,    true,  "The commune is at..." },

  { TIMER_BUTTON,                    true,  "Timer Button" },
  { TIMER_NEW_MENU,                  false, "Timer New Menu" },
  { TIMER_WAIT_BUTTON,               false, "Timer Wait Button" },

#if USE_TIMER_DIFFUSION
  { TIMER_DIFFUSION,                 false,  "Timer diffusion" },
#endif
#if USE_TIMER_ALL_DIFFUSIONS
  { TIMER_ALL_DIFFUSIONS,            false,  "Timer all diffusions" },
#endif

  { TIMER_SDCARD_ACCES,              false,  "Timer SD Card acces" },
  { TIMER_SDCARD_ERROR,              false,  "Timer SD Card error" },
  { TIMER_SDCARD_RETRY_INIT,         true,   "Timer SD Card retry init" },
  { TIMER_SDCARD_INIT_ERROR,         true,   "Timer SD Card init error" },

  { TIMER_GPS_RECEPTION_STATE,       false,  "Timer GPS Reception State" },

  { TIMER_WT2003S_RESP_ERROR,        false,  "Timer reponse en erreur du WT2003S" },

#ifdef USE_SIMULATION
  { TIMER_SIMU_GPS,                  false,   "Timer Simu GPS" },
#endif
};

Timers::Timers() : prompts_duration(0L), num_prompt_played(0), prompts_duration_total(0L)
{
  Serial.println("Timers::Timers()");

  size_t n = 0;
  for (n  = 0; n < NBR_OF_TIMERS; n++) {
    timer[n].in_use       = false;
    timer[n].duration     = -1L;
    timer[n].fct_callback = NULL;
  }
}

Timers::~Timers()
{
  Serial.println("Timers::~Timers()");
}

/*  Mise à jour de tous les timers toutes les 10 mS
 */
void Timers::update()
{
  int n = 0;
  for (n = 0; n < NBR_OF_TIMERS; n++) {
    if (timer[n].in_use == true && timer[n].duration > 0) {
      timer[n].duration -= 1L;
    }
  }
}

/*  Test de tous les timers (hors Its)
 *  => Appel de la méthode 'callback()' renseignée
 */
void Timers::test()
{
  char l__buffer[132];

  int n = 0;
  for (n = 0; n < NBR_OF_TIMERS; n++) {
    if (timer[n].in_use == true) {
      if (timer[n].duration == 0L) {
        if (g__st_timers[n].flg_trace) {
          sprintf(l__buffer, "Timers::test(): Expiration #%d/#%d (%s) -> call [0x%x]\n",
            n, (NBR_OF_TIMERS - 1), getText((DEF_TIMERS)n), (int)timer[n].fct_callback);
          Serial.print(l__buffer);
        }

        // Stop this timer and set to 'not use'
        timer[n].duration = -1L;
        timer[n].in_use   = false;

        if (timer[n].fct_callback != NULL) {
          // Execution of 'callback' method
          (*timer[n].fct_callback)();
        }
      }
    }
  }
}

boolean Timers::isInUse(DEF_TIMERS i__def_timer)
{
  return (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS) ? timer[i__def_timer].in_use : false;
}

/*  Launch a timer in the list
 */
boolean Timers::start(DEF_TIMERS i__def_timer, long l__duration, void (*i__fct_callback)(void))
{
  char l__buffer[132];
  boolean l__rtn = false;

  if (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS) {
    if (timer[i__def_timer].in_use == false) {
      timer[i__def_timer].in_use        = true;
      timer[i__def_timer].duration_init = l__duration;
      timer[i__def_timer].duration      = l__duration;
      timer[i__def_timer].fct_callback  = i__fct_callback;

      l__rtn = true;

      if (g__st_timers[i__def_timer].flg_trace) {
        sprintf(l__buffer, "Timers::start(#%d-%s, %ld, 0x%x)\n", i__def_timer, getText(i__def_timer), l__duration, (int)i__fct_callback);
        Serial.print(l__buffer);
      }
    }
  }
  else {
    sprintf(l__buffer, "Warning: Timers::start(#%d-%s): Unknown\n", i__def_timer, getText(i__def_timer));
    Serial.print(l__buffer);    
  }

  return l__rtn;
}

/*  Relaunch a timer in the list
 */
boolean Timers::restart(DEF_TIMERS i__def_timer, long l__duration)
{
  char l__buffer[132];
  boolean l__rtn = false;

  if (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS) {
    if (timer[i__def_timer].in_use == true) {
      timer[i__def_timer].duration_init = l__duration;
      timer[i__def_timer].duration      = l__duration;

      l__rtn = true;

      if (g__st_timers[i__def_timer].flg_trace) {
        sprintf(l__buffer, "Timers::restart(#%d-%s, %ld)\n", i__def_timer, getText(i__def_timer), l__duration);
        Serial.print(l__buffer);
      }
    }
  }
  else {
    sprintf(l__buffer, "Warning: Timers::restart(#%d-%s): Unknown\n", i__def_timer, getText(i__def_timer));
    Serial.print(l__buffer);    
  }

  return l__rtn;
}

/*  Stop a timer in the list
 */
boolean Timers::stop(DEF_TIMERS i__def_timer)
{
  char l__buffer[132];
  boolean l__rtn = false;

  if (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS && timer[i__def_timer].in_use == true) {
    timer[i__def_timer].in_use        = false;
    timer[i__def_timer].duration_init = -1L;
    timer[i__def_timer].duration      = -1L;
    timer[i__def_timer].fct_callback  = NULL;

    l__rtn = true;

    if (g__st_timers[i__def_timer].flg_trace) {
      sprintf(l__buffer, "Timers::stop(#%d-%s)\n", i__def_timer, getText(i__def_timer));
      Serial.print(l__buffer);
    }
  }
#if 0   // TBC: Fix: Warning: Timers::stop(#4-Activity FIFO/Tx Play): Unknown or not in use
  else {
    sprintf(l__buffer, "Warning: Timers::stop(#%d-%s): Unknown or not in use\n", i__def_timer, getText(i__def_timer));
    Serial.print(l__buffer);    
  }
#endif

  return l__rtn;
}

/*  Return the duration in 100 mS resolution of timer in progress
 *
 *  Remarque: If return value < 0L; Duration expired at the moment of reading
 *            but the timer still in use ;-)
 *
 *  Examples: - Return 150  =>  1500 mS => 1.5 Sec
 *            - Return 599  =>  5990 mS => 5.9 Sec 
 *            - Return 3520 => 35200 mS => 35.2 Sec (32 Sec + 200 mS)
 */
long Timers::getDuration(DEF_TIMERS i__def_timer)
{
  char l__buffer[132];
  long l__duration = -1L;

  if (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS && timer[i__def_timer].in_use == true) {
    //l__duration = (timer[i__def_timer].duration_init - timer[i__def_timer].duration);
    l__duration = timer[i__def_timer].duration;
  }
  else {
    sprintf(l__buffer, "Warning: Timers::getDuration(#%d-%s): Unknown or not in use\n", i__def_timer, getText(i__def_timer));
    Serial.print(l__buffer);    
  }

  return l__duration;
}

/* Ajout d'une durée à un timer en cours d'exécution
 * 
 */
void Timers::addDuration(DEF_TIMERS i__def_timer, long i__duration)
{
  char l__buffer[132];

  if (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS && timer[i__def_timer].in_use == true) {
    long l__new_duration = (timer[i__def_timer].duration + i__duration);

    sprintf(l__buffer, "Timers::addDuration(#%d-%s): [%ld + %ld] -> [%ld]\n",
      i__def_timer, getText(i__def_timer),
      timer[i__def_timer].duration, i__duration, l__new_duration);

    Serial.print(l__buffer);    

    timer[i__def_timer].duration = l__new_duration;
  }
  else {
    sprintf(l__buffer, "Warning: Timers::addDuration(#%d-%s): Unknown or not in use\n", i__def_timer, getText(i__def_timer));
    Serial.print(l__buffer);    
  }
}

const char *Timers::getText(DEF_TIMERS i__def_timer)
{
  if (i__def_timer >= 0 && i__def_timer < NBR_OF_TIMERS) {
    if (i__def_timer == g__st_timers[i__def_timer].idx) {
      return g__st_timers[i__def_timer].text;
    }
  }

  return "Unknown";
}

long Timers::addPromptsDuration(long i__duration)
{
  char l__buffer[80];

  sprintf(l__buffer, "Timers::addPromptsDuration(%ld): [%ld] mS -> [%ld] mS\n",
    i__duration, 10L * prompts_duration, 10L * (prompts_duration + i__duration));
  Serial.print(l__buffer);

  prompts_duration += i__duration;

  return prompts_duration;
}

void Timers::clrPromptsDuration()
{
  char l__buffer[80];

  sprintf(l__buffer, "Timers::clrPromptsDuration(): Current: [%ld] mS -> [0] mS\n", 10L * prompts_duration);
  Serial.print(l__buffer);
  sprintf(l__buffer, "                              Total:   [%ld] mS -> [0] mS\n", 10L * prompts_duration_total);
  Serial.print(l__buffer);

  prompts_duration = 0L;
  prompts_duration_total = 0L;
}

long Timers::addPromptsDurationTotal(long i__duration)
{
  char l__buffer[80];

  sprintf(l__buffer, "Timers::addPromptDurationTotal(): [%ld] mS -> [%ld] mS\n", 10L * prompts_duration_total, 10L * (prompts_duration_total + i__duration));
  Serial.print(l__buffer);

  prompts_duration_total += i__duration;

  return prompts_duration_total;
}
