// $Id: Errors.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef USE_SIMULATION
#include <Arduino.h>
#else
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#endif

#include <sstream>

#include "Misc.h"

#ifndef USE_SIMULATION
#include "RotaryEncoder.h"
#include "SerialNMEA.h"
#include "SerialMP3Player.h"
#endif

#include "Timers.h"

#ifndef USE_SIMULATION
#include "Pilot.h"
#endif

#include "Plots.h"
#include "PromptsSynthesis.h"
#include "Statistics.h"

#include "Errors.h"

static ST_ERROR_TEXT    g__st_error_text[] = {
  { ENUM_ERRORS_NONE,               "ENUM_ERRORS_NONE",               "Non définie" },
  { ENUM_ERROR_STARTUP_GPS_MODULE,  "ENUM_ERROR_STARTUP_GPS_MODULE",  "Redémarrage du module GPS" },
  { ENUM_ERROR_UNKNOWN_PROMPT,      "ENUM_ERROR_UNKNOWN_PROMPT",      "Prompt inconnu" },
  { ENUM_ERROR_SHORT_FAMINE,        "ENUM_ERROR_SHORT_FAMINE",        "Courte famine sur FIFO/Tx Play" },
  { ENUM_ERROR_LONG_FAMINE,         "ENUM_ERROR_LONG_FAMINE",         "Longue famine sur FIFO/Tx Play" },
  { ENUM_ERROR_VERY_LONG_FAMINE,    "ENUM_ERROR_VERY_LONG_FAMINE",    "Très longue famine sur FIFO/Tx Play" },
  { ENUM_ERROR_LOSS_GPS,            "ENUM_ERROR_LOSS_GPS",            "Perte géolocalisation" },
  { ENUM_ERROR_NBR_RETRY,           "ENUM_ERROR_NBR_RETRY",           "Attente géolocalisation" },
  { ENUM_ERROR_NBR_REETABLISHMENTS, "ENUM_ERROR_NBR_REETABLISHMENTS", "Rétablissement géolocalisation" }
};

Errors::Errors() : flg_very_long_famine(false), min_duration_very_long_famine(LONG_MAX), max_duration_very_long_famine(LONG_MAX)
{
  Serial.println("Errors::Errors()");

  memset(&st_errors, '\0', sizeof(ST_GESTION_ERROR));
  st_errors.type_visu = GESTION_ERROR_NUMBER;
}

Errors::~Errors()
{
  Serial.println("Errors::~Errors()");
}

void Errors::activity_error_1()
{
  char l__buffer[80];

  ST_GESTION_ERROR *l__pst = &st_errors;

  // Rearmement systématique du timer
  g__timers->start(TIMER_FOR_ERROR_1, DURATION_TIMER_FOR_ERROR_1, &callback_activity_error_1);

  /*  Gestion des 3 cas:
      1 - GESTION_ERROR_NUMBER: Pas d'erreur à présenter => Flash 'DURATION_TIMER_FOR_ERROR_3_NO'
      2 - GESTION_ERROR_NUMBER: Le nombre d'erreurs présentées => Flash 'DURATION_TIMER_FOR_ERROR_3_NO' suivi du nombre de flash
                                'DURATION_TIMER_FOR_ERROR_3_NBR' correspondant aux nombre d'erreurs
      3 - GESTION_ERROR_VALUE: Erreur à présenter => 8 flash 0/1 'DURATION_TIMER_FOR_ERROR_3_0'/'DURATION_TIMER_FOR_ERROR_3_1'
        + GESTION_ERROR_PAUSE  en tournant sur toutes les erreurs à présenter avec une pause 'DURATION_TIMER_FOR_ERROR_2_PAUSE'
                               entre le 4th et 5th bit
  */
  switch (l__pst->type_visu) {
    case GESTION_ERROR_NUMBER:
      sprintf(l__buffer, "activity_error_1(): %d error(s)\n", l__pst->number);
      Serial.print(l__buffer);

      if (l__pst->number != 0) {
        byte n = 0;
        for (n = 0; n < l__pst->number; n++) {
          sprintf(l__buffer, "\t#%d: Error [%d - 0x%03x]\n", n, l__pst->value[n], l__pst->value[n]);
          Serial.print(l__buffer);
        }
      }

      g__timers->start(TIMER_FOR_ERROR_3, DURATION_TIMER_FOR_ERROR_3_NO, &callback_activity_error_3);

#ifndef USE_SIMULATION
      // g__state_leds |= STATE_LED_RED;
#endif

      if (l__pst->number != 0) {
        // Présentation du nombre d'erreurs dans 'callback_activity_error_2'
        l__pst->wrk_number = l__pst->number;     // Comptabilisation du nombre d'erreurs à présenter

        g__timers->start(TIMER_FOR_ERROR_2, DURATION_TIMER_FOR_ERROR_3_NBR, &callback_activity_error_2);
      }
      break;

    case GESTION_ERROR_VALUE:
      if (l__pst->last_index >= l__pst->number) {
        sprintf(l__buffer, "activity_error_1(): End of presentation\n");
        Serial.print(l__buffer);

        l__pst->last_index = 0;    // Retour à la 1st erreur

        /*  Fin de la présenttaion de toutes les erreurs,
            retour à la présentation du nombre d'erreurs
        */
        l__pst->type_visu = GESTION_ERROR_NUMBER;

        // Execution of 'callback_activity_error_1' pour la présentation du nombre d'erreurs
        callback_activity_error_1();
      }
      else {
        sprintf(l__buffer, "activity_error_1(): #%d: Error [%d - 0x%03x]\n",
                l__pst->last_index, l__pst->value[l__pst->last_index], l__pst->value[l__pst->last_index]);
        Serial.print(l__buffer);

        // Execution of 'callback_activity_error_2' pour la présentation de l'erreur
        callback_activity_error_2();
      }
      break;

    default:
      break;
  }
}

