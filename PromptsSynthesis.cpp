// $Id: PromptsSynthesis.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef USE_SIMULATION
#include <Arduino.h>
#else
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#endif

#include "Misc.h"
#include "PromptsSynthesis.h"
#include "Timers.h"

/* Note pour le passage du SerialMP3Player avec un circuit 'WT2003S'
 * - Aucun changement des definitions de la base sonore
 * - La translation est faite apres la lecture des 3 bytes de la FIFO Tx/Play
 *   avant l'emission de la commande suivant les regles suivantes:
 *   o ...
 */

// Variables locales et communes à toutes les synthèses
static byte                     g__commands[80];        // Tableau des bytes de toutes les commandes
static uint16_t                 g__durations[80];       // Tableau des durées des prompts à synthétiser (multiple de 10 mS)

static size_t                   g__prompts_nbr = 0;     // Nombre de prompts synthétisés

#ifdef USE_SIMULATION
static std::string              g__prompts_str;       // Textes de tous les prompts synthétisés
#else
static String                   g__prompts_str;       // Textes de tous les prompts synthétisés
#endif

// List of definitions for general synthesis
const ST_PROMPTS_DEF            g__prompts_general[BASE_LAST_PROMPT_GENERAL + 1] =
{
  { BASE_HELLO_WORLD,        { 0x12, NUM_PROMPT_HELLO_WORLD / 256, NUM_PROMPT_HELLO_WORLD % 256 },         "mp3/0000_Hello_World.mp3", false, 1, 1774L },
  { BASE_JE_SUIS__FEMALE,    { 0x12, NUM_PROMPT_JE_SUIS__FEMALE / 256, NUM_PROMPT_JE_SUIS__FEMALE % 256 }, "mp3/0001_je_suis_une_boussole_intelligente_femme.mp3", false, 3, 5889L },
  { BASE_JE_SUIS__MALE,      { 0x12, NUM_PROMPT_JE_SUIS__MALE / 256, NUM_PROMPT_JE_SUIS__MALE % 256 },     "mp3/0002_je_suis_une_boussole_intelligente_homme.mp3", false, 3, 6006L },

  { BASE_RETRY_PROMPTS_DIFFUSION,  { 0x12, NUM_RETRY_PROMPTS_DIFFUSION / 256, NUM_RETRY_PROMPTS_DIFFUSION % 256 },   "mp3/0003_rediffusion_du_message.mp3", false, 3, 2284L },

  { BASE_PLEASE_WAIT_GPS,          { 0x12, NUM_PLEASE_WAIT_GPS / 256, NUM_PLEASE_WAIT_GPS % 256 },                   "mp3/0007_veuillez_patienter_en_musique_(etablissement).mp3", false, 4, 6516L },
  { BASE_PLEASE_WAIT_RECOVERY_GPS, { 0x12, NUM_PLEASE_WAIT_RECOVERY_GPS / 256, NUM_PLEASE_WAIT_RECOVERY_GPS % 256 }, "mp3/0008_veuillez_patienter_en_musique_(retablissement).mp3", false, 4, 6751L },

  { BASE_LOSS_GPS,           { 0x12, NUM_LOSS_GPS / 256, NUM_LOSS_GPS % 256 }, "mp3/0004_perte_du_signal_GPS.mp3", false, 1, 2793L },

  { BASE_OUPS_MALE,          { 0x12, NUM_OUPS_MALE / 256, NUM_OUPS_MALE % 256 },     "mp3/0005_Oups_homme.mp3", true, 0, 677L },
  { BASE_OUPS_FEMALE,        { 0x12, NUM_OUPS_FEMALE / 256, NUM_OUPS_FEMALE % 256 }, "mp3/0006_Oups_femme.mp3", true, 0, 834L },

  { BASE_GPS,                { 0x12, NUM_GPS / 256, NUM_GPS % 256 },                   "mp3/0010_etablissement_du_signal_GPS.mp3", false, 2, 3224L },
  { BASE_RECOVERY_GPS,       { 0x12, NUM_RECOVERY_GPS / 256, NUM_RECOVERY_GPS % 256 }, "mp3/0011_retablissement_du_signal_GPS.mp3", false, 2, 3381L },

  { BASE_JE_SUIS__FEMALE_BIS, { 0x12, NUM_PROMPT_JE_SUIS__FEMALE_BIS / 256, NUM_PROMPT_JE_SUIS__FEMALE_BIS % 256 },   "mp3/0012_je_suis_boussole_prete.mp3", false, 3, 4635L },

  { BASE_SORRY_WAIT_AGAIN,   { 0x12, NUM_SORRY_WAIT_AGAIN / 256, NUM_SORRY_WAIT_AGAIN % 256 },   "mp3/0013_desole_geolocalisation_impossible_(homme).mp3", false, 4, 6594L },

  { BASE_INTERSTELLAR_THEME, { 0x12, NUM_INTERSTELLAR_THEME / 256, NUM_INTERSTELLAR_THEME % 256 }, "mp3/0100_InterstellarMainThemeExtraExtended.mp3", false, 12*60+55, 781609L },
  { BASE_ADVERT_WAIT,        { 0x13, NUM_ADVERT_WAIT / 256, NUM_ADVERT_WAIT % 256 },               "advert/0200_veuillez_toujours_patienter.mp3", false, 1, 2362L },

  { BASE_TEST_MESSAGE,       { 0x12, NUM_TEST_MESSAGE / 256, NUM_TEST_MESSAGE % 256 },             "mp3/0999_message_de_test_homme.mp3", false, 1, 1853L },

  { BASE_SUDDEN_EVENT_DIFFERENT,       { 0x14, NUM_SUDDEN_EVENT_DIFFERENT / 256, NUM_SUDDEN_EVENT_DIFFERENT % 256 },           "15/1000_salamisound-1611805-sfx-sudden-event-different.mp3", true, 29, 29059L },
  { BASE_FLAT_STONE_THROW_INTO_WATER,  { 0x14, NUM_FLAT_STONE_THROW_INTO_WATER / 256, NUM_FLAT_STONE_THROW_INTO_WATER % 256 }, "15/1002_salamisound-3569093-flat-stone-thrown-into-water.mp3", false, 4, 4400L },
  { BASE_DING_DONG_BELL_DOORBELL_2,    { 0x14, NUM_DING_DONG_BELL_DOORBELL_2 / 256, NUM_DING_DONG_BELL_DOORBELL_2 % 256 },     "15/1006_salamisound-2028068-ding-dong-bell-doorbell.mp3", true, 11, 11818L },
  { BASE_SHOCK_IMPACT_METALLIC,        { 0x14, NUM_SHOCK_IMPACT_METALLIC / 256, NUM_SHOCK_IMPACT_METALLIC % 256 },             "15/1008_salamisound-3924547-shock-impact-metallic.mp3", true, 2, 2728L },
  { BASE_SHUTTER_SLR_CAMERA_DIGITAL,   { 0x14, NUM_SHUTTER_SLR_CAMERA_DIGITAL / 256, NUM_SHUTTER_SLR_CAMERA_DIGITAL % 256 },   "15/1009_salamisound-1020125-shutter-slr-camera-digital.mp3", true, 0, 3459L },
  { BASE_SWITCH_TOGGLE_OR_ROTARY,      { 0x14, NUM_SWITCH_TOGGLE_OR_ROTARY / 256, NUM_SWITCH_TOGGLE_OR_ROTARY % 256 },         "15/1010_salamisound-1453300-switch-toggle-or-rotary.mp3", true, 2, 1996L },
  { BASE_DING_DONG_BELL_DOORBELL_3,    { 0x14, NUM_DING_DONG_BELL_DOORBELL_3 / 256, NUM_DING_DONG_BELL_DOORBELL_3 % 256 },     "15/1011_salamisound-2655584-ding-dong-bell-will-ring-3.mp3", true, 13, 13281L },
  { BASE_UNLOCK_OLD_CASTEL_ON_AN,      { 0x14, NUM_UNLOCK_OLD_CASTEL_ON_AN / 256, NUM_UNLOCK_OLD_CASTEL_ON_AN % 256 },         "15/1012_salamisound-5789668-that-unlock-old-castle-on-an.mp3", true, 4, 3877L },
  { BASE_DING_DONG_BELL_DOORBELL_1,    { 0x14, NUM_DING_DONG_BELL_DOORBELL_1 / 256, NUM_DING_DONG_BELL_DOORBELL_1 % 256 },     "15/1013_salamisound-8381391-ding-dong-bell-doorbell.mp3", true, 10, 9728L },

  { BASE_BIG_RATCHET_HALF_TURN,        { 0x14, NUM_BIG_RATCHET_HALF_TURN / 256, NUM_BIG_RATCHET_HALF_TURN % 256 },             "15/1015_salamisound-8930906-big-ratchet-half-turn.mp3", true, 9, 9206L },

  { BASE_LAST_PROMPT_GENERAL, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

// List of general methods
/*  Extraction of text from name file
 *
 *  Example: "03/0520_20_heures.mp3" -> "20_heures"
 *                    ^^^^^^^^^
 *                    text between "xxx/yyy_" and "."
 */
static void updateString(const char *i__prompt_text)
{
#if 0   // TODO: Coredump ('strncpy()' inopérant ?!..)
  char l__work[80];
  memset(l__work, '\0', sizeof(l__work));

  strncpy(l__work, i__prompt_text, min(sizeof(l__work) - 2, strlen(i__prompt_text)));

  char *l__pattern = strchr(l__work, '_');
  if (l__pattern != NULL) {
    char *l__pattern2 = strchr(l__work, '.');
    if (l__pattern2 != NULL) {
      *l__pattern2 = '\0';              // Skip the pattern and the next characters
      g__prompts_str += (l__pattern + 1);

      return;
    }
  }
#else
  //strncpy(l__work, i__prompt_text, sizeof(l__work) - 2);
  g__prompts_str += i__prompt_text;
  g__prompts_nbr += 1;
#endif

#if 0
  // Internal error or pattern(s) (first '_' or '.') not found
  g__prompts_str += "???";
#endif
}

static boolean updateCommandsWithOups(size_t *io__size)
{
#ifdef USE_SIMULATION
  g__prompts_str += "\n";
#else
  if (g__prompts_nbr != 0) {
    g__prompts_str += " ";
  }
#endif

  memcpy(&g__commands[*io__size], g__prompts_general[BASE_OUPS_MALE].commands, SIZE_1_COMMAND);
  updateString(g__prompts_general[BASE_OUPS_MALE].text);

  *io__size += SIZE_1_COMMAND;

  return true;
}

void callback_all_diffusions()
{
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_ALL_DIFFUSIONS expired\n");
}

static boolean updateCommands(const ST_PROMPTS_DEF *i__prompts, size_t i__base, size_t *io__size, size_t i__base_last_prompt, size_t *o__nbr_durations = NULL)
{
#if USE_TIMER_ALL_DIFFUSIONS
  char l__buffer[80];
#endif

  boolean l__flg_rtn = false;

#ifdef USE_SIMULATION
  g__prompts_str += "\n";
#else
  if (g__prompts_nbr != 0) {
    g__prompts_str += " ";
  }
#endif

  if (i__base < i__base_last_prompt) {
    memcpy(&g__commands[*io__size], (i__prompts + i__base)->commands, SIZE_1_COMMAND);
    updateString((i__prompts + i__base)->text);

    // Update the duration of the prompt + nbr of durations in the list
    if (o__nbr_durations != NULL) {
      if ((i__prompts + i__base)->flg_duration == true) {

      // 'g__durations' contents the duration coded in multiple of 10 mS
#if USE_REAL_PROMPTS_DURATION_FROM_TIME
        // Get the prompt duration from the field 'duration_from_time' (HH:MM:SS) in Sec.
        g__durations[*o__nbr_durations] = 100 * (i__prompts + i__base)->duration_from_time;
#else
        // Get the prompt duration from the real duration the field 'duration_from_length' in mS
        g__durations[*o__nbr_durations] = (uint16_t)((i__prompts + i__base)->duration_from_length / 10L);

#if USE_TIMER_ALL_DIFFUSIONS
        // Armement/Réarmement 'TIMER_ALL_DIFFUSIONS'
        if ((i__prompts + i__base)->flg_duration == true) {
          long l__duration_from_length = (i__prompts + i__base)->duration_from_length;

          if (g__timers->isInUse(TIMER_ALL_DIFFUSIONS)) {
            sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_ALL_DIFFUSIONS add [%ld] mS\n", l__duration_from_length);
            Serial.print(l__buffer);

            g__timers->addDuration(TIMER_ALL_DIFFUSIONS, (l__duration_from_length / 10L));
          }
          else {
            sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_ALL_DIFFUSIONS started [%ld] mS\n", l__duration_from_length);
            Serial.print(l__buffer);

            g__timers->start(TIMER_ALL_DIFFUSIONS, (l__duration_from_length / 10L), &callback_all_diffusions);
          }
        }
        // Fin: Armement/Réarmement 'TIMER_ALL_DIFFUSIONS'
#endif
#endif
        *o__nbr_durations += 1;
      }
    }
    // End: Update the duration of the prompt + nbr of durations in the list

    l__flg_rtn = true;
  }
  else {
    memcpy(&g__commands[*io__size], g__prompts_general[BASE_OUPS_MALE].commands, SIZE_1_COMMAND);
    updateString(g__prompts_general[BASE_OUPS_MALE].text);

    l__flg_rtn = false;
  }

  *io__size += SIZE_1_COMMAND;

  return l__flg_rtn;
}

void traceOfBuildResult(size_t i__size)
{
    char l__buffer[80];

#if 0
  std::ostringstream l__out;

  sprintf(l__buffer, "Synthesis command (%d bytes)\n", i__size);
  Serial.print(l__buffer);

  hexDump(l__out, (char *)g__commands, i__size);
  Serial.print(l__out.str().c_str());
  Serial.print("\n");
#endif

  // Print the all prompts synthetized
  if (g__prompts_nbr == 0) {
    Serial.print("No prompt");
  }
  else {
    sprintf(l__buffer, "%d prompt%s ", g__prompts_nbr, (g__prompts_nbr != 1) ? "s" : "");

#ifdef USE_SIMULATION
    Serial.printTimestamp(true);
#endif
    Serial.print(l__buffer);
    Serial.print(g__prompts_str.c_str());
  }

  Serial.print(" synthetized\n");

#ifdef USE_SIMULATION
    Serial.printTimestamp(false);
#endif
  // End: Print the all prompts synthetized

  // Effacement pour la prochaine trace
  g__prompts_nbr = 0;
  g__prompts_str = "";
}
// End: List of general methods

// List of methods for general synthesis
size_t buildCommandsPromptsGeneral(int i__base_prompt, byte **o__command)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  if (i__base_prompt >= 0 && i__base_prompt < BASE_LAST_PROMPT_GENERAL) {
    updateCommands(g__prompts_general, i__base_prompt, &l__size, BASE_LAST_PROMPT_GENERAL);
  }
  else {
    updateCommands(g__prompts_general, BASE_OUPS_MALE, &l__size, BASE_LAST_PROMPT_GENERAL);
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  return l__size;
}
// End: List of definitions and methods for general synthesis

// List of definitions and methods for date/time synthesis
#define BASE_2_DAY                      0
#define BASE_FIRST                     30
#define BASE_LAST                      35
#define BASE_MONDAY                    36
#define BASE_JANUARY                   43
#define BASE_AND_LAST                  55
#define BASE_OF_MONTH                  56
#define BASE_TIME_SOMMER               57
#define BASE_TIME_WINTER               58
#define BASE_0_HOUR                    59
#define BASE_5_MINUTES                 83
#define BASE_2000                      94
#define BASE_2020                      95
#define BASE_SOON                     105
#define BASE_PAST                     106
#define BASE_MIDNIGHT                 107

#define BASE_NOUS_SOMMES_LE           108
#define BASE_IL_EST                   109
#define BASE_IL_EST_BIENTOT           110

#define BASE_LAST_PROMPT_DATE_TIME    111

static const ST_PROMPTS_DEF           g__prompts_date_time[BASE_LAST_PROMPT_DATE_TIME + 1] =
{
  { BASE_2_DAY, { 0x14, 0x30, 0x02 }, "03/0002_2.mp3", true, 0, 520L },
  {  1, { 0x14, 0x30, 0x03 }, "03/0003_3.mp3", true, 0, 677L },
  {  2, { 0x14, 0x30, 0x04 }, "03/0004_4.mp3", true, 0, 795L },
  {  3, { 0x14, 0x30, 0x05 }, "03/0005_5.mp3", true, 0, 599L },
  {  4, { 0x14, 0x30, 0x06,}, "03/0006_6.mp3", true, 0, 952L },
  {  5, { 0x14, 0x30, 0x07 }, "03/0007_7.mp3", true, 0, 912L },
  {  6, { 0x14, 0x30, 0x08 }, "03/0008_8.mp3", true, 0, 873L },
  {  7, { 0x14, 0x30, 0x09 }, "03/0009_9.mp3", true, 0, 834L },
  {  8, { 0x14, 0x30, 0x0a }, "03/0010_10.mp3", true, 0, 952L },
  {  9, { 0x14, 0x30, 0x0b }, "03/0011_11.mp3", true, 0, 756L },
  { 10, { 0x14, 0x30, 0x0c }, "03/0012_12.mp3", true, 0, 756L },
  { 11, { 0x14, 0x30, 0x0d }, "03/0013_13.mp3", true, 0, 834L },
  { 12, { 0x14, 0x30, 0x0e }, "03/0014_14.mp3", true, 0, 991L },
  { 13, { 0x14, 0x30, 0x0f }, "03/0015_15.mp3", true, 0, 795L },
  { 14, { 0x14, 0x30, 0x10 }, "03/0016_16.mp3", true, 0, 834L },
  { 15, { 0x14, 0x30, 0x11 }, "03/0017_17.mp3", true, 0, 1187L },
  { 16, { 0x14, 0x30, 0x12 }, "03/0018_18.mp3", true, 0, 1187L },
  { 17, { 0x14, 0x30, 0x13 }, "03/0019_19.mp3", true, 0, 1304L },
  { 18, { 0x14, 0x30, 0x14 }, "03/0020_20.mp3", true, 0, 599L },
  { 19, { 0x14, 0x30, 0x15 }, "03/0021_21.mp3", true, 0, 1187L },
  { 20, { 0x14, 0x30, 0x16 }, "03/0022_22.mp3", true, 0, 1030L },
  { 21, { 0x14, 0x30, 0x17 }, "03/0023_23.mp3", true, 0, 1226L },
  { 22, { 0x14, 0x30, 0x18 }, "03/0024_24.mp3", true, 0, 1265L },
  { 23, { 0x14, 0x30, 0x19 }, "03/0025_25.mp3", true, 0, 1226L },
  { 24, { 0x14, 0x30, 0x1a }, "03/0026_26.mp3", true, 0, 1344L },
  { 25, { 0x14, 0x30, 0x1b,}, "03/0027_27.mp3", true, 0, 1461L },
  { 26, { 0x14, 0x30, 0x1c }, "03/0028_28.mp3", true, 0, 1344L },
  { 27, { 0x14, 0x30, 0x1d }, "03/0029_29.mp3", true, 0, 1304L },
  { 28, { 0x14, 0x30, 0x1e }, "03/0030_30.mp3", true, 0, 795L },
  { 29, { 0x14, 0x30, 0x1f }, "03/0031_31.mp3", true, 0, 1069L },

  { BASE_FIRST, { 0x14, 0x30, 0x65 }, "03/0101_premier.mp3", true, 0, 873L },
  { 31, { 0x14, 0x30, 0x66 }, "03/0102_deuxieme.mp3", true, 0, 991L },
  { 32, { 0x14, 0x30, 0x67 }, "03/0103_troisieme.mp3", true, 0, 1148L },
  { 33, { 0x14, 0x30, 0x68 }, "03/0104_quatrieme.mp3", true, 0, 1226L },
  { 34, { 0x14, 0x30, 0x69 }, "03/0105_cinquieme.mp3", true, 0, 1187L },

  { BASE_LAST, { 0x14, 0x30, 0xc7 }, "03/0199_dernier.mp3", true, 0, 873L },

  { BASE_MONDAY, { 0x14, 0x30, 0xc9 }, "03/0201_lundi.mp3", true, 0, 873L },
  { 37, { 0x14, 0x30, 0xca }, "03/0202_mardi.mp3", true, 0, 952L },
  { 38, { 0x14, 0x30, 0xcb }, "03/0203_mercredi.mp3", true, 0, 1069L },
  { 39, { 0x14, 0x30, 0xcc }, "03/0204_jeudi.mp3", true, 0, 834L },
  { 40, { 0x14, 0x30, 0xcd }, "03/0205_vendredi.mp3", true, 0, 1030L },
  { 41, { 0x14, 0x30, 0xce }, "03/0206_samedi.mp3", true, 0, 1069L },
  { 42, { 0x14, 0x30, 0xcf }, "03/0207_dimanche.mp3", true, 0, 1108L },

  { BASE_JANUARY, { 0x14, 0x31, 0x2d }, "03/0301_janvier.mp3", true, 0, 991L },
  { 44, { 0x14, 0x31, 0x2e }, "03/0302_fevrier.mp3", true, 0, 991L },
  { 45, { 0x14, 0x31, 0x2f }, "03/0303_mars.mp3", true, 0, 952L },
  { 46, { 0x14, 0x31, 0x30 }, "03/0304_avril.mp3", true, 0, 1030L },
  { 47, { 0x14, 0x31, 0x31 }, "03/0305_mai.mp3", true, 0, 520L },
  { 48, { 0x14, 0x31, 0x32 }, "03/0306_juin.mp3", true, 0, 638L },
  { 49, { 0x14, 0x31, 0x33 }, "03/0307_juillet.mp3", true, 0, 912L },
  { 50, { 0x14, 0x31, 0x34 }, "03/0308_aout.mp3", true, 0, 520L },
  { 51, { 0x14, 0x31, 0x35 }, "03/0309_septembre.mp3", true, 0, 1069L },
  { 52, { 0x14, 0x31, 0x36 }, "03/0310_octobre.mp3", true, 0, 1148L },
  { 53, { 0x14, 0x31, 0x37 }, "03/0311_novembre.mp3", true, 0, 1148L },
  { 54, { 0x14, 0x31, 0x38 }, "03/0312_decembre.mp3", true, 0, 1187L },

  { BASE_AND_LAST, { 0x14, 0x31, 0x91 }, "03/0401_et_dernier.mp3", true, 0, 1030L },

  { BASE_OF_MONTH, { 0x14, 0x31, 0x92 }, "03/0402_du_mois.mp3", true, 0, 834L },

  { BASE_TIME_SOMMER, { 0x14, 0x31, 0x93 }, "03/0403_heure_d_ete.mp3", true, 0, 1148L },
  { BASE_TIME_WINTER, { 0x14, 0x31, 0x94 }, "03/0404_heure_d_hiver.mp3", true, 0, 1108L },

  { BASE_0_HOUR, { 0x14, 0x31, 0xf4 }, "03/0500_0_heure.mp3", true, 0, 1265L },
  { 60, { 0x14, 0x31, 0xf5 }, "03/0501_1_heure.mp3", true, 0, 952L },
  { 61, { 0x14, 0x31, 0xf6 }, "03/0502_2_heures.mp3", true, 0, 1069L },
  { 62, { 0x14, 0x31, 0xf7 }, "03/0503_3_heures.mp3", true, 0, 1187L },
  { 63, { 0x14, 0x31, 0xf8 }, "03/0504_4_heures.mp3", true, 0, 1069L },
  { 64, { 0x14, 0x31, 0xf9 }, "03/0505_5_heures.mp3", true, 0, 1226L },
  { 65, { 0x14, 0x31, 0xfa }, "03/0506_6_heures.mp3", true, 0, 1187L },
  { 66, { 0x14, 0x31, 0xfb }, "03/0507_7_heures.mp3", true, 0, 1187L },
  { 67, { 0x14, 0x31, 0xfc }, "03/0508_8_heures.mp3", true, 0, 1069L },
  { 68, { 0x14, 0x31, 0xfd }, "03/0509_9_heures.mp3", true, 0, 1108L },
  { 69, { 0x14, 0x31, 0xfe }, "03/0510_10_heures.mp3", true, 0, 1108L },
  { 70, { 0x14, 0x31, 0xff }, "03/0511_11_heures.mp3", true, 0, 991L },
  { 71, { 0x14, 0x32, 0x00 }, "03/0512_12_heures.mp3", true, 0, 1030L },
  { 72, { 0x14, 0x32, 0x01 }, "03/0513_13_heures.mp3", true, 0, 1382L },
  { 73, { 0x14, 0x32, 0x02 }, "03/0514_14_heures.mp3", true, 0, 1226L },
  { 74, { 0x14, 0x32, 0x03 }, "03/0515_15_heures.mp3", true, 0, 952L },
  { 75, { 0x14, 0x32, 0x04 }, "03/0516_16_heures.mp3", true, 0, 1069L },
  { 76, { 0x14, 0x32, 0x05 }, "03/0517_17_heures.mp3", true, 0, 1382L },
  { 77, { 0x14, 0x32, 0x06 }, "03/0518_18_heures.mp3", true, 0, 1461L },
  { 78, { 0x14, 0x32, 0x07 }, "03/0519_19_heures.mp3", true, 0, 1226L },
  { 79, { 0x14, 0x32, 0x08 }, "03/0520_20_heures.mp3", true, 0, 1226L },
  { 80, { 0x14, 0x32, 0x09 }, "03/0521_21_heures.mp3", true, 0, 1382L },
  { 81, { 0x14, 0x32, 0x0a }, "03/0522_22_heures.mp3", true, 0, 1500L },
  { 82, { 0x14, 0x32, 0x0b }, "03/0523_23_heures.mp3", true, 1, 1814L },

  { BASE_5_MINUTES, { 0x14, 0x32, 0x5d }, "03/0605_05m.mp3", true, 0, 1344L },
  { 84, { 0x14, 0x32, 0x62 }, "03/0610_10m.mp3", true, 0, 952L },
  { 85, { 0x14, 0x32, 0x67 }, "03/0615_15m.mp3", true, 0, 795L },
  { 86, { 0x14, 0x32, 0x6c }, "03/0620_20m.mp3", true, 0, 599L },
  { 87, { 0x14, 0x32, 0x71 }, "03/0625_25m.mp3", true, 0, 1226L },
  { 88, { 0x14, 0x32, 0x76 }, "03/0630_30m.mp3", true, 0, 795L },
  { 89, { 0x14, 0x32, 0x7b }, "03/0635_35m.mp3", true, 0, 1187L },
  { 90, { 0x14, 0x32, 0x80 }, "03/0640_40m.mp3", true, 0, 991L },
  { 91, { 0x14, 0x32, 0x85 }, "03/0645_45m.mp3", true, 0, 1382L },
  { 92, { 0x14, 0x32, 0x8a }, "03/0650_50m.mp3", true, 0, 1226 },
  { 93, { 0x14, 0x32, 0x8f }, "03/0655_55m.mp3", true, 1, 1539L },

  { BASE_2000, { 0x14, 0x37, 0xd0 }, "03/2000_2000.mp3", true, 0, 1069L },

  { BASE_2020, { 0x14, 0x37, 0xe4 }, "03/2020_2020.mp3", true, 0, 1304L },
  { 96, { 0x14, 0x37, 0xe5 }, "03/2021_2021.mp3", true, 1, 1657L },
  { 97, { 0x14, 0x37, 0xe6 }, "03/2022_2022.mp3", true, 1, 1618L },
  { 98, { 0x14, 0x37, 0xe7 }, "03/2023_2023.mp3", true, 1, 1618L },
  { 99, { 0x14, 0x37, 0xe8 }, "03/2024_2024.mp3", true, 1, 1853L },
  { 100, { 0x14, 0x37, 0xe9 }, "03/2025_2025.mp3", true, 1, 1774L },
  { 101, { 0x14, 0x37, 0xea }, "03/2026_2026.mp3", true, 1, 1892L },
  { 102, { 0x14, 0x37, 0xeb }, "03/2027_2027.mp3", true, 1, 1814L },
  { 103, { 0x14, 0x37, 0xec }, "03/2028_2028.mp3", true, 1, 1657L },
  { 104, { 0x14, 0x37, 0xed }, "03/2029_2029.mp3", true, 1, 1735L },

  // Misc prompts
  { BASE_SOON, { 0x14, 0x38, 0x00 }, "03/2048_bientot.mp3", true, 0, 1069L },
  { BASE_PAST, { 0x14, 0x38, 0x01 }, "03/2049_passe.mp3", true, 0, 795L },
  { BASE_MIDNIGHT, { 0x14, 0x38, 0x02 }, "03/2050_minuit.mp3", true, 0, 834L },

  { BASE_NOUS_SOMMES_LE, { 0x14, 0x32, 0xbc }, "03/0700_nous_sommes_le.mp3", true, 0, 1226L },
  { BASE_IL_EST,         { 0x14, 0x32, 0xbd }, "03/0701_il_est.mp3", true, 0, 677L },
  { BASE_IL_EST_BIENTOT, { 0x14, 0x32, 0xbe }, "03/0702_il_est_bientot.mp3", true, 0, 1226L },

  { BASE_LAST_PROMPT_DATE_TIME, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

/*  Construction des commandes correspondant à la synthèse de tous les prompts
 *  d'une date et heure avec jour dans la semaine
 *
 *  Inputs: - Adresse de la structure 'ST_DATE_AND_TIME'
 *          - Adresse du tableau des commandes (ressource allouée par 'PromptsDateHours.cpp') 
 *  Output: - Tableau des commandes renseigné
 *  Return: - Nombre de bytes du tableau des commandes renseigné avec comme convention:
 *            - 0 pour tableau non renseigné (adresse null, date/time incorrect, année incorrecte [2020, ..., 2029], etc.)
 *            - [1..n] pour le nombre de bytes du tableau renseigné
 *
 * Exemples: - "10" "octobre" "2020" "17 heures" "25" "2nd" "samedi" "du mois" (8 prompts)
 *           - "25" "octobre" "2020" "9 heures" "5" "4th" "et dernier" "dimanche" "du mois" (9 prompts)
 *
 * Remarques: - Seules les minutes multiples de 5 sont synthétisées
 *              => Appel de la méthode à ces heures/minutes
 *              => Prises des "5 minutes" les plus proches inférieures
 *            - Des pauses entre les prompts peuvent être insérées ;-)
 */
size_t buildCommandsPromptsDateTime(
  ST_DATE_AND_TIME *i__st_date_time,
  byte **o__command,
  ENUM_SYNTH_DATE_TIME_MODES i__mode,
  uint16_t **o__durations,
  size_t *o__nbr_durations)
{
  size_t  l__size = 0;

  g__prompts_str = "[";

  boolean l__flg_synth_num_day = true;

  if (i__mode == SYNTH_DATE
   || i__mode == SYNTH_DATE_TIME_ALL) {

    // Add prompt: "Nous sommes le"
    updateCommands(g__prompts_date_time, BASE_NOUS_SOMMES_LE, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);

    // Day in the month [1, 2, ..., 31]
    char l__day = i__st_date_time->day;
    if (l__day == 0 || l__day > 31) {
      updateCommands(g__prompts_general, BASE_OUPS_MALE, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);
    }
    else if (l__day == 1) {
      updateCommands(g__prompts_date_time, BASE_FIRST, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
    else {
      updateCommands(g__prompts_date_time, BASE_2_DAY + l__day - 2, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
  
    // Month [1, 2, ..., 12]
    char l__month = i__st_date_time->month;
    if (l__month >= 1 && l__month <= 12) {
      updateCommands(g__prompts_date_time, BASE_JANUARY - 1 + l__month, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
    else {
      updateCommands(g__prompts_general, BASE_OUPS_MALE, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);
    }

    // Year [2020, 2021, ..., 2029]
    char l__year = i__st_date_time->year;
    if (l__year >= 20 && l__year <= 29) {
      updateCommands(g__prompts_date_time, BASE_2020 + l__year - 20, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
    else {
      updateCommands(g__prompts_general, BASE_OUPS_MALE, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);
    }
  }

  if (i__mode == SYNTH_TIME
   || i__mode == SYNTH_DATE_TIME_ALL) {

    // Hour [0, 1, ..., 23]
    boolean l__flg_wrong_hours = false;
    char l__hours = i__st_date_time->hours;
    if (l__hours > 23) {
      l__flg_wrong_hours = true;
    }
    /*  L'appel à 'updateCommands()' se fera après la détermination des "5 minutes"
     *  car 'l__hours' pourrait être incrémenté de +1 avec 24 pour indiquer minuit   
     *  => 'updateCommands(g__prompts_date_time, BASE_0_HOUR + l__hours, &l__size, BASE_LAST_PROMPT_DATE_TIME);'
     *  or 'updateCommands(g__prompts_date_time, BASE_MIDNIGHT, &l__size, BASE_LAST_PROMPT_DATE_TIME);'
     *  avec une insertion avant de:
     *     'updateCommands(g__prompts_date_time, BASE_SOON, &l__size, BASE_LAST_PROMPT_DATE_TIME);'
     */

    /* Minutes [0, 1, ..., 59]   
     *  - Recherche des "5 minutes" les plus proches (inférieures ou supérieures)   
     *  - Test des secondes [0, ..., 29]  -> "5 minutes" précédentes   
     *                      [30, ..., 59] -> "5 minutes" suivantes                   
     */
     
    char l__seconds = i__st_date_time->seconds;
    char l__minutes = i__st_date_time->minutes;
    size_t l__base = 0;
    boolean l__flg_soon = false;
    boolean l__flg_past = false;
    boolean l__flg_inc_hours = false;

    // Pas de synthèse si les minutes sont à 0
    if (l__minutes > 59) {
      updateCommands(g__prompts_general, BASE_OUPS_MALE, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);
    }
    else {
      /* Recherche des "5 minutes" les plus proches (inférieures ou supérieures) si minutes non multiples de 5
       * => Si "5 minutes" déterminées à 60 (passage à l'heure suivante) -> Pas de report sur l'heure suivante
       *    car trop compliqué à gérer le passage à minuit qui peut faire changer de jour, et même de mois ou d'année ;-((
       *    => Absence de synthèse des minutes retenue ;-)
       *
       * Remarques: - Si "5 minutes" inférieures -> Insertion du prompt "passé" après la synthèse des heures/minutes
       *            - Si "5 minutes" supérieures -> Insertion du prompt "il est bientôt" avant la synthèse des heures/minutes
       *            - Si "5 minutes exactes      -> Insertion du prompt "il est" avant la synthèse des heures/minutes
       *
       *            - Si insertion, le texte du prompt "passé" ou "il est [bientôt]" sera inséré au bon endroit (après ou avant)
       *              la synthèse complète des heures/minutes ;-)
       *
       */

      switch (l__minutes % 5) {
      case 0:   // Minutes à [0, 5, 10, 15, ..., 55]
        // "5 minutes" exactes et pas de synthèse si minutes à 0
        l__base = (l__minutes != 0) ? (l__minutes / 5) - 1 : (size_t)-1;
        break;
      case 1:   // Minutes à [1, 6, 11, 16, ..., 56]
        // Ramenées aux "5 minutes" inférieures
        l__base = ((l__minutes - 1)/ 5) - 1;
        l__flg_past = true;
        break;
      case 2:   // Minutes à [2, 7, 12, 17, ..., 57]
       if (l__seconds < 30) {
          // Ramenées aux "5 minutes" inférieures si "secondes" < 30
          l__base = ((l__minutes - 2) / 5) - 1;
          l__flg_past = true;
        }
        else {
          // Ramenées aux "5 minutes" supérieures si "secondes" >= 30
          if (l__minutes != 57) {
            l__base = ((l__minutes - 2 + 5) / 5) - 1;
         }
         else {
           l__base = (size_t)-1;   // Pas de synthèse des minutes
           l__flg_inc_hours = true;
         }
         l__flg_soon = true;
       }
        break;
      case 3:   // Minutes à [3, 8, 13, 18, ..., 58]
       // Ramenées aux "5 minutes" supérieures
        if (l__minutes != 58) {
          l__base = ((l__minutes - 3 + 5) / 5) - 1;
       }
       else {
          l__base = (size_t)-1;   // Pas de synthèse des minutes
         l__flg_inc_hours = true;
        }
        l__flg_soon = true;
        break;
     case 4:   // Minutes à [4, 9, 14, 19, ..., 59]
        // Ramenées aux "5 minutes" supérieures
        if (l__minutes != 59) {
          l__base = ((l__minutes - 4 + 5) / 5) - 1;
       }
       else {
         l__base = (size_t)-1;   // Pas de synthèse des minutes
          l__flg_inc_hours = true;
        }
        l__flg_soon = true;
        break;
      default:
        break;
      }
    }

    if (l__flg_soon == true) {
      // Insertion de "Il est bientot" avant la synthèse des heures/minutes (HH::MM avec MM non multiple de "5 minutes")
      updateCommands(g__prompts_date_time, BASE_IL_EST_BIENTOT, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
    else {
      // Insertion de "Il est" avant la synthèse des heures/minutes (HH::MM avec MM multiple de "5 minutes")
      updateCommands(g__prompts_date_time, BASE_IL_EST, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }

    if (l__flg_wrong_hours == true) {
      updateCommands(g__prompts_general, BASE_OUPS_MALE, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);
    }
    // Test si incrément des heures à faire
    else if (l__flg_inc_hours == false) {
      updateCommands(g__prompts_date_time, BASE_0_HOUR + l__hours, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
    else {
      /*  Replace the synthesis of hours by ('hours' + 1) or 'midnight'
       *  => Le numéro du jour dans la semaine n'est pas recalculé (trop compliqué et serait
       *     pertinent pendant 1 ou 2 heures avant minuit
       *     => Pas de synthèse de ce numéro du jour dans le mois ainsi que du prompt "du mois" ;-)
       */
      l__hours += 1;

      if (l__hours < 24) {
        updateCommands(g__prompts_date_time, BASE_0_HOUR + l__hours, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
      }
      else {
        // 'l__hours' à 24 pour une synthèse de 'minuit'
        updateCommands(g__prompts_date_time, BASE_MIDNIGHT, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);

        l__flg_synth_num_day = false;
      }
    }

    if (l__base != (size_t)-1) {
      updateCommands(g__prompts_date_time, BASE_5_MINUTES + l__base, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }

    if (l__flg_past == true) {
      // Insertion du prompt "passé" après la synthèse des heures/minutes
      updateCommands(g__prompts_date_time, BASE_PAST, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
  }

  if (i__mode == SYNTH_DATE_TIME_ALL) {
    char l__day_of_week = i__st_date_time->day_of_week;

    if (l__flg_synth_num_day == true) {
      /*  Jour dans la semaine avec son rang dans le mois
       * 
       *  Exemples: - "2nd" "jeudi" "du mois"
       *            - "4th" "mercredi" "et dernier" "du mois"
       */
      byte l__nbr_days_before = i__st_date_time->nbr_days_before;
      byte l__nbr_days_after  = i__st_date_time->nbr_days_after;

      Serial.printf("buildCommandsPromptsDateTime(): Days before [%d] after [%d]\n", l__nbr_days_before, l__nbr_days_after);
      updateCommands(g__prompts_date_time, BASE_FIRST + l__nbr_days_before, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);

      if (l__nbr_days_after == 0) {
        updateCommands(g__prompts_date_time, BASE_AND_LAST, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
      }
    }

    /*  Synthesis of Day in the Week (Warning: #0 Jeudi into 'i__st_date_time'
     *  and #0: 'BASE_MONDAY' for synthesis)
     *
     *  l__day_of_week = 0 (Jeudi)    -> 'BASE_MONDAY' + 3
     *  l__day_of_week = 1 (Vendredi) -> 'BASE_MONDAY' + 4
     *  l__day_of_week = 2 (Samedi)   -> 'BASE_MONDAY' + 5
     *  l__day_of_week = 3 (Dimanche) -> 'BASE_MONDAY' + 6
     *  l__day_of_week = 4 (Lundi)    -> 'BASE_MONDAY' + 0
     *  l__day_of_week = 5 (Mardi)    -> 'BASE_MONDAY' + 1
     *  l__day_of_week = 6 (Mercredi) -> 'BASE_MONDAY' + 2
     */
    size_t l__day_offset = (l__day_of_week + 3) % 7;
    updateCommands(g__prompts_date_time, BASE_MONDAY + l__day_offset, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);

    if (l__flg_synth_num_day == true) {
      updateCommands(g__prompts_date_time, BASE_OF_MONTH, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }

    // Heure d'été/hiver
    if (i__st_date_time->sommer_winter == SOMMER) {
      updateCommands(g__prompts_date_time, BASE_TIME_SOMMER, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
    else if (i__st_date_time->sommer_winter == WINTER) {
      updateCommands(g__prompts_date_time, BASE_TIME_WINTER, &l__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
    }
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command   = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for date/time synthesis

// List of definitions and methods for distance
#define NUM_DIST_METER                 2000 
#define NUM_DIST_KILOMETER             2001

#define NUM_LAST_POS_SAVE              3000                 // Last position saved
#define NUM_STARTING_POS               3010
#define NUM_APPROXIMATELY              3020
#define NUM_AS_THE_CROW_FLIES          3030

#define NUM_YOU_ARE_AT                 3040

#define NUM_OF_STARTING_POS            3050
#define NUM_OF_LAST_POS_SAVE           3051

#define NUM_RETURN_OF_STARTING_POS     3060
#define NUM_RETURN_OF_LAST_POS_SAVE    3061

#define BASE_DIST_1                    0                          // 36 values in the range [1, 2, ..., 900] with absence of certain values
#define BASE_DIST_METER                (BASE_DIST_1 + 37)
#define BASE_DIST_KILOMETER            (BASE_DIST_METER + 1)

#define BASE_STARTING_POS              (BASE_DIST_KILOMETER + 1)
#define BASE_STARTING_POS_ALWAYS       (BASE_STARTING_POS + 1)
#define BASE_STARTING_POS_HERE         (BASE_STARTING_POS + 2)
#define BASE_STARTING_POS_HERE_ALWAYS  (BASE_STARTING_POS + 3)

#define BASE_LAST_POS_SAVE             (BASE_STARTING_POS_HERE_ALWAYS + 1)   // Last position saved
#define BASE_LAST_POS_SAVE_ALWAYS      (BASE_LAST_POS_SAVE + 1)
#define BASE_LAST_POS_SAVE_HERE        (BASE_LAST_POS_SAVE + 2)    // You are at the last saved position
#define BASE_LAST_POS_SAVE_HERE_ALWAYS (BASE_LAST_POS_SAVE + 3)

#define BASE_APPROXIMATELY             (BASE_LAST_POS_SAVE_HERE_ALWAYS + 1)
#define BASE_LESS_OF                   (BASE_APPROXIMATELY + 1)
#define BASE_MORE_THAN                 (BASE_APPROXIMATELY + 2)

#define BASE_AS_THE_CROW_FLIES         (BASE_MORE_THAN + 1)          // As the crow flies (à vol d'oiseau)

#define BASE_YOU_ARE_AT                (BASE_AS_THE_CROW_FLIES + 1)  // You are at
#define BASE_YOU_ARE_AT_ALWAYS         (BASE_YOU_ARE_AT + 1)         // You are still at

#define BASE_OF_STARTING_POS           (BASE_YOU_ARE_AT_ALWAYS + 1)
#define BASE_OF_LAST_POS_SAVE          (BASE_OF_STARTING_POS + 1)

#define BASE_RETURN_OF_STARTING_POS    (BASE_OF_LAST_POS_SAVE + 1)
#define BASE_RETURN_OF_LAST_POS_SAVE   (BASE_RETURN_OF_STARTING_POS + 1)

#define BASE_LAST_PROMPT_DISTANCE      (BASE_RETURN_OF_LAST_POS_SAVE + 1)

static const ST_PROMPTS_DEF           g__prompts_distance[BASE_LAST_PROMPT_DISTANCE + 1] =
{
  { BASE_DIST_1,      { 0x14, 0x20, 0x01 }, "02/0001_1.mp3", true, 0, 442L },
  { BASE_DIST_1 + 1,  { 0x14, 0x20, 0x02 }, "02/0002_2.mp3", true, 0, 520L },
  { BASE_DIST_1 + 2,  { 0x14, 0x20, 0x03 }, "02/0003_3.mp3", true, 0, 677L },
  { BASE_DIST_1 + 3,  { 0x14, 0x20, 0x04 }, "02/0004_4.mp3", true, 0, 795L },
  { BASE_DIST_1 + 4,  { 0x14, 0x20, 0x05 }, "02/0005_5.mp3", true, 0, 599L },
  { BASE_DIST_1 + 5,  { 0x14, 0x20, 0x06 }, "02/0006_6.mp3", true, 0, 952L },
  { BASE_DIST_1 + 6,  { 0x14, 0x20, 0x07 }, "02/0007_7.mp3", true, 0, 912L },
  { BASE_DIST_1 + 7,  { 0x14, 0x20, 0x08 }, "02/0008_8.mp3", true, 0, 873L },
  { BASE_DIST_1 + 8,  { 0x14, 0x20, 0x09 }, "02/0009_9.mp3", true, 0, 834L },
  { BASE_DIST_1 + 9,  { 0x14, 0x20, 0x0a }, "02/0010_10.mp3", true, 0, 952L },
  { BASE_DIST_1 + 10, { 0x14, 0x20, 0x0f }, "02/0015_15.mp3", true, 0, 795L },
  { BASE_DIST_1 + 11, { 0x14, 0x20, 0x14 }, "02/0020_20.mp3", true, 0, 599L },
  { BASE_DIST_1 + 12, { 0x14, 0x20, 0x19 }, "02/0025_25.mp3", true, 0, 1226L },
  { BASE_DIST_1 + 13, { 0x14, 0x20, 0x1e }, "02/0030_30.mp3", true, 0, 795L },
  { BASE_DIST_1 + 14, { 0x14, 0x20, 0x23 }, "02/0035_35.mp3", true, 0, 1187L },
  { BASE_DIST_1 + 15, { 0x14, 0x20, 0x28 }, "02/0040_40.mp3", true, 0, 991L },
  { BASE_DIST_1 + 16, { 0x14, 0x20, 0x2d }, "02/0045_45.mp3", true, 0, 1382L },
  { BASE_DIST_1 + 17, { 0x14, 0x20, 0x32 }, "02/0050_50.mp3", true, 0, 1226L },
  { BASE_DIST_1 + 18, { 0x14, 0x20, 0x37 }, "02/0055_55.mp3", true, 1, 1539L },
  { BASE_DIST_1 + 19, { 0x14, 0x20, 0x3c }, "02/0060_60.mp3", true, 0, 1265L },
  { BASE_DIST_1 + 20, { 0x14, 0x20, 0x41 }, "02/0065_65.mp3", true, 1, 1539L },
  { BASE_DIST_1 + 21, { 0x14, 0x20, 0x46 }, "02/0070_70.mp3", true, 1, 1774L },
  { BASE_DIST_1 + 22, { 0x14, 0x20, 0x4b }, "02/0075_75.mp3", true, 1, 1539L },
  { BASE_DIST_1 + 23, { 0x14, 0x20, 0x50 }, "02/0080_80.mp3", true, 0, 1030L },
  { BASE_DIST_1 + 24, { 0x14, 0x20, 0x55 }, "02/0085_85.mp3", true, 0, 1461L },
  { BASE_DIST_1 + 25, { 0x14, 0x20, 0x5a }, "02/0090_90.mp3", true, 1, 1539L },
  { BASE_DIST_1 + 26, { 0x14, 0x20, 0x5f }, "02/0095_95.mp3", true, 0, 1500L },
  { BASE_DIST_1 + 27, { 0x14, 0x20 + 100 / 256, 100 % 256 },   "02/0100_100.mp3", true, 0, 638L },
  { BASE_DIST_1 + 28, { 0x14, 0x20 + 200 / 256, 200 % 256 },   "02/0200_200.mp3", true, 0, 873L },
  { BASE_DIST_1 + 29, { 0x14, 0x20 + 300 / 256, 300 % 256 },   "02/0300_300.mp3", true, 0, 912L },
  { BASE_DIST_1 + 30, { 0x14, 0x20 + 400 / 256, 400 % 256 },   "02/0400_400.mp3", true, 0, 1069L },
  { BASE_DIST_1 + 31, { 0x14, 0x20 + 500 / 256, 500 % 256 },   "02/0500_500.mp3", true, 0, 1030L },
  { BASE_DIST_1 + 32, { 0x14, 0x20 + 600 / 256, 600 % 256 },   "02/0600_600.mp3", true, 0, 952L },
  { BASE_DIST_1 + 33, { 0x14, 0x20 + 700 / 256, 700 % 256 },   "02/0700_700.mp3", true, 0, 1108L },
  { BASE_DIST_1 + 34, { 0x14, 0x20 + 800 / 256, 800 % 256 },   "02/0800_800.mp3", true, 0, 952L },
  { BASE_DIST_1 + 35, { 0x14, 0x20 + 900 / 256, 900 % 256 },   "02/0900_900.mp3", true, 0, 991L },
  { BASE_DIST_1 + 36, { 0x14, 0x20 + 1000 / 256, 1000 % 256 }, "02/1000_1000.mp3", true, 0, 716L },

  { BASE_DIST_METER,     { 0x14, 0x20 + NUM_DIST_METER / 256,     NUM_DIST_METER % 256 },     "02/2000_metres.mp3", true, 1, 834L },
  { BASE_DIST_KILOMETER, { 0x14, 0x20 + NUM_DIST_KILOMETER / 256, NUM_DIST_KILOMETER % 256 }, "02/2001_kilometres.mp3", true, 1, 1265L },

  { BASE_STARTING_POS,             { 0x14, 0x20 +  NUM_STARTING_POS / 256,       NUM_STARTING_POS % 256 },      "02/3010_la_position_de_depart_est_a.mp3", true, 3, 2597L },
  { BASE_STARTING_POS_ALWAYS,      { 0x14, 0x20 + (NUM_STARTING_POS + 1) / 256, (NUM_STARTING_POS + 1) % 256 }, "02/3011_la_position_de_depart_est_toujours_a.mp3", true, 3, 2989L },
  { BASE_STARTING_POS_HERE,        { 0x14, 0x20 + (NUM_STARTING_POS + 2) / 256, (NUM_STARTING_POS + 2) % 256 }, "02/3012_vous_etes_a_la_position_de_depart.mp3", true, 3, 2950L },
  { BASE_STARTING_POS_HERE_ALWAYS, { 0x14, 0x20 + (NUM_STARTING_POS + 3) / 256, (NUM_STARTING_POS + 3) % 256 }, "02/3013_vous_etes_toujours_a_la_position_de_depart.mp3", true, 4, 3616L },

  { BASE_LAST_POS_SAVE,             { 0x14, 0x20 +  NUM_LAST_POS_SAVE / 256,       NUM_LAST_POS_SAVE % 256 },      "02/3000_la_derniere_position_enregistree_est_a.mp3", true, 4, 3655L },
  { BASE_LAST_POS_SAVE_ALWAYS,      { 0x14, 0x20 + (NUM_LAST_POS_SAVE + 1) / 256, (NUM_LAST_POS_SAVE + 1) % 256 }, "02/3001_la_derniere_position_enregistree_est_toujours_a.mp3", true, 4, 3694L },
  { BASE_LAST_POS_SAVE_HERE,        { 0x14, 0x20 + (NUM_LAST_POS_SAVE + 2) / 256, (NUM_LAST_POS_SAVE + 2) % 256 }, "02/3002_vous_etes_a_la_derniere_position_enregistree.mp3", true, 4, 3655L },
  { BASE_LAST_POS_SAVE_HERE_ALWAYS, { 0x14, 0x20 + (NUM_LAST_POS_SAVE + 3) / 256, (NUM_LAST_POS_SAVE + 3) % 256 }, "02/3003_vous_etes_toujours_a_la_derniere_position_enregistree.mp3", true, 4, 4360L },

  { BASE_APPROXIMATELY, { 0x14, 0x20 +  NUM_APPROXIMATELY / 256,       NUM_APPROXIMATELY % 256 },      "02/3020_environ.mp3", true, 1, 952L },
  { BASE_LESS_OF,       { 0x14, 0x20 + (NUM_APPROXIMATELY + 1 )/ 256, (NUM_APPROXIMATELY + 1) % 256 }, "02/3021_moins_de.mp3", true, 1, 991L },
  { BASE_MORE_THAN,     { 0x14, 0x20 + (NUM_APPROXIMATELY + 2) / 256, (NUM_APPROXIMATELY + 2) % 256 }, "02/3022_plus_de.mp3", true, 1, 834L },

  { BASE_AS_THE_CROW_FLIES, { 0x14, 0x20 + NUM_AS_THE_CROW_FLIES / 256, NUM_AS_THE_CROW_FLIES % 256 }, "02/3030_a_vol_d_oiseau.mp3", true, 1, 1304L },

  { BASE_YOU_ARE_AT,        { 0x14, 0x20 + (NUM_YOU_ARE_AT) / 256,     (NUM_YOU_ARE_AT) % 256 },     "02/3040_vous_etes_a.mp3", true, 1, 1069L },
  { BASE_YOU_ARE_AT_ALWAYS, { 0x14, 0x20 + (NUM_YOU_ARE_AT + 1) / 256, (NUM_YOU_ARE_AT + 1) % 256 }, "02/3041_vous_etes_toujours_a.mp3", true, 2, 1618L },

  { BASE_OF_STARTING_POS,  { 0x14, 0x20 + NUM_OF_STARTING_POS / 256,  NUM_OF_STARTING_POS % 256 },  "02/3050_de_la_position_de_depart.mp3", true, 3, 2519L },
  { BASE_OF_LAST_POS_SAVE, { 0x14, 0x20 + NUM_OF_LAST_POS_SAVE / 256, NUM_OF_LAST_POS_SAVE % 256 }, "02/3051_de_la_derniere_position_enregistree.mp3", true, 3, 3028L },

  { BASE_RETURN_OF_STARTING_POS,  { 0x14, 0x20 + NUM_RETURN_OF_STARTING_POS / 256,  NUM_RETURN_OF_STARTING_POS % 256 },  "02/3060_vous_etes_revenu_a_la_position_de_depart.mp3", true, 4, 3538L },
  { BASE_RETURN_OF_LAST_POS_SAVE, { 0x14, 0x20 + NUM_RETURN_OF_LAST_POS_SAVE / 256, NUM_RETURN_OF_LAST_POS_SAVE % 256 }, "02/3061_vous_etes_revenu_a_la_derniere_position_enregistree.mp3", true, 4, 4282L },

  { BASE_LAST_PROMPT_DISTANCE, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

// Pour préciser "toujours" si la distance courante et le mode sont identiques à ceux précédemment synthétisés
static uint32_t                 g__dist_rounded_previous = (uint32_t)-1L;
ENUM_SYNTH_DISTANCE_MODES       g__dist_mode_previous    = SYNTH_DIST_NO_MEAN;

/* Construction des commandes correspondant à la synthèse d'une distance en mètres et/ou kilomètres
 *
 * Inputs: - Distance en mètres
 *          - Type de la distance 'ENUM_SYNTH_DISTANCE_MODES'
 * Output: - Tableau des commandes renseigné
 * Return: - Nombre de bytes du tableau des commandes renseigné avec comme convention:
 *           - 0 pour tableau non renseigné (adresse null, distance incorrecte, etc.)
 *             => TBC: Définir une distance "incorrecte" (à confirmer sur le terrain)
 *           - [1..n] pour le nombre de bytes du tableau renseigné
 *
 * Remarques: La distance passée en argument est éventuellement approximée à la valeur la plus proche
 *            en fonction des valeurs à synthétiser; à savoir:
 *            - Distance <= 1000 M:  [0, 50, 100, ..., 850, 900, 950, 1000] M (arrondi à 50 m)
 *            - Distance <= 10 Km:   [1.1, 1.2, 1.3, ..., 9.8, 9.9, 10.0] Km (arrondi à 100 m)
 *            - Distance <= 100 Km:  [15, 20, 25, ..., 90, 95, 100] Km (arrondi à 5 Km)
 *            - Distance <= 1000 Km: [110, 120, ..., 980, 990, 1000] Km (arrondi à 10 Km)
 *
 *            Remarque: Même arrondis que dans la méthode synthesisDistance()
 *
 *            - Si la distance est arrondie  => synthèse du prompt "environ"
 *            - Si la distance est > 1000 Km => synthèse du prompt "plus de"
 *
 *            - Si distance < 10 M => "Vous êtes ..." (à confirmer sur la terrain)
 *            - Si distance < 30 M => "La position ... est à" "moins de" "50" "mètres" (à confirmer sur la terrain)
 *
 * Exemples: - "La position de départ est à" "1" "kilomètre" "400" "à vol d'oiseau" (5 prompts)
 *           - "Vous êtes toujours à la dernière position enregistrée" (1 prompt)
 *           - "La position de départ est à" "5" "kilomètre[s]" "700" "environ" "à vol d'oiseau" (6 prompts)
 *           - "La position de départ est à" "plus de" "25" "kilomètre[s]" "à vol d'oiseau" (5 prompts)
 */
static ENUM_SYNTH_DISTANCE_TYPES distanceRounded(uint32_t *io__distance, boolean i__flg_extend_11_to_30_km = false)
{
#if 0
  printf("### distanceRounded(%d): Entering...\n", *io__distance);
#endif

  uint32_t l__distance = *io__distance;
  ENUM_SYNTH_DISTANCE_TYPES l__type = SYNTH_DIST_UNKNOWN_TYPE;
  uint32_t l__distance_rounded = 0;

  //if (l__distance != 0L) {
    if (l__distance <= DISTANCE_VERY_NEAR) {
      // Distance très proche => Forçage à 10 m
      l__distance_rounded = DISTANCE_VERY_NEAR;

      l__type = SYNTH_DIST_VERY_NEAR;
    }
    else if (l__distance <= DISTANCE_NEAR) {
      // Distance proche => Forçage à 'DISTANCE_NEAR' m
      l__distance_rounded = DISTANCE_NEAR;

      l__type = SYNTH_DIST_NEAR;
    }
    else if (l__distance <= DISTANCE_1KM) {
      /*  Distance <= 1000 M:  [0, 50, 100, ..., 850, 900, 950, 1000] M (arrondi à 50 m)
       *  Exemples: - 610 m => l__dist_mul_100 = 6; l__pivot = 650; l__diff = -40 => l__distance_rounded = 600 m
       *            - 842 m => l__dist_mul_100 = 8; l__pivot = 850; l__diff = -8  => l__distance_rounded = 850 m
       *            - 57 m  => l__dist_mul_100 = 0; l__pivot = 50; l__diff = 7    => l__distance_rounded = 50 m
       *            - 76 m  => l__dist_mul_100 = 0; l__pivot = 50; l__diff = 26   => l__distance_rounded = 100 m
       */
      uint32_t l__dist_mul_100 = (l__distance / 100L);
      uint32_t l__pivot = (100L * l__dist_mul_100 + 50L);
      int      l__diff  = (l__distance - l__pivot);
      if (l__diff < -DISTANCE_25M) {
        l__distance_rounded = l__pivot - DISTANCE_50M;
      }
      else if (l__diff < DISTANCE_25M) {
        l__distance_rounded = l__pivot;
      }
      else  {
        l__distance_rounded = l__pivot + DISTANCE_50M;
      }
 
      l__type = SYNTH_DIST_DISTANT;
    }
    else if (l__distance <= DISTANCE_10KM) {
      /* Distance <= 10 Km:   [1.1, 1.2, 1.3, ..., 9.8, 9.9, 10.0] Km (arrondi à 100 m)
       * Exemples: - 12450 m => l__dist_mul_100 = 124; l__pivot = 12500; l__diff = -50 => l__distance_rounded = l__pivot = 12500 m
       *           - 16800 m => l__dist_mul_100 = 168; l__pivot = 16900; l__diff = -100 => l__distance_rounded = l__pivot - 100 = 16800 m
       */
      uint32_t l__dist_mul_100 = (l__distance / 100L);
      uint32_t l__pivot = (100L * l__dist_mul_100 + 100L);
      int      l__diff  = (l__distance - l__pivot);
      if (l__diff < -DISTANCE_50M) {
        l__distance_rounded = l__pivot - DISTANCE_100M;
      }
      else if (l__diff < DISTANCE_50M) {
        l__distance_rounded = l__pivot;
      }
      else {
        l__distance_rounded = l__pivot + DISTANCE_100M;
      }

      l__type = SYNTH_DIST_DISTANT;
    }
    else if (i__flg_extend_11_to_30_km == true && l__distance <= DISTANCE_30KM) {
      /* Distance <= 30 Km:   [10.1, 10.2, 10.3, ..., 29.8, 29.9, 30.0] Km (arrondi à 100 m)
       * => 'l__dist_mul_100' = [101, 102, 103, ..., 298, 299, 300]
       *    => 'l__pivot' = [10200, 10300, 10400, ..., 29900, 30000, 30100]
       * 
       * Exemples: - 10001 m => l__dist_mul_100 = 100; l__pivot = 10100; l__diff = (10001 - 10100) = -99 => l__distance_rounded = (10100 - 100) = 10000 m
       *           - 14683 m => l__dist_mul_100 = 146; l__pivot = 14700; l__diff = (14683 - 14700) = -17 => l__distance_rounded = l__pivot = 14700 m
       *           - 18230 m => l__dist_mul_100 = 182; l__pivot = 18300; l__diff = (18230 - 18300) = -70 => l__distance_rounded = (18300 - 100) = 18200 m
       */
      uint32_t l__dist_mul_100 = (l__distance / 100L);
      uint32_t l__pivot = (100L * l__dist_mul_100 + 100L);
      int      l__diff  = (l__distance - l__pivot);
      if (l__diff < -DISTANCE_50M) {
        l__distance_rounded = l__pivot - DISTANCE_100M;
      }
      else if (l__diff < DISTANCE_50M) {
        l__distance_rounded = l__pivot;
      }
      else {
        l__distance_rounded = l__pivot + DISTANCE_100M;
      }      

      l__type = SYNTH_DIST_DISTANT;
    }
    else if (l__distance <= DISTANCE_100KM) {
      /* Distance <= 100 Km:  [15, 20, 25, ..., 90, 95, 100] Km (arrondi à 5 Km)
       * Exemples: - 12456 m => l__dist_mul_10000 = 1; l__pivot = 15000; l__diff = -2544 => l__distance_rounded = 10 Km
       *           - 76100 m => l__dist_mul_10000 = 7; l__pivot = 75000; l__diff =  1100 => l__distance_rounded = 75 Km
       */
      uint32_t l__dist_mul_10000 = (l__distance / 10000L);
      uint32_t l__pivot = (10000L * l__dist_mul_10000 + 5000L);
      int      l__diff  = (l__distance - l__pivot);
      if (l__diff < -DISTANCE_2_5KM) {
        l__distance_rounded = l__pivot - DISTANCE_5KM;
      }
      else if (l__diff < DISTANCE_2_5KM) {
        l__distance_rounded = l__pivot;
      }
      else {
        l__distance_rounded = l__pivot + DISTANCE_5KM;
      }

      l__type = SYNTH_DIST_DISTANT;
    }
    else if (l__distance <= (DISTANCE_1000KM + DISTANCE_10KM)) {
      /*  Distance <= 1050 Km: [110, 120, ..., 980, 990, 1000, 1010] Km (arrondi à 10 Km)
       *  Exemples: - 124560 m => l__dist_mul_10000 = 12; l__pivot = 130000; l__diff =  -5440 => l__distance_rounded = 120 Km
       *            - 761000 m => l__dist_mul_10000 = 76; l__pivot = 770000; l__diff =  -9000 => l__distance_rounded = 750 Km
       *            - 970000 m => l__dist_mul_10000 = 97; l__pivot = 980000; l__diff = -10000 => l__distance_rounded = 970 Km
       *            - 118000 m => l__dist_mul_10000 = 11; l__pivot = 120000; l__diff =  -2000 => l__distance_rounded = 120 Km
       */
      uint32_t l__dist_mul_10000 = (l__distance / 10000L);
      uint32_t l__pivot = (10000L * l__dist_mul_10000 + 10000L);
      int      l__diff  = (l__distance - l__pivot);
      if (l__diff < -DISTANCE_5KM) {
        l__distance_rounded = l__pivot - DISTANCE_10KM;
      }
      else if (l__diff < DISTANCE_5KM) {
        l__distance_rounded = l__pivot;
      }
      else {
        l__distance_rounded = l__pivot - DISTANCE_10KM;
      }

      l__type = SYNTH_DIST_DISTANT;
    }
    else {
      // Distance très éloignée => Forçage à 1000 Km
      l__distance_rounded = DISTANCE_1000KM;

      l__type = SYNTH_DIST_VERY_DISTANT;
    }
  //}

#if 0
  const char *l__text = "Unknown";
  switch (l__type) {
  case SYNTH_DIST_VERY_NEAR:             // 1: Distance très proche
    l__text = "SYNTH_DIST_VERY_NEAR";
    break;
  case SYNTH_DIST_NEAR:                  // 2: Distance proche
    l__text = "SYNTH_DIST_NEAR";
    break;
  case SYNTH_DIST_DISTANT:               // 3: Distance éloignée
    l__text = "SYNTH_DIST_DISTANT";
    break;
  case SYNTH_DIST_VERY_DISTANT:          // 4: Distance très éloignée
    l__text = "SYNTH_DIST_VERY_DISTANT";
    break;
  default:
    break;
  }

  printf("### distanceRounded(%d): Leave with [%d] and return type [%d] (%s)\n\n",
    *io__distance, l__distance_rounded, l__type, l__text);
#endif

  *io__distance = l__distance_rounded;
  return l__type;
}

/*  Synthèse d'une distance:
 *  => Arrondie à 50 M si distance < 1 Km
 *  => Sinon, arrondie à 100 M si distance <= 10 Km
 *  => Sinon, arrondie à 5 Km si distance <= 100 Km
 *  => Sinon, arrondie à 10 Km si distance <= 1010 Km
 */
static void synthesisDistance(uint32_t i__distance, size_t *io__size, size_t *o__nbr_durations, boolean i__flg_extend_11_to_30_km = false)
{
#if 0
  printf("### synthesisDistance(%d): Entering...\n", i__distance);
#endif

  // Extract the meters and kilometers
  uint32_t l__kilometers = i__distance / 1000;
  uint16_t l__meters     = i__distance % 1000;

  if (l__kilometers == 0) {
    // Synthesis of xxx meters only
    uint16_t l__quotient_hundred_meters = l__meters / 100;    // [0, 1, 2, ..., 9]
    uint16_t l__rest_hundred_meters     = l__meters % 100;    // [0, 50]

#if 0
    printf("###\tmeters [%d] quotient [%d] rest [%d]\n",
      l__meters, l__quotient_hundred_meters, l__rest_hundred_meters);
#endif

    if (l__quotient_hundred_meters == 0) {
      /* Distance < 100 m
         => 'l__rest_hundred_meters' must be equal to 50 because round at 50 meters
      */
      if (l__rest_hundred_meters == DISTANCE_50M) {
        updateCommands(g__prompts_distance, BASE_DIST_1 + 17, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
        updateCommands(g__prompts_distance, BASE_DIST_METER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
      }
      else {
        updateCommandsWithOups(io__size);
      }
    }
    else {
      /* Distance >= 100 m
         => 'l__quotient_hundred_meters' in the range [1, 2, ..., 9]
         => 'l__rest_hundred_meters' must be equal to 0 or 50 because round at 50 meters
      */
      updateCommands(g__prompts_distance, BASE_DIST_1 + 27 + (l__quotient_hundred_meters - 1), io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

      if (l__rest_hundred_meters == DISTANCE_50M) {
        updateCommands(g__prompts_distance, BASE_DIST_1 + 17, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
        updateCommands(g__prompts_distance, BASE_DIST_METER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
      }
      else if (l__rest_hundred_meters == 0) {
        updateCommands(g__prompts_distance, BASE_DIST_METER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
      }
      else {
        updateCommandsWithOups(io__size);
      }
    }
  }
  else {
    // Synthesis of xxx kilometers and yyy (without the 'meters' prompt ;-)

#if 0
    printf("###\tkilometers [%d] meters [%d]\n", l__kilometers, l__meters);
#endif

    if (l__kilometers <= (DISTANCE_10KM / 1000L)) {
      // Range into [1, 2, ..., 9, 10] (Round to 100 m)
      updateCommands(g__prompts_distance, BASE_DIST_1 + (l__kilometers - 1), io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
      updateCommands(g__prompts_distance, BASE_DIST_KILOMETER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);

      if (l__meters != 0) {
        uint16_t l__multiple_100m = (l__meters / 100);
        updateCommands(g__prompts_distance, BASE_DIST_1 + 27 + (l__multiple_100m - 1), io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
      }
    }
    else if (i__flg_extend_11_to_30_km == true && l__kilometers <= (DISTANCE_30KM / 1000L)) {
      // Range into [11, 12, ..., 29, 30] (Round to 100 m)
      // => Use the prompts of "Number of day in the month"
      updateCommands(g__prompts_date_time, BASE_2_DAY + l__kilometers - 2, io__size, BASE_LAST_PROMPT_DATE_TIME, o__nbr_durations);
      updateCommands(g__prompts_distance, BASE_DIST_KILOMETER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);

      if (l__meters != 0) {
        uint16_t l__multiple_100m = (l__meters / 100);
        updateCommands(g__prompts_distance, BASE_DIST_1 + 27 + (l__multiple_100m - 1), io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
      }      
    }
    else if (l__kilometers <= (DISTANCE_100KM / 1000L)) {
      /* Range into [15, 20, 25, ..., 90, 95] or [30, 35, 40, ..., 90, 95] (Round to 5 Km) 
         => Index   [ 0,  1,  2, ..., 15, 16]
         => 'l__meters' must be equal to 0 because round at 5 Km
      */
      uint16_t l__index = (l__kilometers / 5) - 3;
      updateCommands(g__prompts_distance, BASE_DIST_1 + 10 + l__index, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
      updateCommands(g__prompts_distance, BASE_DIST_KILOMETER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);

      if (l__meters != 0) {
        updateCommandsWithOups(io__size);
      }
    }
    else if (l__kilometers <= ((DISTANCE_1000KM + DISTANCE_10KM) / 1000L)) {
      // Range into [100, 110, 120, ..., 980, 990, 1000, 1010] (Round to 10 Km)
      uint16_t l__quotient_hundred_kilometers = l__kilometers / 100;    // [1, 2, ..., 9, 10]
      uint16_t l__rest_hundred_kilometers     = l__kilometers % 100;    // [0, 10, 20, ..., 90]

      if (l__kilometers < (DISTANCE_1000KM / 1000L)) {
        if (l__quotient_hundred_kilometers != 1) {
          updateCommands(g__prompts_distance, BASE_DIST_1 + (l__quotient_hundred_kilometers - 1), io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
        }
        updateCommands(g__prompts_distance, BASE_DIST_1 + 27, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
      }
      else {
        updateCommands(g__prompts_distance, BASE_DIST_1 + 36, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
      }

      if (l__rest_hundred_kilometers != 0) {
        /* Range into [10, 20, 30, ..., 80, 90] (Round to 10 Km)
           => Index   [ 0,  2,  4, ..., 14, 16]
           => 'l__rest_hundred_kilometers' must be multiple of 10
        */
        if ((l__rest_hundred_kilometers % 10) == 0) {
          uint16_t l__index = 2 * ((l__rest_hundred_kilometers / 10) - 1);
          updateCommands(g__prompts_distance, BASE_DIST_1 + 9 + l__index, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
        }
        else {
          updateCommandsWithOups(io__size);
        }
      }

      updateCommands(g__prompts_distance, BASE_DIST_KILOMETER, io__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
    }
  }

#ifdef USE_SIMULATION
  printf("\n");
#endif
}

size_t buildCommandsPromptsDistance(
  uint32_t i__distance,
  ENUM_SYNTH_DISTANCE_TYPES *o__dist_type,
  byte **o__command,
  ENUM_SYNTH_POSITION_MODES i__pos_mode,
  ENUM_SYNTH_DISTANCE_MODES i__dist_mode,
  uint16_t **o__durations,
  size_t *o__nbr_durations)
{
  uint32_t l__dist_rounded = i__distance;
  size_t  l__size = 0;
  boolean l__flg_synth_dist  = false;
  boolean l__flg_synth_about = true;

  g__prompts_str = "[";

  /*  Arrondi de la distannce @ à sa valeur et détermination de la proximité
   *  => Pilote la type de prompt synthétisé avant la synthèse de la distance
   *     si celle-ci n'est "très proche"
   */
  ENUM_SYNTH_DISTANCE_TYPES l__type = distanceRounded(&l__dist_rounded);

  boolean l__flg_same_distance_and_mode = (l__dist_rounded == g__dist_rounded_previous && g__dist_mode_previous == i__dist_mode) ? true : false;

  /* Return the type for synthesis or not of the cap
   * - SYNTH_DIST_VERY_NEAR: Pas de synthèse du cap car distance très proche
   * - SYNTH_DIST_NEAR,                  // 2: Distance proche
   * - SYNTH_DIST_DISTANT,               // 3: Distance éloignée
   * - SYNTH_DIST_VERY_DISTANT: Synthèse du cap car distance proche, éloignée ou très éloignée
   */
  if (o__dist_type != NULL) {
    *o__dist_type = l__type;
  }

#ifdef USE_SIMULATION
  g__dist_rounded_previous = l__dist_rounded; 
#endif

  switch (l__type) {
  case SYNTH_DIST_VERY_NEAR:
    if (i__dist_mode == SYNTH_DIST_FROM_STARTING_POS) {
      // Synthesis: "vous_etes_[toujours]_a_la_position_de_depart" [suivant 'l__flg_same_distance_and_mode']
      updateCommands(g__prompts_distance,
        l__flg_same_distance_and_mode ? BASE_STARTING_POS_HERE_ALWAYS : BASE_STARTING_POS_HERE,
        &l__size, BASE_LAST_PROMPT_DISTANCE,
        o__nbr_durations);
    }
    else if (i__dist_mode == SYNTH_DIST_FROM_POS_SAVE) {
      // Synthesis: "vous_etes_[toujours]_a_la_derniere_position_enregistree" [suivant 'l__flg_same_distance_and_mode']
      updateCommands(g__prompts_distance,
        l__flg_same_distance_and_mode ? BASE_LAST_POS_SAVE_HERE_ALWAYS : BASE_LAST_POS_SAVE_HERE,
        &l__size, BASE_LAST_PROMPT_DISTANCE,
        o__nbr_durations);
    }
    else {
      updateCommandsWithOups(&l__size);
    }
    break;

  case SYNTH_DIST_NEAR:
    if (i__dist_mode == SYNTH_DIST_FROM_STARTING_POS) {
      // Synthesis: "vous_etes_[toujours]_a" "moins_de" "DISTANCE_50M" "de_la_position_de_depart"
      updateCommands(g__prompts_distance,
        l__flg_same_distance_and_mode ? BASE_YOU_ARE_AT_ALWAYS : BASE_YOU_ARE_AT,
        &l__size, BASE_LAST_PROMPT_DISTANCE,
        o__nbr_durations); 

      updateCommands(g__prompts_distance, BASE_LESS_OF, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

      synthesisDistance(DISTANCE_50M, &l__size, o__nbr_durations);

      updateCommands(g__prompts_distance, BASE_OF_STARTING_POS, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
    }
    else if (i__dist_mode == SYNTH_DIST_FROM_POS_SAVE) {
      // Synthesis: "vous_etes_[toujours]_a" "moins_de" "DISTANCE_50M" "de_la_derniere_position_enregistree"
      updateCommands(g__prompts_distance,
        l__flg_same_distance_and_mode ? BASE_YOU_ARE_AT_ALWAYS : BASE_YOU_ARE_AT,
        &l__size, BASE_LAST_PROMPT_DISTANCE,
        o__nbr_durations); 

      updateCommands(g__prompts_distance, BASE_LESS_OF, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

      synthesisDistance(DISTANCE_50M, &l__size, o__nbr_durations);

      updateCommands(g__prompts_distance, BASE_OF_LAST_POS_SAVE, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
    }
    else {
      updateCommandsWithOups(&l__size);
    }
    break;

  case SYNTH_DIST_DISTANT:
    if (i__dist_mode == SYNTH_DIST_FROM_STARTING_POS) {
      if (i__pos_mode == SYNTH_POSITION) {
        // Synthesis: "la_position_de_depart_est_[toujours]_a"
       updateCommands(g__prompts_distance,
          l__flg_same_distance_and_mode ? BASE_STARTING_POS_ALWAYS : BASE_STARTING_POS,
          &l__size, BASE_LAST_PROMPT_DISTANCE,
          o__nbr_durations);
      }

      // La distance est synthétisée avec éventuellement le prompt "environ"
      l__flg_synth_dist = true;
    }
    else if (i__dist_mode == SYNTH_DIST_FROM_POS_SAVE) {
      if (i__pos_mode == SYNTH_POSITION) {
        // Synthesis: "la_derniere_position_enregistree_est_[toujours]_a"
        updateCommands(g__prompts_distance,
          l__flg_same_distance_and_mode ? BASE_LAST_POS_SAVE_ALWAYS : BASE_LAST_POS_SAVE,
          &l__size, BASE_LAST_PROMPT_DISTANCE,
          o__nbr_durations);
      }

      // La distance est synthétisée avec éventuellement le prompt "environ"
      l__flg_synth_dist = true;
    }
    else {
      updateCommandsWithOups(&l__size);
    }
    break;

  case SYNTH_DIST_VERY_DISTANT:
    if (i__dist_mode == SYNTH_DIST_FROM_STARTING_POS) {
      // Synthesis: "vous_etes_[toujours]_a" "plus_de" "DISTANCE_1000KM" "de_la_position_de_depart"
      updateCommands(g__prompts_distance,
        l__flg_same_distance_and_mode ? BASE_YOU_ARE_AT_ALWAYS : BASE_YOU_ARE_AT,
        &l__size, BASE_LAST_PROMPT_DISTANCE,
        o__nbr_durations); 

      updateCommands(g__prompts_distance, BASE_MORE_THAN, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

      synthesisDistance(DISTANCE_1000KM, &l__size, o__nbr_durations);

      updateCommands(g__prompts_distance, BASE_OF_STARTING_POS, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
    }
    else if (i__dist_mode == SYNTH_DIST_FROM_POS_SAVE) {
      // Synthesis: "vous_etes_[toujours]_a" "plus_de" "DISTANCE_1000KM" "de_la_derniere_position_enregistree"
      updateCommands(g__prompts_distance,
        l__flg_same_distance_and_mode ? BASE_YOU_ARE_AT_ALWAYS : BASE_YOU_ARE_AT,
        &l__size, BASE_LAST_PROMPT_DISTANCE,
        o__nbr_durations); 

      updateCommands(g__prompts_distance, BASE_MORE_THAN, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

      synthesisDistance(DISTANCE_1000KM, &l__size, o__nbr_durations);

      updateCommands(g__prompts_distance, BASE_OF_LAST_POS_SAVE, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
    }
    else {
      updateCommandsWithOups(&l__size);
    }
    break;

  default:
    updateCommandsWithOups(&l__size);
    break;
  }

  if (l__flg_synth_dist == true) {
    //  Synthèse de la distance arrondie avec une décomposition du nombre, des mètres et des kilomètres dans la plage [50, ..., 1 000 000] mètres
    
    /* - Ajout du prompt "environ" si la "distance réelle" est hors de la plage "distance arrondie" +/-5 % uniquement dans le cas 'SYNTH_DIST_DISTANT'
     *    => Pas d'ajout de "environ" si arrondie à 50 m
     *    => Arrondie à 100 m  => Distance "exacte" si "distance réelle" dans la plage ] "distance arrondie"   - 25 m, "distance arrondie" +   25 m]
     *    => Arrondie à   5 Km => Distance "exacte" si "distance réelle" dans la plage ] "distance arrondie" - 1250 m, "distance arrondie" + 1250 m]
     *    => Arrondie à  10 Km => Distance "exacte" si "distance réelle" dans la plage ] "distance arrondie" - 2500 m, "distance arrondie" + 2500 m]
     */
    uint32_t l__dist_range = 0L;

    if (l__dist_rounded <= DISTANCE_1KM) {
      l__flg_synth_about = false;
    }
    else if (l__dist_rounded <= DISTANCE_10KM) {
      l__dist_range = (DISTANCE_100M / 4);
    }
    else if (l__dist_rounded <= DISTANCE_100KM) {
      l__dist_range = (DISTANCE_5KM / 4);
    }
    else if (l__dist_rounded <= (DISTANCE_1000KM + DISTANCE_10KM)) {
      l__dist_range = (DISTANCE_10KM / 4);
    }

    if (l__dist_rounded > (i__distance - l__dist_range) && l__dist_rounded <= (i__distance + l__dist_range)) {
      l__flg_synth_about = false;
    }

#if 0
    printf("### buildCommandsPromptsDistance: l__dist_range [%d] -> l__flg_synth_about [%d]\n\n",
      l__dist_range, l__flg_synth_about);
#endif

    synthesisDistance(l__dist_rounded, &l__size, o__nbr_durations);

    if (l__flg_synth_about == true) {
      updateCommands(g__prompts_distance, BASE_APPROXIMATELY, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 
    }
  }
 
  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Sauvegarde de la distance arrondie et le mode synthétisés
  g__dist_rounded_previous = l__dist_rounded;
  g__dist_mode_previous    = i__dist_mode;

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for distance

// List of definitions and methods for cap synthesis
#define NUM_CAP_N                      1000
#define NUM_CAP_NNE                    (NUM_CAP_N + 22)
#define NUM_CAP_NE                     (NUM_CAP_N + 45)
#define NUM_CAP_ENE                    (NUM_CAP_N + 67)
#define NUM_CAP_E                      (NUM_CAP_N + 90)
#define NUM_CAP_ESE                    (NUM_CAP_E + 22)
#define NUM_CAP_SE                     (NUM_CAP_E + 45)
#define NUM_CAP_SSE                    (NUM_CAP_E + 67)
#define NUM_CAP_S                      (NUM_CAP_E + 90)
#define NUM_CAP_SSW                    (NUM_CAP_S + 22)
#define NUM_CAP_SW                     (NUM_CAP_S + 45)
#define NUM_CAP_WSW                    (NUM_CAP_S + 67)
#define NUM_CAP_W                      (NUM_CAP_S + 90)
#define NUM_CAP_WNW                    (NUM_CAP_W + 22)
#define NUM_CAP_NW                     (NUM_CAP_W + 45)
#define NUM_CAP_NNW                    (NUM_CAP_W + 67)

#define BASE_CAP_N                        0

#define BASE_LAST_PROMPT_CAP             16       // Values from 'North' to 'North'-'North'-'West'

static const ST_PROMPTS_DEF           g__prompts_cap[BASE_LAST_PROMPT_CAP + 1] =
{
  { BASE_CAP_N,       { 0x14, 0x10 + NUM_CAP_N / 256,   NUM_CAP_N % 256 },   "01/1000_au_nord.mp3", true, 0, 873L },
  { BASE_CAP_N + 1,   { 0x14, 0x10 + NUM_CAP_NNE / 256, NUM_CAP_NNE % 256 }, "01/1022_au_nord_nord_est.mp3", true, 1, 1539L },
  { BASE_CAP_N + 2,   { 0x14, 0x10 + NUM_CAP_NE / 256,  NUM_CAP_NE % 256 },  "01/1045_au_nord_est.mp3", true, 0, 1344L },
  { BASE_CAP_N + 3,   { 0x14, 0x10 + NUM_CAP_ENE / 256, NUM_CAP_ENE % 256 }, "01/1067_a_l_est_nord_est.mp3", true, 1, 1892L },
  { BASE_CAP_N + 4,   { 0x14, 0x10 + NUM_CAP_E / 256,   NUM_CAP_E % 256 },   "01/1090_a_l_est.mp3", true, 0, 1108L },
  { BASE_CAP_N + 5,   { 0x14, 0x10 + NUM_CAP_ESE / 256, NUM_CAP_ESE % 256 }, "01/1112_a_l_est_sud_est.mp3", true, 1, 1892L },
  { BASE_CAP_N + 6,   { 0x14, 0x10 + NUM_CAP_SE / 256,  NUM_CAP_SE % 256 },  "01/1135_au_sud_est.mp3", true, 0, 1500L },
  { BASE_CAP_N + 7,   { 0x14, 0x10 + NUM_CAP_SSE / 256, NUM_CAP_SSE % 256 }, "01/1157_au_sud_sud_est.mp3", true, 1, 1892L },
  { BASE_CAP_N + 8,   { 0x14, 0x10 + NUM_CAP_S / 256,   NUM_CAP_S % 256 },   "01/1180_au_sud.mp3", true, 0, 873L },
  { BASE_CAP_N + 9,   { 0x14, 0x10 + NUM_CAP_SSW / 256, NUM_CAP_SSW % 256 }, "01/1202_au_sud_sud_ouest.mp3", true, 1, 2010L },
  { BASE_CAP_N + 10,  { 0x14, 0x10 + NUM_CAP_SW / 256,  NUM_CAP_SW % 256 },  "01/1225_au_sud_ouest.mp3", true, 0, 1500L },
  { BASE_CAP_N + 11,  { 0x14, 0x10 + NUM_CAP_WSW / 256, NUM_CAP_WSW % 256 }, "01/1247_a_l_ouest_sud_ouest.mp3", true, 1, 2323L },
  { BASE_CAP_N + 12,  { 0x14, 0x10 + NUM_CAP_W / 256,   NUM_CAP_W % 256 },   "01/1270_a_l_ouest.mp3", true, 0, 1265L },
  { BASE_CAP_N + 13,  { 0x14, 0x10 + NUM_CAP_WNW / 256, NUM_CAP_WNW % 256 }, "01/1292_a_l_ouest_nord_ouest.mp3", true, 1, 2127L },
  { BASE_CAP_N + 14,  { 0x14, 0x10 + NUM_CAP_NW / 256,  NUM_CAP_NW % 256 },  "01/1315_au_nord_ouest.mp3", true, 0, 1382L },
  { BASE_CAP_N + 15,  { 0x14, 0x10 + NUM_CAP_NNW / 256, NUM_CAP_NNW % 256 }, "01/1337_au_nord_nord_ouest.mp3", true, 1, 1735L },

  { BASE_LAST_PROMPT_CAP, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

size_t buildCommandsPromptsCap(float i__cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t  l__size = 0;

  g__prompts_str = "[";

  /* Arrondi du cap
     => [0;11.25[       -> NUM_CAP_N   (0 degrees)
     => [11.25;32.75[   -> NUM_CAP_NNE (22 degrees)
     => [32.75;56.25[   -> NUM_CAP_NE  (45 degrees)
     => [56.25;78.75[   -> NUM_CAP_ENE (67 degrees)
     => [78.75;101.25[  -> NUM_CAP_E   (90 degrees)
     => [101.25;123.75[ -> NUM_CAP_ESE (112 degrees)
     => [123.75;146.25[ -> NUM_CAP_SE  (135 degrees)
     => [146.25;168.75[ -> NUM_CAP_SSE (157 degrees)
     => [168.75;191.25[ -> NUM_CAP_S   (180 degrees)
     => [191.25;213.75[ -> NUM_CAP_SSW (202 degrees)
     => [213.75;136.25[ -> NUM_CAP_SW  (225 degrees)
     => [236.25;258.75[ -> NUM_CAP_WSW (247 degrees)
     => [258.75;281.25[ -> NUM_CAP_W   (270 degrees)
     => [281.25;303.75[ -> NUM_CAP_WNW (292 degrees)
     => [303.75;326.25[ -> NUM_CAP_NW  (315 degrees)
     => [326.25;348.75[ -> NUM_CAP_NNW (337 degrees)
     => [348.75;360[    -> NUM_CAP_N   (0 degrees)
  */
  if (i__cap < 0.0 || i__cap > CAP_MAX) {
    updateCommandsWithOups(&l__size);
  }
  else {
    int l__index = -1;          // Invalid index (volontary)

    if (i__cap >= CAP_MIN && i__cap < THRESHOLD_CAP_NNE) {
      l__index = BASE_CAP_N;
    }
    else if (i__cap >= THRESHOLD_CAP_NNE && i__cap < THRESHOLD_CAP_NE) {
      l__index = (BASE_CAP_N + 1);
    }
    else if (i__cap >= THRESHOLD_CAP_NE  && i__cap < THRESHOLD_CAP_ENE) {
      l__index = (BASE_CAP_N + 2);
    }
    else if (i__cap >= THRESHOLD_CAP_ENE && i__cap < THRESHOLD_CAP_E) {
      l__index = (BASE_CAP_N + 3);
    }
    else if (i__cap >= THRESHOLD_CAP_E   && i__cap < THRESHOLD_CAP_ESE) {
      l__index = (BASE_CAP_N + 4);
    }
    else if (i__cap >= THRESHOLD_CAP_ESE && i__cap < THRESHOLD_CAP_SE) {
      l__index = (BASE_CAP_N + 5);
    }
    else if (i__cap >= THRESHOLD_CAP_SE  && i__cap < THRESHOLD_CAP_SSE) {
      l__index = (BASE_CAP_N + 6);
    }
    else if (i__cap >= THRESHOLD_CAP_SSE && i__cap < THRESHOLD_CAP_S) {
      l__index = (BASE_CAP_N + 7);
    }
    else if (i__cap >= THRESHOLD_CAP_S   && i__cap < THRESHOLD_CAP_SSW) {
      l__index = (BASE_CAP_N + 8);
    }
    else if (i__cap >= THRESHOLD_CAP_SSW && i__cap < THRESHOLD_CAP_SW) {
      l__index = (BASE_CAP_N + 9);
    }
    else if (i__cap >= THRESHOLD_CAP_SW  && i__cap < THRESHOLD_CAP_WSW) {
      l__index = (BASE_CAP_N + 10);
    }
    else if (i__cap >= THRESHOLD_CAP_WSW && i__cap < THRESHOLD_CAP_W) {
      l__index = (BASE_CAP_N + 11);
    }
    else if (i__cap >= THRESHOLD_CAP_W   && i__cap < THRESHOLD_CAP_WNW) {
      l__index = (BASE_CAP_N + 12);
    }
    else if (i__cap >= THRESHOLD_CAP_WNW && i__cap < THRESHOLD_CAP_NW) {
      l__index = (BASE_CAP_N + 13);
    }
    else if (i__cap >= THRESHOLD_CAP_NW  && i__cap < THRESHOLD_CAP_NNW) {
      l__index = (BASE_CAP_N + 14);
    }
    else if (i__cap >= THRESHOLD_CAP_NNW && i__cap < THRESHOLD_CAP_N) {
      l__index = (BASE_CAP_N + 15);
    }
    else if (i__cap >= THRESHOLD_CAP_N && i__cap <= CAP_MAX) {
      l__index = BASE_CAP_N;
    }

    if (l__index >= BASE_CAP_N && l__index <= (BASE_CAP_N + 15)) {
      updateCommands(g__prompts_cap, l__index, &l__size, BASE_LAST_PROMPT_CAP, o__nbr_durations); 
    }
    else {
      updateCommandsWithOups(&l__size);
    }
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for cap synthesis

// List of definitions and methods for cap synthesis in hours
#define NUM_CAP_MIDDAY                 0
#define NUM_CAP_13_HOURS               (NUM_CAP_MIDDAY + 1)
#define NUM_CAP_14_HOURS               (NUM_CAP_MIDDAY + 2)
#define NUM_CAP_15_HOURS               (NUM_CAP_MIDDAY + 3)
#define NUM_CAP_16_HOURS               (NUM_CAP_MIDDAY + 4)
#define NUM_CAP_17_HOURS               (NUM_CAP_MIDDAY + 5)
#define NUM_CAP_6_HOURS                (NUM_CAP_MIDDAY + 6)
#define NUM_CAP_7_HOURS                (NUM_CAP_MIDDAY + 7)
#define NUM_CAP_8_HOURS                (NUM_CAP_MIDDAY + 8)
#define NUM_CAP_9_HOURS                (NUM_CAP_MIDDAY + 9)
#define NUM_CAP_10_HOURS               (NUM_CAP_MIDDAY + 10)
#define NUM_CAP_11_HOURS               (NUM_CAP_MIDDAY + 11)

#define BASE_CAP_MIDDAY                0

#define BASE_LAST_PROMPT_CAP_HOURS     12       // Values from 'midday' to '11 hours'

static const ST_PROMPTS_DEF           g__prompts_cap_hours[BASE_LAST_PROMPT_CAP_HOURS + 1] =
{
  { BASE_CAP_MIDDAY,       { 0x14, 0x40 + NUM_CAP_MIDDAY / 256,   NUM_CAP_MIDDAY % 256 },   "04/0000_a_midi.mp3", true, 0, 873L },
  { BASE_CAP_MIDDAY + 1,   { 0x14, 0x40 + NUM_CAP_13_HOURS / 256, NUM_CAP_13_HOURS % 256 }, "04/0001_a_13_heures.mp3", true, 0, 1422L },
  { BASE_CAP_MIDDAY + 2,   { 0x14, 0x40 + NUM_CAP_14_HOURS / 256, NUM_CAP_14_HOURS % 256 }, "04/0002_a_14_heures.mp3", true, 1, 1657L },
  { BASE_CAP_MIDDAY + 3,   { 0x14, 0x40 + NUM_CAP_15_HOURS / 256, NUM_CAP_15_HOURS % 256 }, "04/0003_a_15_heures.mp3", true, 0, 1304L },
  { BASE_CAP_MIDDAY + 4,   { 0x14, 0x40 + NUM_CAP_16_HOURS / 256, NUM_CAP_16_HOURS % 256 }, "04/0004_a_16_heures.mp3", true, 0, 1382L },
  { BASE_CAP_MIDDAY + 5,   { 0x14, 0x40 + NUM_CAP_17_HOURS / 256, NUM_CAP_17_HOURS % 256 }, "04/0005_a_17_heures.mp3", true, 1, 1539L },
  { BASE_CAP_MIDDAY + 6,   { 0x14, 0x40 + NUM_CAP_6_HOURS / 256,  NUM_CAP_6_HOURS % 256 },  "04/0006_a_6_heures.mp3", true, 0, 1461L },
  { BASE_CAP_MIDDAY + 7,   { 0x14, 0x40 + NUM_CAP_7_HOURS / 256,  NUM_CAP_7_HOURS % 256 },  "04/0007_a_7_heures.mp3", true, 0, 1265L },
  { BASE_CAP_MIDDAY + 8,   { 0x14, 0x40 + NUM_CAP_8_HOURS / 256,  NUM_CAP_8_HOURS % 256 },  "04/0008_a_8_heures.mp3", true, 0, 1226L },
  { BASE_CAP_MIDDAY + 9,   { 0x14, 0x40 + NUM_CAP_9_HOURS / 256,  NUM_CAP_9_HOURS % 256 },  "04/0009_a_9_heures.mp3", true, 0, 1108L },
  { BASE_CAP_MIDDAY + 10,  { 0x14, 0x40 + NUM_CAP_10_HOURS / 256, NUM_CAP_10_HOURS % 256 }, "04/0010_a_10_heures.mp3", true, 0, 1226L },
  { BASE_CAP_MIDDAY + 11,  { 0x14, 0x40 + NUM_CAP_11_HOURS / 256, NUM_CAP_11_HOURS % 256 }, "04/0011_a_11_heures.mp3", true, 0, 1187L },

  { BASE_LAST_PROMPT_CAP_HOURS, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

size_t buildCommandsPromptsCapHours(float i__cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t  l__size = 0;

  g__prompts_str = "[";

  /* Arrondi du cap
     => [0;15[    -> NUM_CAP_MIDDAY (0 degrees)
     => [15;45[   -> NUM_CAP_13H    (30 degrees)
     => [45;75[   -> NUM_CAP_14H    (60 degrees)
     => [75;105[  -> NUM_CAP_15H    (90 degrees)
     => [105;135[ -> NUM_CAP_16H    (120 degrees)
     => [135;165[ -> NUM_CAP_17H    (150 degrees)
     => [165;195[ -> NUM_CAP_6H     (180 degrees)
     => [195;225[ -> NUM_CAP_7H     (210 degrees)
     => [225;255[ -> NUM_CAP_8H     (240 degrees)
     => [255;285[ -> NUM_CAP_9H     (270 degrees)
     => [285;345[ -> NUM_CAP_10H    (300 degrees)
     => [345;360[ -> NUM_CAP_11H    (330 degrees)
  */
  if (i__cap < 0.0 || i__cap > CAP_MAX) {
    updateCommandsWithOups(&l__size);
  }
  else {
    int l__index = -1;          // Invalid index (volontary)

    if (i__cap >= CAP_MIN && i__cap < THRESHOLD_CAP_13H) {
      l__index = BASE_CAP_MIDDAY;
    }
    else if (i__cap >= THRESHOLD_CAP_13H && i__cap < THRESHOLD_CAP_14H) {
      l__index = (BASE_CAP_MIDDAY + 1);
    }
    else if (i__cap >= THRESHOLD_CAP_14H  && i__cap < THRESHOLD_CAP_15H) {
      l__index = (BASE_CAP_MIDDAY + 2);
    }
    else if (i__cap >= THRESHOLD_CAP_15H && i__cap < THRESHOLD_CAP_16H) {
      l__index = (BASE_CAP_MIDDAY + 3);
    }
    else if (i__cap >= THRESHOLD_CAP_16H   && i__cap < THRESHOLD_CAP_17H) {
      l__index = (BASE_CAP_MIDDAY + 4);
    }
    else if (i__cap >= THRESHOLD_CAP_17H && i__cap < THRESHOLD_CAP_6H) {
      l__index = (BASE_CAP_MIDDAY + 5);
    }
    else if (i__cap >= THRESHOLD_CAP_6H  && i__cap < THRESHOLD_CAP_7H) {
      l__index = (BASE_CAP_MIDDAY + 6);
    }
    else if (i__cap >= THRESHOLD_CAP_7H && i__cap < THRESHOLD_CAP_8H) {
      l__index = (BASE_CAP_MIDDAY + 7);
    }
    else if (i__cap >= THRESHOLD_CAP_8H   && i__cap < THRESHOLD_CAP_9H) {
      l__index = (BASE_CAP_MIDDAY + 8);
    }
    else if (i__cap >= THRESHOLD_CAP_9H && i__cap < THRESHOLD_CAP_10H) {
      l__index = (BASE_CAP_MIDDAY + 9);
    }
    else if (i__cap >= THRESHOLD_CAP_10H  && i__cap < THRESHOLD_CAP_11H) {
      l__index = (BASE_CAP_MIDDAY + 10);
    }
    else if (i__cap >= THRESHOLD_CAP_11H && i__cap < THRESHOLD_CAP_MIDDAY) {
      l__index = (BASE_CAP_MIDDAY + 11);
    }
    else if (i__cap >= THRESHOLD_CAP_MIDDAY && i__cap <= CAP_MAX) {
      l__index = BASE_CAP_MIDDAY;
    }

    if (l__index >= BASE_CAP_MIDDAY && l__index <= (BASE_CAP_MIDDAY + 12)) {
      updateCommands(g__prompts_cap_hours, l__index, &l__size, BASE_LAST_PROMPT_CAP_HOURS, o__nbr_durations); 
    }
    else {
      updateCommandsWithOups(&l__size);
    }
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for cap synthesis in hours

// List of definitions and methods for elevation synthesis
#define NUM_ELE_100                    4051
#define NUM_ELE_200                    (NUM_ELE_100  + 1)
#define NUM_ELE_300                    (NUM_ELE_200  + 1)
#define NUM_ELE_400                    (NUM_ELE_300  + 1)
#define NUM_ELE_500                    (NUM_ELE_400  + 1)
#define NUM_ELE_600                    (NUM_ELE_500  + 1)
#define NUM_ELE_700                    (NUM_ELE_600  + 1)
#define NUM_ELE_800                    (NUM_ELE_700  + 1)
#define NUM_ELE_900                    (NUM_ELE_800  + 1)
#define NUM_ELE_1000                   (NUM_ELE_900  + 1)
#define NUM_ELE_2000                   (NUM_ELE_1000 + 1)
#define NUM_ELE_3000                   (NUM_ELE_2000 + 1)
#define NUM_ELE_4000                   (NUM_ELE_3000 + 1)

#define NUM_AT_THE_ELE_OF              4010
#define NUM_AT_SEA_LEVEL               4011
#define NUM_FROM_SEA_LEVEL             4012
#define NUM_ELE_ABOVE                  4013
#define NUM_ELE_BELOW                  4014
#define NUM_ELE_LOWER                  4015
#define NUM_ELE_UPPER                  4016

#define BASE_ELE_5M                   0                         // 19 values in the range [5, 10, 15, ..., 90, 95] meters
#define BASE_ELE_100M                 (BASE_ELE_5M + 19)        //  9 values in the range [100, 200, 300, ..., 800, 900] meters
#define BASE_ELE_1000M                (BASE_ELE_100M + 9)       //  4 values in the range [1000, 2000, 3000, 4000] meters

#define BASE_ELE_100                  (BASE_ELE_1000M + 4)      //  9 values in the range [100, 200, 300, ..., 800, 900]
#define BASE_ELE_1000                 (BASE_ELE_100 + 9)        //  4 values in the range [1000, 2000, 3000, 4000]

#define BASE_AT_THE_ELE_OF            (BASE_ELE_1000 + 4)       // à l'altitude de
#define BASE_AT_SEA_LEVEL             (BASE_AT_THE_ELE_OF + 1)  // au niveau de la mer
#define BASE_FROM_SEA_LEVEL           (BASE_AT_SEA_LEVEL + 1)   // du niveau de la mer
#define BASE_ELE_ABOVE                (BASE_FROM_SEA_LEVEL + 1) // au dessus
#define BASE_ELE_BELOW                (BASE_ELE_ABOVE + 1)      // au dessous
#define BASE_ELE_LOWER                (BASE_ELE_BELOW + 1)      // plus bas
#define BASE_ELE_UPPER                (BASE_ELE_LOWER + 1)      // plus haut

#define BASE_LAST_PROMPT_ELEVATION    (BASE_ELE_UPPER + 1)

static const ST_PROMPTS_DEF           g__prompts_elevation[BASE_LAST_PROMPT_ELEVATION + 1] =
{
  { BASE_ELE_5M,      { 0x14, 0x50, 5 },  "05/0005_5_metres.mp3", true, 0, 1304L },
  { BASE_ELE_5M + 1,  { 0x14, 0x50, 10 }, "05/0010_10_metres.mp3", true, 0, 1148L },
  { BASE_ELE_5M + 2,  { 0x14, 0x50, 15 }, "05/0015_15_metres.mp3", true, 0, 1187L },
  { BASE_ELE_5M + 3,  { 0x14, 0x50, 20 }, "05/0020_20_metres.mp3", true, 0, 1148L },
  { BASE_ELE_5M + 4,  { 0x14, 0x50, 25 }, "05/0025_25_metres.mp3", true, 1, 1735L },
  { BASE_ELE_5M + 5,  { 0x14, 0x50, 30 }, "05/0030_30_metres.mp3", true, 0, 1344L },
  { BASE_ELE_5M + 6,  { 0x14, 0x50, 35 }, "05/0035_35_metres.mp3", true, 1, 1814L },
  { BASE_ELE_5M + 7,  { 0x14, 0x50, 40 }, "05/0040_40_metres.mp3", true, 0, 1461L },
  { BASE_ELE_5M + 8,  { 0x14, 0x50, 45 }, "05/0045_45_metres.mp3", true, 1, 1814L },
  { BASE_ELE_5M + 9,  { 0x14, 0x50, 50 }, "05/0050_50_metres.mp3", true, 1, 1696L },
  { BASE_ELE_5M + 10, { 0x14, 0x50, 55 }, "05/0055_55_metres.mp3", true, 1, 1970L },
  { BASE_ELE_5M + 11, { 0x14, 0x50, 60 }, "05/0060_60_metres.mp3", true, 1, 1696L },
  { BASE_ELE_5M + 12, { 0x14, 0x50, 65 }, "05/0065_65_metres.mp3", true, 1, 2010L },
  { BASE_ELE_5M + 13, { 0x14, 0x50, 70 }, "05/0070_70_metres.mp3", true, 1, 1892L },
  { BASE_ELE_5M + 14, { 0x14, 0x50, 75 }, "05/0075_75_metres.mp3", true, 1, 2166L },
  { BASE_ELE_5M + 15, { 0x14, 0x50, 80 }, "05/0080_80_metres.mp3", true, 0, 1461L },
  { BASE_ELE_5M + 16, { 0x14, 0x50, 85 }, "05/0085_85_metres.mp3", true, 1, 1970L },
  { BASE_ELE_5M + 17, { 0x14, 0x50, 90 }, "05/0090_90_metres.mp3", true, 1, 1814L },
  { BASE_ELE_5M + 18, { 0x14, 0x50, 95 }, "05/0095_95_metres.mp3", true, 1, 2010L },

  { BASE_ELE_100M,     { 0x14, 0x50 + 100 / 256, 100 % 256 }, "05/0100_100_metres.mp3", true, 0, 1265L },
  { BASE_ELE_100M + 1, { 0x14, 0x50 + 200 / 256, 200 % 256 }, "05/0200_200_metres.mp3", true, 0, 1500L },
  { BASE_ELE_100M + 2, { 0x14, 0x50 + 300 / 256, 300 % 256 }, "05/0300_300_metres.mp3", true, 0, 1461L },
  { BASE_ELE_100M + 3, { 0x14, 0x50 + 400 / 256, 400 % 256 }, "05/0400_400_metres.mp3", true, 1, 1578L },
  { BASE_ELE_100M + 4, { 0x14, 0x50 + 500 / 256, 500 % 256 }, "05/0500_500_metres.mp3", true, 1, 1618L },
  { BASE_ELE_100M + 5, { 0x14, 0x50 + 600 / 256, 600 % 256 }, "05/0600_600_metres.mp3", true, 0, 1461L },
  { BASE_ELE_100M + 6, { 0x14, 0x50 + 700 / 256, 700 % 256 }, "05/0700_700_metres.mp3", true, 1, 1657L },
  { BASE_ELE_100M + 7, { 0x14, 0x50 + 800 / 256, 800 % 256 }, "05/0800_800_metres.mp3", true, 0, 1422L },
  { BASE_ELE_100M + 8, { 0x14, 0x50 + 900 / 256, 900 % 256 }, "05/0900_900_metres.mp3", true, 1, 1696L },

  { BASE_ELE_1000M,     { 0x14, 0x50 + 1000 / 256, 1000 % 256 }, "05/1000_1000_metres.mp3", true, 0, 1187L },
  { BASE_ELE_1000M + 1, { 0x14, 0x50 + 2000 / 256, 2000 % 256 }, "05/2000_2000_metres.mp3", true, 0, 1461L },
  { BASE_ELE_1000M + 2, { 0x14, 0x50 + 3000 / 256, 3000 % 256 }, "05/3000_3000_metres.mp3", true, 0, 1461L },
  { BASE_ELE_1000M + 3, { 0x14, 0x50 + 4000 / 256, 4000 % 256 }, "05/4000_4000_metres.mp3", true, 1, 1539L },

  { BASE_ELE_100,       { 0x14, 0x50 + NUM_ELE_100 / 256, NUM_ELE_100 % 256 }, "05/4051_100.mp3", true, 0, 638L },
  { BASE_ELE_100 + 1,   { 0x14, 0x50 + NUM_ELE_200 / 256, NUM_ELE_200 % 256 }, "05/4052_200.mp3", true, 0, 912L },
  { BASE_ELE_100 + 2,   { 0x14, 0x50 + NUM_ELE_300 / 256, NUM_ELE_300 % 256 }, "05/4053_300.mp3", true, 0, 873L },
  { BASE_ELE_100 + 3,   { 0x14, 0x50 + NUM_ELE_400 / 256, NUM_ELE_400 % 256 }, "05/4054_400.mp3", true, 0, 1069L },
  { BASE_ELE_100 + 4,   { 0x14, 0x50 + NUM_ELE_500 / 256, NUM_ELE_500 % 256 }, "05/4055_500.mp3", true, 0, 1069L },
  { BASE_ELE_100 + 5,   { 0x14, 0x50 + NUM_ELE_600 / 256, NUM_ELE_600 % 256 }, "05/4056_600.mp3", true, 0, 1030L },
  { BASE_ELE_100 + 6,   { 0x14, 0x50 + NUM_ELE_700 / 256, NUM_ELE_700 % 256 }, "05/4057_700.mp3", true, 0, 1030L },
  { BASE_ELE_100 + 7,   { 0x14, 0x50 + NUM_ELE_800 / 256, NUM_ELE_800 % 256 }, "05/4058_800.mp3", true, 0, 912L },
  { BASE_ELE_100 + 8,   { 0x14, 0x50 + NUM_ELE_900 / 256, NUM_ELE_900 % 256 }, "05/4059_900.mp3", true, 0, 991L },

  { BASE_ELE_1000,      { 0x14, 0x50 + NUM_ELE_1000 / 256, NUM_ELE_1000 % 256 }, "05/4060_1000.mp3", true, 0, 716L },
  { BASE_ELE_1000 + 1,  { 0x14, 0x50 + NUM_ELE_2000 / 256, NUM_ELE_2000 % 256 }, "05/4061_2000.mp3", true, 0, 1069L },
  { BASE_ELE_1000 + 2,  { 0x14, 0x50 + NUM_ELE_3000 / 256, NUM_ELE_3000 % 256 }, "05/4062_3000.mp3", true, 0, 1030L },
  { BASE_ELE_1000 + 3,  { 0x14, 0x50 + NUM_ELE_4000 / 256, NUM_ELE_4000 % 256 }, "05/4063_4000.mp3", true, 0, 1148L },

  { BASE_AT_THE_ELE_OF,  { 0x14, 0x50 + NUM_AT_THE_ELE_OF / 256,  NUM_AT_THE_ELE_OF % 256 },  "05/4010_a_l_altitude_de.mp3", true, 1, 1657L },
  { BASE_AT_SEA_LEVEL,   { 0x14, 0x50 + NUM_AT_SEA_LEVEL / 256,   NUM_AT_SEA_LEVEL % 256 },   "05/4011_au_niveau_de_la_mer.mp3", true, 1, 1814L },
  { BASE_FROM_SEA_LEVEL, { 0x14, 0x50 + NUM_FROM_SEA_LEVEL / 256, NUM_FROM_SEA_LEVEL % 256 }, "05/4012_du_niveau_de_la_mer.mp3", true, 1, 1931L },
  { BASE_ELE_ABOVE,      { 0x14, 0x50 + NUM_ELE_ABOVE / 256,      NUM_ELE_ABOVE % 256 },      "05/4013_au_dessus.mp3", true, 0, 991L },
  { BASE_ELE_BELOW,      { 0x14, 0x50 + NUM_ELE_BELOW / 256,      NUM_ELE_BELOW % 256 },      "05/4014_en_dessous.mp3", true, 0, 952L },
  { BASE_ELE_LOWER,      { 0x14, 0x50 + NUM_ELE_LOWER / 256,      NUM_ELE_LOWER % 256 },      "05/4015_plus_bas.mp3", true, 0, 795L },
  { BASE_ELE_UPPER,      { 0x14, 0x50 + NUM_ELE_UPPER / 256,      NUM_ELE_UPPER % 256 },      "05/4016_plus_haut.mp3", true, 0, 756L },

  { BASE_LAST_PROMPT_ELEVATION, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

static boolean synthesisElevationRound(int i__value, size_t *io__size, ENUM_SYNTH_ELE_MODES i__ele_mode, size_t *o__nbr_durations)
{
  int l__value = i__value;

  if (l__value < 0) {
#if 0
    // Valeur négative non attendue
    updateCommandsWithOups(io__size);
    return false;   // Pas de synthèse après
#else
    // Force l'altitude au niveau de la mer
    l__value = 0;
#endif
  }

  /*  Valeur synthétisée la plus proche des 5 mètres calculée au moyen de l'extraction des unités:
   *  [0;1;2]   -> Round == 10 * (Value / 10) + 0
   *  [3;4;5;6] -> Round == 10 * (Value / 10) + 5
   *  [7;8;9]   -> Round == 10 * (Value / 10) + 10
  */
  int l__value_round = 0;
  int l__unit = (l__value % 10);
  switch (l__unit) {
  case 0:
  case 1:
  case 2:
    l__value_round = 10 * (l__value / 10);
    break;

  case 3:
  case 4:
  case 5:
  case 6:
    l__value_round = 10 * (l__value / 10) + ELEVATION_ROUND_FIX;
    break;

  case 7:
  case 8:
  case 9:
    l__value_round = 10 * (l__value / 10) + 2 * ELEVATION_ROUND_FIX;
    break;

  default:
    break;
  }

  char l__buffer[80];
  sprintf(l__buffer, "synthesisEle: Mode [%d] Value [%d] -> Round [%d]\n", i__ele_mode, l__value, l__value_round);
  Serial.print(l__buffer);

  // Test de la limite supérieure après arrondi
  if (l__value_round > ELEVATION_MAX) {
    updateCommandsWithOups(io__size);
    return false;   // Pas de synthèse après
  }

  /*  Synthèse "au niveau de la mer" dans le cas 'SYNTH_ELE_ABSOLUTE'
   *  => Ne doit pas arriver dans le cas 'SYNTH_ELE_RELATIVE' car |diff| >= 10 mètres dans ce cas
   */
  if (l__value_round == 0) {
    if (i__ele_mode == SYNTH_ELE_ABSOLUTE) {
      updateCommands(g__prompts_elevation, BASE_AT_SEA_LEVEL, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
      return false;
    }
    else {
      updateCommandsWithOups(io__size);
      return false;   // Pas de synthèse après      
    }
  }

  updateCommands(g__prompts_elevation, BASE_AT_THE_ELE_OF, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);

  // Extractions for synthesis
  int l__thousands = (l__value_round / 1000);
  int l__hundreds  = (l__value_round % 1000);
  if (l__value_round >= 1000 && l__hundreds == 0) {
    // Valeur exacte [1000, 2000, 3000, 4000] mètres ?
    updateCommands(g__prompts_elevation, BASE_ELE_1000M + l__thousands - 1, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
    return true;    // Synthèse éventuelle d'une suite
  }

  if (l__value_round >= 1000) {
    updateCommands(g__prompts_elevation, BASE_ELE_1000 + l__thousands - 1, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
  }

  int l__tens = (l__hundreds % 100);
  int l__idx  = (l__hundreds / 100);
  if (l__value_round >= 100 && l__tens == 0) {
     // Valeur exacte [100, 200, 300, 400, 500, 600, 700, 800, 900] mètres ?
     updateCommands(g__prompts_elevation, BASE_ELE_100M + l__idx - 1, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
     return true;    // Synthèse éventuelle d'une suite
  }

  if (l__value_round >= 100) {
    updateCommands(g__prompts_elevation, BASE_ELE_100 + l__idx - 1, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
  }

  if (l__value_round >= 5) {
    l__idx = (l__tens / 5);
    updateCommands(g__prompts_elevation, BASE_ELE_5M + l__idx - 1, io__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
  }

  return true;    // Synthèse éventuelle d'une suite
}

/*  Synthèse de l'altitude:
 *  - Si 'i__ele_mode' = SYNTH_NO_ELEVATION: Pas de synthèse
 *  - Si 'i__ele_mode' = SYNTH_ELE_ABSOLUTE: Synthèse de 'i__ele_current' (valeur positive attendue)
 *  - Si 'i__ele_mode' = SYNTH_ELE_RELATIVE: Synthèse de '(i__ele_ref - i__ele_current)' (valeur positive ou négative attendue)
 */
size_t buildCommandsPromptsElevation(
  float i__ele_current,
  float i__ele_ref,
  byte **o__command,
  ENUM_SYNTH_ELE_MODES i__ele_mode,
  uint16_t **o__durations,
  size_t *o__nbr_durations)
{
  if (i__ele_mode == SYNTH_NO_ELEVATION) {
    return 0;
  }

  size_t              l__size = 0;
  ENUM_SYNTH_ELE_SIGN l__ele_sign = SYNTH_ELEVATION_NONE;

  g__prompts_str = "[";

  if (i__ele_mode == SYNTH_ELE_ABSOLUTE) {
    int l__ele = (int)i__ele_current;

    synthesisElevationRound(l__ele, &l__size, i__ele_mode, o__nbr_durations);
  }
  else if (i__ele_mode == SYNTH_ELE_RELATIVE) {
    int l__ele_diff = (int)(i__ele_ref - i__ele_current);

    if (l__ele_diff < 0) {
      l__ele_diff = -l__ele_diff;

      //l__ele_sign = SYNTH_ELE_BELOW;
      l__ele_sign = SYNTH_ELE_LOWER;
    }
    else {
      //l__ele_sign = SYNTH_ELE_ABOVE;
      l__ele_sign = SYNTH_ELE_UPPER;
    }

    if (l__ele_diff > ELEVATION_ROUND_FIX) {
      boolean l__flr_rtn = synthesisElevationRound(l__ele_diff, &l__size, i__ele_mode, o__nbr_durations);

      if (l__flr_rtn == true) {
        switch (l__ele_sign) {
        case SYNTH_ELE_BELOW:
          updateCommands(g__prompts_elevation, BASE_ELE_BELOW, &l__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
          break;
        case SYNTH_ELE_ABOVE:
          updateCommands(g__prompts_elevation, BASE_ELE_ABOVE, &l__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
          break;
        case SYNTH_ELE_LOWER:
          updateCommands(g__prompts_elevation, BASE_ELE_LOWER, &l__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
          break;
        case SYNTH_ELE_UPPER:
          updateCommands(g__prompts_elevation, BASE_ELE_UPPER, &l__size, BASE_LAST_PROMPT_ELEVATION, o__nbr_durations);
          break;
        default:
          break;
        }
      }
    }
    else {
      // Warning: La difference n'est pas nulle dans le mode 'relatif'
      updateCommandsWithOups(&l__size);
    }
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for elevation synthesis

// List of definitions and methods for distance to plots synthesis
#define NUM_DIST_TO_PLOT_METER         2000            // @ Dossier 02

#define NUM_THE_PLOT_IS_AT             2001                 // Le trace est a... @ Dossier 06
#define NUM_YOU_ARE_BACK_ON_THE_PLOT   2002           // Vous etes de nouveau sur le trace @ Dossier 06
#define NUM_YOU_ARE_ON_THE_PLOT        2003           // Vous etes sur le trace @ Dossier 06
#define NUM_YOU_ARE_STILL_ON_THE_PLOT  2004           // Vous etes toujours sur le trace @ Dossier 06
#define NUM_THE_PLOT_IS_ALWAYS_AT      2005           // Le trace est toujours a... @ Dossier 06
#define NUM_PREPARE_YOURSELVES         2006
#define NUM_BRANCH_OF_NOW              2007
#define NUM_CONTINUE_ON                2008
#define NUM_AND_CONTINUE_ON            2009
#define NUM_CONTINUE_TO_MEANDER        2010

#define BASE_DIST_TO_PLOT_1            0                    // 36 values in the range [1, 2, ..., 900] with absence of certain values @ Dossier 02
#define BASE_DIST_TO_PLOT_METER        (BASE_DIST_TO_PLOT_1 + 37)

#define BASE_THE_PLOT_IS_AT            (BASE_DIST_TO_PLOT_METER + 1)
#define BASE_YOU_ARE_BACK_ON_THE_PLOT  (BASE_DIST_TO_PLOT_METER + 2)
#define BASE_YOU_ARE_ON_THE_PLOT       (BASE_DIST_TO_PLOT_METER + 3)
#define BASE_YOU_ARE_STILL_ON_THE_PLOT (BASE_DIST_TO_PLOT_METER + 4)
#define BASE_THE_PLOT_IS_ALWAYS_AT     (BASE_DIST_TO_PLOT_METER + 5)
#define BASE_PREPARE_YOURSELVES        (BASE_DIST_TO_PLOT_METER + 6)
#define BASE_BRANCH_OF_NOW             (BASE_DIST_TO_PLOT_METER + 7)
#define BASE_CONTINUE_ON               (BASE_DIST_TO_PLOT_METER + 8)
#define BASE_AND_CONTINUE_ON           (BASE_DIST_TO_PLOT_METER + 9)
#define BASE_CONTINUE_TO_MEANDER       (BASE_DIST_TO_PLOT_METER + 10)

#define BASE_LAST_PROMPT_DIST_TO_PLOT  (BASE_CONTINUE_TO_MEANDER + 1)

static const ST_PROMPTS_DEF           g__prompts_dist_to_plot[BASE_LAST_PROMPT_DIST_TO_PLOT + 1] =
{
  { BASE_DIST_TO_PLOT_1,      { 0x14, 0x20, 0x01 }, "02/0001_1.mp3", true, 0, 442L },
  { BASE_DIST_TO_PLOT_1 + 1,  { 0x14, 0x20, 0x02 }, "02/0002_2.mp3", true, 0, 520L },
  { BASE_DIST_TO_PLOT_1 + 2,  { 0x14, 0x20, 0x03 }, "02/0003_3.mp3", true, 0, 677L },
  { BASE_DIST_TO_PLOT_1 + 3,  { 0x14, 0x20, 0x04 }, "02/0004_4.mp3", true, 0, 795L },
  { BASE_DIST_TO_PLOT_1 + 4,  { 0x14, 0x20, 0x05 }, "02/0005_5.mp3", true, 0, 599L },
  { BASE_DIST_TO_PLOT_1 + 5,  { 0x14, 0x20, 0x06 }, "02/0006_6.mp3", true, 0, 952L },
  { BASE_DIST_TO_PLOT_1 + 6,  { 0x14, 0x20, 0x07 }, "02/0007_7.mp3", true, 0, 912L },
  { BASE_DIST_TO_PLOT_1 + 7,  { 0x14, 0x20, 0x08 }, "02/0008_8.mp3", true, 0, 873L },
  { BASE_DIST_TO_PLOT_1 + 8,  { 0x14, 0x20, 0x09 }, "02/0009_9.mp3", true, 0, 834L },
  { BASE_DIST_TO_PLOT_1 + 9,  { 0x14, 0x20, 0x0a }, "02/0010_10.mp3", true, 0, 952L },
  { BASE_DIST_TO_PLOT_1 + 10, { 0x14, 0x20, 0x0f }, "02/0015_15.mp3", true, 0, 795L },
  { BASE_DIST_TO_PLOT_1 + 11, { 0x14, 0x20, 0x14 }, "02/0020_20.mp3", true, 0, 599L },
  { BASE_DIST_TO_PLOT_1 + 12, { 0x14, 0x20, 0x19 }, "02/0025_25.mp3", true, 0, 1226L },
  { BASE_DIST_TO_PLOT_1 + 13, { 0x14, 0x20, 0x1e }, "02/0030_30.mp3", true, 0, 795L },
  { BASE_DIST_TO_PLOT_1 + 14, { 0x14, 0x20, 0x23 }, "02/0035_35.mp3", true, 0, 1187L },
  { BASE_DIST_TO_PLOT_1 + 15, { 0x14, 0x20, 0x28 }, "02/0040_40.mp3", true, 0, 991L },
  { BASE_DIST_TO_PLOT_1 + 16, { 0x14, 0x20, 0x2d }, "02/0045_45.mp3", true, 0, 1382L },
  { BASE_DIST_TO_PLOT_1 + 17, { 0x14, 0x20, 0x32 }, "02/0050_50.mp3", true, 0, 1226L },
  { BASE_DIST_TO_PLOT_1 + 18, { 0x14, 0x20, 0x37 }, "02/0055_55.mp3", true, 1, 1539L },
  { BASE_DIST_TO_PLOT_1 + 19, { 0x14, 0x20, 0x3c }, "02/0060_60.mp3", true, 0, 1265L },
  { BASE_DIST_TO_PLOT_1 + 20, { 0x14, 0x20, 0x41 }, "02/0065_65.mp3", true, 1, 1539L },
  { BASE_DIST_TO_PLOT_1 + 21, { 0x14, 0x20, 0x46 }, "02/0070_70.mp3", true, 1, 1774L },
  { BASE_DIST_TO_PLOT_1 + 22, { 0x14, 0x20, 0x4b }, "02/0075_75.mp3", true, 1, 1539L },
  { BASE_DIST_TO_PLOT_1 + 23, { 0x14, 0x20, 0x50 }, "02/0080_80.mp3", true, 0, 1030L },
  { BASE_DIST_TO_PLOT_1 + 24, { 0x14, 0x20, 0x55 }, "02/0085_85.mp3", true, 0, 1461L },
  { BASE_DIST_TO_PLOT_1 + 25, { 0x14, 0x20, 0x5a }, "02/0090_90.mp3", true, 1, 1539L },
  { BASE_DIST_TO_PLOT_1 + 26, { 0x14, 0x20, 0x5f }, "02/0095_95.mp3", true, 0, 1500L },
  { BASE_DIST_TO_PLOT_1 + 27, { 0x14, 0x20 + 100 / 256, 100 % 256 },   "02/0100_100.mp3", true, 0, 638L },
  { BASE_DIST_TO_PLOT_1 + 28, { 0x14, 0x20 + 200 / 256, 200 % 256 },   "02/0200_200.mp3", true, 0, 873L },
  { BASE_DIST_TO_PLOT_1 + 29, { 0x14, 0x20 + 300 / 256, 300 % 256 },   "02/0300_300.mp3", true, 0, 912L },
  { BASE_DIST_TO_PLOT_1 + 30, { 0x14, 0x20 + 400 / 256, 400 % 256 },   "02/0400_400.mp3", true, 0, 1069L },
  { BASE_DIST_TO_PLOT_1 + 31, { 0x14, 0x20 + 500 / 256, 500 % 256 },   "02/0500_500.mp3", true, 0, 1030L },
  { BASE_DIST_TO_PLOT_1 + 32, { 0x14, 0x20 + 600 / 256, 600 % 256 },   "02/0600_600.mp3", true, 0, 952L },
  { BASE_DIST_TO_PLOT_1 + 33, { 0x14, 0x20 + 700 / 256, 700 % 256 },   "02/0700_700.mp3", true, 0, 1108L },
  { BASE_DIST_TO_PLOT_1 + 34, { 0x14, 0x20 + 800 / 256, 800 % 256 },   "02/0800_800.mp3", true, 0, 952L },
  { BASE_DIST_TO_PLOT_1 + 35, { 0x14, 0x20 + 900 / 256, 900 % 256 },   "02/0900_900.mp3", true, 0, 991L },
  { BASE_DIST_TO_PLOT_1 + 36, { 0x14, 0x20 + 1000 / 256, 1000 % 256 }, "02/1000_1000.mp3", true, 0, 716L },

  { BASE_DIST_TO_PLOT_METER, { 0x14, 0x20 + NUM_DIST_TO_PLOT_METER / 256, NUM_DIST_TO_PLOT_METER % 256 },     "02/2000_metres.mp3", true, 0, 834L },

  { BASE_THE_PLOT_IS_AT,            { 0x14, 0x60 + NUM_THE_PLOT_IS_AT / 256, NUM_THE_PLOT_IS_AT % 256 },                       "06/2001_le_trace_est_a.mp3", true, 1, 1618L },
  { BASE_YOU_ARE_BACK_ON_THE_PLOT,  { 0x14, 0x60 + NUM_YOU_ARE_BACK_ON_THE_PLOT / 256, NUM_YOU_ARE_BACK_ON_THE_PLOT % 256 },   "06/2002_vous_etes_de_nouveau_sur_le_trace.mp3", true, 2, 2832L },
  { BASE_YOU_ARE_ON_THE_PLOT,       { 0x14, 0x60 + NUM_YOU_ARE_ON_THE_PLOT / 256, NUM_YOU_ARE_ON_THE_PLOT % 256 },             "06/2003_vous_etes_sur_le_trace.mp3", true, 2, 2088L },
  { BASE_YOU_ARE_STILL_ON_THE_PLOT, { 0x14, 0x60 + NUM_YOU_ARE_STILL_ON_THE_PLOT / 256, NUM_YOU_ARE_STILL_ON_THE_PLOT % 256 }, "06/2004_vous_etes_toujours_sur_le_trace.mp3", true, 2, 2715L },
  { BASE_THE_PLOT_IS_ALWAYS_AT,     { 0x14, 0x60 + NUM_THE_PLOT_IS_ALWAYS_AT / 256, NUM_THE_PLOT_IS_ALWAYS_AT % 256 },         "06/2005_le_trace_est_toujours_a.mp3", true, 2, 1800L },
  { BASE_PREPARE_YOURSELVES,        { 0x14, 0x60 + NUM_PREPARE_YOURSELVES / 256, NUM_PREPARE_YOURSELVES % 256 },               "06/2006_preparez_vous_a_bifurquer_a.mp3", true, 2, 2558L },
  { BASE_BRANCH_OF_NOW,             { 0x14, 0x60 + NUM_BRANCH_OF_NOW / 256, NUM_BRANCH_OF_NOW % 256 },                         "06/2007_bifurquez_maintenant.mp3", true, 1, 1853L },
  { BASE_CONTINUE_ON,               { 0x14, 0x60 + NUM_CONTINUE_ON / 256, NUM_CONTINUE_ON % 256 },                             "06/2008_continuez_sur.mp3", true, 1, 1735L },
  { BASE_AND_CONTINUE_ON,           { 0x14, 0x60 + NUM_AND_CONTINUE_ON / 256, NUM_AND_CONTINUE_ON % 256 },                     "06/2009_et_continuez_sur.mp3", true, 1, 1892L },
  { BASE_CONTINUE_TO_MEANDER,       { 0x14, 0x60 + NUM_CONTINUE_TO_MEANDER / 256, NUM_CONTINUE_TO_MEANDER % 256 },             "06/2010_continuez_a_serpenter_sur.mp3", true, 2, 2676L },

  { BASE_LAST_PROMPT_DIST_TO_PLOT, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

// Pour préciser "toujours" si la distance est la meme que precedement synthetisee
static boolean                  g__flg_one_or_more_times_on_the_plot = false;   // Init à "jamais sur le tracé"
static uint32_t                 g__dist_to_plots_rounded_previous       = (uint32_t)-1L;
static uint32_t                 g__dist_to_plots_rounded_previous_synth = (uint32_t)-1L;

// Pour la synthèse "asynchrone"
boolean                         g__flg_inh_synth_dist_to_plot = false;          // Init pas d'inhibition synthèse distance au tracé

uint32_t distanceRoundedForThePlot(uint32_t i__distance)
{
  uint32_t l__distance_rounded = i__distance;

  if (i__distance <= (DISTANCE_1KM + 2)) {
    // Distance arrondie à 5 M
    uint32_t l__dist_mul_10 = (i__distance / 10L);
    uint32_t l__dist_mod_10 = (i__distance % 10L);

    switch (l__dist_mod_10) {
    case 0:
    case 1:
    case 2:
      l__distance_rounded = 10L * l__dist_mul_10;
      break;

    case 3:
    case 4:
    case 5:
    case 6:
      l__distance_rounded = 10L * l__dist_mul_10 + 5L;
      break;

    default:    // 7, 8 or 9
      l__distance_rounded = 10L * l__dist_mul_10 + 10L;
      break;
    }
  }
  else if (i__distance <= DISTANCE_10KM) {
    // Distance arrondie à 100 M
    /* Distance <= 10 Km:   [1.1, 1.2, 1.3, ..., 9.8, 9.9, 10.0] Km (arrondi à 100 m)
     * Exemples: - 1245 m => l__dist_mul_100 = 12; l__pivot = 1300; l__diff = -55 => l__distance_rounded = 1200 m
     *           - 1680 m => l__dist_mul_100 = 16; l__pivot = 1700; l__diff = -20  => l__distance_rounded = 1700 m
     */
    uint32_t l__dist_mul_100 = (i__distance / 100L);
    uint32_t l__pivot = (100L * l__dist_mul_100 + 100L);
    int      l__diff  = (i__distance - l__pivot);
    if (l__diff < -DISTANCE_50M) {
      l__distance_rounded = l__pivot - DISTANCE_100M;
    }
    else if (l__diff < DISTANCE_50M) {
      l__distance_rounded = l__pivot;
    }
    else {
      l__distance_rounded = l__pivot + DISTANCE_100M;
    }
  }
  else if (i__distance <= DISTANCE_100KM) {
    /* Distance <= 100 Km:  [15, 20, 25, ..., 90, 95, 100] Km (arrondi à 5 Km)
     * Exemples: - 12456 m => l__dist_mul_10000 = 1; l__pivot = 15000; l__diff = -2544 => l__distance_rounded = 10 Km
     *           - 76100 m => l__dist_mul_10000 = 7; l__pivot = 75000; l__diff =  1100 => l__distance_rounded = 75 Km
     */
    uint32_t l__dist_mul_10000 = (i__distance / 10000L);
    uint32_t l__pivot = (10000L * l__dist_mul_10000 + 5000L);
    int      l__diff  = (i__distance - l__pivot);
    if (l__diff < -DISTANCE_2_5KM) {
      l__distance_rounded = l__pivot - DISTANCE_5KM;
    }
    else if (l__diff < DISTANCE_2_5KM) {
      l__distance_rounded = l__pivot;
    }
    else {
      l__distance_rounded = l__pivot + DISTANCE_5KM;
    }
  }
  else if (i__distance <= (DISTANCE_1000KM + DISTANCE_10KM)) {
    /*  Distance <= 1050 Km: [110, 120, ..., 980, 990, 1000, 1010] Km (arrondi à 10 Km)
     *  Exemples: - 124560 m => l__dist_mul_10000 = 12; l__pivot = 130000; l__diff =  -5440 => l__distance_rounded = 120 Km
     *            - 761000 m => l__dist_mul_10000 = 76; l__pivot = 770000; l__diff =  -9000 => l__distance_rounded = 750 Km
     *            - 970000 m => l__dist_mul_10000 = 97; l__pivot = 980000; l__diff = -10000 => l__distance_rounded = 970 Km
     *            - 118000 m => l__dist_mul_10000 = 11; l__pivot = 120000; l__diff =  -2000 => l__distance_rounded = 120 Km
     */
    uint32_t l__dist_mul_10000 = (i__distance / 10000L);
    uint32_t l__pivot = (10000L * l__dist_mul_10000 + 10000L);
    int      l__diff  = (i__distance - l__pivot);
    if (l__diff < -DISTANCE_5KM) {
      l__distance_rounded = l__pivot - DISTANCE_10KM;
    }
    else if (l__diff < DISTANCE_5KM) {
      l__distance_rounded = l__pivot;
    }
    else {
      l__distance_rounded = l__pivot - DISTANCE_10KM;
    }
  }
  else {
    // Distance très éloignée => Forçage à 1000 Km
    l__distance_rounded = DISTANCE_1000KM;
  }

  return l__distance_rounded;
}

/*  Synthèse d'une distance au [sur le] tracé
 *  => Arrondie à 5 M si distance < 1 Km
 *  => Repliemenet sur 'synthesisDistance()' si >= 1 Km
 */
void synthesisOfDistance(uint32_t i__distance, size_t *o__size, size_t *o__nbr_durations)
{
  if (i__distance < DISTANCE_1KM) {
    // Synthesis of 5, 10, 15, ..., 990, 995 and 1000 meters
    uint16_t l__hundred = (uint16_t)(i__distance / 100L);
    uint16_t l__rest    = (uint16_t)(i__distance % 100L);

#if 0
     printf("l__hundred [%d] l__rest [%d]\n", l__hundred, l__rest);
#endif

    if (l__hundred == 10 && l__rest == 0) {
      // 1000 meters
      updateCommands(g__prompts_dist_to_plot, BASE_DIST_TO_PLOT_1 + 36, o__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 
    }
    else if (l__hundred != 0) {
      // 1XX, 2XX, ..., 9XX
      updateCommands(g__prompts_dist_to_plot, BASE_DIST_TO_PLOT_1 + 26 + l__hundred, o__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 
    }

    if (l__rest == 5) {
      // 5
      updateCommands(g__prompts_dist_to_plot, BASE_DIST_TO_PLOT_1 + 4, o__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 
    }
    else if (l__rest != 0) {
      if ((l__rest % 5) == 0) {
        // 10, 15, ..., 95
        uint16_t l__offset = (l__rest - 10) / 5;
        updateCommands(g__prompts_dist_to_plot, BASE_DIST_TO_PLOT_1 + 9 + l__offset, o__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 
      }
      else {
        updateCommands(g__prompts_general, BASE_OUPS_MALE, o__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);
      }
    }
    // End: Synthesis of 5, 10, 15, ..., 990 and 995 meters

    updateCommands(g__prompts_dist_to_plot, BASE_DIST_TO_PLOT_METER, o__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);
  }
  else {
    // Distance >= 1 Km => Repliement sur la synthèse normale en fonction de la distance
    synthesisDistance(i__distance, o__size, o__nbr_durations);
  }
}

/* Synthèse de la distance au tracé inhibée par 'g__flg_inh_synth_dist_to_plot = true' dans les cas suivants:
 * - "Vous êtes [toujours|de nouveau] sur le tracé..."
 * - "Préparez vous à bifurquer..."
 * - "Bifurquez maintenant..."
 * - "Continuez sur..."
 * - "Le trace est à...
 */
size_t buildCommandsPromptsDistToPlots(ST_RESULTS *i__results, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations, boolean *o__flg_more_synth)
{
  size_t l__size = 0;

#if USE_ATTRACTOR_TO_PLOT
  uint32_t l__dist_rounded = distanceRoundedForThePlot(i__results->D_min_avg);    // Distance minimale au tracé moyennée
#else
  uint32_t l__dist_rounded = distanceRoundedForThePlot(i__results->D_min);        // Distance minimale au tracé instantanée
#endif

  boolean l__flg_same_dist_to_plot = (l__dist_rounded == g__dist_to_plots_rounded_previous_synth) ? true : false;

  // Recopie localement de l'inhibition globale
  boolean l__flg_inh_synth_dist_to_plot = g__flg_inh_synth_dist_to_plot;

#if 0
#ifdef USE_SIMULATION
#if USE_ATTRACTOR_TO_PLOT
  printf("[%d] M (D_min_avg) -> [%d] M\n", i__results->D_min_avg, l__dist_rounded);
#else
  printf("[%d] M (D_min) -> [%d] M\n", i__results->D_min, l__dist_rounded);
#endif
#endif
#endif

  g__prompts_str = "[";

  // Synthesis if rounded value <= DISTANCE_2_5KM
  if (l__dist_rounded <= DISTANCE_2_5KM) {
    if (l__dist_rounded <= DISTANCE_FOR_ON_THE_PLOT) {
      // Le tracé est rejoint...
      if (l__flg_same_dist_to_plot == true) {
        // Add 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1' pour synthèses plus fréquentes à l'approche de la bifurcation
        if (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2 || (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1 && i__results->distance <= DISTANCE_300M)) {
          l__flg_inh_synth_dist_to_plot = false;    // Effacement de la restriction locale
        }

        if (l__flg_inh_synth_dist_to_plot == false) {
        // Pas de synthèse de "Vous êtes toujours sur le tracé" car suivi d'une consigne du type "Continuez sur...", "Préparez vous..." ou "Bifurquez..."
#if USE_SYNTH_YOU_ARE_ON_THE_PLOT
          updateCommands(g__prompts_dist_to_plot, BASE_YOU_ARE_STILL_ON_THE_PLOT, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);
#endif
          *o__flg_more_synth = true;
        }
      }
      else {
        if (g__flg_one_or_more_times_on_the_plot == true && g__dist_to_plots_rounded_previous != (uint32_t)-1L && g__dist_to_plots_rounded_previous > DISTANCE_FOR_ON_THE_PLOT)
        {
          l__flg_inh_synth_dist_to_plot = false;    // Effacement de la restriction locale

          if (l__flg_inh_synth_dist_to_plot == false) {
            updateCommands(g__prompts_dist_to_plot, BASE_YOU_ARE_BACK_ON_THE_PLOT, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);

            *o__flg_more_synth = true;
          }
        }
        else {
          if (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2 || (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1 && i__results->distance <= DISTANCE_300M)) {
            l__flg_inh_synth_dist_to_plot = false;    // Effacement de la restriction locale dans le cas "Préparez vous à bifurquer..."
          }

          if (l__flg_inh_synth_dist_to_plot == false) {
            // 1st positionnement sur le tracé => Synthèse de "Vous êtes sur le tracé"
            if (g__flg_one_or_more_times_on_the_plot == false) {
              updateCommands(g__prompts_dist_to_plot, BASE_YOU_ARE_ON_THE_PLOT, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);
            }

            *o__flg_more_synth = true;
          }
        }
      }

      g__flg_one_or_more_times_on_the_plot = true;
    }
    else {
      // Le tracé n'est pas rejoint (pas de synthèse asynchrone si > 300 M)
      if (i__results->distance <= DISTANCE_300M) {
        l__flg_inh_synth_dist_to_plot = false;    // Effacement de la restriction locale si distance au tracé <= 300 M
      }

      if (l__flg_inh_synth_dist_to_plot == false) {
        updateCommands(g__prompts_dist_to_plot,
          (l__flg_same_dist_to_plot == true) ? BASE_THE_PLOT_IS_ALWAYS_AT : BASE_THE_PLOT_IS_AT, &l__size,
          BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);

        synthesisOfDistance(l__dist_rounded, &l__size, o__nbr_durations);

        *o__flg_more_synth = true;
      }
    }
  }
  else {
    if (l__flg_inh_synth_dist_to_plot == false) {
      updateCommands(g__prompts_dist_to_plot, BASE_THE_PLOT_IS_AT, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);
      updateCommands(g__prompts_distance, BASE_MORE_THAN, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

      synthesisDistance(DISTANCE_2_5KM, &l__size, o__nbr_durations);

      *o__flg_more_synth = true;
    }
  }

  g__prompts_str += "]";
  
  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  // Sauvegarde de la distance arrondie instantanée et synthétisée
  g__dist_to_plots_rounded_previous = l__dist_rounded;
  if (*o__flg_more_synth == true) {
    g__dist_to_plots_rounded_previous_synth = l__dist_rounded;
  }

  // 'l__size' != 0' indique qu'une synthèse a été faite...
  return l__size;
}

size_t buildCommandsPromptsBranchOfNow(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  // "Bifurquez maintenant à..."
  updateCommands(g__prompts_dist_to_plot, BASE_BRANCH_OF_NOW, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsContinueOn(uint32_t i__distance, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;
  uint32_t l__dist_rounded = i__distance;

  l__dist_rounded = distanceRoundedForThePlot(i__distance);

  g__prompts_str = "[";

  // "Continuez sur..."
  updateCommands(g__prompts_dist_to_plot, BASE_CONTINUE_ON, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 

  synthesisOfDistance(l__dist_rounded, &l__size, o__nbr_durations);

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsAndContinueOn(uint32_t i__distance, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;
  uint32_t l__dist_rounded = i__distance;

  l__dist_rounded = distanceRoundedForThePlot(i__distance);

  g__prompts_str = "[";

  // "Et continuez sur..."
  updateCommands(g__prompts_dist_to_plot, BASE_AND_CONTINUE_ON, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations);

  synthesisOfDistance(l__dist_rounded, &l__size, o__nbr_durations);

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsPrepareYourselves(uint32_t i__distance, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;
  uint32_t l__dist_rounded = i__distance;

  l__dist_rounded = distanceRoundedForThePlot(i__distance);

  g__prompts_str = "[";

  // "Préparez vous à bifurquer à..."
  updateCommands(g__prompts_dist_to_plot, BASE_PREPARE_YOURSELVES, &l__size, BASE_LAST_PROMPT_DIST_TO_PLOT, o__nbr_durations); 

  synthesisOfDistance(l__dist_rounded, &l__size, o__nbr_durations);

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for distance to plots synthesis

size_t buildCommandsPromptsRecordPlots(int i__nbr_plots, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  // "Enregistrement d'un ou plusieurs 'plot(s)'..."
  size_t l__base = 0;
  switch (i__nbr_plots) {
  case 1:  l__base = BASE_DING_DONG_BELL_DOORBELL_1; break;
  case 2:  l__base = BASE_DING_DONG_BELL_DOORBELL_2; break;
  case 3:  l__base = BASE_DING_DONG_BELL_DOORBELL_3; break;
  default: l__base = BASE_OUPS_MALE; break;
  }
  updateCommands(g__prompts_general, l__base, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsNoMovementInProgress(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  updateCommands(g__prompts_general, BASE_UNLOCK_OLD_CASTEL_ON_AN, &l__size, BASE_LAST_PROMPT_GENERAL, o__nbr_durations);

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

// List of definitions and methods for misc and statistics
#define MAS_NUM_END_OF_RECORD             0
#define MAS_NUM_SPEED_AVG               200
#define MAS_NUM_HOUR                    210
#define MAS_NUM_MINUTE                  211
#define MAS_NUM_SECOND                  212
#define MAS_NUM_ARRIVING_TIME           300
#define MAS_NUM_YOU_HAVE_WALKED         400
#define MAS_NUM_YOU_HAVE_ARRIVED        410

#define MAS_NUM_0_KMH                  1000

#define BASE_MISC_AND_STATS               0

#define BASE_0_KMH                        (BASE_MISC_AND_STATS + 17)

#define BASE_LAST_PROMPT_MISC_AND_STATS   (BASE_0_KMH + 40)

static const ST_PROMPTS_DEF           g__prompts_misc_and_stats[BASE_LAST_PROMPT_MISC_AND_STATS + 1] =
{
  { BASE_MISC_AND_STATS,     { 0x14, 0xe0 +  MAS_NUM_END_OF_RECORD / 256,        MAS_NUM_END_OF_RECORD % 256 },       "14/0000_fin_de_l_enregistrement_du_trace.mp3", true, 3, 2950L },
  { BASE_MISC_AND_STATS + 1, { 0x14, 0xe0 + (MAS_NUM_END_OF_RECORD + 10) / 256, (MAS_NUM_END_OF_RECORD + 10) % 256 }, "14/0010_trace_a_suivre_sur.mp3", true, 2, 2049L },
  { BASE_MISC_AND_STATS + 2, { 0x14, 0xe0 + (MAS_NUM_END_OF_RECORD + 11) / 256, (MAS_NUM_END_OF_RECORD + 11) % 256 }, "14/0011_dans_le_sens_aller.mp3", true, 2, 1578L },
  { BASE_MISC_AND_STATS + 3, { 0x14, 0xe0 + (MAS_NUM_END_OF_RECORD + 12) / 256, (MAS_NUM_END_OF_RECORD + 12) % 256 }, "14/0012_dans_le_sens_retour.mp3", true, 2, 2010L },

  { BASE_MISC_AND_STATS + 4, { 0x14, 0xe0 + MAS_NUM_SPEED_AVG / 256, MAS_NUM_SPEED_AVG % 256 }, "14/0200_a_la_moynene_de.mp3", true, 1, 1422L },
  { BASE_MISC_AND_STATS + 5, { 0x14, 0xe0 + MAS_NUM_HOUR / 256,      MAS_NUM_HOUR % 256 },      "14/0210_heure.mp3", true, 1, 756L },
  { BASE_MISC_AND_STATS + 6, { 0x14, 0xe0 + MAS_NUM_MINUTE / 256,    MAS_NUM_MINUTE % 256 },    "14/0211_minute.mp3", true, 1, 1069L },
  { BASE_MISC_AND_STATS + 7, { 0x14, 0xe0 + MAS_NUM_SECOND / 256,    MAS_NUM_SECOND % 256 },    "14/0212_seconde.mp3", true, 1, 952L },

// TBC: Rename de prompt with "l_heure" instead of "l'heure"
  { BASE_MISC_AND_STATS + 8, { 0x14, 0xe0 +  MAS_NUM_ARRIVING_TIME / 256,       MAS_NUM_ARRIVING_TIME % 256 },      "14/0300_l_heure_d_arrivee_est_estimee_a.mp3", true, 3, 2793L },
  { BASE_MISC_AND_STATS + 9, { 0x14, 0xe0 + (MAS_NUM_ARRIVING_TIME + 1) / 256, (MAS_NUM_ARRIVING_TIME + 1) % 256 }, "14/0301_l_heure_d_arrivee_est_estimee_dans.mp3", true, 3, 2597L },

  { BASE_MISC_AND_STATS + 10, { 0x14, 0xe0 +  MAS_NUM_YOU_HAVE_WALKED / 256,       MAS_NUM_YOU_HAVE_WALKED % 256 },      "14/0400_vous_avez_parcouru.mp3", true, 2, 1970L },
  { BASE_MISC_AND_STATS + 11, { 0x14, 0xe0 + (MAS_NUM_YOU_HAVE_WALKED + 1) / 256, (MAS_NUM_YOU_HAVE_WALKED + 1) % 256 }, "14/0401_dont.mp3", true, 1, 520L },
  { BASE_MISC_AND_STATS + 12, { 0x14, 0xe0 + (MAS_NUM_YOU_HAVE_WALKED + 2) / 256, (MAS_NUM_YOU_HAVE_WALKED + 2) % 256 }, "14/0402_sur_le_trace_a_suivre.mp3", true, 2, 2166L },

  { BASE_MISC_AND_STATS + 13, { 0x14, 0xe0 +  MAS_NUM_YOU_HAVE_ARRIVED / 256,       MAS_NUM_YOU_HAVE_ARRIVED % 256},       "14/0410_vous_etes_presque_arrive_a_la_fin_du_trace.mp3", true, 3, 3498L },
  { BASE_MISC_AND_STATS + 14, { 0x14, 0xe0 + (MAS_NUM_YOU_HAVE_ARRIVED + 1) / 256, (MAS_NUM_YOU_HAVE_ARRIVED + 1) % 256 }, "14/0411_vous_etes_arrive_a_la_fin_du_trace.mp3", true, 3, 2989L },

  { BASE_MISC_AND_STATS + 15, { 0x14, 0xe0 + (MAS_NUM_END_OF_RECORD + 1) / 256, (MAS_NUM_END_OF_RECORD + 1) % 256 }, "14/0001_fin_de_l_enrg_et_inversion_du_sens_du_trace.mp3", true, 4, 4517L },
  { BASE_MISC_AND_STATS + 16, { 0x14, 0xe0 + (MAS_NUM_END_OF_RECORD + 2) / 256, (MAS_NUM_END_OF_RECORD + 2) % 256 }, "14/0002_inversion_du_sens_du_trace.mp3", true, 2, 2676L },

  { BASE_0_KMH,      { 0x14, 0xe0 +  MAS_NUM_0_KMH / 256,         MAS_NUM_0_KMH % 256 },        "14/1000_0_kmh.mp3", true, 2, 2284L },
  { BASE_0_KMH + 1,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +   5) / 256, (MAS_NUM_0_KMH +   5) % 256 }, "14/1005_0_virgule_5_kmh.mp3", true, 3, 3381L },
  { BASE_0_KMH + 2,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  10) / 256, (MAS_NUM_0_KMH +  10) % 256 }, "14/1010_1_kmh.mp3", true, 21, 1814L },
  { BASE_0_KMH + 3,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  15) / 256, (MAS_NUM_0_KMH +  15) % 256 }, "14/1015_1_virgule_5_kmh.mp3", true, 3, 3068L },
  { BASE_0_KMH + 4,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  20) / 256, (MAS_NUM_0_KMH +  20) % 256 }, "14/1020_2_kmh.mp3", true, 2, 1970L },
  { BASE_0_KMH + 5,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  25) / 256, (MAS_NUM_0_KMH +  25) % 256 }, "14/1025_2_virgule_5_kmh.mp3", true, 3, 3146L },
  { BASE_0_KMH + 6,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  30) / 256, (MAS_NUM_0_KMH +  30) % 256 }, "14/1030_3_kmh.mp3", true, 2, 2049L },
  { BASE_0_KMH + 7,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  35) / 256, (MAS_NUM_0_KMH +  35) % 256 }, "14/1035_3_virgule_5_kmh.mp3", true, 3, 3185L },
  { BASE_0_KMH + 8,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  40) / 256, (MAS_NUM_0_KMH +  40) % 256 }, "14/1040_4_kmh.mp3", true, 2, 2127L },
  { BASE_0_KMH + 9,  { 0x14, 0xe0 + (MAS_NUM_0_KMH +  45) / 256, (MAS_NUM_0_KMH +  45) % 256 }, "14/1045_4_virgule_5_kmh.mp3", true, 3, 3302L },
  { BASE_0_KMH + 10, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  50) / 256, (MAS_NUM_0_KMH +  50) % 256 }, "14/1050_5_kmh.mp3", true, 2, 2088L },
  { BASE_0_KMH + 11, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  55) / 256, (MAS_NUM_0_KMH +  55) % 256 }, "14/1055_5_virgule_5_kmh.mp3", true, 3, 3420L },
  { BASE_0_KMH + 12, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  60) / 256, (MAS_NUM_0_KMH +  60) % 256 }, "14/1060_6_kmh.mp3", true, 2, 2010L },
  { BASE_0_KMH + 13, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  65) / 256, (MAS_NUM_0_KMH +  65) % 256 }, "14/1065_6_virgule_5_kmh.mp3", true, 3, 3538L },
  { BASE_0_KMH + 14, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  70) / 256, (MAS_NUM_0_KMH +  70) % 256 }, "14/1070_7_kmh.mp3", true, 2, 2127L },
  { BASE_0_KMH + 15, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  75) / 256, (MAS_NUM_0_KMH +  75) % 256 }, "14/1075_7_virgule_5_kmh.mp3", true, 3, 3420L },
  { BASE_0_KMH + 16, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  80) / 256, (MAS_NUM_0_KMH +  80) % 256 }, "14/1080_8_kmh.mp3", true, 2, 1970L },
  { BASE_0_KMH + 17, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  85) / 256, (MAS_NUM_0_KMH +  85) % 256 }, "14/1085_8_virgule_5_kmh.mp3", true, 3, 3302L },
  { BASE_0_KMH + 18, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  90) / 256, (MAS_NUM_0_KMH +  90) % 256 }, "14/1090_9_kmh.mp3", true, 2, 2166L },
  { BASE_0_KMH + 19, { 0x14, 0xe0 + (MAS_NUM_0_KMH +  95) / 256, (MAS_NUM_0_KMH +  95) % 256 }, "14/1095_9_virgule_5_kmh.mp3", true, 3, 3459L },
  { BASE_0_KMH + 20, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 100) / 256, (MAS_NUM_0_KMH + 100) % 256 }, "14/1100_10_kmh.mp3", true, 2, 2088L },
  { BASE_0_KMH + 21, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 105) / 256, (MAS_NUM_0_KMH + 105) % 256 }, "14/1105_10_virgule_5_kmh.mp3", true, 3, 3381L },
  { BASE_0_KMH + 22, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 110) / 256, (MAS_NUM_0_KMH + 110) % 256 }, "14/1110_11_kmh.mp3", true, 2, 2010L },
  { BASE_0_KMH + 23, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 115) / 256, (MAS_NUM_0_KMH + 115) % 256 }, "14/1115_11_virgule_5_kmh.mp3", true, 3, 3342L },
  { BASE_0_KMH + 24, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 120) / 256, (MAS_NUM_0_KMH + 120) % 256 }, "14/1120_12_kmh.mp3", true, 2, 2127L },
  { BASE_0_KMH + 25, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 125) / 256, (MAS_NUM_0_KMH + 125) % 256 }, "14/1125_12_virgule_5_kmh.mp3", true, 3, 3342L },
  { BASE_0_KMH + 26, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 130) / 256, (MAS_NUM_0_KMH + 130) % 256 }, "14/1130_13_kmh.mp3", true, 2, 2088L },
  { BASE_0_KMH + 27, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 135) / 256, (MAS_NUM_0_KMH + 135) % 256 }, "14/1135_13_virgule_5_kmh.mp3", true, 3, 3302L },
  { BASE_0_KMH + 28, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 140) / 256, (MAS_NUM_0_KMH + 140) % 256 }, "14/1140_14_kmh.mp3", true, 2, 2402L },
  { BASE_0_KMH + 29, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 145) / 256, (MAS_NUM_0_KMH + 145) % 256 }, "14/1145_14_virgule_5_kmh.mp3", true, 4, 3655L },
  { BASE_0_KMH + 30, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 150) / 256, (MAS_NUM_0_KMH + 150) % 256 }, "14/1150_15_kmh.mp3", true, 1, 1970L },
  { BASE_0_KMH + 31, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 155) / 256, (MAS_NUM_0_KMH + 155) % 256 }, "14/1155_15_virgule_5_kmh.mp3", true, 3, 3264L },
  { BASE_0_KMH + 32, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 200) / 256, (MAS_NUM_0_KMH + 200) % 256 }, "14/1200_20_kmh.mp3", true, 2, 1970L },
  { BASE_0_KMH + 33, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 250) / 256, (MAS_NUM_0_KMH + 250) % 256 }, "14/1250_25_kmh.mp3", true, 3, 2636L },
  { BASE_0_KMH + 34, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 300) / 256, (MAS_NUM_0_KMH + 300) % 256 }, "14/1300_30_kmh.mp3", true, 2, 2166L },
  { BASE_0_KMH + 35, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 350) / 256, (MAS_NUM_0_KMH + 350) % 256 }, "14/1350_35_kmh.mp3", true, 2, 2519L },
  { BASE_0_KMH + 36, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 400) / 256, (MAS_NUM_0_KMH + 400) % 256 }, "14/1400_40_kmh.mp3", true, 2, 2362L },
  { BASE_0_KMH + 37, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 450) / 256, (MAS_NUM_0_KMH + 450) % 256 }, "14/1450_45_kmh.mp3", true, 32, 2715L },
  { BASE_0_KMH + 38, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 500) / 256, (MAS_NUM_0_KMH + 500) % 256 }, "14/1500_50_kmh.mp3", true, 2, 2402L },
  { BASE_0_KMH + 39, { 0x14, 0xe0 + (MAS_NUM_0_KMH + 501) / 256, (MAS_NUM_0_KMH + 501) % 256 }, "14/1501_plus_de_50_kmh.mp3", true, 32, 2715L },

  { BASE_LAST_PROMPT_MISC_AND_STATS, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

size_t buildCommandsPromptsBuildingOfPlots(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  // "Construction du tracé..."
#if 0
  l__size = buildCommandsPromptsGeneral(BASE_SUDDEN_EVENT_DIFFERENT, o__command);
#else
  // "Fin de l'enregistrement et inversion du sens du tracé"
  updateCommands(g__prompts_misc_and_stats, BASE_MISC_AND_STATS + 15, &l__size, BASE_LAST_PROMPT_MISC_AND_STATS, o__nbr_durations); 
#endif

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsChangeSensOfPlots(byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  // "Inversion du sens du tracé"
  updateCommands(g__prompts_misc_and_stats, BASE_MISC_AND_STATS + 16, &l__size, BASE_LAST_PROMPT_MISC_AND_STATS, o__nbr_durations); 

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsPlotsProperties(uint32_t i__total_distance, boolean i__flg_about, ENUM_PLOTS_DIR i__plots_direction, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  // "Tracé à suivre sur..."
  updateCommands(g__prompts_misc_and_stats, BASE_MISC_AND_STATS + 1, &l__size, BASE_LAST_PROMPT_MISC_AND_STATS, o__nbr_durations);

  // Distance arrondie...
  uint32_t l__dist_rounded = i__total_distance;
  distanceRounded(&l__dist_rounded, true);

  if (l__dist_rounded > DISTANCE_30KM) {
    // Si distance > 30 Km => Synthèse de "plus de" + "30 Km"
    updateCommands(g__prompts_distance, BASE_MORE_THAN, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations); 

    synthesisDistance(DISTANCE_30KM, &l__size, o__nbr_durations);
  }
  else {
    synthesisDistance(l__dist_rounded, &l__size, o__nbr_durations, true);

    if (i__flg_about == true) {
      updateCommands(g__prompts_distance, BASE_APPROXIMATELY, &l__size, BASE_LAST_PROMPT_DISTANCE, o__nbr_durations);
    }
  }

  // "Dans le sens..."
  switch (i__plots_direction) {
  case PLOTS_DIR_GO:
    updateCommands(g__prompts_misc_and_stats, BASE_MISC_AND_STATS + 2, &l__size, BASE_LAST_PROMPT_MISC_AND_STATS, o__nbr_durations);
    break;
  case PLOTS_DIR_RETURN:
    updateCommands(g__prompts_misc_and_stats, BASE_MISC_AND_STATS + 3, &l__size, BASE_LAST_PROMPT_MISC_AND_STATS, o__nbr_durations);
    break;
  default:
    break;
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}

size_t buildCommandsPromptsYouHaveArrived(boolean i__flg_almost, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  size_t l__size = 0;

  g__prompts_str = "[";

  // "Vous êtes [presque] arrivé..."
  size_t l__base = (i__flg_almost == true) ? BASE_MISC_AND_STATS + 13 : BASE_MISC_AND_STATS + 14;
  updateCommands(g__prompts_misc_and_stats, l__base, &l__size, BASE_LAST_PROMPT_MISC_AND_STATS, o__nbr_durations);

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for misc and statistics

// List of definitions and methods for positions 'recorded' and 'memorized'
/* TODO: Ajout des prompts:
 * - "07/0202_vous_etes_a.mp3"              Utilisé également pour la synthèse distance et cap à une ville/village
 * - "07/0204_vous_etes_toujours_a.mp3"
 * - "07/0212_vous_etes_au.mp3"             Utilisés pour la synthèse distance et cap à une ville/village
 * - "07/0214_vous_etes_toujours_au.mp3"    (ie. "vous êtes [toujours] au "Tremblay-sur-Mauldre" ou "aux Mesnuls")
 * - "07/0300_est_a.mp3"                    ie. "Elancourt" "est à"
 * - "07/0302_est_toujours_a.mp3"
 * - "07/0310_sont_a.mp3"                   ie. "Les Mesnuls" "sont à"
 * - "07/0312_sont_toujours_a.mp3"
*/
#define POS_NUM_THE_1ST                  101
#define POS_NUM_THE_BEFORE_LAST          198
#define POS_NUM_THE_LAST                 199

#define POS_NUM_YOU_HAVE_ARRIVED         200

#define POS_NUM_YOU_ARE_AT               202
#define POS_NUM_YOU_ARE_STILL_AT         204
#define POS_NUM_YOU_ARE_AT_BIS           212
#define POS_NUM_YOU_ARE_STILL_AT_BIS     214

#define POS_NUM_IS_AT                    300
#define POS_NUM_IS_STILL_AT              302
#define POS_NUM_ARE_AT                   310
#define POS_NUM_ARE_STILL_AT             312

#define POS_NUM_MEMORIZED               1000
#define POS_NUM_RECORDED                1001

#define POS_NUM_MEMORIZED_IS_AT         1002
#define POS_NUM_RECORDED_IS_AT          1003
#define POS_NUM_MEMORIZED_IS_ALWAYS_AT  1004
#define POS_NUM_RECORDED_IS_ALWAYS_AT   1005

#define POS_NUM_MEMORIZED_OF            1010
#define POS_NUM_RECORDED_OF             1020

typedef enum {
  BASE_POSITIONS_THE_1ST = 0,                                     // "la premiere", ..., "la dixieme"    
  BASE_POSITIONS_THE_BEFORE_LAST = BASE_POSITIONS_THE_1ST + 10,   // "l_avant_derniere"
  BASE_POSITIONS_THE_LAST,                                        // "la_derniere"
  BASE_POS_YOU_HAVE_ARRIVED,            // "vous etes arrive a"
  BASE_POS_YOU_ARE_AT,                  // "Vous etes a"
  BASE_POS_YOU_ARE_STILL_AT,            // "vous etes toujours a"
  BASE_POS_YOU_ARE_AT_BIS,              // "Vous etes au"
  BASE_POS_YOU_ARE_STILL_AT_BIS,        // "vous etes toujours au"
  BASE_POS_IS_AT,                       // "est à"
  BASE_POS_IS_STILL_AT,                 // "est toujours à"
  BASE_POS_ARE_AT,                      // "sont à"
  BASE_POS_ARE_STILL_AT,                // "sont toujours à"
  BASE_POS_MEMORIZED,                   // "position memorisee"
  BASE_POS_RECORDED,                    // "position enregistree"
  BASE_POS_MEMORIZED_IS_AT,             // "position memorisee est a"
  BASE_POS_RECORDED_IS_AT,              // "position enregistree est a"
  BASE_POS_MEMORIZED_IS_ALWAYS_AT,      // "position memorisee est toujours a"
  BASE_POS_RECORDED_IS_ALWAYS_AT,       // "position enregistree est toujours a"
  BASE_POS_MEMORIZED_OF,                // "memorisation de"
  BASE_POS_RECORDED_OF,                 // "enregistrement de"

  BASE_LAST_PROMPT_POSITIONS
} ENUM_BASE_POSITIONS;

static const ST_PROMPTS_DEF              g__prompts_positions[BASE_LAST_PROMPT_POSITIONS + 1] =
{
  { BASE_POSITIONS_THE_1ST,     { 0x14, 0x70 +  POS_NUM_THE_1ST / 256,       POS_NUM_THE_1ST % 256 },      "07/0001_la_premiere.mp3",  true, 1, 1461L },
  { BASE_POSITIONS_THE_1ST + 1, { 0x14, 0x70 + (POS_NUM_THE_1ST + 1) / 256, (POS_NUM_THE_1ST + 1) % 256 }, "07/0002_la_deuxieme.mp3",  true, 1, 1226L },
  { BASE_POSITIONS_THE_1ST + 2, { 0x14, 0x70 + (POS_NUM_THE_1ST + 2) / 256, (POS_NUM_THE_1ST + 2) % 256 }, "07/0003_la_troisieme.mp3", true, 1, 1344L },
  { BASE_POSITIONS_THE_1ST + 3, { 0x14, 0x70 + (POS_NUM_THE_1ST + 3) / 256, (POS_NUM_THE_1ST + 3) % 256 }, "07/0004_la_quatrieme.mp3", true, 1, 1500L },
  { BASE_POSITIONS_THE_1ST + 4, { 0x14, 0x70 + (POS_NUM_THE_1ST + 4) / 256, (POS_NUM_THE_1ST + 4) % 256 }, "07/0005_la_cinquieme.mp3", true, 1, 1344L },
  { BASE_POSITIONS_THE_1ST + 5, { 0x14, 0x70 + (POS_NUM_THE_1ST + 5) / 256, (POS_NUM_THE_1ST + 5) % 256 }, "07/0006_la_sixieme.mp3",   true, 1, 1344L },
  { BASE_POSITIONS_THE_1ST + 6, { 0x14, 0x70 + (POS_NUM_THE_1ST + 6) / 256, (POS_NUM_THE_1ST + 6) % 256 }, "07/0007_la_septieme.mp3",  true, 1, 1265L },
  { BASE_POSITIONS_THE_1ST + 7, { 0x14, 0x70 + (POS_NUM_THE_1ST + 7) / 256, (POS_NUM_THE_1ST + 7) % 256 }, "07/0008_la_huitieme.mp3",  true, 1, 1578L },
  { BASE_POSITIONS_THE_1ST + 8, { 0x14, 0x70 + (POS_NUM_THE_1ST + 8) / 256, (POS_NUM_THE_1ST + 8) % 256 }, "07/0009_la_neuvieme.mp3",  true, 1, 1304L },
  { BASE_POSITIONS_THE_1ST + 9, { 0x14, 0x70 + (POS_NUM_THE_1ST + 9) / 256, (POS_NUM_THE_1ST + 9) % 256 }, "07/0010_la_dixieme.mp3",   true, 1, 1265L },

  { BASE_POSITIONS_THE_BEFORE_LAST, { 0x14, 0x70 + POS_NUM_THE_BEFORE_LAST / 256, POS_NUM_THE_BEFORE_LAST % 256 }, "07/0098_l_avant_derniere.mp3", true, 1, 1696L },
  { BASE_POSITIONS_THE_LAST,        { 0x14, 0x70 + POS_NUM_THE_LAST / 256,        POS_NUM_THE_LAST % 256 },        "07/0099_la_derniere.mp3",      true, 1, 1382L },

  { BASE_POS_YOU_HAVE_ARRIVED,     { 0x14, 0x70 + POS_NUM_YOU_HAVE_ARRIVED / 256,     POS_NUM_YOU_HAVE_ARRIVED % 256 },      "07/0200_vous_etes_arrive_a.mp3",     true, 1, 1618L },

  { BASE_POS_YOU_ARE_AT,           { 0x14, 0x70 + POS_NUM_YOU_ARE_AT / 256,           POS_NUM_YOU_ARE_AT % 256 },           "07/0202_vous_etes_a.mp3",           true, 1, 1069L },
  { BASE_POS_YOU_ARE_STILL_AT,     { 0x14, 0x70 + POS_NUM_YOU_ARE_STILL_AT / 256,     POS_NUM_YOU_ARE_STILL_AT % 256 },     "07/0204_vous_etes_toujours_a.mp3",  true, 1, 1618L },
  { BASE_POS_YOU_ARE_AT_BIS,       { 0x14, 0x70 + POS_NUM_YOU_ARE_AT_BIS / 256,       POS_NUM_YOU_ARE_AT_BIS % 256 },       "07/0212_vous_etes_au.mp3",          true, 1, 1030L },
  { BASE_POS_YOU_ARE_STILL_AT_BIS, { 0x14, 0x70 + POS_NUM_YOU_ARE_STILL_AT_BIS / 256, POS_NUM_YOU_ARE_STILL_AT_BIS % 256 }, "07/0214_vous_etes_toujours_au.mp3", true, 1, 1696L },

  { BASE_POS_IS_AT,                { 0x14, 0x70 + POS_NUM_IS_AT / 256, POS_NUM_IS_AT % 256 },               "07/0300_est_a.mp3",           true, 0,  716L },
  { BASE_POS_IS_STILL_AT,          { 0x14, 0x70 + POS_NUM_IS_STILL_AT / 256, POS_NUM_IS_STILL_AT % 256 },   "07/0302_est_toujours_a.mp3",  true, 1, 1265L },
  { BASE_POS_ARE_AT,               { 0x14, 0x70 + POS_NUM_ARE_AT / 256, POS_NUM_ARE_AT % 256 },             "07/0310_sont_a.mp3",          true, 0,  912L },
  { BASE_POS_ARE_STILL_AT,         { 0x14, 0x70 + POS_NUM_ARE_STILL_AT / 256, POS_NUM_ARE_STILL_AT % 256 }, "07/0312_sont_toujours_a.mp3", true, 1, 1382L },

  { BASE_POS_MEMORIZED,            { 0x14, 0x70 + POS_NUM_MEMORIZED / 256,            POS_NUM_MEMORIZED % 256 },            "07/1000_position_memorisee.mp3",         true, 2, 2049L },
  { BASE_POS_RECORDED,             { 0x14, 0x70 + POS_NUM_RECORDED / 256,             POS_NUM_RECORDED % 256 },             "07/1001_position_enregistree.mp3",       true, 2, 2049L },
  { BASE_POS_MEMORIZED_IS_AT,      { 0x14, 0x70 + POS_NUM_MEMORIZED_IS_AT / 256,      POS_NUM_MEMORIZED_IS_AT % 256 },      "07/1002_position_memorisee_est_a.mp3",   true, 2, 2480L },
  { BASE_POS_RECORDED_IS_AT,       { 0x14, 0x70 + POS_NUM_RECORDED_IS_AT / 256,       POS_NUM_RECORDED_IS_AT % 256 },       "07/1003_position_enregistree_est_a.mp3", true, 2, 2240L },

  { BASE_POS_MEMORIZED_IS_ALWAYS_AT, { 0x14, 0x70 + POS_NUM_MEMORIZED_IS_ALWAYS_AT / 256, POS_NUM_MEMORIZED_IS_ALWAYS_AT % 256 }, "07/1004_position_memorisee_est_toujours_a.mp3",   true, 2, 2715L },
  { BASE_POS_RECORDED_IS_ALWAYS_AT,  { 0x14, 0x70 + POS_NUM_RECORDED_IS_ALWAYS_AT / 256,  POS_NUM_RECORDED_IS_ALWAYS_AT % 256 },  "07/1005_position_enregistree_est_toujours_a.mp3", true, 2, 2754L },
  { BASE_POS_MEMORIZED_OF,         { 0x14, 0x70 + POS_NUM_MEMORIZED_OF / 256,         POS_NUM_MEMORIZED_OF % 256 },         "07/1010_memorisation_de.mp3",            true, 1, 1892L },
  { BASE_POS_RECORDED_OF,          { 0x14, 0x70 + POS_NUM_RECORDED_OF / 256,          POS_NUM_RECORDED_OF % 256 },          "07/1020_enregistrement_de.mp3",          true, 1, 1696L },

  { BASE_LAST_PROMPT_POSITIONS, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

// Souvenir du passé pour la synthèse des positions 'enregistrée'/'mémorisée'
static ENUM_TYPE_POSITION       g__type_of_position_previous         = TYPE_POSITION_NO_MEAN;
static uint8_t                  g__range_of_position_previous        = (uint8_t)-1;
static uint32_t                 g__dist_to_position_rounded_previous = (uint32_t)-1L;
static boolean                  g__to_proximity_of_position          = false;

size_t buildCommandsPromptsPosition(ST_COORD_POSITION *i__st_position, int i__nbr_positions, boolean *o__flg_synth_cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  char l__buffer[80];
  size_t l__size = 0;

  g__prompts_str = "[";

  // Synthèse de la distance à la position en fonction de son type 'TYPE_POSITION_RECORDED' eor 'TYPE_POSITION_MEMORIZED'

  // Effacement du passé sur changement de la position séectionnée et à synthétiser
  if (i__st_position->type != g__type_of_position_previous || i__st_position->range != g__range_of_position_previous) {
    Serial.print("buildCommandsPromptsPosition(): Change position:\n");

    sprintf(l__buffer, "\tType [%d] -> [%d]\n\tRange [%d] -> [%d]\n",
      g__type_of_position_previous,  i__st_position->type,
      g__range_of_position_previous, i__st_position->range);
    Serial.print(l__buffer);

    g__type_of_position_previous         = TYPE_POSITION_NO_MEAN;
    g__range_of_position_previous        = (uint8_t)-1;
    g__dist_to_position_rounded_previous = (uint32_t)-1L;
    g__to_proximity_of_position          = false;
  }

  uint32_t l__dist_rounded = distanceRoundedForThePlot(i__st_position->distance);
  boolean  l__flg_same_dist_to_position =
    (g__dist_to_position_rounded_previous != (uint32_t)-1 && (g__to_proximity_of_position == true || l__dist_rounded == g__dist_to_position_rounded_previous)) ? true : false;

  sprintf(l__buffer, "buildCommandsPromptsPosition(): Position (same dist. [%d]):\n", l__flg_same_dist_to_position);
  Serial.print(l__buffer);

  sprintf(l__buffer, "\tProximity [%d]\n\tDist. [%d] -> [%d]\n",
    g__to_proximity_of_position,
    g__dist_to_position_rounded_previous, l__dist_rounded);
  Serial.print(l__buffer);

  /* Prise de la n-ième position avec comme principe:
   * - Si le 'range' est égal au (max - 1) => Synthèse de "la dernière"
   * - Si le 'range' est égal au (max - 2) => Synthèse de "l'avant dernière"
   * - Sinon => Synthèse de "La ('range' + 1')-ième"
   *   => Le 'max' étant 'i__nbr_positions' passé en argument et qui est fonction du type
   */
  size_t l__base_range = (size_t)-1;
  int l__range = (int)i__st_position->range;

  if (i__nbr_positions >= 1 && l__range == (i__nbr_positions - 1)) {
    l__base_range = BASE_POSITIONS_THE_LAST;
  }
  else if (i__nbr_positions >= 2 && l__range == (i__nbr_positions - 2)) {
    l__base_range = BASE_POSITIONS_THE_BEFORE_LAST;
  }
  else if (i__nbr_positions >= 3 && l__range >= 0 && l__range < (i__nbr_positions - 2)) {
    // 'BASE_POSITIONS_1ST' correspond à "la première" pour 'range' == 0
    l__base_range = BASE_POSITIONS_THE_1ST + l__range;
  }
  // Fin: Prise de la n-ième position

  // TODO: Test de débordement @ synthèse définie
  if (l__base_range == (size_t)-1) {
    // Invalid nbr of positions or range
    updateCommandsWithOups(&l__size);
  }
  else {
    size_t l__base = (size_t)-1;

    // Synthèse de la distance à la position en fonction de son type 'TYPE_POSITION_RECORDED' eor 'TYPE_POSITION_MEMORIZED'
    if (l__dist_rounded <= DISTANCE_50M) {
      // "Vous êtes à ..." ou "Vous êtes toujours à ..."
      l__base = (l__flg_same_dist_to_position == true) ? BASE_POS_YOU_ARE_STILL_AT : BASE_POS_YOU_ARE_AT;
      updateCommands(g__prompts_positions, l__base, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);

      // "La n-ième" ...
      updateCommands(g__prompts_positions, l__base_range, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);

      // "... position enregistrée/mémorisée"
      l__base = (i__st_position->type == TYPE_POSITION_RECORDED) ? BASE_POS_RECORDED : BASE_POS_MEMORIZED;
      updateCommands(g__prompts_positions, l__base, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);

      g__to_proximity_of_position = true;    // Sauvegarde du passé

      // Pas de synthèse du cap si trop proche
      *o__flg_synth_cap = false;
    }
    else {
      // "La n-ième" ...
      updateCommands(g__prompts_positions, l__base_range, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);

      // "... position enregistrée/mémorisée est [toujours] à ..."
      if (i__st_position->type == TYPE_POSITION_RECORDED) {
        l__base = (l__flg_same_dist_to_position == true) ? BASE_POS_RECORDED_IS_ALWAYS_AT : BASE_POS_RECORDED_IS_AT;
      }
      else {
        l__base = (l__flg_same_dist_to_position == true) ? BASE_POS_MEMORIZED_IS_ALWAYS_AT : BASE_POS_MEMORIZED_IS_AT;
      }
      updateCommands(g__prompts_positions, l__base, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);

      // Synthèse de la distance
      synthesisOfDistance(l__dist_rounded, &l__size, o__nbr_durations);

      g__to_proximity_of_position = false;    // Sauvegarde du passé

      // Synthèse du cap
      *o__flg_synth_cap = true;
    }

    // Sauvegarde du passé
    g__type_of_position_previous         = i__st_position->type;
    g__range_of_position_previous        = i__st_position->range;
    g__dist_to_position_rounded_previous = l__dist_rounded;
    // Fin: Synthèse de la distance à la position en fonction de son type 'TYPE_POSITION_RECORDED' eor 'TYPE_POSITION_MEMORIZED'
  }

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for positions 'recorded' and 'memorized'

// List of definitions and methods for positions of the communes
// Generated automatically at 2022/02/01 16:18:49 - Version 1.2.0 (Don't edit ;-)
// Villes de France sans Departement (region de randonnees ;-)
// Maine-et-Loire
#define POS_NUM_ALLONNES      1000
#define POS_NUM_BRAIN_SUR_ALLONNES      1001
#define POS_NUM_LA_BREILLE_LES_PINS     1002
#define POS_NUM_CHACE     1003
#define POS_NUM_CHENEHUTTE_TREVES_CUNAULT     1004
#define POS_NUM_DISTRE      1005
#define POS_NUM_GENNES      1006
#define POS_NUM_MEIGNE      1007
#define POS_NUM_MONTSOREAU      1008
#define POS_NUM_NEUILLE     1009
#define POS_NUM_PARNAY      1010
#define POS_NUM_ROU_MARSON      1011
#define POS_NUM_SAINT_CLEMENT_DES_LEVEES      1012
#define POS_NUM_SAINT_MARTIN_DE_LA_PLACE      1013
#define POS_NUM_SAUMUR      1014
#define POS_NUM_SOUZAY_CHAMPIGNY      1015
#define POS_NUM_TURQUANT      1016
#define POS_NUM_LES_ULMES     1017
#define POS_NUM_ULMES     1018
#define POS_NUM_VARENNES_SUR_LOIRE      1019
#define POS_NUM_VARRAINS      1020
#define POS_NUM_VERRIE      1021
#define POS_NUM_VILLEBERNIER      1022
#define POS_NUM_VIVY      1023

// Yvelines et Essonne
#define POS_NUM_ANDELU      1024
#define POS_NUM_AUFFARGIS     1025
#define POS_NUM_AUTEUIL     1026
#define POS_NUM_AUTOUILLET      1027
#define POS_NUM_BAILLY      1028
#define POS_NUM_BAZOCHES_SUR_GUYONNE      1029
#define POS_NUM_BEHOUST     1030
#define POS_NUM_BEYNES      1031
#define POS_NUM_BOIS_D_ARCY     1032
#define POS_NUM_BOISSY_SANS_AVOIR     1033
#define POS_NUM_BOUGIVAL      1034
#define POS_NUM_LES_BREVIAIRES      1035
#define POS_NUM_BREVIAIRES      1036
#define POS_NUM_BUC     1037
#define POS_NUM_LA_CELLE_SAINT_CLOUD      1038
#define POS_NUM_CERNAY_LA_VILLE     1039
#define POS_NUM_CHATEAUFORT     1040
#define POS_NUM_CHAVENAY      1041
#define POS_NUM_LE_CHESNAY      1042
#define POS_NUM_CHESNAY     1043
#define POS_NUM_CHEVREUSE     1044
#define POS_NUM_CHOISEL     1045
#define POS_NUM_LES_CLAYES_SOUS_BOIS      1046
#define POS_NUM_CLAYES_SOUS_BOIS      1047
#define POS_NUM_COIGNIERES_VILLAGE      1048
#define POS_NUM_COIGNIERES      1049
#define POS_NUM_CRESPIERES      1050
#define POS_NUM_CROISSY_SUR_SEINE     1051
#define POS_NUM_DAMPIERRE_EN_YVELINES     1052
#define POS_NUM_DAVRON      1053
#define POS_NUM_ELANCOURT     1054
#define POS_NUM_ELANCOURT_VILLAGE     1055
#define POS_NUM_ELANCOURT_LA_CLE_DE_SAINT_PIERRE      1056
#define POS_NUM_LA_COLLINE_D_ELANCOURT      1057
#define POS_NUM_LES_ESSARTS_LE_ROI      1058
#define POS_NUM_ESSARTS_LE_ROI      1059
#define POS_NUM_L_ETANG_LA_VILLE      1060
#define POS_NUM_FEUCHEROLLES      1061
#define POS_NUM_FLEXANVILLE     1062
#define POS_NUM_FONTENAY_LE_FLEURY      1063
#define POS_NUM_FOURQUEUX     1064
#define POS_NUM_GALLUIS     1065
#define POS_NUM_GAMBAISEUIL     1066
#define POS_NUM_GARANCIERES     1067
#define POS_NUM_GOUPILLIERES      1068
#define POS_NUM_GROSROUVRE      1069
#define POS_NUM_GUYANCOURT      1070
#define POS_NUM_HARGEVILLE      1071
#define POS_NUM_JOUARS_PONTCHARTRAIN      1072
#define POS_NUM_LEVIS_SAINT_NOM     1073
#define POS_NUM_LES_LOGES_EN_JOSAS      1074
#define POS_NUM_LOGES_EN_JOSAS      1075
#define POS_NUM_LOUVECIENNES      1076
#define POS_NUM_MAGNY_LES_HAMEAUX     1077
#define POS_NUM_MAGNY_VILLAGE     1078
#define POS_NUM_MARCQ     1079
#define POS_NUM_MAREIL_LE_GUYON     1080
#define POS_NUM_MAREIL_MARLY      1081
#define POS_NUM_MAREIL_SUR_MAULDRE      1082
#define POS_NUM_MARLY_LE_ROI      1083
#define POS_NUM_MAUREPAS_VILLAGE      1084
#define POS_NUM_MAUREPAS      1085
#define POS_NUM_MERE      1086
#define POS_NUM_LE_MESNIL_SAINT_DENIS     1087
#define POS_NUM_MESNIL_SAINT_DENIS      1088
#define POS_NUM_LES_MESNULS     1089
#define POS_NUM_MESNULS     1090
#define POS_NUM_MILLEMONT     1091
#define POS_NUM_MILON_LA_CHAPELLE     1092
#define POS_NUM_MONTAINVILLE      1093
#define POS_NUM_MONTFORT_L_AMAURY     1094
#define POS_NUM_MONTIGNY_LE_BRETONNEUX      1095
#define POS_NUM_NEAUPHLE_LE_CHATEAU     1096
#define POS_NUM_NEAUPHLE_LE_VIEUX     1097
#define POS_NUM_NOISY_LE_ROI      1098
#define POS_NUM_OSMOY     1099
#define POS_NUM_LE_PECQ     1100
#define POS_NUM_PECQ      1101
#define POS_NUM_LE_PERRAY_EN_YVELINES     1102
#define POS_NUM_PERRAY_EN_YVELINES      1103
#define POS_NUM_PLAISIR     1104
#define POS_NUM_POIGNY_LA_FORET     1105
#define POS_NUM_LE_PORT_MARLY     1106
#define POS_NUM_PORT_MARLY      1107
#define POS_NUM_LA_QUEUE_LES_YVELINES     1108
#define POS_NUM_RENNEMOULIN     1109
#define POS_NUM_ROCQUENCOURT      1110
#define POS_NUM_SAINT_BENOIT      1111
#define POS_NUM_SAINT_CYR_L_ECOLE     1112
#define POS_NUM_SAINT_FORGET      1113
#define POS_NUM_SAINT_GERMAIN_DE_LA_GRANGE      1114
#define POS_NUM_SAINT_GERMAIN_EN_LAYE     1115
#define POS_NUM_SAINT_HUBERT      1116
#define POS_NUM_SAINT_LAMBERT_DES_BOIS      1117
#define POS_NUM_SAINT_LEGER_EN_YVELINES     1118
#define POS_NUM_SAINT_MARTIN_DES_CHAMPS     1119
#define POS_NUM_SAINT_NOM_LA_BRETECHE     1120
#define POS_NUM_SAINT_REMY_L_HONORE     1121
#define POS_NUM_SAINT_REMY_LES_CHEVREUSE      1122
#define POS_NUM_SAULX_MARCHAIS      1123
#define POS_NUM_SENLISSE      1124
#define POS_NUM_THIVERVAL_GRIGNON     1125
#define POS_NUM_THOIRY      1126
#define POS_NUM_TOUSSUS_LE_NOBLE      1127
#define POS_NUM_TRAPPES     1128
#define POS_NUM_LE_TREMBLAY_SUR_MAULDRE     1129
#define POS_NUM_TREMBLAY_SUR_MAULDRE      1130
#define POS_NUM_LA_VERRIERE     1131
#define POS_NUM_VERSAILLES      1132
#define POS_NUM_LE_VESINET      1133
#define POS_NUM_VESINET     1134
#define POS_NUM_VICQ      1135
#define POS_NUM_VIEILLE_EGLISE_EN_YVELINES      1136
#define POS_NUM_VILLEPREUX      1137
#define POS_NUM_VILLIERS_LE_MAHIEU      1138
#define POS_NUM_VILLIERS_SAINT_FREDERIC     1139
#define POS_NUM_VOISINS_LE_BRETONNEUX     1140
#define POS_NUM_BOULLAY_LES_TROUX     1141
#define POS_NUM_GIF_SUR_YVETTE      1142
#define POS_NUM_GOMETZ_LA_VILLE     1143
#define POS_NUM_GOMETZ_LE_CHATEL      1144
#define POS_NUM_LES_MOLIERES      1145
#define POS_NUM_MOLIERES      1146
#define POS_NUM_SAINT_AUBIN     1147
#define POS_NUM_VILLIERS_LE_BACLE     1148

// Villes de France avec Departement
#define POS_NUM_01_BELLEY     2000
#define POS_NUM_01_BOURG_EN_BRESSE      2001
#define POS_NUM_01_GEX      2002
#define POS_NUM_01_NANTUA     2003
#define POS_NUM_02_CHATEAU_THIERRY      2004
#define POS_NUM_02_LAON     2005
#define POS_NUM_02_SAINT_QUENTIN      2006
#define POS_NUM_02_SOISSONS     2007
#define POS_NUM_02_VERVINS      2008
#define POS_NUM_03_MONTLUCON      2009
#define POS_NUM_03_MOULINS      2010
#define POS_NUM_03_VICHY      2011
#define POS_NUM_04_BARCELONNETTE      2012
#define POS_NUM_04_CASTELLANE     2013
#define POS_NUM_04_DIGNE_LES_BAINS      2014
#define POS_NUM_04_FORCALQUIER      2015
#define POS_NUM_05_BRIANCON     2016
#define POS_NUM_05_GAP      2017
#define POS_NUM_06_GRASSE     2018
#define POS_NUM_06_NICE     2019
#define POS_NUM_07_LARGENTIERE      2020
#define POS_NUM_07_PRIVAS     2021
#define POS_NUM_07_TOURNON_SUR_RHONE      2022
#define POS_NUM_08_CHARLEVILLE_MEZIERES     2023
#define POS_NUM_08_RETHEL     2024
#define POS_NUM_08_SEDAN      2025
#define POS_NUM_08_VOUZIERS     2026
#define POS_NUM_09_FOIX     2027
#define POS_NUM_09_PAMIERS      2028
#define POS_NUM_09_SAINT_GIRONS     2029
#define POS_NUM_10_BAR_SUR_AUBE     2030
#define POS_NUM_10_NOGENT_SUR_SEINE     2031
#define POS_NUM_10_TROYES     2032
#define POS_NUM_11_CARCASSONNE      2033
#define POS_NUM_11_LIMOUX     2034
#define POS_NUM_11_NARBONNE     2035
#define POS_NUM_12_MILLAU     2036
#define POS_NUM_12_RODEZ      2037
#define POS_NUM_12_VILLEFRANCHE_DE_ROUERGUE     2038
#define POS_NUM_13_AIX_EN_PROVENCE      2039
#define POS_NUM_13_ARLES      2040
#define POS_NUM_13_ISTRES     2041
#define POS_NUM_13_MARSEILLE      2042
#define POS_NUM_14_BAYEUX     2043
#define POS_NUM_14_CAEN     2044
#define POS_NUM_14_LISIEUX      2045
#define POS_NUM_14_VIRE     2046
#define POS_NUM_15_AURILLAC     2047
#define POS_NUM_15_MAURIAC      2048
#define POS_NUM_15_SAINT_FLOUR      2049
#define POS_NUM_16_ANGOULEME      2050
#define POS_NUM_16_COGNAC     2051
#define POS_NUM_16_CONFOLENS      2052
#define POS_NUM_17_JONZAC     2053
#define POS_NUM_17_ROCHEFORT      2054
#define POS_NUM_17_LA_ROCHELLE      2055
#define POS_NUM_17_SAINT_JEAN_D_ANGELY      2056
#define POS_NUM_17_SAINTES      2057
#define POS_NUM_18_BOURGES      2058
#define POS_NUM_18_SAINT_AMAND_MONTROND     2059
#define POS_NUM_18_VIERZON      2060
#define POS_NUM_19_BRIVE_LA_GAILLARDE     2061
#define POS_NUM_19_TULLE      2062
#define POS_NUM_19_USSEL      2063
#define POS_NUM_2A_AJACCIO      2064
#define POS_NUM_2A_SARTENE      2065
#define POS_NUM_2B_BASTIA     2066
#define POS_NUM_2B_CALVI      2067
#define POS_NUM_2B_CORTE      2068
#define POS_NUM_21_BEAUNE     2069
#define POS_NUM_21_DIJON      2070
#define POS_NUM_21_MONTBARD     2071
#define POS_NUM_22_DINAN      2072
#define POS_NUM_22_GUINGAMP     2073
#define POS_NUM_22_LANNION      2074
#define POS_NUM_22_SAINT_BRIEUC     2075
#define POS_NUM_23_AUBUSSON     2076
#define POS_NUM_23_GUERET     2077
#define POS_NUM_24_BERGERAC     2078
#define POS_NUM_24_NONTRON      2079
#define POS_NUM_24_PERIGUEUX      2080
#define POS_NUM_24_SARLAT_LA_CANEDA     2081
#define POS_NUM_25_BESANCON     2082
#define POS_NUM_25_MONTBELIARD      2083
#define POS_NUM_25_PONTARLIER     2084
#define POS_NUM_26_DIE      2085
#define POS_NUM_26_NYONS      2086
#define POS_NUM_26_VALENCE      2087
#define POS_NUM_27_LES_ANDELYS      2088
#define POS_NUM_27_ANDELYS      2089
#define POS_NUM_27_BERNAY     2090
#define POS_NUM_27_EVREUX     2091
#define POS_NUM_28_CHARTRES     2092
#define POS_NUM_28_CHATEAUDUN     2093
#define POS_NUM_28_DREUX      2094
#define POS_NUM_28_NOGENT_LE_ROTROU     2095
#define POS_NUM_29_BREST      2096
#define POS_NUM_29_CHATEAULIN     2097
#define POS_NUM_29_MORLAIX      2098
#define POS_NUM_29_QUIMPER      2099
#define POS_NUM_30_ALES     2100
#define POS_NUM_30_NIMES      2101
#define POS_NUM_30_LE_VIGAN     2102
#define POS_NUM_30_VIGAN      2103
#define POS_NUM_31_MURET      2104
#define POS_NUM_31_SAINT_GAUDENS      2105
#define POS_NUM_31_TOULOUSE     2106
#define POS_NUM_32_AUCH     2107
#define POS_NUM_32_CONDOM     2108
#define POS_NUM_32_MIRANDE      2109
#define POS_NUM_33_ARCACHON     2110
#define POS_NUM_33_BLAYE      2111
#define POS_NUM_33_BORDEAUX     2112
#define POS_NUM_33_LANGON     2113
#define POS_NUM_33_LESPARRE_MEDOC     2114
#define POS_NUM_33_LIBOURNE     2115
#define POS_NUM_34_BEZIERS      2116
#define POS_NUM_34_LODEVE     2117
#define POS_NUM_34_MONTPELLIER      2118
#define POS_NUM_35_FOUGERES     2119
#define POS_NUM_35_REDON      2120
#define POS_NUM_35_RENNES     2121
#define POS_NUM_35_SAINT_MALO     2122
#define POS_NUM_36_LE_BLANC     2123
#define POS_NUM_36_BLANC      2124
#define POS_NUM_36_CHATEAUROUX      2125
#define POS_NUM_36_LA_CHATRE      2126
#define POS_NUM_36_ISSOUDUN     2127
#define POS_NUM_37_CHINON     2128
#define POS_NUM_37_LOCHES     2129
#define POS_NUM_37_TOURS      2130
#define POS_NUM_38_GRENOBLE     2131
#define POS_NUM_38_LA_TOUR_DU_PIN     2132
#define POS_NUM_38_VIENNE     2133
#define POS_NUM_39_DOLE     2134
#define POS_NUM_39_LONS_LE_SAUNIER      2135
#define POS_NUM_39_SAINT_CLAUDE     2136
#define POS_NUM_40_DAX      2137
#define POS_NUM_40_MONT_DE_MARSAN     2138
#define POS_NUM_41_BLOIS      2139
#define POS_NUM_41_ROMORANTIN_LANTHENAY     2140
#define POS_NUM_41_VENDOME      2141
#define POS_NUM_42_MONTBRISON     2142
#define POS_NUM_42_ROANNE     2143
#define POS_NUM_42_SAINT_ETIENNE      2144
#define POS_NUM_43_BRIOUDE      2145
#define POS_NUM_43_LE_PUY_EN_VELAY      2146
#define POS_NUM_43_PUY_EN_VELAY     2147
#define POS_NUM_43_YSSINGEAUX     2148
#define POS_NUM_44_ANCENIS      2149
#define POS_NUM_44_CHATEAUBRIANT      2150
#define POS_NUM_44_NANTES     2151
#define POS_NUM_44_SAINT_NAZAIRE      2152
#define POS_NUM_45_MONTARGIS      2153
#define POS_NUM_45_ORLEANS      2154
#define POS_NUM_45_PITHIVIERS     2155
#define POS_NUM_46_CAHORS     2156
#define POS_NUM_46_FIGEAC     2157
#define POS_NUM_46_GOURDON      2158
#define POS_NUM_47_AGEN     2159
#define POS_NUM_47_MARMANDE     2160
#define POS_NUM_47_NERAC      2161
#define POS_NUM_47_VILLENEUVE_SUR_LOT     2162
#define POS_NUM_48_FLORAC     2163
#define POS_NUM_48_MENDE      2164
#define POS_NUM_49_ANGERS     2165
#define POS_NUM_49_CHOLET     2166
#define POS_NUM_49_SAUMUR     2167
#define POS_NUM_49_SEGRE      2168
#define POS_NUM_50_AVRANCHES      2169
#define POS_NUM_50_CHERBOURG_OCTEVILLE      2170
#define POS_NUM_50_COUTANCES      2171
#define POS_NUM_50_SAINT_LO     2172
#define POS_NUM_51_CHALONS_EN_CHAMPAGNE     2173
#define POS_NUM_51_EPERNAY      2174
#define POS_NUM_51_REIMS      2175
#define POS_NUM_51_SAINTE_MENEHOULD     2176
#define POS_NUM_51_VITRY_LE_FRANCOIS      2177
#define POS_NUM_52_CHAUMONT     2178
#define POS_NUM_52_LANGRES      2179
#define POS_NUM_52_SAINT_DIZIER     2180
#define POS_NUM_53_CHATEAU_GONTIER      2181
#define POS_NUM_53_LAVAL      2182
#define POS_NUM_53_MAYENNE      2183
#define POS_NUM_54_BRIEY      2184
#define POS_NUM_54_LUNEVILLE      2185
#define POS_NUM_54_NANCY      2186
#define POS_NUM_54_TOUL     2187
#define POS_NUM_55_BAR_LE_DUC     2188
#define POS_NUM_55_COMMERCY     2189
#define POS_NUM_55_VERDUN     2190
#define POS_NUM_56_LORIENT      2191
#define POS_NUM_56_PONTIVY      2192
#define POS_NUM_56_VANNES     2193
#define POS_NUM_57_BOULAY_MOSELLE     2194
#define POS_NUM_57_CHATEAU_SALINS     2195
#define POS_NUM_57_FORBACH      2196
#define POS_NUM_57_METZ     2197
#define POS_NUM_57_SARREBOURG     2198
#define POS_NUM_57_SARREGUEMINES      2199
#define POS_NUM_57_THIONVILLE     2200
#define POS_NUM_58_CHATEAU_CHINON     2201
#define POS_NUM_58_CLAMECY      2202
#define POS_NUM_58_COSNE_COURS_SUR_LOIRE      2203
#define POS_NUM_58_NEVERS     2204
#define POS_NUM_59_AVESNES_SUR_HELPE      2205
#define POS_NUM_59_CAMBRAI      2206
#define POS_NUM_59_DOUAI      2207
#define POS_NUM_59_DUNKERQUE      2208
#define POS_NUM_59_LILLE      2209
#define POS_NUM_59_VALENCIENNES     2210
#define POS_NUM_60_BEAUVAIS     2211
#define POS_NUM_60_CLERMONT     2212
#define POS_NUM_60_COMPIEGNE      2213
#define POS_NUM_60_SENLIS     2214
#define POS_NUM_61_ALENCON      2215
#define POS_NUM_61_ARGENTAN     2216
#define POS_NUM_61_MORTAGNE_AU_PERCHE     2217
#define POS_NUM_62_ARRAS      2218
#define POS_NUM_62_BETHUNE      2219
#define POS_NUM_62_BOULOGNE_SUR_MER     2220
#define POS_NUM_62_CALAIS     2221
#define POS_NUM_62_LENS     2222
#define POS_NUM_62_MONTREUIL      2223
#define POS_NUM_62_SAINT_OMER     2224
#define POS_NUM_63_AMBERT     2225
#define POS_NUM_63_CLERMONT_FERRAND     2226
#define POS_NUM_63_ISSOIRE      2227
#define POS_NUM_63_RIOM     2228
#define POS_NUM_63_THIERS     2229
#define POS_NUM_64_BAYONNE      2230
#define POS_NUM_64_OLORON_SAINTE_MARIE      2231
#define POS_NUM_64_PAU      2232
#define POS_NUM_65_ARGELES_GAZOST     2233
#define POS_NUM_65_BAGNERES_DE_BIGORRE      2234
#define POS_NUM_65_TARBES     2235
#define POS_NUM_66_CERET      2236
#define POS_NUM_66_PERPIGNAN      2237
#define POS_NUM_66_PRADES     2238
#define POS_NUM_67_HAGUENAU     2239
#define POS_NUM_67_MOLSHEIM     2240
#define POS_NUM_67_SAVERNE      2241
#define POS_NUM_67_SELESTAT     2242
#define POS_NUM_67_STRASBOURG     2243
#define POS_NUM_67_WISSEMBOURG      2244
#define POS_NUM_68_ALTKIRCH     2245
#define POS_NUM_68_COLMAR     2246
#define POS_NUM_68_GUEBWILLER     2247
#define POS_NUM_68_MULHOUSE     2248
#define POS_NUM_68_RIBEAUVILLE      2249
#define POS_NUM_68_THANN      2250
#define POS_NUM_69_LYON     2251
#define POS_NUM_69_VILLEFRANCHE_SUR_SAONE     2252
#define POS_NUM_70_LURE     2253
#define POS_NUM_70_VESOUL     2254
#define POS_NUM_71_AUTUN      2255
#define POS_NUM_71_CHALON_SUR_SAONE     2256
#define POS_NUM_71_CHAROLLES      2257
#define POS_NUM_71_LOUHANS      2258
#define POS_NUM_71_MACON      2259
#define POS_NUM_72_LA_FLECHE      2260
#define POS_NUM_72_MAMERS     2261
#define POS_NUM_72_LE_MANS      2262
#define POS_NUM_72_MANS     2263
#define POS_NUM_73_ALBERTVILLE      2264
#define POS_NUM_73_CHAMBERY     2265
#define POS_NUM_73_SAINT_JEAN_DE_MAURIENNE      2266
#define POS_NUM_74_ANNECY     2267
#define POS_NUM_74_BONNEVILLE     2268
#define POS_NUM_74_SAINT_JULIEN_EN_GENEVOIS     2269
#define POS_NUM_74_THONON_LES_BAINS     2270
#define POS_NUM_75_PARIS      2271
#define POS_NUM_76_DIEPPE     2272
#define POS_NUM_76_LE_HAVRE     2273
#define POS_NUM_76_HAVRE      2274
#define POS_NUM_76_ROUEN      2275
#define POS_NUM_77_FONTAINEBLEAU      2276
#define POS_NUM_77_MEAUX      2277
#define POS_NUM_77_MELUN      2278
#define POS_NUM_77_PROVINS      2279
#define POS_NUM_77_TORCY      2280
#define POS_NUM_78_MANTES_LA_JOLIE      2281
#define POS_NUM_78_RAMBOUILLET      2282
#define POS_NUM_78_SAINT_GERMAIN_EN_LAYE      2283
#define POS_NUM_78_VERSAILLES     2284
#define POS_NUM_79_BRESSUIRE      2285
#define POS_NUM_79_NIORT      2286
#define POS_NUM_79_PARTHENAY      2287
#define POS_NUM_80_ABBEVILLE      2288
#define POS_NUM_80_AMIENS     2289
#define POS_NUM_80_MONTDIDIER     2290
#define POS_NUM_80_PERONNE      2291
#define POS_NUM_81_ALBI     2292
#define POS_NUM_81_CASTRES      2293
#define POS_NUM_82_CASTELSARRASIN     2294
#define POS_NUM_82_MONTAUBAN      2295
#define POS_NUM_83_BRIGNOLES      2296
#define POS_NUM_83_DRAGUIGNAN     2297
#define POS_NUM_83_TOULON     2298
#define POS_NUM_84_APT      2299
#define POS_NUM_84_AVIGNON      2300
#define POS_NUM_84_CARPENTRAS     2301
#define POS_NUM_85_FONTENAY_LE_COMTE      2302
#define POS_NUM_85_LA_ROCHE_SUR_YON     2303
#define POS_NUM_85_LES_SABLES_D_OLONNE      2304
#define POS_NUM_85_SABLES_D_OLONNE      2305
#define POS_NUM_86_CHATELLERAULT      2306
#define POS_NUM_86_MONTMORILLON     2307
#define POS_NUM_86_POITIERS     2308
#define POS_NUM_87_BELLAC     2309
#define POS_NUM_87_LIMOGES      2310
#define POS_NUM_87_ROCHECHOUART     2311
#define POS_NUM_88_EPINAL     2312
#define POS_NUM_88_NEUFCHATEAU      2313
#define POS_NUM_88_SAINT_DIE_DES_VOSGES     2314
#define POS_NUM_89_AUXERRE      2315
#define POS_NUM_89_AVALLON      2316
#define POS_NUM_89_SENS     2317
#define POS_NUM_90_BELFORT      2318
#define POS_NUM_91_ETAMPES      2319
#define POS_NUM_91_EVRY     2320
#define POS_NUM_91_PALAISEAU      2321
#define POS_NUM_92_ANTONY     2322
#define POS_NUM_92_BOULOGNE_BILLANCOURT     2323
#define POS_NUM_92_NANTERRE     2324
#define POS_NUM_93_BOBIGNY      2325
#define POS_NUM_93_LE_RAINCY      2326
#define POS_NUM_93_RAINCY     2327
#define POS_NUM_93_SAINT_DENIS      2328
#define POS_NUM_94_CRETEIL      2329
#define POS_NUM_94_L_HAY_LES_ROSES      2330
#define POS_NUM_94_NOGENT_SUR_MARNE     2331
#define POS_NUM_95_ARGENTEUIL     2332
#define POS_NUM_95_CERGY_PONTOISE     2333
#define POS_NUM_95_SARCELLES      2334

// 484 defines prefixed

// TODO: Add 'POS2_NUM_84_D_AVIGNON' ;-)
#define POS2_NUM_D_ALLONNES  1500
#define POS2_NUM_D_ANDELU 1501
#define POS2_NUM_D_AUFFARGIS  1502
#define POS2_NUM_D_AUTEUIL  1503
#define POS2_NUM_D_AUTOUILLET 1504
#define POS2_NUM_D_ELANCOURT  1505
#define POS2_NUM_D_ELANCOURT_LA_CLE_DE_SAINT_PIERRE 1506
#define POS2_NUM_D_ELANCOURT_VILLAGE  1507
#define POS2_NUM_D_ETANG_LA_VILLE 1508
#define POS2_NUM_D_OSMOY  1509
#define POS2_NUM_DE_PORT_MARLY  1510
#define POS2_NUM_DES_BREVIAIRES 1511
#define POS2_NUM_DES_CLAYES_SOUS_BOIS 1512
#define POS2_NUM_DES_ESSARTS_LE_ROI 1513
#define POS2_NUM_DES_LOGES_EN_JOSAS 1514
#define POS2_NUM_DES_MESNULS  1515
#define POS2_NUM_DES_MOLIERES 1516
#define POS2_NUM_DES_ULMES  1517
#define POS2_NUM_DU_MESNIL_SAINT_DENIS  1518
#define POS2_NUM_DU_PECQ  1519
#define POS2_NUM_DU_PERRAY_EN_YVELINES  1520
#define POS2_NUM_DU_TREMBLAY_SUR_MAULDRE  1521
#define POS2_NUM_DU_VESINET 1522
#define POS2_NUM_13_D_AIX_EN_PROVENCE 3000
#define POS2_NUM_13_D_ARLES 3001
#define POS2_NUM_13_D_ISTRES  3002
#define POS2_NUM_15_D_AURILLAC  3003
#define POS2_NUM_16_D_ANGOULEME 3004
#define POS2_NUM_19_D_USSEL 3005
#define POS2_NUM_2A_D_AJACCIO 3006
#define POS2_NUM_27_D_EVREUX  3007
#define POS2_NUM_27_DES_ANDELYS 3008
#define POS2_NUM_30_D_ALES  3009
#define POS2_NUM_30_DU_VIGAN  3010
#define POS2_NUM_32_D_AUCH  3011
#define POS2_NUM_33_D_ARCACHON  3012
#define POS2_NUM_36_D_ISSOUDUN  3013
#define POS2_NUM_36_DU_BLANC  3014
#define POS2_NUM_43_D_YSSINGEAUX  3015
#define POS2_NUM_43_DU_PUY_EN_VELAY 3016
#define POS2_NUM_44_D_ANCENIS 3017
#define POS2_NUM_45_D_ORLEANS 3018
#define POS2_NUM_47_D_AGEN  3019
#define POS2_NUM_49_D_ANGERS  3020
#define POS2_NUM_50_D_AVRANCHES 3021
#define POS2_NUM_51_D_EPERNAY 3022
#define POS2_NUM_59_D_AVESNES_SUR_HELPE 3023
#define POS2_NUM_61_D_ALENCON 3024
#define POS2_NUM_61_D_ARGENTAN  3025
#define POS2_NUM_62_D_ARRAS 3026
#define POS2_NUM_63_D_AMBERT  3027
#define POS2_NUM_63_D_ISSOIRE 3028
#define POS2_NUM_64_D_OLORON_SAINTE_MARIE 3029
#define POS2_NUM_65_D_ARGELES_GAZOST  3030
#define POS2_NUM_68_D_ALTKIRCH  3031
#define POS2_NUM_71_D_AUTUN 3032
#define POS2_NUM_72_DU_MANS 3033
#define POS2_NUM_73_D_ALBERTVILLE 3034
#define POS2_NUM_74_D_ANNECY  3035
#define POS2_NUM_76_DU_HAVRE  3036
#define POS2_NUM_80_D_ABBEVILLE 3037
#define POS2_NUM_80_D_AMIENS  3038
#define POS2_NUM_81_D_ALBI  3039
#define POS2_NUM_84_D_APT 3040
#define POS2_NUM_85_DES_SABLES_D_OLONNE 3041
#define POS2_NUM_88_D_EPINAL  3042
#define POS2_NUM_89_D_AUXERRE 3043
#define POS2_NUM_89_D_AVALLON 3044
#define POS2_NUM_92_D_ANTONY  3045
#define POS2_NUM_93_DU_RAINCY 3046
#define POS2_NUM_95_D_ARGENTEUIL  3047

// 71 defines

typedef enum {
  BASE_POS_ALLONNES = 0,
  BASE_POS_BRAIN_SUR_ALLONNES,
  BASE_POS_LA_BREILLE_LES_PINS,
  BASE_POS_CHACE,
  BASE_POS_CHENEHUTTE_TREVES_CUNAULT,
  BASE_POS_DISTRE,
  BASE_POS_GENNES,
  BASE_POS_MEIGNE,
  BASE_POS_MONTSOREAU,
  BASE_POS_NEUILLE,
  BASE_POS_PARNAY,
  BASE_POS_ROU_MARSON,
  BASE_POS_SAINT_CLEMENT_DES_LEVEES,
  BASE_POS_SAINT_MARTIN_DE_LA_PLACE,
  BASE_POS_SAUMUR,
  BASE_POS_SOUZAY_CHAMPIGNY,
  BASE_POS_TURQUANT,
  BASE_POS_LES_ULMES,
  BASE_POS_ULMES,
  BASE_POS_VARENNES_SUR_LOIRE,
  BASE_POS_VARRAINS,
  BASE_POS_VERRIE,
  BASE_POS_VILLEBERNIER,
  BASE_POS_VIVY,
  BASE_POS_ANDELU,
  BASE_POS_AUFFARGIS,
  BASE_POS_AUTEUIL,
  BASE_POS_AUTOUILLET,
  BASE_POS_BAILLY,
  BASE_POS_BAZOCHES_SUR_GUYONNE,
  BASE_POS_BEHOUST,
  BASE_POS_BEYNES,
  BASE_POS_BOIS_D_ARCY,
  BASE_POS_BOISSY_SANS_AVOIR,
  BASE_POS_BOUGIVAL,
  BASE_POS_LES_BREVIAIRES,
  BASE_POS_BREVIAIRES,
  BASE_POS_BUC,
  BASE_POS_LA_CELLE_SAINT_CLOUD,
  BASE_POS_CERNAY_LA_VILLE,
  BASE_POS_CHATEAUFORT,
  BASE_POS_CHAVENAY,
  BASE_POS_LE_CHESNAY,
  BASE_POS_CHESNAY,
  BASE_POS_CHEVREUSE,
  BASE_POS_CHOISEL,
  BASE_POS_LES_CLAYES_SOUS_BOIS,
  BASE_POS_CLAYES_SOUS_BOIS,
  BASE_POS_COIGNIERES_VILLAGE,
  BASE_POS_COIGNIERES,
  BASE_POS_CRESPIERES,
  BASE_POS_CROISSY_SUR_SEINE,
  BASE_POS_DAMPIERRE_EN_YVELINES,
  BASE_POS_DAVRON,
  BASE_POS_ELANCOURT,
  BASE_POS_ELANCOURT_VILLAGE,
  BASE_POS_ELANCOURT_LA_CLE_DE_SAINT_PIERRE,
  BASE_POS_LA_COLLINE_D_ELANCOURT,
  BASE_POS_LES_ESSARTS_LE_ROI,
  BASE_POS_ESSARTS_LE_ROI,
  BASE_POS_L_ETANG_LA_VILLE,
  BASE_POS_FEUCHEROLLES,
  BASE_POS_FLEXANVILLE,
  BASE_POS_FONTENAY_LE_FLEURY,
  BASE_POS_FOURQUEUX,
  BASE_POS_GALLUIS,
  BASE_POS_GAMBAISEUIL,
  BASE_POS_GARANCIERES,
  BASE_POS_GOUPILLIERES,
  BASE_POS_GROSROUVRE,
  BASE_POS_GUYANCOURT,
  BASE_POS_HARGEVILLE,
  BASE_POS_JOUARS_PONTCHARTRAIN,
  BASE_POS_LEVIS_SAINT_NOM,
  BASE_POS_LES_LOGES_EN_JOSAS,
  BASE_POS_LOGES_EN_JOSAS,
  BASE_POS_LOUVECIENNES,
  BASE_POS_MAGNY_LES_HAMEAUX,
  BASE_POS_MAGNY_VILLAGE,
  BASE_POS_MARCQ,
  BASE_POS_MAREIL_LE_GUYON,
  BASE_POS_MAREIL_MARLY,
  BASE_POS_MAREIL_SUR_MAULDRE,
  BASE_POS_MARLY_LE_ROI,
  BASE_POS_MAUREPAS_VILLAGE,
  BASE_POS_MAUREPAS,
  BASE_POS_MERE,
  BASE_POS_LE_MESNIL_SAINT_DENIS,
  BASE_POS_MESNIL_SAINT_DENIS,
  BASE_POS_LES_MESNULS,
  BASE_POS_MESNULS,
  BASE_POS_MILLEMONT,
  BASE_POS_MILON_LA_CHAPELLE,
  BASE_POS_MONTAINVILLE,
  BASE_POS_MONTFORT_L_AMAURY,
  BASE_POS_MONTIGNY_LE_BRETONNEUX,
  BASE_POS_NEAUPHLE_LE_CHATEAU,
  BASE_POS_NEAUPHLE_LE_VIEUX,
  BASE_POS_NOISY_LE_ROI,
  BASE_POS_OSMOY,
  BASE_POS_LE_PECQ,
  BASE_POS_PECQ,
  BASE_POS_LE_PERRAY_EN_YVELINES,
  BASE_POS_PERRAY_EN_YVELINES,
  BASE_POS_PLAISIR,
  BASE_POS_POIGNY_LA_FORET,
  BASE_POS_LE_PORT_MARLY,
  BASE_POS_PORT_MARLY,
  BASE_POS_LA_QUEUE_LES_YVELINES,
  BASE_POS_RENNEMOULIN,
  BASE_POS_ROCQUENCOURT,
  BASE_POS_SAINT_BENOIT,
  BASE_POS_SAINT_CYR_L_ECOLE,
  BASE_POS_SAINT_FORGET,
  BASE_POS_SAINT_GERMAIN_DE_LA_GRANGE,
  BASE_POS_SAINT_GERMAIN_EN_LAYE,
  BASE_POS_SAINT_HUBERT,
  BASE_POS_SAINT_LAMBERT_DES_BOIS,
  BASE_POS_SAINT_LEGER_EN_YVELINES,
  BASE_POS_SAINT_MARTIN_DES_CHAMPS,
  BASE_POS_SAINT_NOM_LA_BRETECHE,
  BASE_POS_SAINT_REMY_L_HONORE,
  BASE_POS_SAINT_REMY_LES_CHEVREUSE,
  BASE_POS_SAULX_MARCHAIS,
  BASE_POS_SENLISSE,
  BASE_POS_THIVERVAL_GRIGNON,
  BASE_POS_THOIRY,
  BASE_POS_TOUSSUS_LE_NOBLE,
  BASE_POS_TRAPPES,
  BASE_POS_LE_TREMBLAY_SUR_MAULDRE,
  BASE_POS_TREMBLAY_SUR_MAULDRE,
  BASE_POS_LA_VERRIERE,
  BASE_POS_VERSAILLES,
  BASE_POS_LE_VESINET,
  BASE_POS_VESINET,
  BASE_POS_VICQ,
  BASE_POS_VIEILLE_EGLISE_EN_YVELINES,
  BASE_POS_VILLEPREUX,
  BASE_POS_VILLIERS_LE_MAHIEU,
  BASE_POS_VILLIERS_SAINT_FREDERIC,
  BASE_POS_VOISINS_LE_BRETONNEUX,
  BASE_POS_BOULLAY_LES_TROUX,
  BASE_POS_GIF_SUR_YVETTE,
  BASE_POS_GOMETZ_LA_VILLE,
  BASE_POS_GOMETZ_LE_CHATEL,
  BASE_POS_LES_MOLIERES,
  BASE_POS_MOLIERES,
  BASE_POS_SAINT_AUBIN,
  BASE_POS_VILLIERS_LE_BACLE,
  BASE_POS_01_BELLEY,
  BASE_POS_01_BOURG_EN_BRESSE,
  BASE_POS_01_GEX,
  BASE_POS_01_NANTUA,
  BASE_POS_02_CHATEAU_THIERRY,
  BASE_POS_02_LAON,
  BASE_POS_02_SAINT_QUENTIN,
  BASE_POS_02_SOISSONS,
  BASE_POS_02_VERVINS,
  BASE_POS_03_MONTLUCON,
  BASE_POS_03_MOULINS,
  BASE_POS_03_VICHY,
  BASE_POS_04_BARCELONNETTE,
  BASE_POS_04_CASTELLANE,
  BASE_POS_04_DIGNE_LES_BAINS,
  BASE_POS_04_FORCALQUIER,
  BASE_POS_05_BRIANCON,
  BASE_POS_05_GAP,
  BASE_POS_06_GRASSE,
  BASE_POS_06_NICE,
  BASE_POS_07_LARGENTIERE,
  BASE_POS_07_PRIVAS,
  BASE_POS_07_TOURNON_SUR_RHONE,
  BASE_POS_08_CHARLEVILLE_MEZIERES,
  BASE_POS_08_RETHEL,
  BASE_POS_08_SEDAN,
  BASE_POS_08_VOUZIERS,
  BASE_POS_09_FOIX,
  BASE_POS_09_PAMIERS,
  BASE_POS_09_SAINT_GIRONS,
  BASE_POS_10_BAR_SUR_AUBE,
  BASE_POS_10_NOGENT_SUR_SEINE,
  BASE_POS_10_TROYES,
  BASE_POS_11_CARCASSONNE,
  BASE_POS_11_LIMOUX,
  BASE_POS_11_NARBONNE,
  BASE_POS_12_MILLAU,
  BASE_POS_12_RODEZ,
  BASE_POS_12_VILLEFRANCHE_DE_ROUERGUE,
  BASE_POS_13_AIX_EN_PROVENCE,
  BASE_POS_13_ARLES,
  BASE_POS_13_ISTRES,
  BASE_POS_13_MARSEILLE,
  BASE_POS_14_BAYEUX,
  BASE_POS_14_CAEN,
  BASE_POS_14_LISIEUX,
  BASE_POS_14_VIRE,
  BASE_POS_15_AURILLAC,
  BASE_POS_15_MAURIAC,
  BASE_POS_15_SAINT_FLOUR,
  BASE_POS_16_ANGOULEME,
  BASE_POS_16_COGNAC,
  BASE_POS_16_CONFOLENS,
  BASE_POS_17_JONZAC,
  BASE_POS_17_ROCHEFORT,
  BASE_POS_17_LA_ROCHELLE,
  BASE_POS_17_SAINT_JEAN_D_ANGELY,
  BASE_POS_17_SAINTES,
  BASE_POS_18_BOURGES,
  BASE_POS_18_SAINT_AMAND_MONTROND,
  BASE_POS_18_VIERZON,
  BASE_POS_19_BRIVE_LA_GAILLARDE,
  BASE_POS_19_TULLE,
  BASE_POS_19_USSEL,
  BASE_POS_2A_AJACCIO,
  BASE_POS_2A_SARTENE,
  BASE_POS_2B_BASTIA,
  BASE_POS_2B_CALVI,
  BASE_POS_2B_CORTE,
  BASE_POS_21_BEAUNE,
  BASE_POS_21_DIJON,
  BASE_POS_21_MONTBARD,
  BASE_POS_22_DINAN,
  BASE_POS_22_GUINGAMP,
  BASE_POS_22_LANNION,
  BASE_POS_22_SAINT_BRIEUC,
  BASE_POS_23_AUBUSSON,
  BASE_POS_23_GUERET,
  BASE_POS_24_BERGERAC,
  BASE_POS_24_NONTRON,
  BASE_POS_24_PERIGUEUX,
  BASE_POS_24_SARLAT_LA_CANEDA,
  BASE_POS_25_BESANCON,
  BASE_POS_25_MONTBELIARD,
  BASE_POS_25_PONTARLIER,
  BASE_POS_26_DIE,
  BASE_POS_26_NYONS,
  BASE_POS_26_VALENCE,
  BASE_POS_27_LES_ANDELYS,
  BASE_POS_27_ANDELYS,
  BASE_POS_27_BERNAY,
  BASE_POS_27_EVREUX,
  BASE_POS_28_CHARTRES,
  BASE_POS_28_CHATEAUDUN,
  BASE_POS_28_DREUX,
  BASE_POS_28_NOGENT_LE_ROTROU,
  BASE_POS_29_BREST,
  BASE_POS_29_CHATEAULIN,
  BASE_POS_29_MORLAIX,
  BASE_POS_29_QUIMPER,
  BASE_POS_30_ALES,
  BASE_POS_30_NIMES,
  BASE_POS_30_LE_VIGAN,
  BASE_POS_30_VIGAN,
  BASE_POS_31_MURET,
  BASE_POS_31_SAINT_GAUDENS,
  BASE_POS_31_TOULOUSE,
  BASE_POS_32_AUCH,
  BASE_POS_32_CONDOM,
  BASE_POS_32_MIRANDE,
  BASE_POS_33_ARCACHON,
  BASE_POS_33_BLAYE,
  BASE_POS_33_BORDEAUX,
  BASE_POS_33_LANGON,
  BASE_POS_33_LESPARRE_MEDOC,
  BASE_POS_33_LIBOURNE,
  BASE_POS_34_BEZIERS,
  BASE_POS_34_LODEVE,
  BASE_POS_34_MONTPELLIER,
  BASE_POS_35_FOUGERES,
  BASE_POS_35_REDON,
  BASE_POS_35_RENNES,
  BASE_POS_35_SAINT_MALO,
  BASE_POS_36_LE_BLANC,
  BASE_POS_36_BLANC,
  BASE_POS_36_CHATEAUROUX,
  BASE_POS_36_LA_CHATRE,
  BASE_POS_36_ISSOUDUN,
  BASE_POS_37_CHINON,
  BASE_POS_37_LOCHES,
  BASE_POS_37_TOURS,
  BASE_POS_38_GRENOBLE,
  BASE_POS_38_LA_TOUR_DU_PIN,
  BASE_POS_38_VIENNE,
  BASE_POS_39_DOLE,
  BASE_POS_39_LONS_LE_SAUNIER,
  BASE_POS_39_SAINT_CLAUDE,
  BASE_POS_40_DAX,
  BASE_POS_40_MONT_DE_MARSAN,
  BASE_POS_41_BLOIS,
  BASE_POS_41_ROMORANTIN_LANTHENAY,
  BASE_POS_41_VENDOME,
  BASE_POS_42_MONTBRISON,
  BASE_POS_42_ROANNE,
  BASE_POS_42_SAINT_ETIENNE,
  BASE_POS_43_BRIOUDE,
  BASE_POS_43_LE_PUY_EN_VELAY,
  BASE_POS_43_PUY_EN_VELAY,
  BASE_POS_43_YSSINGEAUX,
  BASE_POS_44_ANCENIS,
  BASE_POS_44_CHATEAUBRIANT,
  BASE_POS_44_NANTES,
  BASE_POS_44_SAINT_NAZAIRE,
  BASE_POS_45_MONTARGIS,
  BASE_POS_45_ORLEANS,
  BASE_POS_45_PITHIVIERS,
  BASE_POS_46_CAHORS,
  BASE_POS_46_FIGEAC,
  BASE_POS_46_GOURDON,
  BASE_POS_47_AGEN,
  BASE_POS_47_MARMANDE,
  BASE_POS_47_NERAC,
  BASE_POS_47_VILLENEUVE_SUR_LOT,
  BASE_POS_48_FLORAC,
  BASE_POS_48_MENDE,
  BASE_POS_49_ANGERS,
  BASE_POS_49_CHOLET,
  BASE_POS_49_SAUMUR,
  BASE_POS_49_SEGRE,
  BASE_POS_50_AVRANCHES,
  BASE_POS_50_CHERBOURG_OCTEVILLE,
  BASE_POS_50_COUTANCES,
  BASE_POS_50_SAINT_LO,
  BASE_POS_51_CHALONS_EN_CHAMPAGNE,
  BASE_POS_51_EPERNAY,
  BASE_POS_51_REIMS,
  BASE_POS_51_SAINTE_MENEHOULD,
  BASE_POS_51_VITRY_LE_FRANCOIS,
  BASE_POS_52_CHAUMONT,
  BASE_POS_52_LANGRES,
  BASE_POS_52_SAINT_DIZIER,
  BASE_POS_53_CHATEAU_GONTIER,
  BASE_POS_53_LAVAL,
  BASE_POS_53_MAYENNE,
  BASE_POS_54_BRIEY,
  BASE_POS_54_LUNEVILLE,
  BASE_POS_54_NANCY,
  BASE_POS_54_TOUL,
  BASE_POS_55_BAR_LE_DUC,
  BASE_POS_55_COMMERCY,
  BASE_POS_55_VERDUN,
  BASE_POS_56_LORIENT,
  BASE_POS_56_PONTIVY,
  BASE_POS_56_VANNES,
  BASE_POS_57_BOULAY_MOSELLE,
  BASE_POS_57_CHATEAU_SALINS,
  BASE_POS_57_FORBACH,
  BASE_POS_57_METZ,
  BASE_POS_57_SARREBOURG,
  BASE_POS_57_SARREGUEMINES,
  BASE_POS_57_THIONVILLE,
  BASE_POS_58_CHATEAU_CHINON,
  BASE_POS_58_CLAMECY,
  BASE_POS_58_COSNE_COURS_SUR_LOIRE,
  BASE_POS_58_NEVERS,
  BASE_POS_59_AVESNES_SUR_HELPE,
  BASE_POS_59_CAMBRAI,
  BASE_POS_59_DOUAI,
  BASE_POS_59_DUNKERQUE,
  BASE_POS_59_LILLE,
  BASE_POS_59_VALENCIENNES,
  BASE_POS_60_BEAUVAIS,
  BASE_POS_60_CLERMONT,
  BASE_POS_60_COMPIEGNE,
  BASE_POS_60_SENLIS,
  BASE_POS_61_ALENCON,
  BASE_POS_61_ARGENTAN,
  BASE_POS_61_MORTAGNE_AU_PERCHE,
  BASE_POS_62_ARRAS,
  BASE_POS_62_BETHUNE,
  BASE_POS_62_BOULOGNE_SUR_MER,
  BASE_POS_62_CALAIS,
  BASE_POS_62_LENS,
  BASE_POS_62_MONTREUIL,
  BASE_POS_62_SAINT_OMER,
  BASE_POS_63_AMBERT,
  BASE_POS_63_CLERMONT_FERRAND,
  BASE_POS_63_ISSOIRE,
  BASE_POS_63_RIOM,
  BASE_POS_63_THIERS,
  BASE_POS_64_BAYONNE,
  BASE_POS_64_OLORON_SAINTE_MARIE,
  BASE_POS_64_PAU,
  BASE_POS_65_ARGELES_GAZOST,
  BASE_POS_65_BAGNERES_DE_BIGORRE,
  BASE_POS_65_TARBES,
  BASE_POS_66_CERET,
  BASE_POS_66_PERPIGNAN,
  BASE_POS_66_PRADES,
  BASE_POS_67_HAGUENAU,
  BASE_POS_67_MOLSHEIM,
  BASE_POS_67_SAVERNE,
  BASE_POS_67_SELESTAT,
  BASE_POS_67_STRASBOURG,
  BASE_POS_67_WISSEMBOURG,
  BASE_POS_68_ALTKIRCH,
  BASE_POS_68_COLMAR,
  BASE_POS_68_GUEBWILLER,
  BASE_POS_68_MULHOUSE,
  BASE_POS_68_RIBEAUVILLE,
  BASE_POS_68_THANN,
  BASE_POS_69_LYON,
  BASE_POS_69_VILLEFRANCHE_SUR_SAONE,
  BASE_POS_70_LURE,
  BASE_POS_70_VESOUL,
  BASE_POS_71_AUTUN,
  BASE_POS_71_CHALON_SUR_SAONE,
  BASE_POS_71_CHAROLLES,
  BASE_POS_71_LOUHANS,
  BASE_POS_71_MACON,
  BASE_POS_72_LA_FLECHE,
  BASE_POS_72_MAMERS,
  BASE_POS_72_LE_MANS,
  BASE_POS_72_MANS,
  BASE_POS_73_ALBERTVILLE,
  BASE_POS_73_CHAMBERY,
  BASE_POS_73_SAINT_JEAN_DE_MAURIENNE,
  BASE_POS_74_ANNECY,
  BASE_POS_74_BONNEVILLE,
  BASE_POS_74_SAINT_JULIEN_EN_GENEVOIS,
  BASE_POS_74_THONON_LES_BAINS,
  BASE_POS_75_PARIS,
  BASE_POS_76_DIEPPE,
  BASE_POS_76_LE_HAVRE,
  BASE_POS_76_HAVRE,
  BASE_POS_76_ROUEN,
  BASE_POS_77_FONTAINEBLEAU,
  BASE_POS_77_MEAUX,
  BASE_POS_77_MELUN,
  BASE_POS_77_PROVINS,
  BASE_POS_77_TORCY,
  BASE_POS_78_MANTES_LA_JOLIE,
  BASE_POS_78_RAMBOUILLET,
  BASE_POS_78_SAINT_GERMAIN_EN_LAYE,
  BASE_POS_78_VERSAILLES,
  BASE_POS_79_BRESSUIRE,
  BASE_POS_79_NIORT,
  BASE_POS_79_PARTHENAY,
  BASE_POS_80_ABBEVILLE,
  BASE_POS_80_AMIENS,
  BASE_POS_80_MONTDIDIER,
  BASE_POS_80_PERONNE,
  BASE_POS_81_ALBI,
  BASE_POS_81_CASTRES,
  BASE_POS_82_CASTELSARRASIN,
  BASE_POS_82_MONTAUBAN,
  BASE_POS_83_BRIGNOLES,
  BASE_POS_83_DRAGUIGNAN,
  BASE_POS_83_TOULON,
  BASE_POS_84_APT,
  BASE_POS_84_AVIGNON,
  BASE_POS_84_CARPENTRAS,
  BASE_POS_85_FONTENAY_LE_COMTE,
  BASE_POS_85_LA_ROCHE_SUR_YON,
  BASE_POS_85_LES_SABLES_D_OLONNE,
  BASE_POS_85_SABLES_D_OLONNE,
  BASE_POS_86_CHATELLERAULT,
  BASE_POS_86_MONTMORILLON,
  BASE_POS_86_POITIERS,
  BASE_POS_87_BELLAC,
  BASE_POS_87_LIMOGES,
  BASE_POS_87_ROCHECHOUART,
  BASE_POS_88_EPINAL,
  BASE_POS_88_NEUFCHATEAU,
  BASE_POS_88_SAINT_DIE_DES_VOSGES,
  BASE_POS_89_AUXERRE,
  BASE_POS_89_AVALLON,
  BASE_POS_89_SENS,
  BASE_POS_90_BELFORT,
  BASE_POS_91_ETAMPES,
  BASE_POS_91_EVRY,
  BASE_POS_91_PALAISEAU,
  BASE_POS_92_ANTONY,
  BASE_POS_92_BOULOGNE_BILLANCOURT,
  BASE_POS_92_NANTERRE,
  BASE_POS_93_BOBIGNY,
  BASE_POS_93_LE_RAINCY,
  BASE_POS_93_RAINCY,
  BASE_POS_93_SAINT_DENIS,
  BASE_POS_94_CRETEIL,
  BASE_POS_94_L_HAY_LES_ROSES,
  BASE_POS_94_NOGENT_SUR_MARNE,
  BASE_POS_95_ARGENTEUIL,
  BASE_POS_95_CERGY_PONTOISE,
  BASE_POS_95_SARCELLES,

  BASE_LAST_PROMPT_POS_COMMUNES
} ENUM_BASE_POS_COMMUNES;

// 484 enums

typedef enum {
  BASE_POS_D_ALLONNES = 0,
  BASE_POS_D_ANDELU,
  BASE_POS_D_AUFFARGIS,
  BASE_POS_D_AUTEUIL,
  BASE_POS_D_AUTOUILLET,
  BASE_POS_D_ELANCOURT,
  BASE_POS_D_ELANCOURT_LA_CLE_DE_SAINT_PIERRE,
  BASE_POS_D_ELANCOURT_VILLAGE,
  BASE_POS_D_ETANG_LA_VILLE,
  BASE_POS_D_OSMOY,
  BASE_POS_DE_PORT_MARLY,
  BASE_POS_DES_BREVIAIRES,
  BASE_POS_DES_CLAYES_SOUS_BOIS,
  BASE_POS_DES_ESSARTS_LE_ROI,
  BASE_POS_DES_LOGES_EN_JOSAS,
  BASE_POS_DES_MESNULS,
  BASE_POS_DES_MOLIERES,
  BASE_POS_DES_ULMES,
  BASE_POS_DU_MESNIL_SAINT_DENIS,
  BASE_POS_DU_PECQ,
  BASE_POS_DU_PERRAY_EN_YVELINES,
  BASE_POS_DU_TREMBLAY_SUR_MAULDRE,
  BASE_POS_DU_VESINET,
  BASE_POS_13_D_AIX_EN_PROVENCE,
  BASE_POS_13_D_ARLES,
  BASE_POS_13_D_ISTRES,
  BASE_POS_15_D_AURILLAC,
  BASE_POS_16_D_ANGOULEME,
  BASE_POS_19_D_USSEL,
  BASE_POS_2A_D_AJACCIO,
  BASE_POS_27_D_EVREUX,
  BASE_POS_27_DES_ANDELYS,
  BASE_POS_30_D_ALES,
  BASE_POS_30_DU_VIGAN,
  BASE_POS_32_D_AUCH,
  BASE_POS_33_D_ARCACHON,
  BASE_POS_36_D_ISSOUDUN,
  BASE_POS_36_DU_BLANC,
  BASE_POS_43_D_YSSINGEAUX,
  BASE_POS_43_DU_PUY_EN_VELAY,
  BASE_POS_44_D_ANCENIS,
  BASE_POS_45_D_ORLEANS,
  BASE_POS_47_D_AGEN,
  BASE_POS_49_D_ANGERS,
  BASE_POS_50_D_AVRANCHES,
  BASE_POS_51_D_EPERNAY,
  BASE_POS_59_D_AVESNES_SUR_HELPE,
  BASE_POS_61_D_ALENCON,
  BASE_POS_61_D_ARGENTAN,
  BASE_POS_62_D_ARRAS,
  BASE_POS_63_D_AMBERT,
  BASE_POS_63_D_ISSOIRE,
  BASE_POS_64_D_OLORON_SAINTE_MARIE,
  BASE_POS_65_D_ARGELES_GAZOST,
  BASE_POS_68_D_ALTKIRCH,
  BASE_POS_71_D_AUTUN,
  BASE_POS_72_DU_MANS,
  BASE_POS_73_D_ALBERTVILLE,
  BASE_POS_74_D_ANNECY,
  BASE_POS_76_DU_HAVRE,
  BASE_POS_80_D_ABBEVILLE,
  BASE_POS_80_D_AMIENS,
  BASE_POS_81_D_ALBI,
  BASE_POS_84_D_APT,
  BASE_POS_85_DES_SABLES_D_OLONNE,
  BASE_POS_88_D_EPINAL,
  BASE_POS_89_D_AUXERRE,
  BASE_POS_89_D_AVALLON,
  BASE_POS_92_D_ANTONY,
  BASE_POS_93_DU_RAINCY,
  BASE_POS_95_D_ARGENTEUIL,
  BASE_LAST_PROMPT_POS2_COMMUNES
} ENUM_BASE_POS2_COMMUNES;

// 71 enumerations

static const ST_PROMPTS_DEF              g__prompts_pos_communes[BASE_LAST_PROMPT_POS_COMMUNES + 1] =
{
  { BASE_POS_ALLONNES, { 0x14, 0x80 + POS_NUM_ALLONNES / 256, POS_NUM_ALLONNES % 256 }, "08/1000_Allonnes.mp3", true, 1, 873L },
  { BASE_POS_BRAIN_SUR_ALLONNES, { 0x14, 0x80 + POS_NUM_BRAIN_SUR_ALLONNES / 256, POS_NUM_BRAIN_SUR_ALLONNES % 256 }, "08/1001_Brain_sur_Allonnes.mp3", true, 2, 1657L },
  { BASE_POS_LA_BREILLE_LES_PINS, { 0x14, 0x80 + POS_NUM_LA_BREILLE_LES_PINS / 256, POS_NUM_LA_BREILLE_LES_PINS % 256 }, "08/1002_la_Breille_les_Pins.mp3", true, 2, 1539L },
  { BASE_POS_CHACE, { 0x14, 0x80 + POS_NUM_CHACE / 256, POS_NUM_CHACE % 256 }, "08/1003_Chace.mp3", true, 1, 951L },
  { BASE_POS_CHENEHUTTE_TREVES_CUNAULT, { 0x14, 0x80 + POS_NUM_CHENEHUTTE_TREVES_CUNAULT / 256, POS_NUM_CHENEHUTTE_TREVES_CUNAULT % 256 }, "08/1004_Chenehutte_Treves_Cunault.mp3", true, 2, 2088L },
  { BASE_POS_DISTRE, { 0x14, 0x80 + POS_NUM_DISTRE / 256, POS_NUM_DISTRE % 256 }, "08/1005_Distre.mp3", true, 1, 912L },
  { BASE_POS_GENNES, { 0x14, 0x80 + POS_NUM_GENNES / 256, POS_NUM_GENNES % 256 }, "08/1006_Gennes.mp3", true, 1, 834L },
  { BASE_POS_MEIGNE, { 0x14, 0x80 + POS_NUM_MEIGNE / 256, POS_NUM_MEIGNE % 256 }, "08/1007_Meigne.mp3", true, 1, 834L },
  { BASE_POS_MONTSOREAU, { 0x14, 0x80 + POS_NUM_MONTSOREAU / 256, POS_NUM_MONTSOREAU % 256 }, "08/1008_Montsoreau.mp3", true, 1, 990L },
  { BASE_POS_NEUILLE, { 0x14, 0x80 + POS_NUM_NEUILLE / 256, POS_NUM_NEUILLE % 256 }, "08/1009_Neuille.mp3", true, 1, 794L },
  { BASE_POS_PARNAY, { 0x14, 0x80 + POS_NUM_PARNAY / 256, POS_NUM_PARNAY % 256 }, "08/1010_Parnay.mp3", true, 1, 834L },
  { BASE_POS_ROU_MARSON, { 0x14, 0x80 + POS_NUM_ROU_MARSON / 256, POS_NUM_ROU_MARSON % 256 }, "08/1011_Rou_Marson.mp3", true, 1, 1186L },
  { BASE_POS_SAINT_CLEMENT_DES_LEVEES, { 0x14, 0x80 + POS_NUM_SAINT_CLEMENT_DES_LEVEES / 256, POS_NUM_SAINT_CLEMENT_DES_LEVEES % 256 }, "08/1012_Saint_Clement_des_Levees.mp3", true, 2, 1970L },
  { BASE_POS_SAINT_MARTIN_DE_LA_PLACE, { 0x14, 0x80 + POS_NUM_SAINT_MARTIN_DE_LA_PLACE / 256, POS_NUM_SAINT_MARTIN_DE_LA_PLACE % 256 }, "08/1013_Saint_Martin_de_la_Place.mp3", true, 2, 2088L },
  { BASE_POS_SAUMUR, { 0x14, 0x80 + POS_NUM_SAUMUR / 256, POS_NUM_SAUMUR % 256 }, "09/1014_Saumur.mp3", true, 1, 1108L },
  { BASE_POS_SOUZAY_CHAMPIGNY, { 0x14, 0x80 + POS_NUM_SOUZAY_CHAMPIGNY / 256, POS_NUM_SOUZAY_CHAMPIGNY % 256 }, "08/1015_Souzay_Champigny.mp3", true, 2, 1617L },
  { BASE_POS_TURQUANT, { 0x14, 0x80 + POS_NUM_TURQUANT / 256, POS_NUM_TURQUANT % 256 }, "08/1016_Turquant.mp3", true, 1, 834L },
  { BASE_POS_LES_ULMES, { 0x14, 0x80 + POS_NUM_LES_ULMES / 256, POS_NUM_LES_ULMES % 256 }, "08/1017_les_Ulmes.mp3", true, 1, 1147L },
  { BASE_POS_ULMES, { 0x14, 0x80 + POS_NUM_ULMES / 256, POS_NUM_ULMES % 256 }, "08/1018_Ulmes.mp3", true, 1, 716L },
  { BASE_POS_VARENNES_SUR_LOIRE, { 0x14, 0x80 + POS_NUM_VARENNES_SUR_LOIRE / 256, POS_NUM_VARENNES_SUR_LOIRE % 256 }, "08/1019_Varennes_sur_Loire.mp3", true, 2, 1970L },
  { BASE_POS_VARRAINS, { 0x14, 0x80 + POS_NUM_VARRAINS / 256, POS_NUM_VARRAINS % 256 }, "08/1020_Varrains.mp3", true, 1, 755L },
  { BASE_POS_VERRIE, { 0x14, 0x80 + POS_NUM_VERRIE / 256, POS_NUM_VERRIE % 256 }, "08/1021_Verrie.mp3", true, 1, 755L },
  { BASE_POS_VILLEBERNIER, { 0x14, 0x80 + POS_NUM_VILLEBERNIER / 256, POS_NUM_VILLEBERNIER % 256 }, "08/1022_Villebernier.mp3", true, 1, 1304L },
  { BASE_POS_VIVY, { 0x14, 0x80 + POS_NUM_VIVY / 256, POS_NUM_VIVY % 256 }, "08/1023_Vivy.mp3", true, 1, 755L },
  { BASE_POS_ANDELU, { 0x14, 0x80 + POS_NUM_ANDELU / 256, POS_NUM_ANDELU % 256 }, "08/1024_Andelu.mp3", true, 1, 834L },
  { BASE_POS_AUFFARGIS, { 0x14, 0x80 + POS_NUM_AUFFARGIS / 256, POS_NUM_AUFFARGIS % 256 }, "08/1025_Auffargis.mp3", true, 1, 1304L },
  { BASE_POS_AUTEUIL, { 0x14, 0x80 + POS_NUM_AUTEUIL / 256, POS_NUM_AUTEUIL % 256 }, "08/1026_Auteuil.mp3", true, 1, 834L },
  { BASE_POS_AUTOUILLET, { 0x14, 0x80 + POS_NUM_AUTOUILLET / 256, POS_NUM_AUTOUILLET % 256 }, "08/1027_Autouillet.mp3", true, 1, 951L },
  { BASE_POS_BAILLY, { 0x14, 0x80 + POS_NUM_BAILLY / 256, POS_NUM_BAILLY % 256 }, "08/1028_Bailly.mp3", true, 1, 834L },
  { BASE_POS_BAZOCHES_SUR_GUYONNE, { 0x14, 0x80 + POS_NUM_BAZOCHES_SUR_GUYONNE / 256, POS_NUM_BAZOCHES_SUR_GUYONNE % 256 }, "08/1029_Bazoches_sur_Guyonne.mp3", true, 2, 2088L },
  { BASE_POS_BEHOUST, { 0x14, 0x80 + POS_NUM_BEHOUST / 256, POS_NUM_BEHOUST % 256 }, "08/1030_Behoust.mp3", true, 1, 1108L },
  { BASE_POS_BEYNES, { 0x14, 0x80 + POS_NUM_BEYNES / 256, POS_NUM_BEYNES % 256 }, "08/1031_Beynes.mp3", true, 1, 794L },
  { BASE_POS_BOIS_D_ARCY, { 0x14, 0x80 + POS_NUM_BOIS_D_ARCY / 256, POS_NUM_BOIS_D_ARCY % 256 }, "08/1032_Bois_d_Arcy.mp3", true, 1, 1147L },
  { BASE_POS_BOISSY_SANS_AVOIR, { 0x14, 0x80 + POS_NUM_BOISSY_SANS_AVOIR / 256, POS_NUM_BOISSY_SANS_AVOIR % 256 }, "08/1033_Boissy_sans_Avoir.mp3", true, 2, 2127L },
  { BASE_POS_BOUGIVAL, { 0x14, 0x80 + POS_NUM_BOUGIVAL / 256, POS_NUM_BOUGIVAL % 256 }, "08/1034_Bougival.mp3", true, 1, 1226L },
  { BASE_POS_LES_BREVIAIRES, { 0x14, 0x80 + POS_NUM_LES_BREVIAIRES / 256, POS_NUM_LES_BREVIAIRES % 256 }, "08/1035_les_Breviaires.mp3", true, 1, 1421L },
  { BASE_POS_BREVIAIRES, { 0x14, 0x80 + POS_NUM_BREVIAIRES / 256, POS_NUM_BREVIAIRES % 256 }, "08/1036_Breviaires.mp3", true, 1, 1226L },
  { BASE_POS_BUC, { 0x14, 0x80 + POS_NUM_BUC / 256, POS_NUM_BUC % 256 }, "08/1037_Buc.mp3", true, 1, 755L },
  { BASE_POS_LA_CELLE_SAINT_CLOUD, { 0x14, 0x80 + POS_NUM_LA_CELLE_SAINT_CLOUD / 256, POS_NUM_LA_CELLE_SAINT_CLOUD % 256 }, "08/1038_la_Celle_Saint_Cloud.mp3", true, 2, 1696L },
  { BASE_POS_CERNAY_LA_VILLE, { 0x14, 0x80 + POS_NUM_CERNAY_LA_VILLE / 256, POS_NUM_CERNAY_LA_VILLE % 256 }, "08/1039_Cernay_la_Ville.mp3", true, 2, 1500L },
  { BASE_POS_CHATEAUFORT, { 0x14, 0x80 + POS_NUM_CHATEAUFORT / 256, POS_NUM_CHATEAUFORT % 256 }, "08/1040_Chateaufort.mp3", true, 1, 1382L },
  { BASE_POS_CHAVENAY, { 0x14, 0x80 + POS_NUM_CHAVENAY / 256, POS_NUM_CHAVENAY % 256 }, "08/1041_Chavenay.mp3", true, 1, 1069L },
  { BASE_POS_LE_CHESNAY, { 0x14, 0x80 + POS_NUM_LE_CHESNAY / 256, POS_NUM_LE_CHESNAY % 256 }, "08/1042_le_Chesnay.mp3", true, 1, 1147L },
  { BASE_POS_CHESNAY, { 0x14, 0x80 + POS_NUM_CHESNAY / 256, POS_NUM_CHESNAY % 256 }, "08/1043_Chesnay.mp3", true, 1, 912L },
  { BASE_POS_CHEVREUSE, { 0x14, 0x80 + POS_NUM_CHEVREUSE / 256, POS_NUM_CHEVREUSE % 256 }, "08/1044_Chevreuse.mp3", true, 1, 1265L },
  { BASE_POS_CHOISEL, { 0x14, 0x80 + POS_NUM_CHOISEL / 256, POS_NUM_CHOISEL % 256 }, "08/1045_Choisel.mp3", true, 1, 1108L },
  { BASE_POS_LES_CLAYES_SOUS_BOIS, { 0x14, 0x80 + POS_NUM_LES_CLAYES_SOUS_BOIS / 256, POS_NUM_LES_CLAYES_SOUS_BOIS % 256 }, "08/1046_les_Clayes_sous_Bois.mp3", true, 2, 1578L },
  { BASE_POS_CLAYES_SOUS_BOIS, { 0x14, 0x80 + POS_NUM_CLAYES_SOUS_BOIS / 256, POS_NUM_CLAYES_SOUS_BOIS % 256 }, "08/1047_Clayes_sous_Bois.mp3", true, 1, 1226L },
  { BASE_POS_COIGNIERES_VILLAGE, { 0x14, 0x80 + POS_NUM_COIGNIERES_VILLAGE / 256, POS_NUM_COIGNIERES_VILLAGE % 256 }, "08/1048_Coignieres_Village.mp3", true, 2, 1617L },
  { BASE_POS_COIGNIERES, { 0x14, 0x80 + POS_NUM_COIGNIERES / 256, POS_NUM_COIGNIERES % 256 }, "08/1049_Coignieres.mp3", true, 1, 1069L },
  { BASE_POS_CRESPIERES, { 0x14, 0x80 + POS_NUM_CRESPIERES / 256, POS_NUM_CRESPIERES % 256 }, "08/1050_Crespieres.mp3", true, 1, 1304L },
  { BASE_POS_CROISSY_SUR_SEINE, { 0x14, 0x80 + POS_NUM_CROISSY_SUR_SEINE / 256, POS_NUM_CROISSY_SUR_SEINE % 256 }, "08/1051_Croissy_sur_Seine.mp3", true, 2, 1774L },
  { BASE_POS_DAMPIERRE_EN_YVELINES, { 0x14, 0x80 + POS_NUM_DAMPIERRE_EN_YVELINES / 256, POS_NUM_DAMPIERRE_EN_YVELINES % 256 }, "08/1052_Dampierre_en_Yvelines.mp3", true, 2, 2009L },
  { BASE_POS_DAVRON, { 0x14, 0x80 + POS_NUM_DAVRON / 256, POS_NUM_DAVRON % 256 }, "08/1053_Davron.mp3", true, 1, 990L },
  { BASE_POS_ELANCOURT, { 0x14, 0x80 + POS_NUM_ELANCOURT / 256, POS_NUM_ELANCOURT % 256 }, "08/1054_Elancourt.mp3", true, 1, 1147L },
  { BASE_POS_ELANCOURT_VILLAGE, { 0x14, 0x80 + POS_NUM_ELANCOURT_VILLAGE / 256, POS_NUM_ELANCOURT_VILLAGE % 256 }, "08/1055_Elancourt_Village.mp3", true, 2, 1852L },
  { BASE_POS_ELANCOURT_LA_CLE_DE_SAINT_PIERRE, { 0x14, 0x80 + POS_NUM_ELANCOURT_LA_CLE_DE_SAINT_PIERRE / 256, POS_NUM_ELANCOURT_LA_CLE_DE_SAINT_PIERRE % 256 }, "08/1056_Elancourt_La_Cle_de_Saint_Pierre.mp3", true, 3, 2754L },
  { BASE_POS_LA_COLLINE_D_ELANCOURT, { 0x14, 0x80 + POS_NUM_LA_COLLINE_D_ELANCOURT / 256, POS_NUM_LA_COLLINE_D_ELANCOURT % 256 }, "08/1057_la_Colline_d_Elancourt.mp3", true, 2, 1970L },
  { BASE_POS_LES_ESSARTS_LE_ROI, { 0x14, 0x80 + POS_NUM_LES_ESSARTS_LE_ROI / 256, POS_NUM_LES_ESSARTS_LE_ROI % 256 }, "08/1058_les_Essarts_le_Roi.mp3", true, 2, 1657L },
  { BASE_POS_ESSARTS_LE_ROI, { 0x14, 0x80 + POS_NUM_ESSARTS_LE_ROI / 256, POS_NUM_ESSARTS_LE_ROI % 256 }, "08/1059_Essarts_le_Roi.mp3", true, 1, 1382L },
  { BASE_POS_L_ETANG_LA_VILLE, { 0x14, 0x80 + POS_NUM_L_ETANG_LA_VILLE / 256, POS_NUM_L_ETANG_LA_VILLE % 256 }, "08/1060_l_Etang_la_Ville.mp3", true, 2, 1539L },
  { BASE_POS_FEUCHEROLLES, { 0x14, 0x80 + POS_NUM_FEUCHEROLLES / 256, POS_NUM_FEUCHEROLLES % 256 }, "08/1061_Feucherolles.mp3", true, 1, 1265L },
  { BASE_POS_FLEXANVILLE, { 0x14, 0x80 + POS_NUM_FLEXANVILLE / 256, POS_NUM_FLEXANVILLE % 256 }, "08/1062_Flexanville.mp3", true, 1, 1382L },
  { BASE_POS_FONTENAY_LE_FLEURY, { 0x14, 0x80 + POS_NUM_FONTENAY_LE_FLEURY / 256, POS_NUM_FONTENAY_LE_FLEURY % 256 }, "08/1063_Fontenay_le_Fleury.mp3", true, 2, 1892L },
  { BASE_POS_FOURQUEUX, { 0x14, 0x80 + POS_NUM_FOURQUEUX / 256, POS_NUM_FOURQUEUX % 256 }, "08/1064_Fourqueux.mp3", true, 1, 951L },
  { BASE_POS_GALLUIS, { 0x14, 0x80 + POS_NUM_GALLUIS / 256, POS_NUM_GALLUIS % 256 }, "08/1065_Galluis.mp3", true, 1, 834L },
  { BASE_POS_GAMBAISEUIL, { 0x14, 0x80 + POS_NUM_GAMBAISEUIL / 256, POS_NUM_GAMBAISEUIL % 256 }, "08/1066_Gambaiseuil.mp3", true, 1, 1304L },
  { BASE_POS_GARANCIERES, { 0x14, 0x80 + POS_NUM_GARANCIERES / 256, POS_NUM_GARANCIERES % 256 }, "08/1067_Garancieres.mp3", true, 1, 1304L },
  { BASE_POS_GOUPILLIERES, { 0x14, 0x80 + POS_NUM_GOUPILLIERES / 256, POS_NUM_GOUPILLIERES % 256 }, "08/1068_Goupillieres.mp3", true, 1, 1304L },
  { BASE_POS_GROSROUVRE, { 0x14, 0x80 + POS_NUM_GROSROUVRE / 256, POS_NUM_GROSROUVRE % 256 }, "08/1069_Grosrouvre.mp3", true, 1, 1226L },
  { BASE_POS_GUYANCOURT, { 0x14, 0x80 + POS_NUM_GUYANCOURT / 256, POS_NUM_GUYANCOURT % 256 }, "08/1070_Guyancourt.mp3", true, 1, 1304L },
  { BASE_POS_HARGEVILLE, { 0x14, 0x80 + POS_NUM_HARGEVILLE / 256, POS_NUM_HARGEVILLE % 256 }, "08/1071_Hargeville.mp3", true, 1, 1186L },
  { BASE_POS_JOUARS_PONTCHARTRAIN, { 0x14, 0x80 + POS_NUM_JOUARS_PONTCHARTRAIN / 256, POS_NUM_JOUARS_PONTCHARTRAIN % 256 }, "08/1072_Jouars_Pontchartrain.mp3", true, 2, 1970L },
  { BASE_POS_LEVIS_SAINT_NOM, { 0x14, 0x80 + POS_NUM_LEVIS_SAINT_NOM / 256, POS_NUM_LEVIS_SAINT_NOM % 256 }, "08/1073_Levis_Saint_Nom.mp3", true, 2, 1500L },
  { BASE_POS_LES_LOGES_EN_JOSAS, { 0x14, 0x80 + POS_NUM_LES_LOGES_EN_JOSAS / 256, POS_NUM_LES_LOGES_EN_JOSAS % 256 }, "08/1074_les_Loges_en_Josas.mp3", true, 2, 1970L },
  { BASE_POS_LOGES_EN_JOSAS, { 0x14, 0x80 + POS_NUM_LOGES_EN_JOSAS / 256, POS_NUM_LOGES_EN_JOSAS % 256 }, "08/1075_Loges_en_Josas.mp3", true, 2, 1696L },
  { BASE_POS_LOUVECIENNES, { 0x14, 0x80 + POS_NUM_LOUVECIENNES / 256, POS_NUM_LOUVECIENNES % 256 }, "08/1076_Louveciennes.mp3", true, 1, 1265L },
  { BASE_POS_MAGNY_LES_HAMEAUX, { 0x14, 0x80 + POS_NUM_MAGNY_LES_HAMEAUX / 256, POS_NUM_MAGNY_LES_HAMEAUX % 256 }, "08/1077_Magny_les_Hameaux.mp3", true, 1, 1382L },
  { BASE_POS_MAGNY_VILLAGE, { 0x14, 0x80 + POS_NUM_MAGNY_VILLAGE / 256, POS_NUM_MAGNY_VILLAGE % 256 }, "08/1078_Magny_Village.mp3", true, 1, 1461L },
  { BASE_POS_MARCQ, { 0x14, 0x80 + POS_NUM_MARCQ / 256, POS_NUM_MARCQ % 256 }, "08/1079_Marcq.mp3", true, 1, 834L },
  { BASE_POS_MAREIL_LE_GUYON, { 0x14, 0x80 + POS_NUM_MAREIL_LE_GUYON / 256, POS_NUM_MAREIL_LE_GUYON % 256 }, "08/1080_Mareil_le_Guyon.mp3", true, 2, 1696L },
  { BASE_POS_MAREIL_MARLY, { 0x14, 0x80 + POS_NUM_MAREIL_MARLY / 256, POS_NUM_MAREIL_MARLY % 256 }, "08/1081_Mareil_Marly.mp3", true, 1, 1421L },
  { BASE_POS_MAREIL_SUR_MAULDRE, { 0x14, 0x80 + POS_NUM_MAREIL_SUR_MAULDRE / 256, POS_NUM_MAREIL_SUR_MAULDRE % 256 }, "08/1082_Mareil_sur_Mauldre.mp3", true, 2, 1892L },
  { BASE_POS_MARLY_LE_ROI, { 0x14, 0x80 + POS_NUM_MARLY_LE_ROI / 256, POS_NUM_MARLY_LE_ROI % 256 }, "08/1083_Marly_le_Roi.mp3", true, 1, 1343L },
  { BASE_POS_MAUREPAS_VILLAGE, { 0x14, 0x80 + POS_NUM_MAUREPAS_VILLAGE / 256, POS_NUM_MAUREPAS_VILLAGE % 256 }, "08/1084_Maurepas_Village.mp3", true, 2, 1696L },
  { BASE_POS_MAUREPAS, { 0x14, 0x80 + POS_NUM_MAUREPAS / 256, POS_NUM_MAUREPAS % 256 }, "08/1085_Maurepas.mp3", true, 1, 1108L },
  { BASE_POS_MERE, { 0x14, 0x80 + POS_NUM_MERE / 256, POS_NUM_MERE % 256 }, "08/1086_Mere.mp3", true, 1, 873L },
  { BASE_POS_LE_MESNIL_SAINT_DENIS, { 0x14, 0x80 + POS_NUM_LE_MESNIL_SAINT_DENIS / 256, POS_NUM_LE_MESNIL_SAINT_DENIS % 256 }, "08/1087_le_Mesnil_Saint_Denis.mp3", true, 2, 1813L },
  { BASE_POS_MESNIL_SAINT_DENIS, { 0x14, 0x80 + POS_NUM_MESNIL_SAINT_DENIS / 256, POS_NUM_MESNIL_SAINT_DENIS % 256 }, "08/1088_Mesnil_Saint_Denis.mp3", true, 2, 1617L },
  { BASE_POS_LES_MESNULS, { 0x14, 0x80 + POS_NUM_LES_MESNULS / 256, POS_NUM_LES_MESNULS % 256 }, "08/1089_les_Mesnuls.mp3", true, 1, 1147L },
  { BASE_POS_MESNULS, { 0x14, 0x80 + POS_NUM_MESNULS / 256, POS_NUM_MESNULS % 256 }, "08/1090_Mesnuls.mp3", true, 1, 951L },
  { BASE_POS_MILLEMONT, { 0x14, 0x80 + POS_NUM_MILLEMONT / 256, POS_NUM_MILLEMONT % 256 }, "08/1091_Millemont.mp3", true, 1, 912L },
  { BASE_POS_MILON_LA_CHAPELLE, { 0x14, 0x80 + POS_NUM_MILON_LA_CHAPELLE / 256, POS_NUM_MILON_LA_CHAPELLE % 256 }, "08/1092_Milon_la_Chapelle.mp3", true, 2, 1657L },
  { BASE_POS_MONTAINVILLE, { 0x14, 0x80 + POS_NUM_MONTAINVILLE / 256, POS_NUM_MONTAINVILLE % 256 }, "08/1093_Montainville.mp3", true, 1, 1382L },
  { BASE_POS_MONTFORT_L_AMAURY, { 0x14, 0x80 + POS_NUM_MONTFORT_L_AMAURY / 256, POS_NUM_MONTFORT_L_AMAURY % 256 }, "08/1094_Montfort_l_Amaury.mp3", true, 2, 1500L },
  { BASE_POS_MONTIGNY_LE_BRETONNEUX, { 0x14, 0x80 + POS_NUM_MONTIGNY_LE_BRETONNEUX / 256, POS_NUM_MONTIGNY_LE_BRETONNEUX % 256 }, "08/1095_Montigny_le_Bretonneux.mp3", true, 2, 2205L },
  { BASE_POS_NEAUPHLE_LE_CHATEAU, { 0x14, 0x80 + POS_NUM_NEAUPHLE_LE_CHATEAU / 256, POS_NUM_NEAUPHLE_LE_CHATEAU % 256 }, "08/1096_Neauphle_le_Chateau.mp3", true, 2, 1696L },
  { BASE_POS_NEAUPHLE_LE_VIEUX, { 0x14, 0x80 + POS_NUM_NEAUPHLE_LE_VIEUX / 256, POS_NUM_NEAUPHLE_LE_VIEUX % 256 }, "08/1097_Neauphle_le_Vieux.mp3", true, 1, 1461L },
  { BASE_POS_NOISY_LE_ROI, { 0x14, 0x80 + POS_NUM_NOISY_LE_ROI / 256, POS_NUM_NOISY_LE_ROI % 256 }, "08/1098_Noisy_le_Roi.mp3", true, 1, 1382L },
  { BASE_POS_OSMOY, { 0x14, 0x80 + POS_NUM_OSMOY / 256, POS_NUM_OSMOY % 256 }, "08/1099_Osmoy.mp3", true, 1, 873L },
  { BASE_POS_LE_PECQ, { 0x14, 0x80 + POS_NUM_LE_PECQ / 256, POS_NUM_LE_PECQ % 256 }, "08/1100_le_Pecq.mp3", true, 1, 1108L },
  { BASE_POS_PECQ, { 0x14, 0x80 + POS_NUM_PECQ / 256, POS_NUM_PECQ % 256 }, "08/1101_Pecq.mp3", true, 1, 794L },
  { BASE_POS_LE_PERRAY_EN_YVELINES, { 0x14, 0x80 + POS_NUM_LE_PERRAY_EN_YVELINES / 256, POS_NUM_LE_PERRAY_EN_YVELINES % 256 }, "08/1102_le_Perray_en_Yvelines.mp3", true, 2, 1931L },
  { BASE_POS_PERRAY_EN_YVELINES, { 0x14, 0x80 + POS_NUM_PERRAY_EN_YVELINES / 256, POS_NUM_PERRAY_EN_YVELINES % 256 }, "08/1103_Perray_en_Yvelines.mp3", true, 2, 1539L },
  { BASE_POS_PLAISIR, { 0x14, 0x80 + POS_NUM_PLAISIR / 256, POS_NUM_PLAISIR % 256 }, "08/1104_Plaisir.mp3", true, 1, 1069L },
  { BASE_POS_POIGNY_LA_FORET, { 0x14, 0x80 + POS_NUM_POIGNY_LA_FORET / 256, POS_NUM_POIGNY_LA_FORET % 256 }, "08/1105_Poigny_la_Foret.mp3", true, 2, 1578L },
  { BASE_POS_LE_PORT_MARLY, { 0x14, 0x80 + POS_NUM_LE_PORT_MARLY / 256, POS_NUM_LE_PORT_MARLY % 256 }, "08/1106_le_Port_Marly.mp3", true, 1, 1382L },
  { BASE_POS_PORT_MARLY, { 0x14, 0x80 + POS_NUM_PORT_MARLY / 256, POS_NUM_PORT_MARLY % 256 }, "08/1107_Port_Marly.mp3", true, 1, 1030L },
  { BASE_POS_LA_QUEUE_LES_YVELINES, { 0x14, 0x80 + POS_NUM_LA_QUEUE_LES_YVELINES / 256, POS_NUM_LA_QUEUE_LES_YVELINES % 256 }, "08/1108_la_Queue_les_Yvelines.mp3", true, 2, 1657L },
  { BASE_POS_RENNEMOULIN, { 0x14, 0x80 + POS_NUM_RENNEMOULIN / 256, POS_NUM_RENNEMOULIN % 256 }, "08/1109_Rennemoulin.mp3", true, 1, 1147L },
  { BASE_POS_ROCQUENCOURT, { 0x14, 0x80 + POS_NUM_ROCQUENCOURT / 256, POS_NUM_ROCQUENCOURT % 256 }, "08/1110_Rocquencourt.mp3", true, 1, 1265L },
  { BASE_POS_SAINT_BENOIT, { 0x14, 0x80 + POS_NUM_SAINT_BENOIT / 256, POS_NUM_SAINT_BENOIT % 256 }, "08/1111_Saint_Benoit.mp3", true, 1, 1147L },
  { BASE_POS_SAINT_CYR_L_ECOLE, { 0x14, 0x80 + POS_NUM_SAINT_CYR_L_ECOLE / 256, POS_NUM_SAINT_CYR_L_ECOLE % 256 }, "08/1112_Saint_Cyr_l_Ecole.mp3", true, 2, 1696L },
  { BASE_POS_SAINT_FORGET, { 0x14, 0x80 + POS_NUM_SAINT_FORGET / 256, POS_NUM_SAINT_FORGET % 256 }, "08/1113_Saint_Forget.mp3", true, 1, 1343L },
  { BASE_POS_SAINT_GERMAIN_DE_LA_GRANGE, { 0x14, 0x80 + POS_NUM_SAINT_GERMAIN_DE_LA_GRANGE / 256, POS_NUM_SAINT_GERMAIN_DE_LA_GRANGE % 256 }, "08/1114_Saint_Germain_de_la_Grange.mp3", true, 2, 2244L },
  { BASE_POS_SAINT_GERMAIN_EN_LAYE, { 0x14, 0x80 + POS_NUM_SAINT_GERMAIN_EN_LAYE / 256, POS_NUM_SAINT_GERMAIN_EN_LAYE % 256 }, "09/1115_Saint_Germain_en_Laye.mp3", true, 2, 1617L },
  { BASE_POS_SAINT_HUBERT, { 0x14, 0x80 + POS_NUM_SAINT_HUBERT / 256, POS_NUM_SAINT_HUBERT % 256 }, "08/1116_Saint_Hubert.mp3", true, 1, 1461L },
  { BASE_POS_SAINT_LAMBERT_DES_BOIS, { 0x14, 0x80 + POS_NUM_SAINT_LAMBERT_DES_BOIS / 256, POS_NUM_SAINT_LAMBERT_DES_BOIS % 256 }, "08/1117_Saint_Lambert_des_Bois.mp3", true, 2, 1696L },
  { BASE_POS_SAINT_LEGER_EN_YVELINES, { 0x14, 0x80 + POS_NUM_SAINT_LEGER_EN_YVELINES / 256, POS_NUM_SAINT_LEGER_EN_YVELINES % 256 }, "08/1118_Saint_Leger_en_Yvelines.mp3", true, 2, 1892L },
  { BASE_POS_SAINT_MARTIN_DES_CHAMPS, { 0x14, 0x80 + POS_NUM_SAINT_MARTIN_DES_CHAMPS / 256, POS_NUM_SAINT_MARTIN_DES_CHAMPS % 256 }, "08/1119_Saint_Martin_des_Champs.mp3", true, 2, 1696L },
  { BASE_POS_SAINT_NOM_LA_BRETECHE, { 0x14, 0x80 + POS_NUM_SAINT_NOM_LA_BRETECHE / 256, POS_NUM_SAINT_NOM_LA_BRETECHE % 256 }, "08/1120_Saint_Nom_la_Breteche.mp3", true, 2, 2088L },
  { BASE_POS_SAINT_REMY_L_HONORE, { 0x14, 0x80 + POS_NUM_SAINT_REMY_L_HONORE / 256, POS_NUM_SAINT_REMY_L_HONORE % 256 }, "08/1121_Saint_Remy_l_Honore.mp3", true, 2, 1813L },
  { BASE_POS_SAINT_REMY_LES_CHEVREUSE, { 0x14, 0x80 + POS_NUM_SAINT_REMY_LES_CHEVREUSE / 256, POS_NUM_SAINT_REMY_LES_CHEVREUSE % 256 }, "08/1122_Saint_Remy_les_Chevreuse.mp3", true, 2, 2283L },
  { BASE_POS_SAULX_MARCHAIS, { 0x14, 0x80 + POS_NUM_SAULX_MARCHAIS / 256, POS_NUM_SAULX_MARCHAIS % 256 }, "08/1123_Saulx_Marchais.mp3", true, 2, 1617L },
  { BASE_POS_SENLISSE, { 0x14, 0x80 + POS_NUM_SENLISSE / 256, POS_NUM_SENLISSE % 256 }, "08/1124_Senlisse.mp3", true, 1, 1147L },
  { BASE_POS_THIVERVAL_GRIGNON, { 0x14, 0x80 + POS_NUM_THIVERVAL_GRIGNON / 256, POS_NUM_THIVERVAL_GRIGNON % 256 }, "08/1125_Thiverval_Grignon.mp3", true, 2, 1774L },
  { BASE_POS_THOIRY, { 0x14, 0x80 + POS_NUM_THOIRY / 256, POS_NUM_THOIRY % 256 }, "08/1126_Thoiry.mp3", true, 1, 873L },
  { BASE_POS_TOUSSUS_LE_NOBLE, { 0x14, 0x80 + POS_NUM_TOUSSUS_LE_NOBLE / 256, POS_NUM_TOUSSUS_LE_NOBLE % 256 }, "08/1127_Toussus_le_Noble.mp3", true, 2, 1539L },
  { BASE_POS_TRAPPES, { 0x14, 0x80 + POS_NUM_TRAPPES / 256, POS_NUM_TRAPPES % 256 }, "08/1128_Trappes.mp3", true, 1, 834L },
  { BASE_POS_LE_TREMBLAY_SUR_MAULDRE, { 0x14, 0x80 + POS_NUM_LE_TREMBLAY_SUR_MAULDRE / 256, POS_NUM_LE_TREMBLAY_SUR_MAULDRE % 256 }, "08/1129_le_Tremblay_sur_Mauldre.mp3", true, 2, 2205L },
  { BASE_POS_TREMBLAY_SUR_MAULDRE, { 0x14, 0x80 + POS_NUM_TREMBLAY_SUR_MAULDRE / 256, POS_NUM_TREMBLAY_SUR_MAULDRE % 256 }, "08/1130_Tremblay_sur_Mauldre.mp3", true, 2, 1970L },
  { BASE_POS_LA_VERRIERE, { 0x14, 0x80 + POS_NUM_LA_VERRIERE / 256, POS_NUM_LA_VERRIERE % 256 }, "08/1131_la_Verriere.mp3", true, 1, 1265L },
  { BASE_POS_VERSAILLES, { 0x14, 0x80 + POS_NUM_VERSAILLES / 256, POS_NUM_VERSAILLES % 256 }, "09/1132_Versailles.mp3", true, 1, 1108L },
  { BASE_POS_LE_VESINET, { 0x14, 0x80 + POS_NUM_LE_VESINET / 256, POS_NUM_LE_VESINET % 256 }, "08/1133_le_Vesinet.mp3", true, 1, 1265L },
  { BASE_POS_VESINET, { 0x14, 0x80 + POS_NUM_VESINET / 256, POS_NUM_VESINET % 256 }, "08/1134_Vesinet.mp3", true, 1, 990L },
  { BASE_POS_VICQ, { 0x14, 0x80 + POS_NUM_VICQ / 256, POS_NUM_VICQ % 256 }, "08/1135_Vicq.mp3", true, 1, 794L },
  { BASE_POS_VIEILLE_EGLISE_EN_YVELINES, { 0x14, 0x80 + POS_NUM_VIEILLE_EGLISE_EN_YVELINES / 256, POS_NUM_VIEILLE_EGLISE_EN_YVELINES % 256 }, "08/1136_Vieille_Eglise_en_Yvelines.mp3", true, 2, 2244L },
  { BASE_POS_VILLEPREUX, { 0x14, 0x80 + POS_NUM_VILLEPREUX / 256, POS_NUM_VILLEPREUX % 256 }, "08/1137_Villepreux.mp3", true, 1, 1147L },
  { BASE_POS_VILLIERS_LE_MAHIEU, { 0x14, 0x80 + POS_NUM_VILLIERS_LE_MAHIEU / 256, POS_NUM_VILLIERS_LE_MAHIEU % 256 }, "08/1138_Villiers_le_Mahieu.mp3", true, 2, 1500L },
  { BASE_POS_VILLIERS_SAINT_FREDERIC, { 0x14, 0x80 + POS_NUM_VILLIERS_SAINT_FREDERIC / 256, POS_NUM_VILLIERS_SAINT_FREDERIC % 256 }, "08/1139_Villiers_Saint_Frederic.mp3", true, 2, 2127L },
  { BASE_POS_VOISINS_LE_BRETONNEUX, { 0x14, 0x80 + POS_NUM_VOISINS_LE_BRETONNEUX / 256, POS_NUM_VOISINS_LE_BRETONNEUX % 256 }, "08/1140_Voisins_le_Bretonneux.mp3", true, 2, 1931L },
  { BASE_POS_BOULLAY_LES_TROUX, { 0x14, 0x80 + POS_NUM_BOULLAY_LES_TROUX / 256, POS_NUM_BOULLAY_LES_TROUX % 256 }, "08/1141_Boullay_les_Troux.mp3", true, 1, 1304L },
  { BASE_POS_GIF_SUR_YVETTE, { 0x14, 0x80 + POS_NUM_GIF_SUR_YVETTE / 256, POS_NUM_GIF_SUR_YVETTE % 256 }, "08/1142_Gif_sur_Yvette.mp3", true, 2, 1931L },
  { BASE_POS_GOMETZ_LA_VILLE, { 0x14, 0x80 + POS_NUM_GOMETZ_LA_VILLE / 256, POS_NUM_GOMETZ_LA_VILLE % 256 }, "08/1143_Gometz_la_Ville.mp3", true, 2, 1657L },
  { BASE_POS_GOMETZ_LE_CHATEL, { 0x14, 0x80 + POS_NUM_GOMETZ_LE_CHATEL / 256, POS_NUM_GOMETZ_LE_CHATEL % 256 }, "08/1144_Gometz_le_Chatel.mp3", true, 2, 1852L },
  { BASE_POS_LES_MOLIERES, { 0x14, 0x80 + POS_NUM_LES_MOLIERES / 256, POS_NUM_LES_MOLIERES % 256 }, "08/1145_les_Molieres.mp3", true, 1, 1343L },
  { BASE_POS_MOLIERES, { 0x14, 0x80 + POS_NUM_MOLIERES / 256, POS_NUM_MOLIERES % 256 }, "08/1146_Molieres.mp3", true, 1, 1186L },
  { BASE_POS_SAINT_AUBIN, { 0x14, 0x80 + POS_NUM_SAINT_AUBIN / 256, POS_NUM_SAINT_AUBIN % 256 }, "08/1147_Saint_Aubin.mp3", true, 1, 1226L },
  { BASE_POS_VILLIERS_LE_BACLE, { 0x14, 0x80 + POS_NUM_VILLIERS_LE_BACLE / 256, POS_NUM_VILLIERS_LE_BACLE % 256 }, "08/1148_Villiers_le_Bacle.mp3", true, 2, 1657L },
  { BASE_POS_01_BELLEY, { 0x14, 0x90 + POS_NUM_01_BELLEY / 256, POS_NUM_01_BELLEY % 256 }, "09/2000_01_Belley.mp3", true, 1, 1265L },
  { BASE_POS_01_BOURG_EN_BRESSE, { 0x14, 0x90 + POS_NUM_01_BOURG_EN_BRESSE / 256, POS_NUM_01_BOURG_EN_BRESSE % 256 }, "09/2001_01_Bourg_en_Bresse.mp3", true, 2, 1813L },
  { BASE_POS_01_GEX, { 0x14, 0x90 + POS_NUM_01_GEX / 256, POS_NUM_01_GEX % 256 }, "09/2002_01_Gex.mp3", true, 1, 1382L },
  { BASE_POS_01_NANTUA, { 0x14, 0x90 + POS_NUM_01_NANTUA / 256, POS_NUM_01_NANTUA % 256 }, "09/2003_01_Nantua.mp3", true, 1, 1382L },
  { BASE_POS_02_CHATEAU_THIERRY, { 0x14, 0x90 + POS_NUM_02_CHATEAU_THIERRY / 256, POS_NUM_02_CHATEAU_THIERRY % 256 }, "09/2004_02_Chateau_Thierry.mp3", true, 2, 2009L },
  { BASE_POS_02_LAON, { 0x14, 0x90 + POS_NUM_02_LAON / 256, POS_NUM_02_LAON % 256 }, "09/2005_02_Laon.mp3", true, 1, 1461L },
  { BASE_POS_02_SAINT_QUENTIN, { 0x14, 0x90 + POS_NUM_02_SAINT_QUENTIN / 256, POS_NUM_02_SAINT_QUENTIN % 256 }, "09/2006_02_Saint_Quentin.mp3", true, 2, 1931L },
  { BASE_POS_02_SOISSONS, { 0x14, 0x90 + POS_NUM_02_SOISSONS / 256, POS_NUM_02_SOISSONS % 256 }, "09/2007_02_Soissons.mp3", true, 2, 1617L },
  { BASE_POS_02_VERVINS, { 0x14, 0x90 + POS_NUM_02_VERVINS / 256, POS_NUM_02_VERVINS % 256 }, "09/2008_02_Vervins.mp3", true, 2, 1539L },
  { BASE_POS_03_MONTLUCON, { 0x14, 0x90 + POS_NUM_03_MONTLUCON / 256, POS_NUM_03_MONTLUCON % 256 }, "09/2009_03_Montlucon.mp3", true, 2, 1774L },
  { BASE_POS_03_MOULINS, { 0x14, 0x90 + POS_NUM_03_MOULINS / 256, POS_NUM_03_MOULINS % 256 }, "09/2010_03_Moulins.mp3", true, 2, 1500L },
  { BASE_POS_03_VICHY, { 0x14, 0x90 + POS_NUM_03_VICHY / 256, POS_NUM_03_VICHY % 256 }, "09/2011_03_Vichy.mp3", true, 2, 1617L },
  { BASE_POS_04_BARCELONNETTE, { 0x14, 0x90 + POS_NUM_04_BARCELONNETTE / 256, POS_NUM_04_BARCELONNETTE % 256 }, "09/2012_04_Barcelonnette.mp3", true, 4, 4046L },
  { BASE_POS_04_CASTELLANE, { 0x14, 0x90 + POS_NUM_04_CASTELLANE / 256, POS_NUM_04_CASTELLANE % 256 }, "09/2013_04_Castellane.mp3", true, 4, 3694L },
  { BASE_POS_04_DIGNE_LES_BAINS, { 0x14, 0x90 + POS_NUM_04_DIGNE_LES_BAINS / 256, POS_NUM_04_DIGNE_LES_BAINS % 256 }, "09/2014_04_Digne_les_Bains.mp3", true, 4, 3772L },
  { BASE_POS_04_FORCALQUIER, { 0x14, 0x90 + POS_NUM_04_FORCALQUIER / 256, POS_NUM_04_FORCALQUIER % 256 }, "09/2015_04_Forcalquier.mp3", true, 4, 3772L },
  { BASE_POS_05_BRIANCON, { 0x14, 0x90 + POS_NUM_05_BRIANCON / 256, POS_NUM_05_BRIANCON % 256 }, "09/2016_05_Briancon.mp3", true, 2, 2440L },
  { BASE_POS_05_GAP, { 0x14, 0x90 + POS_NUM_05_GAP / 256, POS_NUM_05_GAP % 256 }, "09/2017_05_Gap.mp3", true, 2, 1970L },
  { BASE_POS_06_GRASSE, { 0x14, 0x90 + POS_NUM_06_GRASSE / 256, POS_NUM_06_GRASSE % 256 }, "09/2018_06_Grasse.mp3", true, 3, 2675L },
  { BASE_POS_06_NICE, { 0x14, 0x90 + POS_NUM_06_NICE / 256, POS_NUM_06_NICE % 256 }, "09/2019_06_Nice.mp3", true, 3, 2519L },
  { BASE_POS_07_LARGENTIERE, { 0x14, 0x90 + POS_NUM_07_LARGENTIERE / 256, POS_NUM_07_LARGENTIERE % 256 }, "09/2020_07_Largentiere.mp3", true, 2, 2323L },
  { BASE_POS_07_PRIVAS, { 0x14, 0x90 + POS_NUM_07_PRIVAS / 256, POS_NUM_07_PRIVAS % 256 }, "09/2021_07_Privas.mp3", true, 2, 1696L },
  { BASE_POS_07_TOURNON_SUR_RHONE, { 0x14, 0x90 + POS_NUM_07_TOURNON_SUR_RHONE / 256, POS_NUM_07_TOURNON_SUR_RHONE % 256 }, "09/2022_07_Tournon_sur_Rhone.mp3", true, 3, 2636L },
  { BASE_POS_08_CHARLEVILLE_MEZIERES, { 0x14, 0x90 + POS_NUM_08_CHARLEVILLE_MEZIERES / 256, POS_NUM_08_CHARLEVILLE_MEZIERES % 256 }, "09/2023_08_Charleville_Mezieres.mp3", true, 3, 3067L },
  { BASE_POS_08_RETHEL, { 0x14, 0x90 + POS_NUM_08_RETHEL / 256, POS_NUM_08_RETHEL % 256 }, "09/2024_08_Rethel.mp3", true, 2, 1852L },
  { BASE_POS_08_SEDAN, { 0x14, 0x90 + POS_NUM_08_SEDAN / 256, POS_NUM_08_SEDAN % 256 }, "09/2025_08_Sedan.mp3", true, 2, 1970L },
  { BASE_POS_08_VOUZIERS, { 0x14, 0x90 + POS_NUM_08_VOUZIERS / 256, POS_NUM_08_VOUZIERS % 256 }, "09/2026_08_Vouziers.mp3", true, 2, 1892L },
  { BASE_POS_09_FOIX, { 0x14, 0x90 + POS_NUM_09_FOIX / 256, POS_NUM_09_FOIX % 256 }, "09/2027_09_Foix.mp3", true, 2, 1539L },
  { BASE_POS_09_PAMIERS, { 0x14, 0x90 + POS_NUM_09_PAMIERS / 256, POS_NUM_09_PAMIERS % 256 }, "09/2028_09_Pamiers.mp3", true, 2, 1735L },
  { BASE_POS_09_SAINT_GIRONS, { 0x14, 0x90 + POS_NUM_09_SAINT_GIRONS / 256, POS_NUM_09_SAINT_GIRONS % 256 }, "09/2029_09_Saint_Girons.mp3", true, 2, 2244L },
  { BASE_POS_10_BAR_SUR_AUBE, { 0x14, 0x90 + POS_NUM_10_BAR_SUR_AUBE / 256, POS_NUM_10_BAR_SUR_AUBE % 256 }, "09/2030_10_Bar_sur_Aube.mp3", true, 2, 2048L },
  { BASE_POS_10_NOGENT_SUR_SEINE, { 0x14, 0x90 + POS_NUM_10_NOGENT_SUR_SEINE / 256, POS_NUM_10_NOGENT_SUR_SEINE % 256 }, "09/2031_10_Nogent_sur_Seine.mp3", true, 2, 2244L },
  { BASE_POS_10_TROYES, { 0x14, 0x90 + POS_NUM_10_TROYES / 256, POS_NUM_10_TROYES % 256 }, "09/2032_10_Troyes.mp3", true, 1, 1343L },
  { BASE_POS_11_CARCASSONNE, { 0x14, 0x90 + POS_NUM_11_CARCASSONNE / 256, POS_NUM_11_CARCASSONNE % 256 }, "09/2033_11_Carcassonne.mp3", true, 2, 1852L },
  { BASE_POS_11_LIMOUX, { 0x14, 0x90 + POS_NUM_11_LIMOUX / 256, POS_NUM_11_LIMOUX % 256 }, "09/2034_11_Limoux.mp3", true, 2, 1539L },
  { BASE_POS_11_NARBONNE, { 0x14, 0x90 + POS_NUM_11_NARBONNE / 256, POS_NUM_11_NARBONNE % 256 }, "09/2035_11_Narbonne.mp3", true, 2, 1500L },
  { BASE_POS_12_MILLAU, { 0x14, 0x90 + POS_NUM_12_MILLAU / 256, POS_NUM_12_MILLAU % 256 }, "09/2036_12_Millau.mp3", true, 2, 1735L },
  { BASE_POS_12_RODEZ, { 0x14, 0x90 + POS_NUM_12_RODEZ / 256, POS_NUM_12_RODEZ % 256 }, "09/2037_12_Rodez.mp3", true, 2, 1892L },
  { BASE_POS_12_VILLEFRANCHE_DE_ROUERGUE, { 0x14, 0x90 + POS_NUM_12_VILLEFRANCHE_DE_ROUERGUE / 256, POS_NUM_12_VILLEFRANCHE_DE_ROUERGUE % 256 }, "09/2038_12_Villefranche_de_Rouergue.mp3", true, 3, 3146L },
  { BASE_POS_13_AIX_EN_PROVENCE, { 0x14, 0x90 + POS_NUM_13_AIX_EN_PROVENCE / 256, POS_NUM_13_AIX_EN_PROVENCE % 256 }, "09/2039_13_Aix_en_Provence.mp3", true, 3, 3185L },
  { BASE_POS_13_ARLES, { 0x14, 0x90 + POS_NUM_13_ARLES / 256, POS_NUM_13_ARLES % 256 }, "09/2040_13_Arles.mp3", true, 2, 2205L },
  { BASE_POS_13_ISTRES, { 0x14, 0x90 + POS_NUM_13_ISTRES / 256, POS_NUM_13_ISTRES % 256 }, "09/2041_13_Istres.mp3", true, 2, 2479L },
  { BASE_POS_13_MARSEILLE, { 0x14, 0x90 + POS_NUM_13_MARSEILLE / 256, POS_NUM_13_MARSEILLE % 256 }, "09/2042_13_Marseille.mp3", true, 3, 2597L },
  { BASE_POS_14_BAYEUX, { 0x14, 0x90 + POS_NUM_14_BAYEUX / 256, POS_NUM_14_BAYEUX % 256 }, "09/2043_14_Bayeux.mp3", true, 2, 2166L },
  { BASE_POS_14_CAEN, { 0x14, 0x90 + POS_NUM_14_CAEN / 256, POS_NUM_14_CAEN % 256 }, "09/2044_14_Caen.mp3", true, 2, 2009L },
  { BASE_POS_14_LISIEUX, { 0x14, 0x90 + POS_NUM_14_LISIEUX / 256, POS_NUM_14_LISIEUX % 256 }, "09/2045_14_Lisieux.mp3", true, 2, 2244L },
  { BASE_POS_14_VIRE, { 0x14, 0x90 + POS_NUM_14_VIRE / 256, POS_NUM_14_VIRE % 256 }, "09/2046_14_Vire.mp3", true, 2, 2048L },
  { BASE_POS_15_AURILLAC, { 0x14, 0x90 + POS_NUM_15_AURILLAC / 256, POS_NUM_15_AURILLAC % 256 }, "09/2047_15_Aurillac.mp3", true, 2, 2244L },
  { BASE_POS_15_MAURIAC, { 0x14, 0x90 + POS_NUM_15_MAURIAC / 256, POS_NUM_15_MAURIAC % 256 }, "09/2048_15_Mauriac.mp3", true, 2, 2166L },
  { BASE_POS_15_SAINT_FLOUR, { 0x14, 0x90 + POS_NUM_15_SAINT_FLOUR / 256, POS_NUM_15_SAINT_FLOUR % 256 }, "09/2049_15_Saint_Flour.mp3", true, 2, 2244L },
  { BASE_POS_16_ANGOULEME, { 0x14, 0x90 + POS_NUM_16_ANGOULEME / 256, POS_NUM_16_ANGOULEME % 256 }, "09/2050_16_Angouleme.mp3", true, 2, 2283L },
  { BASE_POS_16_COGNAC, { 0x14, 0x90 + POS_NUM_16_COGNAC / 256, POS_NUM_16_COGNAC % 256 }, "09/2051_16_Cognac.mp3", true, 2, 2127L },
  { BASE_POS_16_CONFOLENS, { 0x14, 0x90 + POS_NUM_16_CONFOLENS / 256, POS_NUM_16_CONFOLENS % 256 }, "09/2052_16_Confolens.mp3", true, 2, 2205L },
  { BASE_POS_17_JONZAC, { 0x14, 0x90 + POS_NUM_17_JONZAC / 256, POS_NUM_17_JONZAC % 256 }, "09/2053_17_Jonzac.mp3", true, 3, 3028L },
  { BASE_POS_17_ROCHEFORT, { 0x14, 0x90 + POS_NUM_17_ROCHEFORT / 256, POS_NUM_17_ROCHEFORT % 256 }, "09/2054_17_Rochefort.mp3", true, 3, 3146L },
  { BASE_POS_17_LA_ROCHELLE, { 0x14, 0x90 + POS_NUM_17_LA_ROCHELLE / 256, POS_NUM_17_LA_ROCHELLE % 256 }, "09/2055_17_la_Rochelle.mp3", true, 3, 3224L },
  { BASE_POS_17_SAINT_JEAN_D_ANGELY, { 0x14, 0x90 + POS_NUM_17_SAINT_JEAN_D_ANGELY / 256, POS_NUM_17_SAINT_JEAN_D_ANGELY % 256 }, "09/2056_17_Saint_Jean_d_Angely.mp3", true, 4, 3890L },
  { BASE_POS_17_SAINTES, { 0x14, 0x90 + POS_NUM_17_SAINTES / 256, POS_NUM_17_SAINTES % 256 }, "09/2057_17_Saintes.mp3", true, 3, 2989L },
  { BASE_POS_18_BOURGES, { 0x14, 0x90 + POS_NUM_18_BOURGES / 256, POS_NUM_18_BOURGES % 256 }, "09/2058_18_Bourges.mp3", true, 2, 1852L },
  { BASE_POS_18_SAINT_AMAND_MONTROND, { 0x14, 0x90 + POS_NUM_18_SAINT_AMAND_MONTROND / 256, POS_NUM_18_SAINT_AMAND_MONTROND % 256 }, "09/2059_18_Saint_Amand_Montrond.mp3", true, 2, 2479L },
  { BASE_POS_18_VIERZON, { 0x14, 0x90 + POS_NUM_18_VIERZON / 256, POS_NUM_18_VIERZON % 256 }, "09/2060_18_Vierzon.mp3", true, 2, 1931L },
  { BASE_POS_19_BRIVE_LA_GAILLARDE, { 0x14, 0x90 + POS_NUM_19_BRIVE_LA_GAILLARDE / 256, POS_NUM_19_BRIVE_LA_GAILLARDE % 256 }, "09/2061_19_Brive_la_Gaillarde.mp3", true, 3, 2871L },
  { BASE_POS_19_TULLE, { 0x14, 0x90 + POS_NUM_19_TULLE / 256, POS_NUM_19_TULLE % 256 }, "09/2062_19_Tulle.mp3", true, 1, 1343L },
  { BASE_POS_19_USSEL, { 0x14, 0x90 + POS_NUM_19_USSEL / 256, POS_NUM_19_USSEL % 256 }, "09/2063_19_Ussel.mp3", true, 2, 1813L },
  { BASE_POS_2A_AJACCIO, { 0x14, 0x90 + POS_NUM_2A_AJACCIO / 256, POS_NUM_2A_AJACCIO % 256 }, "09/2064_2A_Ajaccio.mp3", true, 3, 2558L },
  { BASE_POS_2A_SARTENE, { 0x14, 0x90 + POS_NUM_2A_SARTENE / 256, POS_NUM_2A_SARTENE % 256 }, "09/2065_2A_Sartene.mp3", true, 3, 2714L },
  { BASE_POS_2B_BASTIA, { 0x14, 0x90 + POS_NUM_2B_BASTIA / 256, POS_NUM_2B_BASTIA % 256 }, "09/2066_2B_Bastia.mp3", true, 2, 2323L },
  { BASE_POS_2B_CALVI, { 0x14, 0x90 + POS_NUM_2B_CALVI / 256, POS_NUM_2B_CALVI % 256 }, "09/2067_2B_Calvi.mp3", true, 2, 2166L },
  { BASE_POS_2B_CORTE, { 0x14, 0x90 + POS_NUM_2B_CORTE / 256, POS_NUM_2B_CORTE % 256 }, "09/2068_2B_Corte.mp3", true, 2, 2088L },
  { BASE_POS_21_BEAUNE, { 0x14, 0x90 + POS_NUM_21_BEAUNE / 256, POS_NUM_21_BEAUNE % 256 }, "09/2069_21_Beaune.mp3", true, 2, 2127L },
  { BASE_POS_21_DIJON, { 0x14, 0x90 + POS_NUM_21_DIJON / 256, POS_NUM_21_DIJON % 256 }, "09/2070_21_Dijon.mp3", true, 2, 2127L },
  { BASE_POS_21_MONTBARD, { 0x14, 0x90 + POS_NUM_21_MONTBARD / 256, POS_NUM_21_MONTBARD % 256 }, "09/2071_21_Montbard.mp3", true, 2, 2244L },
  { BASE_POS_22_DINAN, { 0x14, 0x90 + POS_NUM_22_DINAN / 256, POS_NUM_22_DINAN % 256 }, "09/2072_22_Dinan.mp3", true, 2, 2440L },
  { BASE_POS_22_GUINGAMP, { 0x14, 0x90 + POS_NUM_22_GUINGAMP / 256, POS_NUM_22_GUINGAMP % 256 }, "09/2073_22_Guingamp.mp3", true, 3, 2519L },
  { BASE_POS_22_LANNION, { 0x14, 0x90 + POS_NUM_22_LANNION / 256, POS_NUM_22_LANNION % 256 }, "09/2074_22_Lannion.mp3", true, 3, 2675L },
  { BASE_POS_22_SAINT_BRIEUC, { 0x14, 0x90 + POS_NUM_22_SAINT_BRIEUC / 256, POS_NUM_22_SAINT_BRIEUC % 256 }, "09/2075_22_Saint_Brieuc.mp3", true, 3, 2832L },
  { BASE_POS_23_AUBUSSON, { 0x14, 0x90 + POS_NUM_23_AUBUSSON / 256, POS_NUM_23_AUBUSSON % 256 }, "09/2076_23_Aubusson.mp3", true, 2, 2009L },
  { BASE_POS_23_GUERET, { 0x14, 0x90 + POS_NUM_23_GUERET / 256, POS_NUM_23_GUERET % 256 }, "09/2077_23_Gueret.mp3", true, 2, 1735L },
  { BASE_POS_24_BERGERAC, { 0x14, 0x90 + POS_NUM_24_BERGERAC / 256, POS_NUM_24_BERGERAC % 256 }, "09/2078_24_Bergerac.mp3", true, 2, 2440L },
  { BASE_POS_24_NONTRON, { 0x14, 0x90 + POS_NUM_24_NONTRON / 256, POS_NUM_24_NONTRON % 256 }, "09/2079_24_Nontron.mp3", true, 2, 2166L },
  { BASE_POS_24_PERIGUEUX, { 0x14, 0x90 + POS_NUM_24_PERIGUEUX / 256, POS_NUM_24_PERIGUEUX % 256 }, "09/2080_24_Perigueux.mp3", true, 2, 1970L },
  { BASE_POS_24_SARLAT_LA_CANEDA, { 0x14, 0x90 + POS_NUM_24_SARLAT_LA_CANEDA / 256, POS_NUM_24_SARLAT_LA_CANEDA % 256 }, "09/2081_24_Sarlat_la_Caneda.mp3", true, 3, 2950L },
  { BASE_POS_25_BESANCON, { 0x14, 0x90 + POS_NUM_25_BESANCON / 256, POS_NUM_25_BESANCON % 256 }, "09/2082_25_Besancon.mp3", true, 2, 1813L },
  { BASE_POS_25_MONTBELIARD, { 0x14, 0x90 + POS_NUM_25_MONTBELIARD / 256, POS_NUM_25_MONTBELIARD % 256 }, "09/2083_25_Montbeliard.mp3", true, 2, 1931L },
  { BASE_POS_25_PONTARLIER, { 0x14, 0x90 + POS_NUM_25_PONTARLIER / 256, POS_NUM_25_PONTARLIER % 256 }, "09/2084_25_Pontarlier.mp3", true, 2, 1774L },
  { BASE_POS_26_DIE, { 0x14, 0x90 + POS_NUM_26_DIE / 256, POS_NUM_26_DIE % 256 }, "09/2085_26_Die.mp3", true, 2, 1578L },
  { BASE_POS_26_NYONS, { 0x14, 0x90 + POS_NUM_26_NYONS / 256, POS_NUM_26_NYONS % 256 }, "09/2086_26_Nyons.mp3", true, 2, 1657L },
  { BASE_POS_26_VALENCE, { 0x14, 0x90 + POS_NUM_26_VALENCE / 256, POS_NUM_26_VALENCE % 256 }, "09/2087_26_Valence.mp3", true, 2, 2009L },
  { BASE_POS_27_LES_ANDELYS, { 0x14, 0x90 + POS_NUM_27_LES_ANDELYS / 256, POS_NUM_27_LES_ANDELYS % 256 }, "09/2088_27_les_Andelys.mp3", true, 2, 1892L },
  { BASE_POS_27_ANDELYS, { 0x14, 0x90 + POS_NUM_27_ANDELYS / 256, POS_NUM_27_ANDELYS % 256 }, "09/2089_27_Andelys.mp3", true, 2, 1578L },
  { BASE_POS_27_BERNAY, { 0x14, 0x90 + POS_NUM_27_BERNAY / 256, POS_NUM_27_BERNAY % 256 }, "09/2090_27_Bernay.mp3", true, 2, 1578L },
  { BASE_POS_27_EVREUX, { 0x14, 0x90 + POS_NUM_27_EVREUX / 256, POS_NUM_27_EVREUX % 256 }, "09/2091_27_Evreux.mp3", true, 2, 1500L },
  { BASE_POS_28_CHARTRES, { 0x14, 0x90 + POS_NUM_28_CHARTRES / 256, POS_NUM_28_CHARTRES % 256 }, "09/2092_28_Chartres.mp3", true, 2, 2166L },
  { BASE_POS_28_CHATEAUDUN, { 0x14, 0x90 + POS_NUM_28_CHATEAUDUN / 256, POS_NUM_28_CHATEAUDUN % 256 }, "09/2093_28_Chateaudun.mp3", true, 2, 2323L },
  { BASE_POS_28_DREUX, { 0x14, 0x90 + POS_NUM_28_DREUX / 256, POS_NUM_28_DREUX % 256 }, "09/2094_28_Dreux.mp3", true, 2, 1931L },
  { BASE_POS_28_NOGENT_LE_ROTROU, { 0x14, 0x90 + POS_NUM_28_NOGENT_LE_ROTROU / 256, POS_NUM_28_NOGENT_LE_ROTROU % 256 }, "09/2095_28_Nogent_le_Rotrou.mp3", true, 3, 2950L },
  { BASE_POS_29_BREST, { 0x14, 0x90 + POS_NUM_29_BREST / 256, POS_NUM_29_BREST % 256 }, "09/2096_29_Brest.mp3", true, 2, 2401L },
  { BASE_POS_29_CHATEAULIN, { 0x14, 0x90 + POS_NUM_29_CHATEAULIN / 256, POS_NUM_29_CHATEAULIN % 256 }, "09/2097_29_Chateaulin.mp3", true, 3, 2636L },
  { BASE_POS_29_MORLAIX, { 0x14, 0x90 + POS_NUM_29_MORLAIX / 256, POS_NUM_29_MORLAIX % 256 }, "09/2098_29_Morlaix.mp3", true, 2, 2323L },
  { BASE_POS_29_QUIMPER, { 0x14, 0x90 + POS_NUM_29_QUIMPER / 256, POS_NUM_29_QUIMPER % 256 }, "09/2099_29_Quimper.mp3", true, 2, 2323L },
  { BASE_POS_30_ALES, { 0x14, 0x90 + POS_NUM_30_ALES / 256, POS_NUM_30_ALES % 256 }, "09/2100_30_Ales.mp3", true, 2, 1657L },
  { BASE_POS_30_NIMES, { 0x14, 0x90 + POS_NUM_30_NIMES / 256, POS_NUM_30_NIMES % 256 }, "09/2101_30_Nimes.mp3", true, 2, 1539L },
  { BASE_POS_30_LE_VIGAN, { 0x14, 0x90 + POS_NUM_30_LE_VIGAN / 256, POS_NUM_30_LE_VIGAN % 256 }, "09/2102_30_le_Vigan.mp3", true, 2, 1892L },
  { BASE_POS_30_VIGAN, { 0x14, 0x90 + POS_NUM_30_VIGAN / 256, POS_NUM_30_VIGAN % 256 }, "09/2103_30_Vigan.mp3", true, 2, 1735L },
  { BASE_POS_31_MURET, { 0x14, 0x90 + POS_NUM_31_MURET / 256, POS_NUM_31_MURET % 256 }, "09/2104_31_Muret.mp3", true, 2, 2088L },
  { BASE_POS_31_SAINT_GAUDENS, { 0x14, 0x90 + POS_NUM_31_SAINT_GAUDENS / 256, POS_NUM_31_SAINT_GAUDENS % 256 }, "09/2105_31_Saint_Gaudens.mp3", true, 3, 2871L },
  { BASE_POS_31_TOULOUSE, { 0x14, 0x90 + POS_NUM_31_TOULOUSE / 256, POS_NUM_31_TOULOUSE % 256 }, "09/2106_31_Toulouse.mp3", true, 2, 2479L },
  { BASE_POS_32_AUCH, { 0x14, 0x90 + POS_NUM_32_AUCH / 256, POS_NUM_32_AUCH % 256 }, "09/2107_32_Auch.mp3", true, 2, 1696L },
  { BASE_POS_32_CONDOM, { 0x14, 0x90 + POS_NUM_32_CONDOM / 256, POS_NUM_32_CONDOM % 256 }, "09/2108_32_Condom.mp3", true, 2, 1774L },
  { BASE_POS_32_MIRANDE, { 0x14, 0x90 + POS_NUM_32_MIRANDE / 256, POS_NUM_32_MIRANDE % 256 }, "09/2109_32_Mirande.mp3", true, 2, 2088L },
  { BASE_POS_33_ARCACHON, { 0x14, 0x90 + POS_NUM_33_ARCACHON / 256, POS_NUM_33_ARCACHON % 256 }, "09/2110_33_Arcachon.mp3", true, 2, 2283L },
  { BASE_POS_33_BLAYE, { 0x14, 0x90 + POS_NUM_33_BLAYE / 256, POS_NUM_33_BLAYE % 256 }, "09/2111_33_Blaye.mp3", true, 2, 1813L },
  { BASE_POS_33_BORDEAUX, { 0x14, 0x90 + POS_NUM_33_BORDEAUX / 256, POS_NUM_33_BORDEAUX % 256 }, "09/2112_33_Bordeaux.mp3", true, 2, 1813L },
  { BASE_POS_33_LANGON, { 0x14, 0x90 + POS_NUM_33_LANGON / 256, POS_NUM_33_LANGON % 256 }, "09/2113_33_Langon.mp3", true, 2, 2009L },
  { BASE_POS_33_LESPARRE_MEDOC, { 0x14, 0x90 + POS_NUM_33_LESPARRE_MEDOC / 256, POS_NUM_33_LESPARRE_MEDOC % 256 }, "09/2114_33_Lesparre_Medoc.mp3", true, 3, 2714L },
  { BASE_POS_33_LIBOURNE, { 0x14, 0x90 + POS_NUM_33_LIBOURNE / 256, POS_NUM_33_LIBOURNE % 256 }, "09/2115_33_Libourne.mp3", true, 2, 2088L },
  { BASE_POS_34_BEZIERS, { 0x14, 0x90 + POS_NUM_34_BEZIERS / 256, POS_NUM_34_BEZIERS % 256 }, "09/2116_34_Beziers.mp3", true, 1, 1421L },
  { BASE_POS_34_LODEVE, { 0x14, 0x90 + POS_NUM_34_LODEVE / 256, POS_NUM_34_LODEVE % 256 }, "09/2117_34_Lodeve.mp3", true, 2, 1617L },
  { BASE_POS_34_MONTPELLIER, { 0x14, 0x90 + POS_NUM_34_MONTPELLIER / 256, POS_NUM_34_MONTPELLIER % 256 }, "09/2118_34_Montpellier.mp3", true, 2, 1774L },
  { BASE_POS_35_FOUGERES, { 0x14, 0x90 + POS_NUM_35_FOUGERES / 256, POS_NUM_35_FOUGERES % 256 }, "09/2119_35_Fougeres.mp3", true, 2, 2401L },
  { BASE_POS_35_REDON, { 0x14, 0x90 + POS_NUM_35_REDON / 256, POS_NUM_35_REDON % 256 }, "09/2120_35_Redon.mp3", true, 2, 2088L },
  { BASE_POS_35_RENNES, { 0x14, 0x90 + POS_NUM_35_RENNES / 256, POS_NUM_35_RENNES % 256 }, "09/2121_35_Rennes.mp3", true, 2, 2048L },
  { BASE_POS_35_SAINT_MALO, { 0x14, 0x90 + POS_NUM_35_SAINT_MALO / 256, POS_NUM_35_SAINT_MALO % 256 }, "09/2122_35_Saint_Malo.mp3", true, 2, 2479L },
  { BASE_POS_36_LE_BLANC, { 0x14, 0x90 + POS_NUM_36_LE_BLANC / 256, POS_NUM_36_LE_BLANC % 256 }, "09/2123_36_le_Blanc.mp3", true, 2, 1578L },
  { BASE_POS_36_BLANC, { 0x14, 0x90 + POS_NUM_36_BLANC / 256, POS_NUM_36_BLANC % 256 }, "09/2124_36_Blanc.mp3", true, 1, 1304L },
  { BASE_POS_36_CHATEAUROUX, { 0x14, 0x90 + POS_NUM_36_CHATEAUROUX / 256, POS_NUM_36_CHATEAUROUX % 256 }, "09/2125_36_Chateauroux.mp3", true, 2, 1774L },
  { BASE_POS_36_LA_CHATRE, { 0x14, 0x90 + POS_NUM_36_LA_CHATRE / 256, POS_NUM_36_LA_CHATRE % 256 }, "09/2126_36_la_Chatre.mp3", true, 2, 1852L },
  { BASE_POS_36_ISSOUDUN, { 0x14, 0x90 + POS_NUM_36_ISSOUDUN / 256, POS_NUM_36_ISSOUDUN % 256 }, "09/2127_36_Issoudun.mp3", true, 2, 1539L },
  { BASE_POS_37_CHINON, { 0x14, 0x90 + POS_NUM_37_CHINON / 256, POS_NUM_37_CHINON % 256 }, "09/2128_37_Chinon.mp3", true, 2, 2244L },
  { BASE_POS_37_LOCHES, { 0x14, 0x90 + POS_NUM_37_LOCHES / 256, POS_NUM_37_LOCHES % 256 }, "09/2129_37_Loches.mp3", true, 2, 2283L },
  { BASE_POS_37_TOURS, { 0x14, 0x90 + POS_NUM_37_TOURS / 256, POS_NUM_37_TOURS % 256 }, "09/2130_37_Tours.mp3", true, 2, 1970L },
  { BASE_POS_38_GRENOBLE, { 0x14, 0x90 + POS_NUM_38_GRENOBLE / 256, POS_NUM_38_GRENOBLE % 256 }, "09/2131_38_Grenoble.mp3", true, 2, 1892L },
  { BASE_POS_38_LA_TOUR_DU_PIN, { 0x14, 0x90 + POS_NUM_38_LA_TOUR_DU_PIN / 256, POS_NUM_38_LA_TOUR_DU_PIN % 256 }, "09/2132_38_la_Tour_du_Pin.mp3", true, 2, 2401L },
  { BASE_POS_38_VIENNE, { 0x14, 0x90 + POS_NUM_38_VIENNE / 256, POS_NUM_38_VIENNE % 256 }, "09/2133_38_Vienne.mp3", true, 2, 1617L },
  { BASE_POS_39_DOLE, { 0x14, 0x90 + POS_NUM_39_DOLE / 256, POS_NUM_39_DOLE % 256 }, "09/2134_39_Dole.mp3", true, 2, 1578L },
  { BASE_POS_39_LONS_LE_SAUNIER, { 0x14, 0x90 + POS_NUM_39_LONS_LE_SAUNIER / 256, POS_NUM_39_LONS_LE_SAUNIER % 256 }, "09/2135_39_Lons_le_Saunier.mp3", true, 3, 2636L },
  { BASE_POS_39_SAINT_CLAUDE, { 0x14, 0x90 + POS_NUM_39_SAINT_CLAUDE / 256, POS_NUM_39_SAINT_CLAUDE % 256 }, "09/2136_39_Saint_Claude.mp3", true, 2, 2009L },
  { BASE_POS_40_DAX, { 0x14, 0x90 + POS_NUM_40_DAX / 256, POS_NUM_40_DAX % 256 }, "09/2137_40_Dax.mp3", true, 2, 1735L },
  { BASE_POS_40_MONT_DE_MARSAN, { 0x14, 0x90 + POS_NUM_40_MONT_DE_MARSAN / 256, POS_NUM_40_MONT_DE_MARSAN % 256 }, "09/2138_40_Mont_de_Marsan.mp3", true, 2, 2205L },
  { BASE_POS_41_BLOIS, { 0x14, 0x90 + POS_NUM_41_BLOIS / 256, POS_NUM_41_BLOIS % 256 }, "09/2139_41_Blois.mp3", true, 2, 2323L },
  { BASE_POS_41_ROMORANTIN_LANTHENAY, { 0x14, 0x90 + POS_NUM_41_ROMORANTIN_LANTHENAY / 256, POS_NUM_41_ROMORANTIN_LANTHENAY % 256 }, "09/2140_41_Romorantin_Lanthenay.mp3", true, 4, 3655L },
  { BASE_POS_41_VENDOME, { 0x14, 0x90 + POS_NUM_41_VENDOME / 256, POS_NUM_41_VENDOME % 256 }, "09/2141_41_Vendome.mp3", true, 3, 2519L },
  { BASE_POS_42_MONTBRISON, { 0x14, 0x90 + POS_NUM_42_MONTBRISON / 256, POS_NUM_42_MONTBRISON % 256 }, "09/2142_42_Montbrison.mp3", true, 2, 2088L },
  { BASE_POS_42_ROANNE, { 0x14, 0x90 + POS_NUM_42_ROANNE / 256, POS_NUM_42_ROANNE % 256 }, "09/2143_42_Roanne.mp3", true, 2, 1774L },
  { BASE_POS_42_SAINT_ETIENNE, { 0x14, 0x90 + POS_NUM_42_SAINT_ETIENNE / 256, POS_NUM_42_SAINT_ETIENNE % 256 }, "09/2144_42_Saint_Etienne.mp3", true, 2, 2205L },
  { BASE_POS_43_BRIOUDE, { 0x14, 0x90 + POS_NUM_43_BRIOUDE / 256, POS_NUM_43_BRIOUDE % 256 }, "09/2145_43_Brioude.mp3", true, 2, 2205L },
  { BASE_POS_43_LE_PUY_EN_VELAY, { 0x14, 0x90 + POS_NUM_43_LE_PUY_EN_VELAY / 256, POS_NUM_43_LE_PUY_EN_VELAY % 256 }, "09/2146_43_le_Puy_en_Velay.mp3", true, 3, 2950L },
  { BASE_POS_43_PUY_EN_VELAY, { 0x14, 0x90 + POS_NUM_43_PUY_EN_VELAY / 256, POS_NUM_43_PUY_EN_VELAY % 256 }, "09/2147_43_Puy_en_Velay.mp3", true, 3, 2558L },
  { BASE_POS_43_YSSINGEAUX, { 0x14, 0x90 + POS_NUM_43_YSSINGEAUX / 256, POS_NUM_43_YSSINGEAUX % 256 }, "09/2148_43_Yssingeaux.mp3", true, 2, 2440L },
  { BASE_POS_44_ANCENIS, { 0x14, 0x90 + POS_NUM_44_ANCENIS / 256, POS_NUM_44_ANCENIS % 256 }, "09/2149_44_Ancenis.mp3", true, 3, 2950L },
  { BASE_POS_44_CHATEAUBRIANT, { 0x14, 0x90 + POS_NUM_44_CHATEAUBRIANT / 256, POS_NUM_44_CHATEAUBRIANT % 256 }, "09/2150_44_Chateaubriant.mp3", true, 3, 3224L },
  { BASE_POS_44_NANTES, { 0x14, 0x90 + POS_NUM_44_NANTES / 256, POS_NUM_44_NANTES % 256 }, "09/2151_44_Nantes.mp3", true, 2, 2401L },
  { BASE_POS_44_SAINT_NAZAIRE, { 0x14, 0x90 + POS_NUM_44_SAINT_NAZAIRE / 256, POS_NUM_44_SAINT_NAZAIRE % 256 }, "09/2152_44_Saint_Nazaire.mp3", true, 3, 3028L },
  { BASE_POS_45_MONTARGIS, { 0x14, 0x90 + POS_NUM_45_MONTARGIS / 256, POS_NUM_45_MONTARGIS % 256 }, "09/2153_45_Montargis.mp3", true, 2, 2127L },
  { BASE_POS_45_ORLEANS, { 0x14, 0x90 + POS_NUM_45_ORLEANS / 256, POS_NUM_45_ORLEANS % 256 }, "09/2154_45_Orleans.mp3", true, 2, 1852L },
  { BASE_POS_45_PITHIVIERS, { 0x14, 0x90 + POS_NUM_45_PITHIVIERS / 256, POS_NUM_45_PITHIVIERS % 256 }, "09/2155_45_Pithiviers.mp3", true, 2, 1970L },
  { BASE_POS_46_CAHORS, { 0x14, 0x90 + POS_NUM_46_CAHORS / 256, POS_NUM_46_CAHORS % 256 }, "09/2156_46_Cahors.mp3", true, 2, 1617L },
  { BASE_POS_46_FIGEAC, { 0x14, 0x90 + POS_NUM_46_FIGEAC / 256, POS_NUM_46_FIGEAC % 256 }, "09/2157_46_Figeac.mp3", true, 2, 1852L },
  { BASE_POS_46_GOURDON, { 0x14, 0x90 + POS_NUM_46_GOURDON / 256, POS_NUM_46_GOURDON % 256 }, "09/2158_46_Gourdon.mp3", true, 2, 1696L },
  { BASE_POS_47_AGEN, { 0x14, 0x90 + POS_NUM_47_AGEN / 256, POS_NUM_47_AGEN % 256 }, "09/2159_47_Agen.mp3", true, 2, 2362L },
  { BASE_POS_47_MARMANDE, { 0x14, 0x90 + POS_NUM_47_MARMANDE / 256, POS_NUM_47_MARMANDE % 256 }, "09/2160_47_Marmande.mp3", true, 3, 2871L },
  { BASE_POS_47_NERAC, { 0x14, 0x90 + POS_NUM_47_NERAC / 256, POS_NUM_47_NERAC % 256 }, "09/2161_47_Nerac.mp3", true, 2, 2401L },
  { BASE_POS_47_VILLENEUVE_SUR_LOT, { 0x14, 0x90 + POS_NUM_47_VILLENEUVE_SUR_LOT / 256, POS_NUM_47_VILLENEUVE_SUR_LOT % 256 }, "09/2162_47_Villeneuve_sur_Lot.mp3", true, 3, 3459L },
  { BASE_POS_48_FLORAC, { 0x14, 0x90 + POS_NUM_48_FLORAC / 256, POS_NUM_48_FLORAC % 256 }, "09/2163_48_Florac.mp3", true, 2, 2166L },
  { BASE_POS_48_MENDE, { 0x14, 0x90 + POS_NUM_48_MENDE / 256, POS_NUM_48_MENDE % 256 }, "09/2164_48_Mende.mp3", true, 2, 2009L },
  { BASE_POS_49_ANGERS, { 0x14, 0x90 + POS_NUM_49_ANGERS / 256, POS_NUM_49_ANGERS % 256 }, "09/2165_49_Angers.mp3", true, 2, 2205L },
  { BASE_POS_49_CHOLET, { 0x14, 0x90 + POS_NUM_49_CHOLET / 256, POS_NUM_49_CHOLET % 256 }, "09/2166_49_Cholet.mp3", true, 2, 2283L },
  { BASE_POS_49_SAUMUR, { 0x14, 0x90 + POS_NUM_49_SAUMUR / 256, POS_NUM_49_SAUMUR % 256 }, "09/2167_49_Saumur.mp3", true, 2, 2362L },
  { BASE_POS_49_SEGRE, { 0x14, 0x90 + POS_NUM_49_SEGRE / 256, POS_NUM_49_SEGRE % 256 }, "09/2168_49_Segre.mp3", true, 2, 2283L },
  { BASE_POS_50_AVRANCHES, { 0x14, 0x90 + POS_NUM_50_AVRANCHES / 256, POS_NUM_50_AVRANCHES % 256 }, "09/2169_50_Avranches.mp3", true, 2, 1931L },
  { BASE_POS_50_CHERBOURG_OCTEVILLE, { 0x14, 0x90 + POS_NUM_50_CHERBOURG_OCTEVILLE / 256, POS_NUM_50_CHERBOURG_OCTEVILLE % 256 }, "09/2170_50_Cherbourg_Octeville.mp3", true, 3, 2793L },
  { BASE_POS_50_COUTANCES, { 0x14, 0x90 + POS_NUM_50_COUTANCES / 256, POS_NUM_50_COUTANCES % 256 }, "09/2171_50_Coutances.mp3", true, 2, 1852L },
  { BASE_POS_50_SAINT_LO, { 0x14, 0x90 + POS_NUM_50_SAINT_LO / 256, POS_NUM_50_SAINT_LO % 256 }, "09/2172_50_Saint_Lo.mp3", true, 2, 1852L },
  { BASE_POS_51_CHALONS_EN_CHAMPAGNE, { 0x14, 0x90 + POS_NUM_51_CHALONS_EN_CHAMPAGNE / 256, POS_NUM_51_CHALONS_EN_CHAMPAGNE % 256 }, "09/2173_51_Chalons_en_Champagne.mp3", true, 3, 2754L },
  { BASE_POS_51_EPERNAY, { 0x14, 0x90 + POS_NUM_51_EPERNAY / 256, POS_NUM_51_EPERNAY % 256 }, "09/2174_51_Epernay.mp3", true, 2, 1852L },
  { BASE_POS_51_REIMS, { 0x14, 0x90 + POS_NUM_51_REIMS / 256, POS_NUM_51_REIMS % 256 }, "09/2175_51_Reims.mp3", true, 2, 1578L },
  { BASE_POS_51_SAINTE_MENEHOULD, { 0x14, 0x90 + POS_NUM_51_SAINTE_MENEHOULD / 256, POS_NUM_51_SAINTE_MENEHOULD % 256 }, "09/2176_51_Sainte_Menehould.mp3", true, 2, 2048L },
  { BASE_POS_51_VITRY_LE_FRANCOIS, { 0x14, 0x90 + POS_NUM_51_VITRY_LE_FRANCOIS / 256, POS_NUM_51_VITRY_LE_FRANCOIS % 256 }, "09/2177_51_Vitry_le_Francois.mp3", true, 3, 2675L },
  { BASE_POS_52_CHAUMONT, { 0x14, 0x90 + POS_NUM_52_CHAUMONT / 256, POS_NUM_52_CHAUMONT % 256 }, "09/2178_52_Chaumont.mp3", true, 2, 2244L },
  { BASE_POS_52_LANGRES, { 0x14, 0x90 + POS_NUM_52_LANGRES / 256, POS_NUM_52_LANGRES % 256 }, "09/2179_52_Langres.mp3", true, 2, 2127L },
  { BASE_POS_52_SAINT_DIZIER, { 0x14, 0x90 + POS_NUM_52_SAINT_DIZIER / 256, POS_NUM_52_SAINT_DIZIER % 256 }, "09/2180_52_Saint_Dizier.mp3", true, 2, 2362L },
  { BASE_POS_53_CHATEAU_GONTIER, { 0x14, 0x90 + POS_NUM_53_CHATEAU_GONTIER / 256, POS_NUM_53_CHATEAU_GONTIER % 256 }, "09/2181_53_Chateau_Gontier.mp3", true, 2, 2401L },
  { BASE_POS_53_LAVAL, { 0x14, 0x90 + POS_NUM_53_LAVAL / 256, POS_NUM_53_LAVAL % 256 }, "09/2182_53_Laval.mp3", true, 2, 1892L },
  { BASE_POS_53_MAYENNE, { 0x14, 0x90 + POS_NUM_53_MAYENNE / 256, POS_NUM_53_MAYENNE % 256 }, "09/2183_53_Mayenne.mp3", true, 2, 2088L },
  { BASE_POS_54_BRIEY, { 0x14, 0x90 + POS_NUM_54_BRIEY / 256, POS_NUM_54_BRIEY % 256 }, "09/2184_54_Briey.mp3", true, 3, 2597L },
  { BASE_POS_54_LUNEVILLE, { 0x14, 0x90 + POS_NUM_54_LUNEVILLE / 256, POS_NUM_54_LUNEVILLE % 256 }, "09/2185_54_Luneville.mp3", true, 3, 2832L },
  { BASE_POS_54_NANCY, { 0x14, 0x90 + POS_NUM_54_NANCY / 256, POS_NUM_54_NANCY % 256 }, "09/2186_54_Nancy.mp3", true, 3, 2793L },
  { BASE_POS_54_TOUL, { 0x14, 0x90 + POS_NUM_54_TOUL / 256, POS_NUM_54_TOUL % 256 }, "09/2187_54_Toul.mp3", true, 2, 2362L },
  { BASE_POS_55_BAR_LE_DUC, { 0x14, 0x90 + POS_NUM_55_BAR_LE_DUC / 256, POS_NUM_55_BAR_LE_DUC % 256 }, "09/2188_55_Bar_le_Duc.mp3", true, 2, 1970L },
  { BASE_POS_55_COMMERCY, { 0x14, 0x90 + POS_NUM_55_COMMERCY / 256, POS_NUM_55_COMMERCY % 256 }, "09/2189_55_Commercy.mp3", true, 2, 1931L },
  { BASE_POS_55_VERDUN, { 0x14, 0x90 + POS_NUM_55_VERDUN / 256, POS_NUM_55_VERDUN % 256 }, "09/2190_55_Verdun.mp3", true, 2, 1735L },
  { BASE_POS_56_LORIENT, { 0x14, 0x90 + POS_NUM_56_LORIENT / 256, POS_NUM_56_LORIENT % 256 }, "09/2191_56_Lorient.mp3", true, 2, 2088L },
  { BASE_POS_56_PONTIVY, { 0x14, 0x90 + POS_NUM_56_PONTIVY / 256, POS_NUM_56_PONTIVY % 256 }, "09/2192_56_Pontivy.mp3", true, 2, 2127L },
  { BASE_POS_56_VANNES, { 0x14, 0x90 + POS_NUM_56_VANNES / 256, POS_NUM_56_VANNES % 256 }, "09/2193_56_Vannes.mp3", true, 2, 1813L },
  { BASE_POS_57_BOULAY_MOSELLE, { 0x14, 0x90 + POS_NUM_57_BOULAY_MOSELLE / 256, POS_NUM_57_BOULAY_MOSELLE % 256 }, "09/2194_57_Boulay_Moselle.mp3", true, 2, 2323L },
  { BASE_POS_57_CHATEAU_SALINS, { 0x14, 0x90 + POS_NUM_57_CHATEAU_SALINS / 256, POS_NUM_57_CHATEAU_SALINS % 256 }, "09/2195_57_Chateau_Salins.mp3", true, 2, 2440L },
  { BASE_POS_57_FORBACH, { 0x14, 0x90 + POS_NUM_57_FORBACH / 256, POS_NUM_57_FORBACH % 256 }, "09/2196_57_Forbach.mp3", true, 2, 2479L },
  { BASE_POS_57_METZ, { 0x14, 0x90 + POS_NUM_57_METZ / 256, POS_NUM_57_METZ % 256 }, "09/2197_57_Metz.mp3", true, 2, 1735L },
  { BASE_POS_57_SARREBOURG, { 0x14, 0x90 + POS_NUM_57_SARREBOURG / 256, POS_NUM_57_SARREBOURG % 256 }, "09/2198_57_Sarrebourg.mp3", true, 2, 2127L },
  { BASE_POS_57_SARREGUEMINES, { 0x14, 0x90 + POS_NUM_57_SARREGUEMINES / 256, POS_NUM_57_SARREGUEMINES % 256 }, "09/2199_57_Sarreguemines.mp3", true, 2, 2323L },
  { BASE_POS_57_THIONVILLE, { 0x14, 0x90 + POS_NUM_57_THIONVILLE / 256, POS_NUM_57_THIONVILLE % 256 }, "09/2200_57_Thionville.mp3", true, 2, 2009L },
  { BASE_POS_58_CHATEAU_CHINON, { 0x14, 0x90 + POS_NUM_58_CHATEAU_CHINON / 256, POS_NUM_58_CHATEAU_CHINON % 256 }, "09/2201_58_Chateau_Chinon.mp3", true, 3, 2597L },
  { BASE_POS_58_CLAMECY, { 0x14, 0x90 + POS_NUM_58_CLAMECY / 256, POS_NUM_58_CLAMECY % 256 }, "09/2202_58_Clamecy.mp3", true, 2, 1970L },
  { BASE_POS_58_COSNE_COURS_SUR_LOIRE, { 0x14, 0x90 + POS_NUM_58_COSNE_COURS_SUR_LOIRE / 256, POS_NUM_58_COSNE_COURS_SUR_LOIRE % 256 }, "09/2203_58_Cosne_Cours_sur_Loire.mp3", true, 3, 2832L },
  { BASE_POS_58_NEVERS, { 0x14, 0x90 + POS_NUM_58_NEVERS / 256, POS_NUM_58_NEVERS % 256 }, "09/2204_58_Nevers.mp3", true, 2, 1813L },
  { BASE_POS_59_AVESNES_SUR_HELPE, { 0x14, 0x90 + POS_NUM_59_AVESNES_SUR_HELPE / 256, POS_NUM_59_AVESNES_SUR_HELPE % 256 }, "09/2205_59_Avesnes_sur_Helpe.mp3", true, 3, 2519L },
  { BASE_POS_59_CAMBRAI, { 0x14, 0x90 + POS_NUM_59_CAMBRAI / 256, POS_NUM_59_CAMBRAI % 256 }, "09/2206_59_Cambrai.mp3", true, 2, 1774L },
  { BASE_POS_59_DOUAI, { 0x14, 0x90 + POS_NUM_59_DOUAI / 256, POS_NUM_59_DOUAI % 256 }, "09/2207_59_Douai.mp3", true, 2, 1617L },
  { BASE_POS_59_DUNKERQUE, { 0x14, 0x90 + POS_NUM_59_DUNKERQUE / 256, POS_NUM_59_DUNKERQUE % 256 }, "09/2208_59_Dunkerque.mp3", true, 2, 2205L },
  { BASE_POS_59_LILLE, { 0x14, 0x90 + POS_NUM_59_LILLE / 256, POS_NUM_59_LILLE % 256 }, "09/2209_59_Lille.mp3", true, 2, 1539L },
  { BASE_POS_59_VALENCIENNES, { 0x14, 0x90 + POS_NUM_59_VALENCIENNES / 256, POS_NUM_59_VALENCIENNES % 256 }, "09/2210_59_Valenciennes.mp3", true, 2, 2205L },
  { BASE_POS_60_BEAUVAIS, { 0x14, 0x90 + POS_NUM_60_BEAUVAIS / 256, POS_NUM_60_BEAUVAIS % 256 }, "09/2211_60_Beauvais.mp3", true, 2, 1578L },
  { BASE_POS_60_CLERMONT, { 0x14, 0x90 + POS_NUM_60_CLERMONT / 256, POS_NUM_60_CLERMONT % 256 }, "09/2212_60_Clermont.mp3", true, 2, 1696L },
  { BASE_POS_60_COMPIEGNE, { 0x14, 0x90 + POS_NUM_60_COMPIEGNE / 256, POS_NUM_60_COMPIEGNE % 256 }, "09/2213_60_Compiegne.mp3", true, 2, 1735L },
  { BASE_POS_60_SENLIS, { 0x14, 0x90 + POS_NUM_60_SENLIS / 256, POS_NUM_60_SENLIS % 256 }, "09/2214_60_Senlis.mp3", true, 2, 1931L },
  { BASE_POS_61_ALENCON, { 0x14, 0x90 + POS_NUM_61_ALENCON / 256, POS_NUM_61_ALENCON % 256 }, "09/2215_61_Alencon.mp3", true, 2, 1657L },
  { BASE_POS_61_ARGENTAN, { 0x14, 0x90 + POS_NUM_61_ARGENTAN / 256, POS_NUM_61_ARGENTAN % 256 }, "09/2216_61_Argentan.mp3", true, 2, 1735L },
  { BASE_POS_61_MORTAGNE_AU_PERCHE, { 0x14, 0x90 + POS_NUM_61_MORTAGNE_AU_PERCHE / 256, POS_NUM_61_MORTAGNE_AU_PERCHE % 256 }, "09/2217_61_Mortagne_au_Perche.mp3", true, 3, 2597L },
  { BASE_POS_62_ARRAS, { 0x14, 0x90 + POS_NUM_62_ARRAS / 256, POS_NUM_62_ARRAS % 256 }, "09/2218_62_Arras.mp3", true, 2, 2244L },
  { BASE_POS_62_BETHUNE, { 0x14, 0x90 + POS_NUM_62_BETHUNE / 256, POS_NUM_62_BETHUNE % 256 }, "09/2219_62_Bethune.mp3", true, 2, 2166L },
  { BASE_POS_62_BOULOGNE_SUR_MER, { 0x14, 0x90 + POS_NUM_62_BOULOGNE_SUR_MER / 256, POS_NUM_62_BOULOGNE_SUR_MER % 256 }, "09/2220_62_Boulogne_sur_Mer.mp3", true, 3, 3028L },
  { BASE_POS_62_CALAIS, { 0x14, 0x90 + POS_NUM_62_CALAIS / 256, POS_NUM_62_CALAIS % 256 }, "09/2221_62_Calais.mp3", true, 2, 1970L },
  { BASE_POS_62_LENS, { 0x14, 0x90 + POS_NUM_62_LENS / 256, POS_NUM_62_LENS % 256 }, "09/2222_62_Lens.mp3", true, 2, 1970L },
  { BASE_POS_62_MONTREUIL, { 0x14, 0x90 + POS_NUM_62_MONTREUIL / 256, POS_NUM_62_MONTREUIL % 256 }, "09/2223_62_Montreuil.mp3", true, 2, 2323L },
  { BASE_POS_62_SAINT_OMER, { 0x14, 0x90 + POS_NUM_62_SAINT_OMER / 256, POS_NUM_62_SAINT_OMER % 256 }, "09/2224_62_Saint_Omer.mp3", true, 3, 2519L },
  { BASE_POS_63_AMBERT, { 0x14, 0x90 + POS_NUM_63_AMBERT / 256, POS_NUM_63_AMBERT % 256 }, "09/2225_63_Ambert.mp3", true, 2, 2127L },
  { BASE_POS_63_CLERMONT_FERRAND, { 0x14, 0x90 + POS_NUM_63_CLERMONT_FERRAND / 256, POS_NUM_63_CLERMONT_FERRAND % 256 }, "09/2226_63_Clermont_Ferrand.mp3", true, 3, 2832L },
  { BASE_POS_63_ISSOIRE, { 0x14, 0x90 + POS_NUM_63_ISSOIRE / 256, POS_NUM_63_ISSOIRE % 256 }, "09/2227_63_Issoire.mp3", true, 2, 2127L },
  { BASE_POS_63_RIOM, { 0x14, 0x90 + POS_NUM_63_RIOM / 256, POS_NUM_63_RIOM % 256 }, "09/2228_63_Riom.mp3", true, 2, 2166L },
  { BASE_POS_63_THIERS, { 0x14, 0x90 + POS_NUM_63_THIERS / 256, POS_NUM_63_THIERS % 256 }, "09/2229_63_Thiers.mp3", true, 2, 2166L },
  { BASE_POS_64_BAYONNE, { 0x14, 0x90 + POS_NUM_64_BAYONNE / 256, POS_NUM_64_BAYONNE % 256 }, "09/2230_64_Bayonne.mp3", true, 3, 3106L },
  { BASE_POS_64_OLORON_SAINTE_MARIE, { 0x14, 0x90 + POS_NUM_64_OLORON_SAINTE_MARIE / 256, POS_NUM_64_OLORON_SAINTE_MARIE % 256 }, "09/2231_64_Oloron_Sainte_Marie.mp3", true, 4, 3929L },
  { BASE_POS_64_PAU, { 0x14, 0x90 + POS_NUM_64_PAU / 256, POS_NUM_64_PAU % 256 }, "09/2232_64_Pau.mp3", true, 3, 2754L },
  { BASE_POS_65_ARGELES_GAZOST, { 0x14, 0x90 + POS_NUM_65_ARGELES_GAZOST / 256, POS_NUM_65_ARGELES_GAZOST % 256 }, "09/2233_65_Argeles_Gazost.mp3", true, 4, 3694L },
  { BASE_POS_65_BAGNERES_DE_BIGORRE, { 0x14, 0x90 + POS_NUM_65_BAGNERES_DE_BIGORRE / 256, POS_NUM_65_BAGNERES_DE_BIGORRE % 256 }, "09/2234_65_Bagneres_de_Bigorre.mp3", true, 3, 3341L },
  { BASE_POS_65_TARBES, { 0x14, 0x90 + POS_NUM_65_TARBES / 256, POS_NUM_65_TARBES % 256 }, "09/2235_65_Tarbes.mp3", true, 2, 2323L },
  { BASE_POS_66_CERET, { 0x14, 0x90 + POS_NUM_66_CERET / 256, POS_NUM_66_CERET % 256 }, "09/2236_66_Ceret.mp3", true, 3, 3067L },
  { BASE_POS_66_PERPIGNAN, { 0x14, 0x90 + POS_NUM_66_PERPIGNAN / 256, POS_NUM_66_PERPIGNAN % 256 }, "09/2237_66_Perpignan.mp3", true, 3, 3341L },
  { BASE_POS_66_PRADES, { 0x14, 0x90 + POS_NUM_66_PRADES / 256, POS_NUM_66_PRADES % 256 }, "09/2238_66_Prades.mp3", true, 3, 3067L },
  { BASE_POS_67_HAGUENAU, { 0x14, 0x90 + POS_NUM_67_HAGUENAU / 256, POS_NUM_67_HAGUENAU % 256 }, "09/2239_67_Haguenau.mp3", true, 2, 1774L },
  { BASE_POS_67_MOLSHEIM, { 0x14, 0x90 + POS_NUM_67_MOLSHEIM / 256, POS_NUM_67_MOLSHEIM % 256 }, "09/2240_67_Molsheim.mp3", true, 2, 2127L },
  { BASE_POS_67_SAVERNE, { 0x14, 0x90 + POS_NUM_67_SAVERNE / 256, POS_NUM_67_SAVERNE % 256 }, "09/2241_67_Saverne.mp3", true, 2, 2009L },
  { BASE_POS_67_SELESTAT, { 0x14, 0x90 + POS_NUM_67_SELESTAT / 256, POS_NUM_67_SELESTAT % 256 }, "09/2242_67_Selestat.mp3", true, 2, 1931L },
  { BASE_POS_67_STRASBOURG, { 0x14, 0x90 + POS_NUM_67_STRASBOURG / 256, POS_NUM_67_STRASBOURG % 256 }, "09/2243_67_Strasbourg.mp3", true, 2, 2166L },
  { BASE_POS_67_WISSEMBOURG, { 0x14, 0x90 + POS_NUM_67_WISSEMBOURG / 256, POS_NUM_67_WISSEMBOURG % 256 }, "09/2244_67_Wissembourg.mp3", true, 2, 2205L },
  { BASE_POS_68_ALTKIRCH, { 0x14, 0x90 + POS_NUM_68_ALTKIRCH / 256, POS_NUM_68_ALTKIRCH % 256 }, "09/2245_68_Altkirch.mp3", true, 2, 2205L },
  { BASE_POS_68_COLMAR, { 0x14, 0x90 + POS_NUM_68_COLMAR / 256, POS_NUM_68_COLMAR % 256 }, "09/2246_68_Colmar.mp3", true, 2, 1813L },
  { BASE_POS_68_GUEBWILLER, { 0x14, 0x90 + POS_NUM_68_GUEBWILLER / 256, POS_NUM_68_GUEBWILLER % 256 }, "09/2247_68_Guebwiller.mp3", true, 2, 1970L },
  { BASE_POS_68_MULHOUSE, { 0x14, 0x90 + POS_NUM_68_MULHOUSE / 256, POS_NUM_68_MULHOUSE % 256 }, "09/2248_68_Mulhouse.mp3", true, 2, 1852L },
  { BASE_POS_68_RIBEAUVILLE, { 0x14, 0x90 + POS_NUM_68_RIBEAUVILLE / 256, POS_NUM_68_RIBEAUVILLE % 256 }, "09/2249_68_Ribeauville.mp3", true, 2, 2166L },
  { BASE_POS_68_THANN, { 0x14, 0x90 + POS_NUM_68_THANN / 256, POS_NUM_68_THANN % 256 }, "09/2250_68_Thann.mp3", true, 1, 1343L },
  { BASE_POS_69_LYON, { 0x14, 0x90 + POS_NUM_69_LYON / 256, POS_NUM_69_LYON % 256 }, "09/2251_69_Lyon.mp3", true, 2, 1539L },
  { BASE_POS_69_VILLEFRANCHE_SUR_SAONE, { 0x14, 0x90 + POS_NUM_69_VILLEFRANCHE_SUR_SAONE / 256, POS_NUM_69_VILLEFRANCHE_SUR_SAONE % 256 }, "09/2252_69_Villefranche_sur_Saone.mp3", true, 3, 2871L },
  { BASE_POS_70_LURE, { 0x14, 0x90 + POS_NUM_70_LURE / 256, POS_NUM_70_LURE % 256 }, "09/2253_70_Lure.mp3", true, 2, 1892L },
  { BASE_POS_70_VESOUL, { 0x14, 0x90 + POS_NUM_70_VESOUL / 256, POS_NUM_70_VESOUL % 256 }, "09/2254_70_Vesoul.mp3", true, 2, 2048L },
  { BASE_POS_71_AUTUN, { 0x14, 0x90 + POS_NUM_71_AUTUN / 256, POS_NUM_71_AUTUN % 256 }, "09/2255_71_Autun.mp3", true, 2, 2362L },
  { BASE_POS_71_CHALON_SUR_SAONE, { 0x14, 0x90 + POS_NUM_71_CHALON_SUR_SAONE / 256, POS_NUM_71_CHALON_SUR_SAONE % 256 }, "09/2256_71_Chalon_sur_Saone.mp3", true, 3, 3067L },
  { BASE_POS_71_CHAROLLES, { 0x14, 0x90 + POS_NUM_71_CHAROLLES / 256, POS_NUM_71_CHAROLLES % 256 }, "09/2257_71_Charolles.mp3", true, 2, 2479L },
  { BASE_POS_71_LOUHANS, { 0x14, 0x90 + POS_NUM_71_LOUHANS / 256, POS_NUM_71_LOUHANS % 256 }, "09/2258_71_Louhans.mp3", true, 3, 2597L },
  { BASE_POS_71_MACON, { 0x14, 0x90 + POS_NUM_71_MACON / 256, POS_NUM_71_MACON % 256 }, "09/2259_71_Macon.mp3", true, 2, 2401L },
  { BASE_POS_72_LA_FLECHE, { 0x14, 0x90 + POS_NUM_72_LA_FLECHE / 256, POS_NUM_72_LA_FLECHE % 256 }, "09/2260_72_la_Fleche.mp3", true, 2, 2283L },
  { BASE_POS_72_MAMERS, { 0x14, 0x90 + POS_NUM_72_MAMERS / 256, POS_NUM_72_MAMERS % 256 }, "09/2261_72_Mamers.mp3", true, 2, 2088L },
  { BASE_POS_72_LE_MANS, { 0x14, 0x90 + POS_NUM_72_LE_MANS / 256, POS_NUM_72_LE_MANS % 256 }, "09/2262_72_le_Mans.mp3", true, 2, 1970L },
  { BASE_POS_72_MANS, { 0x14, 0x90 + POS_NUM_72_MANS / 256, POS_NUM_72_MANS % 256 }, "09/2263_72_Mans.mp3", true, 2, 1774L },
  { BASE_POS_73_ALBERTVILLE, { 0x14, 0x90 + POS_NUM_73_ALBERTVILLE / 256, POS_NUM_73_ALBERTVILLE % 256 }, "09/2264_73_Albertville.mp3", true, 2, 2205L },
  { BASE_POS_73_CHAMBERY, { 0x14, 0x90 + POS_NUM_73_CHAMBERY / 256, POS_NUM_73_CHAMBERY % 256 }, "09/2265_73_Chambery.mp3", true, 2, 2009L },
  { BASE_POS_73_SAINT_JEAN_DE_MAURIENNE, { 0x14, 0x90 + POS_NUM_73_SAINT_JEAN_DE_MAURIENNE / 256, POS_NUM_73_SAINT_JEAN_DE_MAURIENNE % 256 }, "09/2266_73_Saint_Jean_de_Maurienne.mp3", true, 3, 2989L },
  { BASE_POS_74_ANNECY, { 0x14, 0x90 + POS_NUM_74_ANNECY / 256, POS_NUM_74_ANNECY % 256 }, "09/2267_74_Annecy.mp3", true, 2, 2283L },
  { BASE_POS_74_BONNEVILLE, { 0x14, 0x90 + POS_NUM_74_BONNEVILLE / 256, POS_NUM_74_BONNEVILLE % 256 }, "09/2268_74_Bonneville.mp3", true, 3, 2558L },
  { BASE_POS_74_SAINT_JULIEN_EN_GENEVOIS, { 0x14, 0x90 + POS_NUM_74_SAINT_JULIEN_EN_GENEVOIS / 256, POS_NUM_74_SAINT_JULIEN_EN_GENEVOIS % 256 }, "09/2269_74_Saint_Julien_en_Genevois.mp3", true, 4, 3577L },
  { BASE_POS_74_THONON_LES_BAINS, { 0x14, 0x90 + POS_NUM_74_THONON_LES_BAINS / 256, POS_NUM_74_THONON_LES_BAINS % 256 }, "09/2270_74_Thonon_les_Bains.mp3", true, 3, 2675L },
  { BASE_POS_75_PARIS, { 0x14, 0x90 + POS_NUM_75_PARIS / 256, POS_NUM_75_PARIS % 256 }, "09/2271_75_Paris.mp3", true, 1, 794L },
  { BASE_POS_76_DIEPPE, { 0x14, 0x90 + POS_NUM_76_DIEPPE / 256, POS_NUM_76_DIEPPE % 256 }, "09/2272_76_Dieppe.mp3", true, 2, 2362L },
  { BASE_POS_76_LE_HAVRE, { 0x14, 0x90 + POS_NUM_76_LE_HAVRE / 256, POS_NUM_76_LE_HAVRE % 256 }, "09/2273_76_le_Havre.mp3", true, 3, 2558L },
  { BASE_POS_76_HAVRE, { 0x14, 0x90 + POS_NUM_76_HAVRE / 256, POS_NUM_76_HAVRE % 256 }, "09/2274_76_Havre.mp3", true, 3, 2636L },
  { BASE_POS_76_ROUEN, { 0x14, 0x90 + POS_NUM_76_ROUEN / 256, POS_NUM_76_ROUEN % 256 }, "09/2275_76_Rouen.mp3", true, 2, 2479L },
  { BASE_POS_77_FONTAINEBLEAU, { 0x14, 0x90 + POS_NUM_77_FONTAINEBLEAU / 256, POS_NUM_77_FONTAINEBLEAU % 256 }, "09/2276_77_Fontainebleau.mp3", true, 3, 2910L },
  { BASE_POS_77_MEAUX, { 0x14, 0x90 + POS_NUM_77_MEAUX / 256, POS_NUM_77_MEAUX % 256 }, "09/2277_77_Meaux.mp3", true, 2, 2088L },
  { BASE_POS_77_MELUN, { 0x14, 0x90 + POS_NUM_77_MELUN / 256, POS_NUM_77_MELUN % 256 }, "09/2278_77_Melun.mp3", true, 2, 2166L },
  { BASE_POS_77_PROVINS, { 0x14, 0x90 + POS_NUM_77_PROVINS / 256, POS_NUM_77_PROVINS % 256 }, "09/2279_77_Provins.mp3", true, 2, 2323L },
  { BASE_POS_77_TORCY, { 0x14, 0x90 + POS_NUM_77_TORCY / 256, POS_NUM_77_TORCY % 256 }, "09/2280_77_Torcy.mp3", true, 2, 2401L },
  { BASE_POS_78_MANTES_LA_JOLIE, { 0x14, 0x90 + POS_NUM_78_MANTES_LA_JOLIE / 256, POS_NUM_78_MANTES_LA_JOLIE % 256 }, "09/2281_78_Mantes_la_Jolie.mp3", true, 3, 2558L },
  { BASE_POS_78_RAMBOUILLET, { 0x14, 0x90 + POS_NUM_78_RAMBOUILLET / 256, POS_NUM_78_RAMBOUILLET % 256 }, "09/2282_78_Rambouillet.mp3", true, 2, 2244L },
  { BASE_POS_78_SAINT_GERMAIN_EN_LAYE, { 0x14, 0x90 + POS_NUM_78_SAINT_GERMAIN_EN_LAYE / 256, POS_NUM_78_SAINT_GERMAIN_EN_LAYE % 256 }, "09/2283_78_Saint_Germain_en_Laye.mp3", true, 3, 2910L },
  { BASE_POS_78_VERSAILLES, { 0x14, 0x90 + POS_NUM_78_VERSAILLES / 256, POS_NUM_78_VERSAILLES % 256 }, "09/2284_78_Versailles.mp3", true, 2, 2166L },
  { BASE_POS_79_BRESSUIRE, { 0x14, 0x90 + POS_NUM_79_BRESSUIRE / 256, POS_NUM_79_BRESSUIRE % 256 }, "09/2285_79_Bressuire.mp3", true, 2, 2323L },
  { BASE_POS_79_NIORT, { 0x14, 0x90 + POS_NUM_79_NIORT / 256, POS_NUM_79_NIORT % 256 }, "09/2286_79_Niort.mp3", true, 2, 1970L },
  { BASE_POS_79_PARTHENAY, { 0x14, 0x90 + POS_NUM_79_PARTHENAY / 256, POS_NUM_79_PARTHENAY % 256 }, "09/2287_79_Parthenay.mp3", true, 2, 2244L },
  { BASE_POS_80_ABBEVILLE, { 0x14, 0x90 + POS_NUM_80_ABBEVILLE / 256, POS_NUM_80_ABBEVILLE % 256 }, "09/2288_80_Abbeville.mp3", true, 2, 1852L },
  { BASE_POS_80_AMIENS, { 0x14, 0x90 + POS_NUM_80_AMIENS / 256, POS_NUM_80_AMIENS % 256 }, "09/2289_80_Amiens.mp3", true, 2, 1657L },
  { BASE_POS_80_MONTDIDIER, { 0x14, 0x90 + POS_NUM_80_MONTDIDIER / 256, POS_NUM_80_MONTDIDIER % 256 }, "09/2290_80_Montdidier.mp3", true, 2, 2009L },
  { BASE_POS_80_PERONNE, { 0x14, 0x90 + POS_NUM_80_PERONNE / 256, POS_NUM_80_PERONNE % 256 }, "09/2291_80_Peronne.mp3", true, 2, 1696L },
  { BASE_POS_81_ALBI, { 0x14, 0x90 + POS_NUM_81_ALBI / 256, POS_NUM_81_ALBI % 256 }, "09/2292_81_Albi.mp3", true, 2, 1774L },
  { BASE_POS_81_CASTRES, { 0x14, 0x90 + POS_NUM_81_CASTRES / 256, POS_NUM_81_CASTRES % 256 }, "09/2293_81_Castres.mp3", true, 2, 1774L },
  { BASE_POS_82_CASTELSARRASIN, { 0x14, 0x90 + POS_NUM_82_CASTELSARRASIN / 256, POS_NUM_82_CASTELSARRASIN % 256 }, "09/2294_82_Castelsarrasin.mp3", true, 3, 3420L },
  { BASE_POS_82_MONTAUBAN, { 0x14, 0x90 + POS_NUM_82_MONTAUBAN / 256, POS_NUM_82_MONTAUBAN % 256 }, "09/2295_82_Montauban.mp3", true, 3, 2793L },
  { BASE_POS_83_BRIGNOLES, { 0x14, 0x90 + POS_NUM_83_BRIGNOLES / 256, POS_NUM_83_BRIGNOLES % 256 }, "09/2296_83_Brignoles.mp3", true, 2, 2009L },
  { BASE_POS_83_DRAGUIGNAN, { 0x14, 0x90 + POS_NUM_83_DRAGUIGNAN / 256, POS_NUM_83_DRAGUIGNAN % 256 }, "09/2297_83_Draguignan.mp3", true, 2, 2127L },
  { BASE_POS_83_TOULON, { 0x14, 0x90 + POS_NUM_83_TOULON / 256, POS_NUM_83_TOULON % 256 }, "09/2298_83_Toulon.mp3", true, 2, 1539L },
  { BASE_POS_84_APT, { 0x14, 0x90 + POS_NUM_84_APT / 256, POS_NUM_84_APT % 256 }, "09/2299_84_Apt.mp3", true, 2, 2127L },
  { BASE_POS_84_AVIGNON, { 0x14, 0x90 + POS_NUM_84_AVIGNON / 256, POS_NUM_84_AVIGNON % 256 }, "09/2300_84_Avignon.mp3", true, 2, 2166L },
  { BASE_POS_84_CARPENTRAS, { 0x14, 0x90 + POS_NUM_84_CARPENTRAS / 256, POS_NUM_84_CARPENTRAS % 256 }, "09/2301_84_Carpentras.mp3", true, 2, 2362L },
  { BASE_POS_85_FONTENAY_LE_COMTE, { 0x14, 0x90 + POS_NUM_85_FONTENAY_LE_COMTE / 256, POS_NUM_85_FONTENAY_LE_COMTE % 256 }, "09/2302_85_Fontenay_le_Comte.mp3", true, 3, 2636L },
  { BASE_POS_85_LA_ROCHE_SUR_YON, { 0x14, 0x90 + POS_NUM_85_LA_ROCHE_SUR_YON / 256, POS_NUM_85_LA_ROCHE_SUR_YON % 256 }, "09/2303_85_la_Roche_sur_Yon.mp3", true, 2, 2401L },
  { BASE_POS_85_LES_SABLES_D_OLONNE, { 0x14, 0x90 + POS_NUM_85_LES_SABLES_D_OLONNE / 256, POS_NUM_85_LES_SABLES_D_OLONNE % 256 }, "09/2304_85_les_Sables_d_Olonne.mp3", true, 2, 2479L },
  { BASE_POS_85_SABLES_D_OLONNE, { 0x14, 0x90 + POS_NUM_85_SABLES_D_OLONNE / 256, POS_NUM_85_SABLES_D_OLONNE % 256 }, "09/2305_85_Sables_d_Olonne.mp3", true, 2, 2244L },
  { BASE_POS_86_CHATELLERAULT, { 0x14, 0x90 + POS_NUM_86_CHATELLERAULT / 256, POS_NUM_86_CHATELLERAULT % 256 }, "09/2306_86_Chatellerault.mp3", true, 2, 2048L },
  { BASE_POS_86_MONTMORILLON, { 0x14, 0x90 + POS_NUM_86_MONTMORILLON / 256, POS_NUM_86_MONTMORILLON % 256 }, "09/2307_86_Montmorillon.mp3", true, 2, 2127L },
  { BASE_POS_86_POITIERS, { 0x14, 0x90 + POS_NUM_86_POITIERS / 256, POS_NUM_86_POITIERS % 256 }, "09/2308_86_Poitiers.mp3", true, 2, 1735L },
  { BASE_POS_87_BELLAC, { 0x14, 0x90 + POS_NUM_87_BELLAC / 256, POS_NUM_87_BELLAC % 256 }, "09/2309_87_Bellac.mp3", true, 2, 2205L },
  { BASE_POS_87_LIMOGES, { 0x14, 0x90 + POS_NUM_87_LIMOGES / 256, POS_NUM_87_LIMOGES % 256 }, "09/2310_87_Limoges.mp3", true, 2, 2088L },
  { BASE_POS_87_ROCHECHOUART, { 0x14, 0x90 + POS_NUM_87_ROCHECHOUART / 256, POS_NUM_87_ROCHECHOUART % 256 }, "09/2311_87_Rochechouart.mp3", true, 2, 2440L },
  { BASE_POS_88_EPINAL, { 0x14, 0x90 + POS_NUM_88_EPINAL / 256, POS_NUM_88_EPINAL % 256 }, "09/2312_88_Epinal.mp3", true, 2, 1852L },
  { BASE_POS_88_NEUFCHATEAU, { 0x14, 0x90 + POS_NUM_88_NEUFCHATEAU / 256, POS_NUM_88_NEUFCHATEAU % 256 }, "09/2313_88_Neufchateau.mp3", true, 2, 2205L },
  { BASE_POS_88_SAINT_DIE_DES_VOSGES, { 0x14, 0x90 + POS_NUM_88_SAINT_DIE_DES_VOSGES / 256, POS_NUM_88_SAINT_DIE_DES_VOSGES % 256 }, "09/2314_88_Saint_Die_des_Vosges.mp3", true, 3, 2675L },
  { BASE_POS_89_AUXERRE, { 0x14, 0x90 + POS_NUM_89_AUXERRE / 256, POS_NUM_89_AUXERRE % 256 }, "09/2315_89_Auxerre.mp3", true, 2, 1774L },
  { BASE_POS_89_AVALLON, { 0x14, 0x90 + POS_NUM_89_AVALLON / 256, POS_NUM_89_AVALLON % 256 }, "09/2316_89_Avallon.mp3", true, 2, 1735L },
  { BASE_POS_89_SENS, { 0x14, 0x90 + POS_NUM_89_SENS / 256, POS_NUM_89_SENS % 256 }, "09/2317_89_Sens.mp3", true, 2, 1696L },
  { BASE_POS_90_BELFORT, { 0x14, 0x90 + POS_NUM_90_BELFORT / 256, POS_NUM_90_BELFORT % 256 }, "09/2318_90_Belfort.mp3", true, 3, 3302L },
  { BASE_POS_91_ETAMPES, { 0x14, 0x90 + POS_NUM_91_ETAMPES / 256, POS_NUM_91_ETAMPES % 256 }, "09/2319_91_Etampes.mp3", true, 2, 1774L },
  { BASE_POS_91_EVRY, { 0x14, 0x90 + POS_NUM_91_EVRY / 256, POS_NUM_91_EVRY % 256 }, "09/2320_91_Evry.mp3", true, 2, 1657L },
  { BASE_POS_91_PALAISEAU, { 0x14, 0x90 + POS_NUM_91_PALAISEAU / 256, POS_NUM_91_PALAISEAU % 256 }, "09/2321_91_Palaiseau.mp3", true, 2, 1813L },
  { BASE_POS_92_ANTONY, { 0x14, 0x90 + POS_NUM_92_ANTONY / 256, POS_NUM_92_ANTONY % 256 }, "09/2322_92_Antony.mp3", true, 2, 2283L },
  { BASE_POS_92_BOULOGNE_BILLANCOURT, { 0x14, 0x90 + POS_NUM_92_BOULOGNE_BILLANCOURT / 256, POS_NUM_92_BOULOGNE_BILLANCOURT % 256 }, "09/2323_92_Boulogne_Billancourt.mp3", true, 3, 3146L },
  { BASE_POS_92_NANTERRE, { 0x14, 0x90 + POS_NUM_92_NANTERRE / 256, POS_NUM_92_NANTERRE % 256 }, "09/2324_92_Nanterre.mp3", true, 2, 2244L },
  { BASE_POS_93_BOBIGNY, { 0x14, 0x90 + POS_NUM_93_BOBIGNY / 256, POS_NUM_93_BOBIGNY % 256 }, "09/2325_93_Bobigny.mp3", true, 3, 2675L },
  { BASE_POS_93_LE_RAINCY, { 0x14, 0x90 + POS_NUM_93_LE_RAINCY / 256, POS_NUM_93_LE_RAINCY % 256 }, "09/2326_93_le_Raincy.mp3", true, 3, 2636L },
  { BASE_POS_93_RAINCY, { 0x14, 0x90 + POS_NUM_93_RAINCY / 256, POS_NUM_93_RAINCY % 256 }, "09/2327_93_Raincy.mp3", true, 3, 2558L },
  { BASE_POS_93_SAINT_DENIS, { 0x14, 0x90 + POS_NUM_93_SAINT_DENIS / 256, POS_NUM_93_SAINT_DENIS % 256 }, "09/2328_93_Saint_Denis.mp3", true, 3, 2675L },
  { BASE_POS_94_CRETEIL, { 0x14, 0x90 + POS_NUM_94_CRETEIL / 256, POS_NUM_94_CRETEIL % 256 }, "09/2329_94_Creteil.mp3", true, 2, 2323L },
  { BASE_POS_94_L_HAY_LES_ROSES, { 0x14, 0x90 + POS_NUM_94_L_HAY_LES_ROSES / 256, POS_NUM_94_L_HAY_LES_ROSES % 256 }, "09/2330_94_l_Hay_les_Roses.mp3", true, 3, 2558L },
  { BASE_POS_94_NOGENT_SUR_MARNE, { 0x14, 0x90 + POS_NUM_94_NOGENT_SUR_MARNE / 256, POS_NUM_94_NOGENT_SUR_MARNE % 256 }, "09/2331_94_Nogent_sur_Marne.mp3", true, 3, 3028L },
  { BASE_POS_95_ARGENTEUIL, { 0x14, 0x90 + POS_NUM_95_ARGENTEUIL / 256, POS_NUM_95_ARGENTEUIL % 256 }, "09/2332_95_Argenteuil.mp3", true, 3, 2597L },
  { BASE_POS_95_CERGY_PONTOISE, { 0x14, 0x90 + POS_NUM_95_CERGY_PONTOISE / 256, POS_NUM_95_CERGY_PONTOISE % 256 }, "09/2333_95_Cergy_Pontoise.mp3", true, 3, 3146L },
  { BASE_POS_95_SARCELLES, { 0x14, 0x90 + POS_NUM_95_SARCELLES / 256, POS_NUM_95_SARCELLES % 256 }, "09/2334_95_Sarcelles.mp3", true, 2, 2401L },

  { BASE_LAST_PROMPT_POS_COMMUNES, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

// 484 elements

// Definitions des communes avec "liaison" dans la phrase "aux alentours d'..." (RULE_1_SINGULAR) ou "aux alentours des..." (RULE_1_SINGULAR)
// TOOD: Set the duration of the prompts...
static const ST_PROMPTS_DEF              g__prompts_pos2_communes[BASE_LAST_PROMPT_POS2_COMMUNES + 1] =
{
  { BASE_POS_D_ALLONNES, { 0x14, 0x80 + POS2_NUM_D_ALLONNES / 256, POS2_NUM_D_ALLONNES % 256 }, "08/1500_d_Allonnes.mp3", true, 1, 0L },
  { BASE_POS_D_ANDELU, { 0x14, 0x80 + POS2_NUM_D_ANDELU / 256, POS2_NUM_D_ANDELU % 256 }, "08/1501_d_Andelu.mp3", true, 1, 0L },
  { BASE_POS_D_AUFFARGIS, { 0x14, 0x80 + POS2_NUM_D_AUFFARGIS / 256, POS2_NUM_D_AUFFARGIS % 256 }, "08/1502_d_Auffargis.mp3", true, 1, 0L },
  { BASE_POS_D_AUTEUIL, { 0x14, 0x80 + POS2_NUM_D_AUTEUIL / 256, POS2_NUM_D_AUTEUIL % 256 }, "08/1503_d_Auteuil.mp3", true, 1, 0L },
  { BASE_POS_D_AUTOUILLET, { 0x14, 0x80 + POS2_NUM_D_AUTOUILLET / 256, POS2_NUM_D_AUTOUILLET % 256 }, "08/1504_d_Autouillet.mp3", true, 1, 0L },
  { BASE_POS_D_ELANCOURT, { 0x14, 0x80 + POS2_NUM_D_ELANCOURT / 256, POS2_NUM_D_ELANCOURT % 256 }, "08/1505_d_Elancourt.mp3", true, 1, 0L },
  { BASE_POS_D_ELANCOURT_LA_CLE_DE_SAINT_PIERRE, { 0x14, 0x80 + POS2_NUM_D_ELANCOURT_LA_CLE_DE_SAINT_PIERRE / 256, POS2_NUM_D_ELANCOURT_LA_CLE_DE_SAINT_PIERRE % 256 }, "08/1506_d_Elancourt_La_Cle_de_Saint_Pierre.mp3", true, 1, 0L },
  { BASE_POS_D_ELANCOURT_VILLAGE, { 0x14, 0x80 + POS2_NUM_D_ELANCOURT_VILLAGE / 256, POS2_NUM_D_ELANCOURT_VILLAGE % 256 }, "08/1507_d_Elancourt_Village.mp3", true, 1, 0L },
  { BASE_POS_D_ETANG_LA_VILLE, { 0x14, 0x80 + POS2_NUM_D_ETANG_LA_VILLE / 256, POS2_NUM_D_ETANG_LA_VILLE % 256 }, "08/1508_d_Etang_la_Ville.mp3", true, 1, 0L },
  { BASE_POS_D_OSMOY, { 0x14, 0x80 + POS2_NUM_D_OSMOY / 256, POS2_NUM_D_OSMOY % 256 }, "08/1509_d_Osmoy.mp3", true, 1, 0L },
  { BASE_POS_DE_PORT_MARLY, { 0x14, 0x80 + POS2_NUM_DE_PORT_MARLY / 256, POS2_NUM_DE_PORT_MARLY % 256 }, "08/1510_de_Port_Marly.mp3", true, 1, 0L },
  { BASE_POS_DES_BREVIAIRES, { 0x14, 0x80 + POS2_NUM_DES_BREVIAIRES / 256, POS2_NUM_DES_BREVIAIRES % 256 }, "08/1511_des_Breviaires.mp3", true, 1, 0L },
  { BASE_POS_DES_CLAYES_SOUS_BOIS, { 0x14, 0x80 + POS2_NUM_DES_CLAYES_SOUS_BOIS / 256, POS2_NUM_DES_CLAYES_SOUS_BOIS % 256 }, "08/1512_des_Clayes_sous_Bois.mp3", true, 1, 0L },
  { BASE_POS_DES_ESSARTS_LE_ROI, { 0x14, 0x80 + POS2_NUM_DES_ESSARTS_LE_ROI / 256, POS2_NUM_DES_ESSARTS_LE_ROI % 256 }, "08/1513_des_Essarts_le_Roi.mp3", true, 1, 0L },
  { BASE_POS_DES_LOGES_EN_JOSAS, { 0x14, 0x80 + POS2_NUM_DES_LOGES_EN_JOSAS / 256, POS2_NUM_DES_LOGES_EN_JOSAS % 256 }, "08/1514_des_Loges_en_Josas.mp3", true, 1, 0L },
  { BASE_POS_DES_MESNULS, { 0x14, 0x80 + POS2_NUM_DES_MESNULS / 256, POS2_NUM_DES_MESNULS % 256 }, "08/1515_des_Mesnuls.mp3", true, 1, 0L },
  { BASE_POS_DES_MOLIERES, { 0x14, 0x80 + POS2_NUM_DES_MOLIERES / 256, POS2_NUM_DES_MOLIERES % 256 }, "08/1516_des_Molieres.mp3", true, 1, 0L },
  { BASE_POS_DES_ULMES, { 0x14, 0x80 + POS2_NUM_DES_ULMES / 256, POS2_NUM_DES_ULMES % 256 }, "08/1517_des_Ulmes.mp3", true, 1, 0L },
  { BASE_POS_DU_MESNIL_SAINT_DENIS, { 0x14, 0x80 + POS2_NUM_DU_MESNIL_SAINT_DENIS / 256, POS2_NUM_DU_MESNIL_SAINT_DENIS % 256 }, "08/1518_du_Mesnil_Saint_Denis.mp3", true, 1, 0L },
  { BASE_POS_DU_PECQ, { 0x14, 0x80 + POS2_NUM_DU_PECQ / 256, POS2_NUM_DU_PECQ % 256 }, "08/1519_du_Pecq.mp3", true, 1, 0L },
  { BASE_POS_DU_PERRAY_EN_YVELINES, { 0x14, 0x80 + POS2_NUM_DU_PERRAY_EN_YVELINES / 256, POS2_NUM_DU_PERRAY_EN_YVELINES % 256 }, "08/1520_du_Perray_en_Yvelines.mp3", true, 1, 0L },
  { BASE_POS_DU_TREMBLAY_SUR_MAULDRE, { 0x14, 0x80 + POS2_NUM_DU_TREMBLAY_SUR_MAULDRE / 256, POS2_NUM_DU_TREMBLAY_SUR_MAULDRE % 256 }, "08/1521_du_Tremblay_sur_Mauldre.mp3", true, 1, 0L },
  { BASE_POS_DU_VESINET, { 0x14, 0x80 + POS2_NUM_DU_VESINET / 256, POS2_NUM_DU_VESINET % 256 }, "08/1522_du_Vesinet.mp3", true, 1, 0L },
  { BASE_POS_13_D_AIX_EN_PROVENCE, { 0x14, 0x90 + POS2_NUM_13_D_AIX_EN_PROVENCE / 256, POS2_NUM_13_D_AIX_EN_PROVENCE % 256 }, "09/3000_13_d_Aix_en_Provence.mp3", true, 1, 0L },
  { BASE_POS_13_D_ARLES, { 0x14, 0x90 + POS2_NUM_13_D_ARLES / 256, POS2_NUM_13_D_ARLES % 256 }, "09/3001_13_d_Arles.mp3", true, 1, 0L },
  { BASE_POS_13_D_ISTRES, { 0x14, 0x90 + POS2_NUM_13_D_ISTRES / 256, POS2_NUM_13_D_ISTRES % 256 }, "09/3002_13_d_Istres.mp3", true, 1, 0L },
  { BASE_POS_15_D_AURILLAC, { 0x14, 0x90 + POS2_NUM_15_D_AURILLAC / 256, POS2_NUM_15_D_AURILLAC % 256 }, "09/3003_15_d_Aurillac.mp3", true, 1, 0L },
  { BASE_POS_16_D_ANGOULEME, { 0x14, 0x90 + POS2_NUM_16_D_ANGOULEME / 256, POS2_NUM_16_D_ANGOULEME % 256 }, "09/3004_16_d_Angouleme.mp3", true, 1, 0L },
  { BASE_POS_19_D_USSEL, { 0x14, 0x90 + POS2_NUM_19_D_USSEL / 256, POS2_NUM_19_D_USSEL % 256 }, "09/3005_19_d_Ussel.mp3", true, 1, 0L },
  { BASE_POS_2A_D_AJACCIO, { 0x14, 0x90 + POS2_NUM_2A_D_AJACCIO / 256, POS2_NUM_2A_D_AJACCIO % 256 }, "09/3006_2A_d_Ajaccio.mp3", true, 1, 0L },
  { BASE_POS_27_D_EVREUX, { 0x14, 0x90 + POS2_NUM_27_D_EVREUX / 256, POS2_NUM_27_D_EVREUX % 256 }, "09/3007_27_d_Evreux.mp3", true, 1, 0L },
  { BASE_POS_27_DES_ANDELYS, { 0x14, 0x90 + POS2_NUM_27_DES_ANDELYS / 256, POS2_NUM_27_DES_ANDELYS % 256 }, "09/3008_27_des_Andelys.mp3", true, 1, 0L },
  { BASE_POS_30_D_ALES, { 0x14, 0x90 + POS2_NUM_30_D_ALES / 256, POS2_NUM_30_D_ALES % 256 }, "09/3009_30_d_Ales.mp3", true, 1, 0L },
  { BASE_POS_30_DU_VIGAN, { 0x14, 0x90 + POS2_NUM_30_DU_VIGAN / 256, POS2_NUM_30_DU_VIGAN % 256 }, "09/3010_30_du_Vigan.mp3", true, 1, 0L },
  { BASE_POS_32_D_AUCH, { 0x14, 0x90 + POS2_NUM_32_D_AUCH / 256, POS2_NUM_32_D_AUCH % 256 }, "09/3011_32_d_Auch.mp3", true, 1, 0L },
  { BASE_POS_33_D_ARCACHON, { 0x14, 0x90 + POS2_NUM_33_D_ARCACHON / 256, POS2_NUM_33_D_ARCACHON % 256 }, "09/3012_33_d_Arcachon.mp3", true, 1, 0L },
  { BASE_POS_36_D_ISSOUDUN, { 0x14, 0x90 + POS2_NUM_36_D_ISSOUDUN / 256, POS2_NUM_36_D_ISSOUDUN % 256 }, "09/3013_36_d_Issoudun.mp3", true, 1, 0L },
  { BASE_POS_36_DU_BLANC, { 0x14, 0x90 + POS2_NUM_36_DU_BLANC / 256, POS2_NUM_36_DU_BLANC % 256 }, "09/3014_36_du_Blanc.mp3", true, 1, 0L },
  { BASE_POS_43_D_YSSINGEAUX, { 0x14, 0x90 + POS2_NUM_43_D_YSSINGEAUX / 256, POS2_NUM_43_D_YSSINGEAUX % 256 }, "09/3015_43_d_Yssingeaux.mp3", true, 1, 0L },
  { BASE_POS_43_DU_PUY_EN_VELAY, { 0x14, 0x90 + POS2_NUM_43_DU_PUY_EN_VELAY / 256, POS2_NUM_43_DU_PUY_EN_VELAY % 256 }, "09/3016_43_du_Puy_en_Velay.mp3", true, 1, 0L },
  { BASE_POS_44_D_ANCENIS, { 0x14, 0x90 + POS2_NUM_44_D_ANCENIS / 256, POS2_NUM_44_D_ANCENIS % 256 }, "09/3017_44_d_Ancenis.mp3", true, 1, 0L },
  { BASE_POS_45_D_ORLEANS, { 0x14, 0x90 + POS2_NUM_45_D_ORLEANS / 256, POS2_NUM_45_D_ORLEANS % 256 }, "09/3018_45_d_Orleans.mp3", true, 1, 0L },
  { BASE_POS_47_D_AGEN, { 0x14, 0x90 + POS2_NUM_47_D_AGEN / 256, POS2_NUM_47_D_AGEN % 256 }, "09/3019_47_d_Agen.mp3", true, 1, 0L },
  { BASE_POS_49_D_ANGERS, { 0x14, 0x90 + POS2_NUM_49_D_ANGERS / 256, POS2_NUM_49_D_ANGERS % 256 }, "09/3020_49_d_Angers.mp3", true, 1, 0L },
  { BASE_POS_50_D_AVRANCHES, { 0x14, 0x90 + POS2_NUM_50_D_AVRANCHES / 256, POS2_NUM_50_D_AVRANCHES % 256 }, "09/3021_50_d_Avranches.mp3", true, 1, 0L },
  { BASE_POS_51_D_EPERNAY, { 0x14, 0x90 + POS2_NUM_51_D_EPERNAY / 256, POS2_NUM_51_D_EPERNAY % 256 }, "09/3022_51_d_Epernay.mp3", true, 1, 0L },
  { BASE_POS_59_D_AVESNES_SUR_HELPE, { 0x14, 0x90 + POS2_NUM_59_D_AVESNES_SUR_HELPE / 256, POS2_NUM_59_D_AVESNES_SUR_HELPE % 256 }, "09/3023_59_d_Avesnes_sur_Helpe.mp3", true, 1, 0L },
  { BASE_POS_61_D_ALENCON, { 0x14, 0x90 + POS2_NUM_61_D_ALENCON / 256, POS2_NUM_61_D_ALENCON % 256 }, "09/3024_61_d_Alencon.mp3", true, 1, 0L },
  { BASE_POS_61_D_ARGENTAN, { 0x14, 0x90 + POS2_NUM_61_D_ARGENTAN / 256, POS2_NUM_61_D_ARGENTAN % 256 }, "09/3025_61_d_Argentan.mp3", true, 1, 0L },
  { BASE_POS_62_D_ARRAS, { 0x14, 0x90 + POS2_NUM_62_D_ARRAS / 256, POS2_NUM_62_D_ARRAS % 256 }, "09/3026_62_d_Arras.mp3", true, 1, 0L },
  { BASE_POS_63_D_AMBERT, { 0x14, 0x90 + POS2_NUM_63_D_AMBERT / 256, POS2_NUM_63_D_AMBERT % 256 }, "09/3027_63_d_Ambert.mp3", true, 1, 0L },
  { BASE_POS_63_D_ISSOIRE, { 0x14, 0x90 + POS2_NUM_63_D_ISSOIRE / 256, POS2_NUM_63_D_ISSOIRE % 256 }, "09/3028_63_d_Issoire.mp3", true, 1, 0L },
  { BASE_POS_64_D_OLORON_SAINTE_MARIE, { 0x14, 0x90 + POS2_NUM_64_D_OLORON_SAINTE_MARIE / 256, POS2_NUM_64_D_OLORON_SAINTE_MARIE % 256 }, "09/3029_64_d_Oloron_Sainte_Marie.mp3", true, 1, 0L },
  { BASE_POS_65_D_ARGELES_GAZOST, { 0x14, 0x90 + POS2_NUM_65_D_ARGELES_GAZOST / 256, POS2_NUM_65_D_ARGELES_GAZOST % 256 }, "09/3030_65_d_Argeles_Gazost.mp3", true, 1, 0L },
  { BASE_POS_68_D_ALTKIRCH, { 0x14, 0x90 + POS2_NUM_68_D_ALTKIRCH / 256, POS2_NUM_68_D_ALTKIRCH % 256 }, "09/3031_68_d_Altkirch.mp3", true, 1, 0L },
  { BASE_POS_71_D_AUTUN, { 0x14, 0x90 + POS2_NUM_71_D_AUTUN / 256, POS2_NUM_71_D_AUTUN % 256 }, "09/3032_71_d_Autun.mp3", true, 1, 0L },
  { BASE_POS_72_DU_MANS, { 0x14, 0x90 + POS2_NUM_72_DU_MANS / 256, POS2_NUM_72_DU_MANS % 256 }, "09/3033_72_du_Mans.mp3", true, 1, 0L },
  { BASE_POS_73_D_ALBERTVILLE, { 0x14, 0x90 + POS2_NUM_73_D_ALBERTVILLE / 256, POS2_NUM_73_D_ALBERTVILLE % 256 }, "09/3034_73_d_Albertville.mp3", true, 1, 0L },
  { BASE_POS_74_D_ANNECY, { 0x14, 0x90 + POS2_NUM_74_D_ANNECY / 256, POS2_NUM_74_D_ANNECY % 256 }, "09/3035_74_d_Annecy.mp3", true, 1, 0L },
  { BASE_POS_76_DU_HAVRE, { 0x14, 0x90 + POS2_NUM_76_DU_HAVRE / 256, POS2_NUM_76_DU_HAVRE % 256 }, "09/3036_76_du_Havre.mp3", true, 1, 0L },
  { BASE_POS_80_D_ABBEVILLE, { 0x14, 0x90 + POS2_NUM_80_D_ABBEVILLE / 256, POS2_NUM_80_D_ABBEVILLE % 256 }, "09/3037_80_d_Abbeville.mp3", true, 1, 0L },
  { BASE_POS_80_D_AMIENS, { 0x14, 0x90 + POS2_NUM_80_D_AMIENS / 256, POS2_NUM_80_D_AMIENS % 256 }, "09/3038_80_d_Amiens.mp3", true, 1, 0L },
  { BASE_POS_81_D_ALBI, { 0x14, 0x90 + POS2_NUM_81_D_ALBI / 256, POS2_NUM_81_D_ALBI % 256 }, "09/3039_81_d_Albi.mp3", true, 1, 0L },
  { BASE_POS_84_D_APT, { 0x14, 0x90 + POS2_NUM_84_D_APT / 256, POS2_NUM_84_D_APT % 256 }, "09/3040_84_d_Apt.mp3", true, 1, 0L },
  { BASE_POS_85_DES_SABLES_D_OLONNE, { 0x14, 0x90 + POS2_NUM_85_DES_SABLES_D_OLONNE / 256, POS2_NUM_85_DES_SABLES_D_OLONNE % 256 }, "09/3041_85_des_Sables_d_Olonne.mp3", true, 1, 0L },
  { BASE_POS_88_D_EPINAL, { 0x14, 0x90 + POS2_NUM_88_D_EPINAL / 256, POS2_NUM_88_D_EPINAL % 256 }, "09/3042_88_d_Epinal.mp3", true, 1, 0L },
  { BASE_POS_89_D_AUXERRE, { 0x14, 0x90 + POS2_NUM_89_D_AUXERRE / 256, POS2_NUM_89_D_AUXERRE % 256 }, "09/3043_89_d_Auxerre.mp3", true, 1, 0L },
  { BASE_POS_89_D_AVALLON, { 0x14, 0x90 + POS2_NUM_89_D_AVALLON / 256, POS2_NUM_89_D_AVALLON % 256 }, "09/3044_89_d_Avallon.mp3", true, 1, 0L },
  { BASE_POS_92_D_ANTONY, { 0x14, 0x90 + POS2_NUM_92_D_ANTONY / 256, POS2_NUM_92_D_ANTONY % 256 }, "09/3045_92_d_Antony.mp3", true, 1, 0L },
  { BASE_POS_93_DU_RAINCY, { 0x14, 0x90 + POS2_NUM_93_DU_RAINCY / 256, POS2_NUM_93_DU_RAINCY % 256 }, "09/3046_93_du_Raincy.mp3", true, 1, 0L },
  { BASE_POS_95_D_ARGENTEUIL, { 0x14, 0x90 + POS2_NUM_95_D_ARGENTEUIL / 256, POS2_NUM_95_D_ARGENTEUIL % 256 }, "09/3047_95_d_Argenteuil.mp3", true, 1, 0L },

  { BASE_LAST_PROMPT_POS2_COMMUNES, { 0x00, 0x00, 0x00 }, NULL, false, 0, 0L }
};

// 71 communes

typedef enum {
  RULE_1_NO_MEAN = 0,
  RULE_1_SINGULAR,
  RULE_1_PLURAL
} ENUM_POS_COMM_RULE_1;

typedef enum {
  RULE_2_NO_MEAN = 0,
  RULE_2_AT,                // "à"  (ie. "Vous êtes à" "Toussus le Noble")
  RULE_2_TO                 // "au" (ie. "Vous êtes au "Mesnuls" car la commune se nomme "Les Mesnuls")
} ENUM_POS_COMM_RULE_2;

typedef enum {
  TOWN_NO_MEAN = 0,
  TOWN_WITHOUT_RADIUS,
  TOWN_WITH_RADIUS,
} ENUM_TYPE_TOWN;

typedef struct {
    int                       idx;
    float                     lat;
    float                     lon;
    float                     ele;
    float                     radius;
    ENUM_BASE_POS_COMMUNES    base;
    ENUM_POS_COMM_RULE_1      rule_1;
    ENUM_POS_COMM_RULE_2      rule_2;
    ENUM_TYPE_TOWN            type_town;
    const char                *name_commune;

    boolean                   flg_base2;
    ENUM_BASE_POS2_COMMUNES   base2;
} ST_POS_COMMUNES_COORD;

#define NBR_OF_DEF_WITH_SPEC_RULES    22

static ST_POS_COMMUNES_COORD      g__pos_communes_coord[BASE_LAST_PROMPT_POS_COMMUNES + 1 - NBR_OF_DEF_WITH_SPEC_RULES] =
{
  { 0, 47.293331, 0.023611, 27.0, 3.41, BASE_POS_ALLONNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Allonnes", true, BASE_POS_D_ALLONNES },
  { 1, 47.302223, 0.068056, 45.0, 3.27, BASE_POS_BRAIN_SUR_ALLONNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Brain-sur-Allonnes" },
  { 2, 47.341110, 0.076667, 105.0, 2.98, BASE_POS_LA_BREILLE_LES_PINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "la Breille-les-Pins" },
  { 3, 47.214169, -0.073611, 40.0, 1.45, BASE_POS_CHACE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Chacé" },
  { 4, 47.308056, -0.155000, 30.0, 3.01, BASE_POS_CHENEHUTTE_TREVES_CUNAULT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Chênehutte-Trèves-Cunault" },
  { 5, 47.222221, -0.110278, 46.0, 2.18, BASE_POS_DISTRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Distré" },
  { 6, 47.341110, -0.230278, 35.0, 3.24, BASE_POS_GENNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Gennes" },
  { 7, 47.232498, -0.208056, 70.0, 2.07, BASE_POS_MEIGNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Meigné" },
  { 8, 47.216110, 0.056944, 33.0, 1.33, BASE_POS_MONTSOREAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Montsoreau" },
  { 9, 47.341389, -0.033611, 40.0, 2.11, BASE_POS_NEUILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Neuillé" },
  { 10, 47.228058, 0.010278, 32.0, 1.49, BASE_POS_PARNAY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Parnay" },
  { 11, 47.234444, -0.153611, 40.0, 2.03, BASE_POS_ROU_MARSON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Rou-Marson" },
  { 12, 47.331669, -0.182778, 24.0, 1.83, BASE_POS_SAINT_CLEMENT_DES_LEVEES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Clément-des-Levées" },
  { 13, 47.315834, -0.148889, 25.0, 2.18, BASE_POS_SAINT_MARTIN_DE_LA_PLACE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Martin-de-la-Place" },
  { 14, 47.257221, -0.076667, 40.0, 4.60, BASE_POS_SAUMUR, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "Saumur" },
  { 15, 47.233055, -0.007778, 32.0, 1.70, BASE_POS_SOUZAY_CHAMPIGNY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Souzay-Champigny" },
  { 16, 47.221668, 0.028056, 40.0, 1.60, BASE_POS_TURQUANT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Turquant" },
  { 17, 47.220554, -0.178889, 55.0, 1.62, BASE_POS_LES_ULMES, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Ulmes", true, BASE_POS_DES_ULMES },
  { 18, 47.237778, 0.053889, 27.0, 2.72, BASE_POS_VARENNES_SUR_LOIRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Varennes-sur-Loire" },
  { 19, 47.224998, -0.065278, 51.0, 1.05, BASE_POS_VARRAINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Varrains" },
  { 20, 47.266666, -0.176944, 70.0, 2.34, BASE_POS_VERRIE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Verrie" },
  { 21, 47.255001, -0.030000, 32.0, 1.79, BASE_POS_VILLEBERNIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Villebernier" },
  { 22, 47.325279, -0.055278, 29.0, 2.74, BASE_POS_VIVY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Vivy" },
  { 23, 48.880001, 1.825000, 119.0, 1.13, BASE_POS_ANDELU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Andelu", true, BASE_POS_D_ANDELU },
  { 24, 48.702221, 1.886944, 148.0, 2.36, BASE_POS_AUFFARGIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Auffargis", true, BASE_POS_D_AUFFARGIS },
  { 25, 48.841667, 1.818611, 90.0, 1.18, BASE_POS_AUTEUIL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Auteuil", true, BASE_POS_D_AUTEUIL },
  { 26, 48.849724, 1.804444, 122.0, 1.28, BASE_POS_AUTOUILLET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Autouillet", true, BASE_POS_D_AUTOUILLET },
  { 27, 48.842777, 2.081111, 130.0, 1.45, BASE_POS_BAILLY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Bailly" },
  { 28, 48.778889, 1.860556, 130.0, 1.34, BASE_POS_BAZOCHES_SUR_GUYONNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Bazoches-sur-Guyonne" },
  { 29, 48.830276, 1.722778, 130.0, 1.31, BASE_POS_BEHOUST, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Béhoust" },
  { 30, 48.856945, 1.873333, 50.0, 2.44, BASE_POS_BEYNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Beynes" },
  { 31, 48.803333, 2.040000, 166.0, 1.70, BASE_POS_BOIS_D_ARCY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Bois-d'Arcy" },
  { 32, 48.820278, 1.794444, 100.0, 1.14, BASE_POS_BOISSY_SANS_AVOIR, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Boissy-sans-Avoir" },
  { 33, 48.865276, 2.143889, 50.0, 0.95, BASE_POS_BOUGIVAL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Bougival" },
  { 34, 48.707500, 1.813056, 177.0, 2.51, BASE_POS_LES_BREVIAIRES, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Bréviaires", true, BASE_POS_DES_BREVIAIRES },
  { 35, 48.773609, 2.127500, 120.0, 1.60, BASE_POS_BUC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Buc" },
  { 36, 48.841110, 2.134444, 116.0, 1.37, BASE_POS_LA_CELLE_SAINT_CLOUD, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "la Celle-Saint-Cloud" },
  { 37, 48.674168, 1.974722, 174.0, 1.79, BASE_POS_CERNAY_LA_VILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Cernay-la-Ville" },
  { 38, 48.737499, 2.092500, 120.0, 1.26, BASE_POS_CHATEAUFORT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Châteaufort" },
  { 39, 48.853889, 1.987500, 100.0, 1.39, BASE_POS_CHAVENAY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Chavenay" },
  { 40, 48.830555, 2.121667, 130.0, 1.17, BASE_POS_LE_CHESNAY, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Chesnay" },
  { 41, 48.706669, 2.040000, 90.0, 2.08, BASE_POS_CHEVREUSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Chevreuse" },
  { 42, 48.687222, 2.019167, 110.0, 1.69, BASE_POS_CHOISEL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Choisel" },
  { 43, 48.822777, 1.988333, 120.0, 1.40, BASE_POS_LES_CLAYES_SOUS_BOIS, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Clayes-sous-Bois", true, BASE_POS_DES_CLAYES_SOUS_BOIS},
  { 44, 48.746387, 1.919444, 167.0, 1.00, BASE_POS_COIGNIERES_VILLAGE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Coignières Village" },
  { 45, 48.746520, 1.926470, 166.0, 1.63, BASE_POS_COIGNIERES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Coignières" },
  { 46, 48.883610, 1.922778, 100.0, 2.18, BASE_POS_CRESPIERES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Crespières" },
  { 47, 48.883057, 2.142500, 30.0, 1.03, BASE_POS_CROISSY_SUR_SEINE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Croissy-sur-Seine" },
  { 48, 48.706669, 1.987500, 100.0, 1.90, BASE_POS_DAMPIERRE_EN_YVELINES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Dampierre-en-Yvelines" },
  { 49, 48.866112, 1.946111, 110.0, 1.39, BASE_POS_DAVRON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Davron" },
  { 50, 48.768330, 1.949130, 174.0, 1.71, BASE_POS_ELANCOURT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Élancourt", true, BASE_POS_D_ELANCOURT},
  { 51, 48.784168, 1.959444, 130.0, 1.00, BASE_POS_ELANCOURT_VILLAGE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Élancourt Village", true, BASE_POS_D_ELANCOURT_VILLAGE },
  { 52, 48.793910, 1.971410, 168.0, 1.00, BASE_POS_ELANCOURT_LA_CLE_DE_SAINT_PIERRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Élancourt La Clé de Saint-Pierre", true, BASE_POS_D_ELANCOURT_LA_CLE_DE_SAINT_PIERRE},
  { 53, 48.788420, 1.968250, 231.0, 0.31, BASE_POS_LA_COLLINE_D_ELANCOURT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "la Colline d'Élancourt", true, BASE_POS_D_ELANCOURT_VILLAGE},
  { 54, 48.716667, 1.893611, 170.0, 2.50, BASE_POS_LES_ESSARTS_LE_ROI, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Essarts-le-Roi", true, BASE_POS_DES_ESSARTS_LE_ROI },
  { 55, 48.870556, 2.073333, 70.0, 1.31, BASE_POS_L_ETANG_LA_VILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "l'Étang-la-Ville", true, BASE_POS_D_ETANG_LA_VILLE },
  { 56, 48.873055, 1.973611, 140.0, 2.05, BASE_POS_FEUCHEROLLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Feucherolles" },
  { 57, 48.853889, 1.736944, 120.0, 1.69, BASE_POS_FLEXANVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Flexanville" },
  { 58, 48.815277, 2.050000, 130.0, 1.34, BASE_POS_FONTENAY_LE_FLEURY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Fontenay-le-Fleury" },
  { 59, 48.887222, 2.063333, 120.0, 1.09, BASE_POS_FOURQUEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Fourqueux" },
  { 60, 48.796112, 1.794444, 135.0, 1.20, BASE_POS_GALLUIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Galluis" },
  { 61, 48.756668, 1.732500, 130.0, 2.47, BASE_POS_GAMBAISEUIL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Gambaiseuil" },
  { 62, 48.821667, 1.758611, 100.0, 1.82, BASE_POS_GARANCIERES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Garancières" },
  { 63, 48.879166, 1.761667, 140.0, 1.34, BASE_POS_GOUPILLIERES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Goupillières" },
  { 64, 48.783054, 1.760833, 150.0, 2.00, BASE_POS_GROSROUVRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Grosrouvre" },
  { 65, 48.771942, 2.071667, 160.0, 2.06, BASE_POS_GUYANCOURT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Guyancourt" },
  { 66, 48.890278, 1.745278, 145.0, 1.49, BASE_POS_HARGEVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Hargeville" },
  { 67, 48.802223, 1.902222, 112.0, 1.77, BASE_POS_JOUARS_PONTCHARTRAIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Jouars-Pontchartrain" },
  { 68, 48.724445, 1.954722, 172.0, 1.64, BASE_POS_LEVIS_SAINT_NOM, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Lévis-Saint-Nom" },
  { 69, 48.765556, 2.141667, 140.0, 0.89, BASE_POS_LES_LOGES_EN_JOSAS, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Loges-en-Josas", true, BASE_POS_DES_LOGES_EN_JOSAS },
  { 70, 48.863056, 2.114167, 150.0, 1.31, BASE_POS_LOUVECIENNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Louveciennes" },
  { 71, 48.723700, 2.087010, 163.0, 2.30, BASE_POS_MAGNY_LES_HAMEAUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Magny-les-Hameaux" },
  { 72, 48.743889, 2.060833, 150.0, 1.00, BASE_POS_MAGNY_VILLAGE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Magny Village" },
  { 73, 48.862499, 1.816944, 130.0, 1.24, BASE_POS_MARCQ, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Marcq" },
  { 74, 48.790833, 1.853611, 73.0, 1.15, BASE_POS_MAREIL_LE_GUYON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Mareil-le-Guyon" },
  { 75, 48.882778, 2.076111, 110.0, 0.75, BASE_POS_MAREIL_MARLY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Mareil-Marly" },
  { 76, 48.892776, 1.863333, 50.0, 1.18, BASE_POS_MAREIL_SUR_MAULDRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Mareil-sur-Mauldre" },
  { 77, 48.868057, 2.091667, 140.0, 1.46, BASE_POS_MARLY_LE_ROI, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Marly-le-Roi" },
  { 78, 48.767776, 1.920556, 160.0, 1.00, BASE_POS_MAUREPAS_VILLAGE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Maurepas Village" },
  { 79, 48.762560, 1.944810, 171.0, 1.64, BASE_POS_MAUREPAS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Maurepas" },
  { 80, 48.789165, 1.819722, 120.0, 1.83, BASE_POS_MERE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Méré" },
  { 81, 48.743889, 1.962778, 171.0, 1.70, BASE_POS_LE_MESNIL_SAINT_DENIS, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Mesnil-Saint-Denis", true, BASE_POS_DU_MESNIL_SAINT_DENIS },
  { 82, 48.758888, 1.838056, 120.0, 1.45, BASE_POS_LES_MESNULS, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Mesnuls", true, BASE_POS_DES_MESNULS },
  { 83, 48.809723, 1.746389, 140.0, 1.36, BASE_POS_MILLEMONT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Millemont" },
  { 84, 48.727779, 2.051667, 100.0, 0.99, BASE_POS_MILON_LA_CHAPELLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Milon-la-Chapelle" },
  { 85, 48.882778, 1.863889, 107.0, 1.24, BASE_POS_MONTAINVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Montainville" },
  { 86, 48.777222, 1.809167, 126.0, 1.35, BASE_POS_MONTFORT_L_AMAURY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Montfort-l'Amaury" },
  { 87, 48.771667, 2.028056, 164.0, 1.52, BASE_POS_MONTIGNY_LE_BRETONNEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Montigny-le-Bretonneux" },
  { 88, 48.814999, 1.901667, 140.0, 0.84, BASE_POS_NEAUPHLE_LE_CHATEAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Neauphle-le-Château" },
  { 89, 48.816113, 1.863611, 70.0, 1.56, BASE_POS_NEAUPHLE_LE_VIEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Neauphle-le-Vieux" },
  { 90, 48.845554, 2.064444, 139.0, 1.34, BASE_POS_NOISY_LE_ROI, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Noisy-le-Roi" },
  { 91, 48.863056, 1.717500, 110.0, 0.91, BASE_POS_OSMOY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Osmoy", true, BASE_POS_D_OSMOY },
  { 92, 48.894169, 2.115556, 30.0, 0.97, BASE_POS_LE_PECQ, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Pecq", true, BASE_POS_DU_PECQ },
  { 93, 48.695000, 1.854722, 175.0, 2.07, BASE_POS_LE_PERRAY_EN_YVELINES, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Perray-en-Yvelines", true, BASE_POS_DU_PERRAY_EN_YVELINES },
  { 94, 48.816666, 1.947778, 110.0, 2.38, BASE_POS_PLAISIR, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Plaisir" },
  { 95, 48.675278, 1.754444, 140.0, 2.74, BASE_POS_POIGNY_LA_FORET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Poigny-la-Forêt" },
  { 96, 48.878613, 2.108889, 50.0, 0.67, BASE_POS_LE_PORT_MARLY, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Port-Marly", true, BASE_POS_DE_PORT_MARLY },
  { 97, 48.806389, 1.767500, 130.0, 1.36, BASE_POS_LA_QUEUE_LES_YVELINES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "la Queue-les-Yvelines" },
  { 98, 48.835556, 2.044722, 106.0, 0.84, BASE_POS_RENNEMOULIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Rennemoulin" },
  { 99, 48.837502, 2.112500, 140.0, 0.96, BASE_POS_ROCQUENCOURT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Rocquencourt" },
  { 100, 48.675320, 1.913250, 175.0, 1.00, BASE_POS_SAINT_BENOIT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Benoît" },
  { 101, 48.801666, 2.062778, 124.0, 1.27, BASE_POS_SAINT_CYR_L_ECOLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Cyr-l'École" },
  { 102, 48.709660, 1.993847, 120.0, 1.39, BASE_POS_SAINT_FORGET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Forget" },
  { 103, 48.835278, 1.899167, 110.0, 1.30, BASE_POS_SAINT_GERMAIN_DE_LA_GRANGE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Germain-de-la-Grange" },
  { 104, 48.895279, 2.089167, 92.0, 3.96, BASE_POS_SAINT_GERMAIN_EN_LAYE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "Saint-Germain-en-Laye" },
  { 105, 48.716650, 1.858810, 177.0, 1.00, BASE_POS_SAINT_HUBERT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Hubert" },
  { 106, 48.732777, 2.020833, 120.0, 1.45, BASE_POS_SAINT_LAMBERT_DES_BOIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Lambert-des-Bois" },
  { 107, 48.720833, 1.766944, 150.0, 3.34, BASE_POS_SAINT_LEGER_EN_YVELINES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Léger-en-Yvelines" },
  { 108, 48.881390, 1.718889, 130.0, 1.41, BASE_POS_SAINT_MARTIN_DES_CHAMPS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Martin-des-Champs" },
  { 109, 48.857498, 2.018611, 136.0, 1.94, BASE_POS_SAINT_NOM_LA_BRETECHE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Nom-la-Bretèche" },
  { 110, 48.756943, 1.882222, 150.0, 1.81, BASE_POS_SAINT_REMY_L_HONORE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Rémy-l'Honoré" },
  { 111, 48.709721, 2.068611, 80.0, 1.77, BASE_POS_SAINT_REMY_LES_CHEVREUSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Rémy-lès-Chevreuse" },
  { 112, 48.841667, 1.835556, 115.0, 0.84, BASE_POS_SAULX_MARCHAIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saulx-Marchais" },
  { 113, 48.689167, 1.983889, 100.0, 1.61, BASE_POS_SENLISSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Senlisse" },
  { 114, 48.849445, 1.917222, 70.0, 1.90, BASE_POS_THIVERVAL_GRIGNON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Thiverval-Grignon" },
  { 115, 48.866669, 1.798611, 150.0, 1.51, BASE_POS_THOIRY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Thoiry" },
  { 116, 48.746666, 2.113056, 157.0, 1.13, BASE_POS_TOUSSUS_LE_NOBLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Toussus-le-Noble" },
  { 117, 48.777222, 2.008611, 168.0, 2.09, BASE_POS_TRAPPES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Trappes" },
  { 118, 48.778610, 1.878056, 110.0, 1.39, BASE_POS_LE_TREMBLAY_SUR_MAULDRE, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Tremblay-sur-Mauldre", true, BASE_POS_DU_TREMBLAY_SUR_MAULDRE },
  { 119, 48.755554, 1.956111, 170.0, 0.76, BASE_POS_LA_VERRIERE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "la Verrière" },
  { 120, 48.806946, 2.136667, 135.0, 2.88, BASE_POS_VERSAILLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "Versailles" },
  { 121, 48.888889, 2.131667, 43.0, 1.26, BASE_POS_LE_VESINET, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITHOUT_RADIUS, "le Vésinet", true, BASE_POS_DU_VESINET },
  { 122, 48.816113, 1.836111, 70.0, 1.20, BASE_POS_VICQ, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Vicq" },
  { 123, 48.669998, 1.875278, 169.0, 1.76, BASE_POS_VIEILLE_EGLISE_EN_YVELINES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Vieille-Église-en-Yvelines" },
  { 124, 48.834442, 2.013056, 90.0, 1.82, BASE_POS_VILLEPREUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Villepreux" },
  { 125, 48.861389, 1.773611, 127.0, 1.48, BASE_POS_VILLIERS_LE_MAHIEU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Villiers-le-Mahieu" },
  { 126, 48.820557, 1.892778, 140.0, 1.27, BASE_POS_VILLIERS_SAINT_FREDERIC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Villiers-Saint-Fréderic" },
  { 127, 48.759167, 2.051667, 165.0, 1.08, BASE_POS_VOISINS_LE_BRETONNEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Voisins-le-Bretonneux" },
  { 128, 48.678612, 2.051111, 170.0, 1.25, BASE_POS_BOULLAY_LES_TROUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Boullay-les-Troux" },
  { 129, 48.703609, 2.128889, 80.0, 1.95, BASE_POS_GIF_SUR_YVETTE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Gif-sur-Yvette" },
  { 130, 48.672222, 2.125556, 165.0, 1.75, BASE_POS_GOMETZ_LA_VILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Gometz-la-Ville" },
  { 131, 48.678890, 2.140278, 140.0, 1.25, BASE_POS_GOMETZ_LE_CHATEL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Gometz-le-Châtel" },
  { 132, 48.673332, 2.069722, 171.0, 1.50, BASE_POS_LES_MOLIERES, RULE_1_PLURAL, RULE_2_TO, TOWN_WITHOUT_RADIUS, "les Molières", true,BASE_POS_DES_MOLIERES },
  { 133, 48.715000, 2.141389, 163.0, 1.06, BASE_POS_SAINT_AUBIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Saint-Aubin" },
  { 134, 48.725555, 2.126944, 150.0, 1.39, BASE_POS_VILLIERS_LE_BACLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITHOUT_RADIUS, "Villiers-le-Bâcle" },
  { 135, 45.759167, 5.689167, 290.0, 2.68, BASE_POS_01_BELLEY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "01_Belley" },
  { 136, 46.205276, 5.226389, 230.0, 2.77, BASE_POS_01_BOURG_EN_BRESSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "01_Bourg-en-Bresse" },
  { 137, 46.333332, 6.058333, 610.0, 3.19, BASE_POS_01_GEX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "01_Gex" },
  { 138, 46.155277, 5.605278, 478.0, 2.14, BASE_POS_01_NANTUA, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "01_Nantua" },
  { 139, 49.045277, 3.401667, 80.0, 2.32, BASE_POS_02_CHATEAU_THIERRY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "02_Château-Thierry" },
  { 140, 49.569168, 3.626389, 160.0, 3.77, BASE_POS_02_LAON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "02_Laon" },
  { 141, 49.848888, 3.285833, 96.0, 2.70, BASE_POS_02_SAINT_QUENTIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "02_Saint-Quentin" },
  { 142, 49.380833, 3.326111, 48.0, 1.98, BASE_POS_02_SOISSONS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "02_Soissons" },
  { 143, 49.834999, 3.909167, 170.0, 1.82, BASE_POS_02_VERVINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "02_Vervins" },
  { 144, 46.341110, 2.606111, 207.0, 2.57, BASE_POS_03_MONTLUCON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "03_Montluçon" },
  { 145, 46.567501, 3.334722, 219.0, 1.66, BASE_POS_03_MOULINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "03_Moulins" },
  { 146, 46.131111, 3.427500, 268.0, 1.37, BASE_POS_03_VICHY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "03_Vichy" },
  { 147, 44.389442, 6.650278, 1140.0, 2.30, BASE_POS_04_BARCELONNETTE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "04_Barcelonnette" },
  { 148, 43.847500, 6.511944, 720.0, 6.25, BASE_POS_04_CASTELLANE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "04_Castellane" },
  { 149, 44.095001, 6.235000, 600.0, 5.91, BASE_POS_04_DIGNE_LES_BAINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "04_Digne-les-Bains" },
  { 150, 43.959167, 5.781667, 560.0, 3.72, BASE_POS_04_FORCALQUIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "04_Forcalquier" },
  { 151, 44.896667, 6.639167, 1200.0, 3.00, BASE_POS_05_BRIANCON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "05_Briançon" },
  { 152, 44.558334, 6.076944, 767.0, 5.89, BASE_POS_05_GAP, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "05_Gap" },
  { 153, 43.659168, 6.922778, 300.0, 3.76, BASE_POS_06_GRASSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "06_Grasse" },
  { 154, 43.706112, 7.262222, 10.0, 4.85, BASE_POS_06_NICE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "06_Nice" },
  { 155, 44.541111, 4.291111, 240.0, 1.53, BASE_POS_07_LARGENTIERE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "07_Largentière" },
  { 156, 44.735832, 4.597222, 298.0, 1.96, BASE_POS_07_PRIVAS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "07_Privas" },
  { 157, 45.064720, 4.835556, 121.0, 2.60, BASE_POS_07_TOURNON_SUR_RHONE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "07_Tournon-sur-Rhône" },
  { 158, 49.762501, 4.720278, 150.0, 3.20, BASE_POS_08_CHARLEVILLE_MEZIERES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "08_Charleville-Mézières" },
  { 159, 49.509998, 4.366389, 130.0, 2.43, BASE_POS_08_RETHEL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "08_Rethel" },
  { 160, 49.704166, 4.944444, 155.0, 2.28, BASE_POS_08_SEDAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "08_Sedan" },
  { 161, 49.398056, 4.698333, 100.0, 2.99, BASE_POS_08_VOUZIERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "08_Vouziers" },
  { 162, 42.959721, 1.608889, 380.0, 2.49, BASE_POS_09_FOIX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "09_Foix" },
  { 163, 43.114166, 1.614722, 295.0, 3.85, BASE_POS_09_PAMIERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "09_Pamiers" },
  { 164, 42.985279, 1.142500, 390.0, 2.46, BASE_POS_09_SAINT_GIRONS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "09_Saint-Girons" },
  { 165, 48.232224, 4.708611, 166.0, 2.25, BASE_POS_10_BAR_SUR_AUBE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "10_Bar-sur-Aube" },
  { 166, 48.492222, 3.501389, 75.0, 2.52, BASE_POS_10_NOGENT_SUR_SEINE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "10_Nogent-sur-Seine" },
  { 167, 48.296391, 4.071667, 107.0, 2.06, BASE_POS_10_TROYES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "10_Troyes" },
  { 168, 43.216667, 2.348333, 110.0, 4.56, BASE_POS_11_CARCASSONNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "11_Carcassonne" },
  { 169, 43.053890, 2.218889, 169.0, 3.22, BASE_POS_11_LIMOUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "11_Limoux" },
  { 170, 43.185280, 3.005278, 13.0, 7.47, BASE_POS_11_NARBONNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "11_Narbonne" },
  { 171, 44.098610, 3.082222, 366.0, 7.37, BASE_POS_12_MILLAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "12_Millau" },
  { 172, 44.353611, 2.571389, 618.0, 1.90, BASE_POS_12_RODEZ, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "12_Rodez" },
  { 173, 44.351112, 2.033333, 323.0, 3.83, BASE_POS_12_VILLEFRANCHE_DE_ROUERGUE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "12_Villefranche-de-Rouergue" },
  { 174, 43.527500, 5.447500, 200.0, 7.73, BASE_POS_13_AIX_EN_PROVENCE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "13_Aix-en-Provence", true, BASE_POS_13_D_AIX_EN_PROVENCE },
  { 175, 43.678612, 4.630278, 5.0, 15.66, BASE_POS_13_ARLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "13_Arles", true, BASE_POS_13_D_ARLES },
  { 176, 43.514168, 4.989167, 20.0, 6.04, BASE_POS_13_ISTRES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "13_Istres", true, BASE_POS_13_D_ISTRES },
  { 177, 43.297476, 5.399485, 20.0, 8.73, BASE_POS_13_MARSEILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "13_Marseille" },
  { 178, 49.275276, -0.703333, 50.0, 1.50, BASE_POS_14_BAYEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "14_Bayeux" },
  { 179, 49.184444, -0.360000, 25.0, 2.86, BASE_POS_14_CAEN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "14_Caen" },
  { 180, 49.144169, 0.226389, 46.0, 2.04, BASE_POS_14_LISIEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "14_Lisieux" },
  { 181, 48.843613, -0.885278, 160.0, 2.69, BASE_POS_14_VIRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "14_Vire" },
  { 182, 44.926666, 2.444444, 620.0, 3.05, BASE_POS_15_AURILLAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "15_Aurillac", true, BASE_POS_15_D_AURILLAC },
  { 183, 45.220280, 2.334722, 700.0, 3.00, BASE_POS_15_MAURIAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "15_Mauriac" },
  { 184, 45.034721, 3.094722, 860.0, 2.94, BASE_POS_15_SAINT_FLOUR, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "15_Saint-Flour" },
  { 185, 45.652779, 0.154722, 86.0, 2.64, BASE_POS_16_ANGOULEME, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "16_Angoulême", true, BASE_POS_16_D_ANGOULEME },
  { 186, 45.695557, -0.325000, 21.0, 2.18, BASE_POS_16_COGNAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "16_Cognac" },
  { 187, 46.012222, 0.671667, 150.0, 2.46, BASE_POS_16_CONFOLENS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "16_Confolens" },
  { 188, 45.445831, -0.438056, 40.0, 2.04, BASE_POS_17_JONZAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "17_Jonzac" },
  { 189, 45.943333, -0.963333, 10.0, 2.65, BASE_POS_17_ROCHEFORT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "17_Rochefort" },
  { 190, 46.161945, -1.151944, 10.0, 3.18, BASE_POS_17_LA_ROCHELLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "17_la Rochelle" },
  { 191, 45.945831, -0.518611, 20.0, 2.45, BASE_POS_17_SAINT_JEAN_D_ANGELY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "17_Saint-Jean-d'Angély" },
  { 192, 45.747780, -0.638333, 20.0, 3.82, BASE_POS_17_SAINTES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "17_Saintes" },
  { 193, 47.080276, 2.396944, 130.0, 4.67, BASE_POS_18_BOURGES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "18_Bourges" },
  { 194, 46.728889, 2.504722, 160.0, 2.54, BASE_POS_18_SAINT_AMAND_MONTROND, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "18_Saint-Amand-Montrond" },
  { 195, 47.224167, 2.065556, 120.0, 4.88, BASE_POS_18_VIERZON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "18_Vierzon" },
  { 196, 45.157501, 1.535000, 118.0, 3.93, BASE_POS_19_BRIVE_LA_GAILLARDE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "19_Brive-la-Gaillarde" },
  { 197, 45.263058, 1.768333, 220.0, 2.81, BASE_POS_19_TULLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "19_Tulle" },
  { 198, 45.549442, 2.309722, 620.0, 4.03, BASE_POS_19_USSEL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "19_Ussel", true, BASE_POS_19_D_USSEL },
  { 199, 41.920555, 8.735000, 20.0, 5.15, BASE_POS_2A_AJACCIO, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "2A_Ajaccio", true, BASE_POS_2A_D_AJACCIO },
  { 200, 41.621944, 8.974167, 320.0, 8.02, BASE_POS_2A_SARTENE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "2A_Sartène" },
  { 201, 42.694443, 9.446944, 20.0, 2.51, BASE_POS_2B_BASTIA, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "2B_Bastia" },
  { 202, 42.566944, 8.755000, 20.0, 3.18, BASE_POS_2B_CALVI, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "2B_Calvi" },
  { 203, 42.304169, 9.151389, 400.0, 6.91, BASE_POS_2B_CORTE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "2B_Corte" },
  { 204, 47.024166, 4.840278, 220.0, 3.15, BASE_POS_21_BEAUNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "21_Beaune" },
  { 205, 47.322498, 5.035556, 245.0, 3.63, BASE_POS_21_DIJON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "21_Dijon" },
  { 206, 47.625832, 4.341389, 211.0, 3.84, BASE_POS_21_MONTBARD, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "21_Montbard" },
  { 207, 48.452221, -2.047222, 69.0, 1.12, BASE_POS_22_DINAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "22_Dinan" },
  { 208, 48.561111, -3.147500, 100.0, 1.05, BASE_POS_22_GUINGAMP, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "22_Guingamp" },
  { 209, 48.733891, -3.456667, 10.0, 3.91, BASE_POS_22_LANNION, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "22_Lannion" },
  { 210, 48.514168, -2.773056, 100.0, 2.49, BASE_POS_22_SAINT_BRIEUC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "22_Saint-Brieuc" },
  { 211, 45.954723, 2.169167, 440.0, 2.48, BASE_POS_23_AUBUSSON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "23_Aubusson" },
  { 212, 46.169167, 1.874722, 431.0, 2.88, BASE_POS_23_GUERET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "23_Guéret" },
  { 213, 44.852222, 0.484167, 39.0, 4.25, BASE_POS_24_BERGERAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "24_Bergerac" },
  { 214, 45.531113, 0.663333, 200.0, 2.86, BASE_POS_24_NONTRON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "24_Nontron" },
  { 215, 45.185001, 0.721111, 90.0, 1.80, BASE_POS_24_PERIGUEUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "24_Périgueux" },
  { 216, 44.888058, 1.215556, 129.0, 3.93, BASE_POS_24_SARLAT_LA_CANEDA, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "24_Sarlat-la-Canéda" },
  { 217, 47.245277, 6.025833, 261.0, 4.56, BASE_POS_25_BESANCON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "25_Besançon" },
  { 218, 47.511944, 6.800000, 330.0, 2.18, BASE_POS_25_MONTBELIARD, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "25_Montbéliard" },
  { 219, 46.904999, 6.355278, 840.0, 3.63, BASE_POS_25_PONTARLIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "25_Pontarlier" },
  { 220, 44.756390, 5.368056, 402.0, 4.28, BASE_POS_26_DIE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "26_Die" },
  { 221, 44.359165, 5.135833, 280.0, 2.74, BASE_POS_26_NYONS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "26_Nyons" },
  { 222, 44.926945, 4.895000, 126.0, 3.42, BASE_POS_26_VALENCE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "26_Valence" },
  { 223, 49.246944, 1.423333, 30.0, 3.55, BASE_POS_27_LES_ANDELYS, RULE_1_PLURAL, RULE_2_TO, TOWN_WITH_RADIUS, "27_les Andelys", true, BASE_POS_27_DES_ANDELYS },
  { 224, 49.090279, 0.599167, 110.0, 2.77, BASE_POS_27_BERNAY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "27_Bernay" },
  { 225, 49.023335, 1.153889, 70.0, 2.89, BASE_POS_27_EVREUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "27_Évreux", true, BASE_POS_27_D_EVREUX },
  { 226, 48.445831, 1.491944, 140.0, 2.32, BASE_POS_28_CHARTRES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "28_Chartres" },
  { 227, 48.071945, 1.333889, 141.0, 3.01, BASE_POS_28_CHATEAUDUN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "28_Châteaudun" },
  { 228, 48.731388, 1.371667, 100.0, 2.78, BASE_POS_28_DREUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "28_Dreux" },
  { 229, 48.321110, 0.820000, 110.0, 2.73, BASE_POS_28_NOGENT_LE_ROTROU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "28_Nogent-le-Rotrou" },
  { 230, 48.387779, -4.490278, 50.0, 3.95, BASE_POS_29_BREST, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "29_Brest" },
  { 231, 48.195278, -4.091389, 40.0, 2.53, BASE_POS_29_CHATEAULIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "29_Châteaulin" },
  { 232, 48.580833, -3.830000, 80.0, 2.82, BASE_POS_29_MORLAIX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "29_Morlaix" },
  { 233, 47.994999, -4.109167, 30.0, 5.19, BASE_POS_29_QUIMPER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "29_Quimper" },
  { 234, 44.128334, 4.081389, 140.0, 2.72, BASE_POS_30_ALES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "30_Alès", true, BASE_POS_30_D_ALES },
  { 235, 43.838612, 4.360833, 46.0, 7.17, BASE_POS_30_NIMES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "30_Nîmes" },
  { 236, 43.989445, 3.603889, 233.0, 2.34, BASE_POS_30_LE_VIGAN, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITH_RADIUS, "30_le Vigan", true, BASE_POS_30_DU_VIGAN },
  { 237, 43.460835, 1.326944, 169.0, 4.31, BASE_POS_31_MURET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "31_Muret" },
  { 238, 43.109722, 0.724444, 401.0, 3.28, BASE_POS_31_SAINT_GAUDENS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "31_Saint-Gaudens" },
  { 239, 43.599998, 1.450278, 146.0, 6.16, BASE_POS_31_TOULOUSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "31_Toulouse" },
  { 240, 43.645000, 0.588056, 134.0, 4.84, BASE_POS_32_AUCH, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "32_Auch", true, BASE_POS_32_D_AUCH },
  { 241, 43.958889, 0.371111, 73.0, 5.59, BASE_POS_32_CONDOM, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "32_Condom" },
  { 242, 43.515278, 0.403889, 163.0, 2.75, BASE_POS_32_MIRANDE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "32_Mirande" },
  { 243, 44.658333, -1.170278, 10.0, 1.57, BASE_POS_33_ARCACHON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "33_Arcachon", true, BASE_POS_33_D_ARCACHON },
  { 244, 45.127777, -0.658056, 8.0, 2.23, BASE_POS_33_BLAYE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "33_Blaye" },
  { 245, 44.843056, -0.575000, 16.0, 3.97, BASE_POS_33_BORDEAUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "33_Bordeaux" },
  { 246, 44.551388, -0.248611, 20.0, 2.10, BASE_POS_33_LANGON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "33_Langon" },
  { 247, 45.307499, -0.935556, 10.0, 3.42, BASE_POS_33_LESPARRE_MEDOC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "33_Lesparre-Médoc" },
  { 248, 44.916943, -0.233889, 10.0, 2.57, BASE_POS_33_LIBOURNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "33_Libourne" },
  { 249, 43.345001, 3.218333, 50.0, 5.52, BASE_POS_34_BEZIERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "34_Béziers" },
  { 250, 43.733612, 3.322778, 160.0, 2.72, BASE_POS_34_LODEVE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "34_Lodève" },
  { 251, 43.608891, 3.874167, 35.0, 4.27, BASE_POS_34_MONTPELLIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "34_Montpellier" },
  { 252, 48.352779, -1.201389, 90.0, 1.82, BASE_POS_35_FOUGERES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "35_Fougères" },
  { 253, 47.653889, -2.079444, 50.0, 2.21, BASE_POS_35_REDON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "35_Redon" },
  { 254, 48.107777, -1.680278, 35.0, 4.01, BASE_POS_35_RENNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "35_Rennes" },
  { 255, 48.634724, -2.011667, 30.0, 3.41, BASE_POS_35_SAINT_MALO, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "35_Saint-Malo" },
  { 256, 46.635555, 1.064167, 90.0, 4.28, BASE_POS_36_LE_BLANC, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITH_RADIUS, "36_le Blanc", true, BASE_POS_36_DU_BLANC },
  { 257, 46.811390, 1.698889, 143.0, 2.85, BASE_POS_36_CHATEAUROUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "36_Châteauroux" },
  { 258, 46.581944, 1.989722, 210.0, 1.39, BASE_POS_36_LA_CHATRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "36_la Châtre" },
  { 259, 46.947498, 1.990833, 140.0, 3.44, BASE_POS_36_ISSOUDUN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "36_Issoudun", true, BASE_POS_36_D_ISSOUDUN },
  { 260, 47.167778, 0.246944, 50.0, 3.52, BASE_POS_37_CHINON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "37_Chinon" },
  { 261, 47.128334, 0.995556, 80.0, 2.95, BASE_POS_37_LOCHES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "37_Loches" },
  { 262, 47.392776, 0.683611, 52.0, 3.24, BASE_POS_37_TOURS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "37_Tours" },
  { 263, 45.185001, 5.722778, 212.0, 2.43, BASE_POS_38_GRENOBLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "38_Grenoble" },
  { 264, 45.564720, 5.445000, 330.0, 1.23, BASE_POS_38_LA_TOUR_DU_PIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "38_la Tour-du-Pin" },
  { 265, 45.525276, 4.875833, 160.0, 2.68, BASE_POS_38_VIENNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "38_Vienne" },
  { 266, 47.091667, 5.496389, 225.0, 3.48, BASE_POS_39_DOLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "39_Dole" },
// Invalid nbr of fields (13 != 9)
  { 267, 46.672501, 5.552222, 267.0, 1.56, BASE_POS_39_LONS_LE_SAUNIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "39_Lons-le-Saunier" },
  { 268, 46.388058, 5.863611, 434.0, 4.73, BASE_POS_39_SAINT_CLAUDE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "39_Saint-Claude" },
  { 269, 43.707222, -1.055278, 13.0, 2.44, BASE_POS_40_DAX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "40_Dax" },
  { 270, 43.891109, -0.501111, 52.0, 3.42, BASE_POS_40_MONT_DE_MARSAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "40_Mont-de-Marsan" },
  { 271, 47.588612, 1.327500, 100.0, 3.46, BASE_POS_41_BLOIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "41_Blois" },
  { 272, 47.360558, 1.746667, 90.0, 3.79, BASE_POS_41_ROMORANTIN_LANTHENAY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "41_Romorantin-Lanthenay" },
  { 273, 47.793610, 1.066667, 80.0, 2.76, BASE_POS_41_VENDOME, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "41_Vendôme" },
  { 274, 45.607777, 4.065000, 420.0, 2.28, BASE_POS_42_MONTBRISON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "42_Montbrison" },
  { 275, 46.034721, 4.069722, 278.0, 2.26, BASE_POS_42_ROANNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "42_Roanne" },
  { 276, 45.440834, 4.390000, 515.0, 5.05, BASE_POS_42_SAINT_ETIENNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "42_Saint-Étienne" },
  { 277, 45.294724, 3.382778, 421.0, 2.07, BASE_POS_43_BRIOUDE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "43_Brioude" },
  { 278, 45.041943, 3.887500, 628.0, 2.31, BASE_POS_43_LE_PUY_EN_VELAY, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITH_RADIUS, "43_le Puy-en-Velay", true, BASE_POS_43_DU_PUY_EN_VELAY },
  { 279, 45.142502, 4.123611, 869.0, 5.09, BASE_POS_43_YSSINGEAUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "43_Yssingeaux", true, BASE_POS_43_D_YSSINGEAUX },
  { 280, 47.370277, -1.178056, 19.0, 2.54, BASE_POS_44_ANCENIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "44_Ancenis", true, BASE_POS_44_D_ANCENIS },
  { 281, 47.719444, -1.375278, 60.0, 3.27, BASE_POS_44_CHATEAUBRIANT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "44_Châteaubriant" },
  { 282, 47.219166, -1.553889, 20.0, 4.58, BASE_POS_44_NANTES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "44_Nantes" },
  { 283, 47.279446, -2.219167, 10.0, 3.92, BASE_POS_44_SAINT_NAZAIRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "44_Saint-Nazaire" },
  { 284, 47.998333, 2.747222, 90.0, 1.19, BASE_POS_45_MONTARGIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "45_Montargis" },
  { 285, 47.903889, 1.907222, 110.0, 2.97, BASE_POS_45_ORLEANS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "45_Orléans", true, BASE_POS_45_D_ORLEANS },
  { 286, 48.173889, 2.256667, 110.0, 1.49, BASE_POS_45_PITHIVIERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "45_Pithiviers" },
  { 287, 44.447777, 1.441111, 122.0, 4.55, BASE_POS_46_CAHORS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "46_Cahors" },
  { 288, 44.609165, 2.032222, 200.0, 3.36, BASE_POS_46_FIGEAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "46_Figeac" },
  { 289, 44.737499, 1.383056, 230.0, 3.82, BASE_POS_46_GOURDON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "46_Gourdon" },
  { 290, 44.207779, 0.621111, 50.0, 1.90, BASE_POS_47_AGEN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "47_Agen", true, BASE_POS_47_D_AGEN },
  { 291, 44.502220, 0.161111, 30.0, 4.41, BASE_POS_47_MARMANDE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "47_Marmande" },
  { 292, 44.135834, 0.336667, 56.0, 4.47, BASE_POS_47_NERAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "47_Nérac" },
  { 293, 44.408054, 0.705278, 56.0, 5.11, BASE_POS_47_VILLENEUVE_SUR_LOT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "47_Villeneuve-sur-Lot" },
  { 294, 44.324444, 3.591389, 560.0, 3.06, BASE_POS_48_FLORAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "48_Florac" },
  { 295, 44.518612, 3.498333, 740.0, 3.43, BASE_POS_48_MENDE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "48_Mende" },
  { 296, 47.470554, -0.555833, 30.0, 3.76, BASE_POS_49_ANGERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "49_Angers", true, BASE_POS_49_D_ANGERS },
  { 297, 47.059444, -0.875556, 100.0, 5.28, BASE_POS_49_CHOLET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "49_Cholet" },
  { 298, 47.257221, -0.076667, 40.0, 4.60, BASE_POS_49_SAUMUR, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "49_Saumur" },
  { 299, 47.688610, -0.867778, 30.0, 2.27, BASE_POS_49_SEGRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "49_Segré" },
  { 300, 48.685833, -1.360556, 80.0, 1.20, BASE_POS_50_AVRANCHES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "50_Avranches", true, BASE_POS_50_D_AVRANCHES },
  { 301, 49.641666, -1.626389, 19.0, 1.65, BASE_POS_50_CHERBOURG_OCTEVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "50_Cherbourg-Octeville" },
  { 302, 49.050278, -1.443333, 70.0, 2.01, BASE_POS_50_COUTANCES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "50_Coutances" },
  { 303, 49.113888, -1.091944, 30.0, 2.72, BASE_POS_50_SAINT_LO, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "50_Saint-Lô" },
  { 304, 48.957222, 4.361667, 85.0, 2.88, BASE_POS_51_CHALONS_EN_CHAMPAGNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "51_Châlons-en-Champagne" },
  { 305, 49.043331, 3.956389, 80.0, 2.70, BASE_POS_51_EPERNAY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "51_Épernay", true, BASE_POS_51_D_EPERNAY },
  { 306, 49.258057, 4.031389, 83.0, 3.97, BASE_POS_51_REIMS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "51_Reims" },
  { 307, 49.091389, 4.898333, 150.0, 4.27, BASE_POS_51_SAINTE_MENEHOULD, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "51_Sainte-Menehould" },
  { 308, 48.721943, 4.591667, 102.0, 1.43, BASE_POS_51_VITRY_LE_FRANCOIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "51_Vitry-le-François" },
  { 309, 48.110279, 5.139722, 290.0, 4.20, BASE_POS_52_CHAUMONT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "52_Chaumont" },
  { 310, 47.864166, 5.335000, 450.0, 2.72, BASE_POS_52_LANGRES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "52_Langres" },
  { 311, 48.639721, 4.950556, 143.0, 3.93, BASE_POS_52_SAINT_DIZIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "52_Saint-Dizier" },
  { 312, 47.829445, -0.707222, 50.0, 2.98, BASE_POS_53_CHATEAU_GONTIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "53_Château-Gontier" },
  { 313, 48.071945, -0.773056, 51.0, 3.31, BASE_POS_53_LAVAL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "53_Laval" },
  { 314, 48.301109, -0.614444, 110.0, 2.54, BASE_POS_53_MAYENNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "53_Mayenne" },
  { 315, 49.247780, 5.940556, 250.0, 2.94, BASE_POS_54_BRIEY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "54_Briey" },
  { 316, 48.589169, 6.499444, 240.0, 2.29, BASE_POS_54_LUNEVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "54_Lunéville" },
  { 317, 48.689720, 6.174444, 222.0, 2.19, BASE_POS_54_NANCY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "54_Nancy" },
  { 318, 48.675278, 5.889167, 210.0, 3.14, BASE_POS_54_TOUL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "54_Toul" },
  { 319, 48.771111, 5.172778, 220.0, 2.75, BASE_POS_55_BAR_LE_DUC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "55_Bar-le-Duc" },
  { 320, 48.761391, 5.591111, 250.0, 3.37, BASE_POS_55_COMMERCY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "55_Commercy" },
  { 321, 49.164165, 5.391667, 200.0, 3.19, BASE_POS_55_VERDUN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "55_Verdun" },
  { 322, 47.745277, -3.363611, 5.0, 2.17, BASE_POS_56_LORIENT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "56_Lorient" },
  { 323, 48.068890, -2.968611, 59.0, 2.83, BASE_POS_56_PONTIVY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "56_Pontivy" },
  { 324, 47.654999, -2.760556, 10.0, 3.25, BASE_POS_56_VANNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "56_Vannes" },
  { 325, 49.183613, 6.498056, 220.0, 2.50, BASE_POS_57_BOULAY_MOSELLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Boulay-Moselle" },
  { 326, 48.823612, 6.506389, 230.0, 1.87, BASE_POS_57_CHATEAU_SALINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Château-Salins" },
  { 327, 49.187222, 6.900278, 220.0, 2.29, BASE_POS_57_FORBACH, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Forbach" },
  { 328, 49.109165, 6.183056, 182.0, 3.64, BASE_POS_57_METZ, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Metz" },
  { 329, 48.732498, 7.051111, 260.0, 2.30, BASE_POS_57_SARREBOURG, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Sarrebourg" },
  { 330, 49.106388, 7.064167, 210.0, 3.09, BASE_POS_57_SARREGUEMINES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Sarreguemines" },
  { 331, 49.366390, 6.146389, 155.0, 3.99, BASE_POS_57_THIONVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "57_Thionville" },
  { 332, 47.064999, 3.933889, 534.0, 1.15, BASE_POS_58_CHATEAU_CHINON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "58_Château-Chinon" },
  { 333, 47.458611, 3.518889, 150.0, 3.14, BASE_POS_58_CLAMECY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "58_Clamecy" },
  { 334, 47.409168, 2.923333, 150.0, 4.14, BASE_POS_58_COSNE_COURS_SUR_LOIRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "58_Cosne-Cours-sur-Loire" },
  { 335, 46.994446, 3.155556, 201.0, 2.36, BASE_POS_58_NEVERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "58_Nevers" },
  { 336, 50.123333, 3.929722, 152.0, 0.85, BASE_POS_59_AVESNES_SUR_HELPE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "59_Avesnes-sur-Helpe", true, BASE_POS_59_D_AVESNES_SUR_HELPE },
  { 337, 50.176666, 3.239722, 50.0, 2.41, BASE_POS_59_CAMBRAI, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "59_Cambrai" },
  { 338, 50.370277, 3.079444, 25.0, 2.32, BASE_POS_59_DOUAI, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "59_Douai" },
  { 339, 51.035278, 2.378333, 4.0, 3.39, BASE_POS_59_DUNKERQUE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "59_Dunkerque" },
  { 340, 50.628056, 3.044722, 20.0, 2.84, BASE_POS_59_LILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "59_Lille" },
  { 341, 50.359444, 3.524444, 22.0, 2.09, BASE_POS_59_VALENCIENNES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "59_Valenciennes" },
  { 342, 49.435280, 2.081944, 67.0, 3.25, BASE_POS_60_BEAUVAIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "60_Beauvais" },
  { 343, 49.378334, 2.413333, 130.0, 1.36, BASE_POS_60_CLERMONT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "60_Clermont" },
  { 344, 49.412777, 2.825833, 50.0, 4.12, BASE_POS_60_COMPIEGNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "60_Compiègne" },
  { 345, 49.206390, 2.585000, 73.0, 2.78, BASE_POS_60_SENLIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "60_Senlis" },
  { 346, 48.433056, 0.088889, 130.0, 1.85, BASE_POS_61_ALENCON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "61_Alençon", true, BASE_POS_61_D_ALENCON },
  { 347, 48.743610, -0.020278, 161.0, 2.42, BASE_POS_61_ARGENTAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "61_Argentan", true, BASE_POS_61_D_ARGENTAN },
  { 348, 48.523609, 0.547222, 240.0, 1.66, BASE_POS_61_MORTAGNE_AU_PERCHE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "61_Mortagne-au-Perche" },
  { 349, 50.289165, 2.767222, 61.0, 1.94, BASE_POS_62_ARRAS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Arras", true, BASE_POS_62_D_ARRAS },
  { 350, 50.525276, 2.639722, 33.0, 2.04, BASE_POS_62_BETHUNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Béthune" },
  { 351, 50.727779, 1.610278, 50.0, 1.58, BASE_POS_62_BOULOGNE_SUR_MER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Boulogne-sur-Mer" },
  { 352, 50.950279, 1.862778, 5.0, 3.26, BASE_POS_62_CALAIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Calais" },
  { 353, 50.432777, 2.824167, 39.0, 1.93, BASE_POS_62_LENS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Lens" },
  { 354, 50.464722, 1.764444, 54.0, 0.94, BASE_POS_62_MONTREUIL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Montreuil" },
  { 355, 50.755554, 2.255833, 10.0, 2.29, BASE_POS_62_SAINT_OMER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "62_Saint-Omer" },
  { 356, 45.551109, 3.740000, 560.0, 4.39, BASE_POS_63_AMBERT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "63_Ambert", true, BASE_POS_63_D_AMBERT },
  { 357, 45.779167, 3.085000, 365.0, 3.71, BASE_POS_63_CLERMONT_FERRAND, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "63_Clermont-Ferrand" },
  { 358, 45.542778, 3.247778, 390.0, 2.50, BASE_POS_63_ISSOIRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "63_Issoire", true, BASE_POS_63_D_ISSOIRE },
  { 359, 45.890835, 3.112222, 340.0, 3.23, BASE_POS_63_RIOM, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "63_Riom" },
  { 360, 45.856388, 3.547222, 340.0, 3.78, BASE_POS_63_THIERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "63_Thiers" },
  { 361, 43.491112, -1.478056, 4.0, 2.87, BASE_POS_64_BAYONNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "64_Bayonne" },
  { 362, 43.192780, -0.609167, 216.0, 4.68, BASE_POS_64_OLORON_SAINTE_MARIE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "64_Oloron-Sainte-Marie", true, BASE_POS_64_D_OLORON_SAINTE_MARIE },
  { 363, 43.302776, -0.367500, 210.0, 3.17, BASE_POS_64_PAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "64_Pau" },
  { 364, 43.005833, -0.098889, 508.0, 0.99, BASE_POS_65_ARGELES_GAZOST, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "65_Argelès-Gazost", true, BASE_POS_65_D_ARGELES_GAZOST },
  { 365, 43.064999, 0.151667, 550.0, 6.34, BASE_POS_65_BAGNERES_DE_BIGORRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "65_Bagnères-de-Bigorre" },
  { 366, 43.233891, 0.071111, 311.0, 2.22, BASE_POS_65_TARBES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "65_Tarbes" },
  { 367, 42.489445, 2.748889, 160.0, 3.48, BASE_POS_66_CERET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "66_Céret" },
  { 368, 42.698055, 2.893333, 40.0, 4.67, BASE_POS_66_PERPIGNAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "66_Perpignan" },
  { 369, 42.617779, 2.421944, 356.0, 1.88, BASE_POS_66_PRADES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "66_Prades" },
  { 370, 48.821388, 7.790833, 140.0, 7.62, BASE_POS_67_HAGUENAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "67_Haguenau" },
  { 371, 48.541943, 7.494722, 175.0, 1.84, BASE_POS_67_MOLSHEIM, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "67_Molsheim" },
  { 372, 48.741943, 7.360556, 190.0, 2.91, BASE_POS_67_SAVERNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "67_Saverne" },
  { 373, 48.260555, 7.446944, 175.0, 3.89, BASE_POS_67_SELESTAT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "67_Sélestat" },
  { 374, 48.584442, 7.755833, 144.0, 4.99, BASE_POS_67_STRASBOURG, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "67_Strasbourg" },
  { 375, 49.036945, 7.940278, 170.0, 3.90, BASE_POS_67_WISSEMBOURG, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "67_Wissembourg" },
  { 376, 47.624168, 7.238056, 290.0, 1.74, BASE_POS_68_ALTKIRCH, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "68_Altkirch", true, BASE_POS_68_D_ALTKIRCH },
  { 377, 48.077778, 7.355278, 198.0, 4.60, BASE_POS_68_COLMAR, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "68_Colmar" },
  { 378, 47.908054, 7.210833, 270.0, 1.75, BASE_POS_68_GUEBWILLER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "68_Guebwiller" },
  { 379, 47.748055, 7.332778, 255.0, 2.67, BASE_POS_68_MULHOUSE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "68_Mulhouse" },
  { 380, 48.191666, 7.322222, 242.0, 3.24, BASE_POS_68_RIBEAUVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "68_Ribeauvillé" },
  { 381, 47.808887, 7.104722, 329.0, 1.99, BASE_POS_68_THANN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "68_Thann" },
  { 382, 45.759800, 4.834815, 166.0, 3.91, BASE_POS_69_LYON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "69_Lyon" },
  { 383, 45.988888, 4.723889, 194.0, 1.73, BASE_POS_69_VILLEFRANCHE_SUR_SAONE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "69_Villefranche-sur-Saône" },
  { 384, 47.682499, 6.498056, 295.0, 2.80, BASE_POS_70_LURE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "70_Lure" },
  { 385, 47.620834, 6.158056, 220.0, 1.70, BASE_POS_70_VESOUL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "70_Vesoul" },
  { 386, 46.951942, 4.302222, 312.0, 4.43, BASE_POS_71_AUTUN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "71_Autun", true, BASE_POS_71_D_AUTUN },
  { 387, 46.786110, 4.849722, 178.0, 2.21, BASE_POS_71_CHALON_SUR_SAONE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "71_Chalon-sur-Saône" },
  { 388, 46.434723, 4.275556, 290.0, 2.52, BASE_POS_71_CHAROLLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "71_Charolles" },
  { 389, 46.627220, 5.223889, 180.0, 2.69, BASE_POS_71_LOUHANS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "71_Louhans" },
  { 390, 46.308334, 4.831389, 190.0, 2.94, BASE_POS_71_MACON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "71_Mâcon" },
  { 391, 47.698891, -0.071111, 30.0, 5.02, BASE_POS_72_LA_FLECHE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "72_la Flèche" },
  { 392, 48.349167, 0.368611, 120.0, 1.28, BASE_POS_72_MAMERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "72_Mamers" },
  { 393, 47.996387, 0.203056, 50.0, 4.09, BASE_POS_72_LE_MANS, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITH_RADIUS, "72_le Mans", true, BASE_POS_72_DU_MANS },
  { 394, 45.674168, 6.386389, 340.0, 2.34, BASE_POS_73_ALBERTVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "73_Albertville", true, BASE_POS_73_D_ALBERTVILLE },
  { 395, 45.571388, 5.918611, 273.0, 2.60, BASE_POS_73_CHAMBERY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "73_Chambéry" },
  { 396, 45.275833, 6.346667, 540.0, 1.91, BASE_POS_73_SAINT_JEAN_DE_MAURIENNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "73_Saint-Jean-de-Maurienne" },
  { 397, 45.906944, 6.126667, 453.0, 2.26, BASE_POS_74_ANNECY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "74_Annecy", true, BASE_POS_74_D_ANNECY },
  { 398, 46.078609, 6.408611, 450.0, 2.95, BASE_POS_74_BONNEVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "74_Bonneville" },
  { 399, 46.143333, 6.081944, 450.0, 1.84, BASE_POS_74_SAINT_JULIEN_EN_GENEVOIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "74_Saint-Julien-en-Genevois" },
  { 400, 46.369167, 6.484444, 431.0, 2.28, BASE_POS_74_THONON_LES_BAINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "74_Thonon-les-Bains" },
  { 401, 48.860802, 2.345800, 60.0, 5.80, BASE_POS_75_PARIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "75_Paris" },
  { 402, 49.919998, 1.078333, 10.0, 1.97, BASE_POS_76_DIEPPE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "76_Dieppe" },
  { 403, 49.501110, 0.126944, 70.0, 4.17, BASE_POS_76_LE_HAVRE, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITH_RADIUS, "76_le Havre", true, BASE_POS_76_DU_HAVRE },
  { 404, 49.437778, 1.089167, 22.0, 2.62, BASE_POS_76_ROUEN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "76_Rouen" },
  { 405, 48.408054, 2.699444, 80.0, 7.42, BASE_POS_77_FONTAINEBLEAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "77_Fontainebleau" },
  { 406, 48.958889, 2.887778, 50.0, 2.18, BASE_POS_77_MEAUX, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "77_Meaux" },
  { 407, 48.540558, 2.657222, 50.0, 1.57, BASE_POS_77_MELUN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "77_Melun" },
  { 408, 48.561943, 3.300833, 92.0, 2.17, BASE_POS_77_PROVINS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "77_Provins" },
  { 409, 48.851112, 2.650000, 100.0, 1.40, BASE_POS_77_TORCY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "77_Torcy" },
  { 410, 48.993610, 1.711111, 27.0, 1.74, BASE_POS_78_MANTES_LA_JOLIE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "78_Mantes-la-Jolie" },
  { 411, 48.649445, 1.825556, 154.0, 3.38, BASE_POS_78_RAMBOUILLET, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "78_Rambouillet" },
  { 412, 48.895279, 2.089167, 92.0, 3.96, BASE_POS_78_SAINT_GERMAIN_EN_LAYE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "78_Saint-Germain-en-Laye" },
  { 413, 48.806946, 2.136667, 135.0, 2.88, BASE_POS_78_VERSAILLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "78_Versailles" },
  { 414, 46.839722, -0.490556, 182.0, 7.62, BASE_POS_79_BRESSUIRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "79_Bressuire" },
  { 415, 46.324444, -0.462500, 28.0, 4.67, BASE_POS_79_NIORT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "79_Niort" },
  { 416, 46.648335, -0.245000, 169.0, 1.90, BASE_POS_79_PARTHENAY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "79_Parthenay" },
  { 417, 50.106667, 1.836389, 10.0, 2.90, BASE_POS_80_ABBEVILLE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "80_Abbeville", true, BASE_POS_80_D_ABBEVILLE },
  { 418, 49.894444, 2.293333, 35.0, 4.00, BASE_POS_80_AMIENS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "80_Amiens", true, BASE_POS_80_D_AMIENS },
  { 419, 49.649166, 2.570833, 82.0, 2.00, BASE_POS_80_MONTDIDIER, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "80_Montdidier" },
  { 420, 49.933334, 2.934444, 50.0, 2.13, BASE_POS_80_PERONNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "80_Péronne" },
  { 421, 43.928333, 2.142778, 174.0, 3.79, BASE_POS_81_ALBI, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "81_Albi", true, BASE_POS_81_D_ALBI },
  { 422, 43.607498, 2.239722, 173.0, 5.61, BASE_POS_81_CASTRES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "81_Castres" },
  { 423, 44.040001, 1.106667, 87.0, 4.95, BASE_POS_82_CASTELSARRASIN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "82_Castelsarrasin" },
  { 424, 44.017776, 1.360000, 103.0, 6.59, BASE_POS_82_MONTAUBAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "82_Montauban" },
  { 425, 43.408054, 6.063333, 220.0, 4.75, BASE_POS_83_BRIGNOLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "83_Brignoles" },
  { 426, 43.539165, 6.466944, 200.0, 4.15, BASE_POS_83_DRAGUIGNAN, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "83_Draguignan" },
  { 427, 43.126667, 5.933611, 30.0, 3.75, BASE_POS_83_TOULON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "83_Toulon" },
  { 428, 43.875557, 5.397778, 222.0, 3.79, BASE_POS_84_APT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "84_Apt", true, BASE_POS_84_D_APT },
  
// TODO: Add 'BASE_POS_84_D_AVIGNON' ;-)
  { 429, 43.946110, 4.810278, 19.0, 4.55, BASE_POS_84_AVIGNON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "84_Avignon" },

  { 430, 44.056389, 5.048889, 85.0, 3.47, BASE_POS_84_CARPENTRAS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "84_Carpentras" },
  { 431, 46.468613, -0.804167, 80.0, 3.30, BASE_POS_85_FONTENAY_LE_COMTE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "85_Fontenay-le-Comte" },
  { 432, 46.672779, -1.430000, 50.0, 5.29, BASE_POS_85_LA_ROCHE_SUR_YON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "85_la Roche-sur-Yon" },
  { 433, 46.496387, -1.785000, 10.0, 1.67, BASE_POS_85_LES_SABLES_D_OLONNE, RULE_1_PLURAL, RULE_2_TO, TOWN_WITH_RADIUS, "85_les Sables-d'Olonne", true, BASE_POS_85_DES_SABLES_D_OLONNE },
  { 434, 46.818333, 0.544444, 52.0, 4.06, BASE_POS_86_CHATELLERAULT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "86_Châtellerault" },
  { 435, 46.426388, 0.871389, 90.0, 4.25, BASE_POS_86_MONTMORILLON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "86_Montmorillon" },
  { 436, 46.584721, 0.343889, 116.0, 3.68, BASE_POS_86_POITIERS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "86_Poitiers" },
  { 437, 46.121944, 1.050000, 250.0, 2.79, BASE_POS_87_BELLAC, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "87_Bellac" },
  { 438, 45.831944, 1.258611, 306.0, 5.04, BASE_POS_87_LIMOGES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "87_Limoges" },
  { 439, 45.824444, 0.823889, 250.0, 4.14, BASE_POS_87_ROCHECHOUART, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "87_Rochechouart" },
  { 440, 48.176666, 6.447500, 330.0, 4.35, BASE_POS_88_EPINAL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "88_Épinal", true, BASE_POS_88_D_EPINAL },
  { 441, 48.355278, 5.696944, 300.0, 2.76, BASE_POS_88_NEUFCHATEAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "88_Neufchâteau" },
  { 442, 48.285000, 6.949167, 340.0, 3.84, BASE_POS_88_SAINT_DIE_DES_VOSGES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "88_Saint-Dié-des-Vosges" },
  { 443, 47.798332, 3.571389, 120.0, 3.98, BASE_POS_89_AUXERRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "89_Auxerre", true, BASE_POS_89_D_AUXERRE },
  { 444, 47.488888, 3.910278, 220.0, 2.91, BASE_POS_89_AVALLON, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "89_Avallon", true, BASE_POS_89_D_AVALLON },
  { 445, 48.197224, 3.285833, 70.0, 2.98, BASE_POS_89_SENS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "89_Sens" },
  { 446, 47.642223, 6.855833, 361.0, 2.33, BASE_POS_90_BELFORT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "90_Belfort" },
  { 447, 48.432220, 2.160833, 75.0, 3.83, BASE_POS_91_ETAMPES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "91_Étampes" },
  { 448, 48.635834, 2.442500, 75.0, 1.65, BASE_POS_91_EVRY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "91_Évry" },
  { 449, 48.712223, 2.238889, 120.0, 1.93, BASE_POS_91_PALAISEAU, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "91_Palaiseau" },
  { 450, 48.753056, 2.298333, 50.0, 1.75, BASE_POS_92_ANTONY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "92_Antony", true, BASE_POS_92_D_ANTONY },
  { 451, 48.839722, 2.242222, 33.0, 1.40, BASE_POS_92_BOULOGNE_BILLANCOURT, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "92_Boulogne-Billancourt" },
  { 452, 48.890556, 2.208889, 42.0, 1.97, BASE_POS_92_NANTERRE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "92_Nanterre" },
  { 453, 48.907780, 2.441944, 50.0, 1.46, BASE_POS_93_BOBIGNY, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "93_Bobigny" },
  { 454, 48.897221, 2.519167, 76.0, 0.85, BASE_POS_93_LE_RAINCY, RULE_1_SINGULAR, RULE_2_TO, TOWN_WITH_RADIUS, "93_le Raincy", true, BASE_POS_93_DU_RAINCY },
  { 455, 48.930557, 2.355556, 32.0, 1.99, BASE_POS_93_SAINT_DENIS, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "93_Saint-Denis" },
  { 456, 48.793331, 2.460833, 34.0, 1.91, BASE_POS_94_CRETEIL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "94_Créteil" },
  { 457, 48.776943, 2.337778, 93.0, 1.08, BASE_POS_94_L_HAY_LES_ROSES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "94_l'Haÿ-les-Roses" },
  { 458, 48.838333, 2.483333, 50.0, 0.94, BASE_POS_94_NOGENT_SUR_MARNE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "94_Nogent-sur-Marne" },
  { 459, 48.959721, 2.239722, 29.0, 2.35, BASE_POS_95_ARGENTEUIL, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "95_Argenteuil", true, BASE_POS_95_D_ARGENTEUIL },
  { 460, 49.051666, 2.094167, 62.0, 1.52, BASE_POS_95_CERGY_PONTOISE, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "95_Cergy-Pontoise" },
  { 461, 48.999722, 2.377500, 68.0, 1.64, BASE_POS_95_SARCELLES, RULE_1_SINGULAR, RULE_2_AT, TOWN_WITH_RADIUS, "95_Sarcelles" },

  { -1, 0.0, 0.0, 0.0, 0.0, BASE_LAST_PROMPT_POS_COMMUNES, RULE_1_NO_MEAN, RULE_2_NO_MEAN, TOWN_NO_MEAN, NULL }
};

// 462 elements = (484 definitions - 22 spec rules)

// End of the generation automatic ------------------

size_t getPosCommNbrPositions()
{
  // -1 car la dernière définition n'est pas une position
  return (int)((sizeof(g__pos_communes_coord) / sizeof(g__pos_communes_coord[0])) - 1);
}

bool getPosCommPositionOfCommune(size_t i__idx, float *o__lat, float *o__lon, float *o__ele)
{
  bool l__flg_rtn = false;

  if (i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) {
    *o__lat = g__pos_communes_coord[i__idx].lat;
    *o__lon = g__pos_communes_coord[i__idx].lon;
    *o__ele = g__pos_communes_coord[i__idx].ele;

    l__flg_rtn = true;
  }

  return l__flg_rtn;
}

const char *getPosCommNameOfCommune(size_t i__idx)
{
  return (i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) ? g__pos_communes_coord[i__idx].name_commune : "Unknown";
}

int getPosCommRule1(size_t i__idx)
{
  return (int)((i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) ? g__pos_communes_coord[i__idx].rule_1 : RULE_1_NO_MEAN);
}

int getPosCommRule2(size_t i__idx)
{
  return (int)((i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) ? g__pos_communes_coord[i__idx].rule_2 : RULE_2_NO_MEAN);
}

ENUM_TYPE_TOWN getPosCommTypeTown(size_t i__idx)
{
  return ((i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) ? g__pos_communes_coord[i__idx].type_town : TOWN_NO_MEAN);
}

float getPosCommRadius(size_t i__idx)
{
  return ((i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) ? g__pos_communes_coord[i__idx].radius : 0.3);
}

int getPosCommBase(size_t i__idx)
{
  return (int)((i__idx != (size_t)-1 && i__idx < getPosCommNbrPositions()) ? g__pos_communes_coord[i__idx].base : -1);
}

// Souvenir du passé pour la synthèse des positions des communes
static int                      g__idx_of_pos_commune_previous          = -1;
static uint32_t                 g__dist_to_pos_commune_rounded_previous = (uint32_t)-1L;
static boolean                  g__to_proximity_of_pos_commune          = false;

size_t buildCommandsPromptsPosCommune(ST_COORD_POSITION *i__st_position, boolean *o__flg_synth_cap, byte **o__command, uint16_t **o__durations, size_t *o__nbr_durations)
{
  char l__buffer[80];
  size_t l__size = 0;

  g__prompts_str = "[";

  // Effacement du passé sur changement de la position séectionnée et à synthétiser
  if (i__st_position->idx != g__idx_of_pos_commune_previous) {
    Serial.print("buildCommandsPromptsPosCommune(): Change position:\n");

    sprintf(l__buffer, "\tIdx [%d] -> [%d]\n", g__idx_of_pos_commune_previous,  i__st_position->idx);
    Serial.print(l__buffer);

    g__idx_of_pos_commune_previous          = -1;
    g__dist_to_pos_commune_rounded_previous = (uint32_t)-1L;
    g__to_proximity_of_pos_commune          = false;
  }

  uint32_t l__dist_rounded = distanceRoundedForThePlot(i__st_position->distance);
  boolean  l__flg_same_dist_to_position =
    (g__dist_to_pos_commune_rounded_previous != (uint32_t)-1 && (g__to_proximity_of_pos_commune == true || l__dist_rounded == g__dist_to_pos_commune_rounded_previous)) ? true : false;

  sprintf(l__buffer, "buildCommandsPromptsPosCommune(): Position (same dist. [%d]):\n", l__flg_same_dist_to_position);
  Serial.print(l__buffer);

  sprintf(l__buffer, "\tProximity [%d]\n\tDist. [%d] -> [%d]\n",
    g__to_proximity_of_pos_commune,
    g__dist_to_pos_commune_rounded_previous, l__dist_rounded);
  Serial.print(l__buffer);

  /* Distance en fonction du rayon d'action de la commune si 'TOWN_WITH_RADIUS'
   * - 0.3 Km = 300 M si 'type_town' != 'TOWN_WITH_RADIUS'
   * - 'radius' si == 'TOWN_WITH_RADIUS'
   */
  float l__radius_in_km = (getPosCommTypeTown(i__st_position->idx) == TOWN_WITH_RADIUS) ? getPosCommRadius(i__st_position->idx) : 0.3;
  uint32_t l__radius_in_meters = (uint32_t)(1000.0 * l__radius_in_km);
  sprintf(l__buffer, "\tType Town [%d] Radius [%.2f] Km -> [%d] M\n", getPosCommTypeTown(i__st_position->idx), getPosCommRadius(i__st_position->idx), l__radius_in_meters);
  Serial.print(l__buffer);
  // Fin: Distance en fonction du rayon d'action de la commune si 'TOWN_WITH_RADIUS'

  if (l__dist_rounded <= l__radius_in_meters) {
    // "Vous êtes à/au ..." ou "Vous êtes toujours à/au ..."
    ENUM_POS_COMM_RULE_2 l__rule_2 = (ENUM_POS_COMM_RULE_2)getPosCommRule2(i__st_position->idx);
    size_t l__rule = (size_t)-1;
    switch (l__rule_2) {
    case RULE_2_AT:
      l__rule = BASE_POS_YOU_ARE_AT;
      if (l__flg_same_dist_to_position == true) {
        l__rule = BASE_POS_YOU_ARE_STILL_AT;
      }
      break;

    case RULE_2_TO:
      l__rule = BASE_POS_YOU_ARE_AT_BIS;
      if (l__flg_same_dist_to_position == true) {
        l__rule = BASE_POS_YOU_ARE_STILL_AT_BIS;
      }
      break;

    default:
      break;
    }
    // Fin: "Vous êtes à/au ..." ou "Vous êtes toujours à/au ..."

    if (l__rule != (size_t)-1) {
      updateCommands(g__prompts_positions, l__rule, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);
    }
    else {
      updateCommandsWithOups(&l__size);
    }

    // ... "commune"
    ENUM_BASE_POS_COMMUNES l__base = (ENUM_BASE_POS_COMMUNES)getPosCommBase(i__st_position->idx);

    if (l__base != -1) {
      // Prompt suivant si le nom de la commune est au pluriel (ie. [les] Mesnuls)
      ENUM_POS_COMM_RULE_1 l__rule_1 = (ENUM_POS_COMM_RULE_1)getPosCommRule1(i__st_position->idx);
      if (l__rule_1 == RULE_1_PLURAL) {
        l__base = (ENUM_BASE_POS_COMMUNES)(l__base + 1);
      }
    
      updateCommands(g__prompts_pos_communes, l__base, &l__size, BASE_LAST_PROMPT_POS_COMMUNES, o__nbr_durations);
    }
    else {
      updateCommandsWithOups(&l__size);
    }
    // Fin: ... "commune"

    g__to_proximity_of_pos_commune = true;    // Sauvegarde du passé

    // Pas de synthèse du cap si trop proche
    *o__flg_synth_cap = false;
  }
  else {
    // "commune selectionnée" ... "est/sont [toujours]à" ...
    ENUM_BASE_POS_COMMUNES l__base = (ENUM_BASE_POS_COMMUNES)getPosCommBase(i__st_position->idx);

    if (l__base != -1) {    
      updateCommands(g__prompts_pos_communes, l__base, &l__size, BASE_LAST_PROMPT_POS_COMMUNES, o__nbr_durations);

      ENUM_POS_COMM_RULE_1 l__rule_1 = (ENUM_POS_COMM_RULE_1)getPosCommRule1(i__st_position->idx);
      switch (l__rule_1) {
      case RULE_1_SINGULAR:
        updateCommands(g__prompts_positions, (l__flg_same_dist_to_position == true) ? BASE_POS_IS_STILL_AT : BASE_POS_IS_AT, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);
        break;

      case RULE_1_PLURAL:
        updateCommands(g__prompts_positions, (l__flg_same_dist_to_position == true) ? BASE_POS_ARE_STILL_AT : BASE_POS_ARE_AT, &l__size, BASE_LAST_PROMPT_POSITIONS, o__nbr_durations);
        break;

      default:
        updateCommandsWithOups(&l__size);
        break;
      }

      // Synthèse de la distance
      synthesisOfDistance(l__dist_rounded, &l__size, o__nbr_durations);

      g__to_proximity_of_pos_commune = false;    // Sauvegarde du passé

      // Synthèse du cap
      *o__flg_synth_cap = true;
    }
    else {
      updateCommandsWithOups(&l__size);
    }
  }

  // Sauvegarde du passé
  g__idx_of_pos_commune_previous          = i__st_position->idx;
  g__dist_to_pos_commune_rounded_previous = l__dist_rounded;

  g__prompts_str += "]";

  traceOfBuildResult(l__size);

  // Return the build result (list of command byte and size)
  *o__command = g__commands;
  *o__durations = g__durations;

  return l__size;
}
// End: List of definitions and methods for positions of the communes