/*  Flashs indiquant:
    - l'activité tous les 'DURATION_TIMER_FOR_ERROR_1'
    - le nombre d'erreurs si != 0
    - l'état 0/1 des 12 bits (MSB en tête) d'une erreur
*/
void Errors::activity_error_2()
{
  ST_GESTION_ERROR *l__pst = &st_errors;

  switch (l__pst->type_visu) {
    case GESTION_ERROR_NUMBER:
      l__pst->wrk_number--;

      g__timers->start(TIMER_FOR_ERROR_3, DURATION_TIMER_FOR_ERROR_3_NBR, &callback_activity_error_3);

#ifndef USE_SIMULATION
      // g__state_leds |= STATE_LED_RED;
#endif

      if (l__pst->wrk_number == 0) {
        /*  Fin de la présentation du nombre d'erreurs => Changement de visu
            => 'wrk_number' devient le rang du bit à présenter
        */
        l__pst->type_visu = GESTION_ERROR_VALUE;
      }
      else {
        // Réarmement timer pour la prochaine erreur
        g__timers->start(TIMER_FOR_ERROR_2, DURATION_TIMER_FOR_ERROR_2, &callback_activity_error_2);
      }
      break;

    case GESTION_ERROR_VALUE:
    case GESTION_ERROR_PAUSE:
      if (l__pst->wrk_number < (3 * 4)) {   // Rang du bit à présenter
        boolean l__flg_bit = (*(l__pst->value + l__pst->last_index) & (1 << (11 - l__pst->wrk_number)));

        if ((l__pst->wrk_number == 4 || l__pst->wrk_number == 8) && l__pst->type_visu != GESTION_ERROR_PAUSE) {
          // Pause 'DURATION_TIMER_FOR_ERROR_2_PAUSE' entre le 4th et 5th bit et 8th et 9th bit
          g__timers->start(TIMER_FOR_ERROR_2, DURATION_TIMER_FOR_ERROR_2_PAUSE, &callback_activity_error_2);
          l__pst->type_visu = GESTION_ERROR_PAUSE;
        }
        else {
          l__pst->type_visu = GESTION_ERROR_VALUE;      // Effacement éventuel de 'GESTION_ERROR_PAUSE'

          g__timers->start(TIMER_FOR_ERROR_3, (l__flg_bit == true ? DURATION_TIMER_FOR_ERROR_3_1 : DURATION_TIMER_FOR_ERROR_3_0), &callback_activity_error_3);
          g__timers->start(TIMER_FOR_ERROR_2, DURATION_TIMER_FOR_ERROR_2, &callback_activity_error_2);

          l__pst->wrk_number++;    // Next bit

#ifndef USE_SIMULATION
          // g__state_leds |= STATE_LED_RED;
#endif
        }
      }
      else {
        /*  Fin de la présenttaion des 8 bits,
            préparation prochaine erreur
        */
        l__pst->last_index++;     // Next error
        l__pst->wrk_number = 0;
      }
      break;

    default:
      break;
  }
}

// Extinction de la Led RED
void Errors::activity_error_3()
{
#ifndef USE_SIMULATION
  // g__state_leds &= ~STATE_LED_RED;
#endif
}


// Clear the all aerrors of the list
void Errors::clear()
{
  st_errors.type_visu = GESTION_ERROR_NUMBER;
  st_errors.number = 0;
  st_errors.last_index = 0;
  memset(st_errors.value, '\0', sizeof(st_errors.value));
}

/*  Add a new error at the end of the list even if exists
 *  => Not used ;-)
 */
void Errors::add(uint16_t i__value)
{
  if (st_errors.number < NBR_MAX_ERRORS) {
    st_errors.value[st_errors.number++] = i__value;
  }
  else {
    char l__buffer[80];

    sprintf(l__buffer, "Errors::add(0x%03x): Too many errors\n", i__value);
    Serial.print(l__buffer);
  }
}

/*  Increment the error if exists @ MSB 'nibble' (NNNN XXXX YYYY) -> (NNNN (XXXX YYY)+1)
 *  => If (XXXX) == 0x0f before test => Ignore (256 increments max [0xX00, ..., 0xXff
 */
void Errors::update(ENUM_ERRORS i__family)
{
  char l__buffer[80];

  if (st_errors.number < NBR_MAX_ERRORS) {
    uint16_t l__init_mask = (16 * (uint16_t)i__family) & 0xFF0;
    uint16_t l__max       = (16 * (uint16_t)i__family) | 0x00F;

    boolean l__flg_exists = false;
    byte n = 0;
    for (n = 0; n < NBR_MAX_ERRORS; n++) {
      if ((st_errors.value[n] / 16) == (uint16_t)i__family) {
        l__flg_exists = true;
        break;
      }
    }

    if (l__flg_exists == false) {
      // Not exists => Add the new 'l__init_mask' value
      st_errors.value[st_errors.number] = l__init_mask;
      sprintf(l__buffer, "Errors::update(0x%02x): New value (0x%03x)\n", i__family, l__init_mask);

      st_errors.counter[st_errors.number]++;    // Total errors

      st_errors.number++;
    }
    else if (st_errors.value[n] < l__max) {
      // Exists and max. not reached
      uint16_t l__new_value = (st_errors.value[n] + 1);
      sprintf(l__buffer, "Errors::update(0x%02x): Update from [0x%03x] to [0x%03x]\n", i__family, st_errors.value[n], l__new_value);
      st_errors.value[n] = l__new_value;

      st_errors.counter[n]++;                   // Total errors
    }
    else {
      // Exists and max. reached => Ignore
      sprintf(l__buffer, "Errors::update(0x%02x): Max reached (0x%03x)\n", i__family, l__max);

      st_errors.counter[n]++;                   // Total errors
    }
  }
  else {
    sprintf(l__buffer, "Errors::update(0x%02x): Too many errors\n", i__family);
  }

  Serial.print(l__buffer);
}

const char *Errors::getNameEnum(ENUM_ERRORS i__family) const
{
  size_t n = 0;
  for (n = 0; n < sizeof(g__st_error_text) / sizeof(g__st_error_text[0]); n++) {
    if (g__st_error_text[n].family == i__family) {
      return g__st_error_text[n].name_enum;
    }
  }
  return "Unknown";
}

const char *Errors::getText(ENUM_ERRORS i__family) const
{
  size_t n = 0;
  for (n = 0; n < sizeof(g__st_error_text) / sizeof(g__st_error_text[0]); n++) {
    if (g__st_error_text[n].family == i__family) {
      return g__st_error_text[n].text;
    }
  }
  return "Unknown";
}

void Errors::printAll(std::ostringstream &io__out)
{
  char l__buffer[80];

  if (st_errors.number != 0) {
    sprintf(l__buffer, "\t%d errors:\n", st_errors.number);
    io__out << l__buffer;

    byte n = 0;
    for (n = 0; n < st_errors.number; n++) {
      // Value on 3 'nibble' in the range [0x000, 0x001, ..., 0xffe, 0xfff]
      sprintf(l__buffer, "\t\t#%d: [%d - 0x%03x] (%d x %s)",
        n, st_errors.value[n], st_errors.value[n], (st_errors.value[n] & 0x00f) + 1,
        getNameEnum((ENUM_ERRORS)(st_errors.value[n] >> 4)));

      io__out << l__buffer;

      // Warning: Build en 2 passes (char l__buffer[80];)
      sprintf(l__buffer, " [%s] %s", getText((ENUM_ERRORS)(st_errors.value[n] >> 4)),
      ((st_errors.value[n] & 0x00F) == 0x00F) ? "(max)" : "");

      io__out << l__buffer;

      // Warning: Build en 3 passes (char l__buffer[80];)
      sprintf(l__buffer, " (Total [%u])\n", st_errors.counter[n]);

      io__out << l__buffer;
    }
  }
  else {
    sprintf(l__buffer, "\tNo errors\n");
    io__out << l__buffer;
  }
}

void Errors::updateMinMaxDurationsOfVeryLongFamine(long i__duration)
{
  if (i__duration < min_duration_very_long_famine || min_duration_very_long_famine == LONG_MAX) {
    min_duration_very_long_famine = i__duration;
  }

  if (i__duration > max_duration_very_long_famine || max_duration_very_long_famine == LONG_MAX) {
    max_duration_very_long_famine = i__duration;
  }
}
