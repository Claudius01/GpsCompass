/// $Id: GpsCompass.ino,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

/*
   Boussole GPS
*/

/* Evolutions:
    - 1.0.0 - Recopie a partir de 'GpsPilot_2.4.0' avec changement 'GpsPilot' -> 'GpsCompass'
              Buts:
              - Abandon de l'UNO qui realise le decodage des trames NMEA et l'enregistrement/lecture de traces GPS
              - Interfacage d'un module GPS de chez Microstack L80 et du GP 735T de chez ADH-tech
              - Gestion d'une SDCard pour l'enregistrement/lecture de traces GPS
                => Pourra servir egalement aux traces de fonctionnement ;-)

    - 1.0.1 - Accueil de 'SerialNMEA' inspire + remplacement de 'SerialTLV'
    - 1.0.3 - Extraction des informations des trames NMEA
              => Code inspire de 'UART-4800-NoDev' de l'Arduino UNO recopie partiellement dans 'SerialNMEA'
    - 1.0.4 - Generation de la trame TLV et injection de celle-ci pour rejoindre le fonctionnement avec l'UNO ;-)
    - 1.0.5 - Extraction de la trame TLV et nettoyage code mort comme le traitement des trames emises par l'UNO
              correspondant au fichier 'GpsPilot.txt' + suppression de traces (traitements des trames NMEA => TLV)
              => Abandon de l'attribut 'sub_state3' et des define associes presentant la progression des
                 etapes de traitements de la connexion a l'UNO et de la reception du fichier 'GpsPilot.txt' 
            - Ajout du nombre de satellites dans la trame TLV (extrait de la trame 'GPGSV' @ 3rd champ)
              => ie. "$GPGSV,1,1,00*79" (aucun satellite)
                     "$GPGSV,1,1,04,03,68,059,36,04,65,164,34,17,45,247,,19,43,284,29*76" (4 satellites)
              Remarque: Afin de ne pas faire evoluer la chaine de traitement sous Java, le type 'a' est utilise
                        (qui representait l'Id du 1st satellite "vu") et represente maintenant le nbr de satellites
                        "vus" des lors que les infos GPS sont valides ('A' dans le 2nd champ de 'GPRMC')
    - 1.0.6 - Bug20220723-Famine-Demarrage: Analyse du probleme de famine au demarrage (debloque par le forcage diffusion par 'tx_play 1413e8')
              => Abandon du test de comparaison des Ids pour la diffusion successive des prompts a l'initialisation (cf. 'GpsPilot')
                 => La trace "End of #%d - 0x%04x prompt played and expected (idx #%d)" peut ne pas etre significative ;-)
              => Difference de comportement entre 2 KT403A ?!..
                 => A surveiller sur Lea 'GpsPilot' ;-)
                    => A l'occasion, mettre le KT403A du 'GpsCompass' sur Lea 'GpsPilot' et confirmer/infirmer le comportement...
            - Suite du nettoyage du code mort...
              => Suppression des 5 timers 'TIMER_READY_FOR_INFOS', 'TIMER_DETECT_MODULE_GPS', 'TIMER_DETECT_MODULE_GPS_END',
                 'TIMER_ERROR_PLOT_INFOS' et 'TIMER_MISSING_USB_END'

    - 1.1.0 - Preparation accueil SDCard (utilise sur GPIO_18 (SPI_SCK), GPIO_19 (SPI_MISO), GPIO_23 (SPI_MOSI) et GPIO_05 (SPI_SS))
              - Cf. https://microcontrollerslab.com/esp32-uart-communication-pins-example/ pour les affectations Rx/Tx des UART
              => Chgt Led Yellow           (GPIO_17) -> GPIO_0
              => Chgt Led Green            (GPIO_16) -> GPIO_33
              => Chgt SerialMP3Player RXD2 (GPIO_5)  -> GPIO_16
              => Chgt SerialMP3Player TXD2 (GPIO_18) -> GPIO_17
              => Chgt SerialNMEA RXD1      (GPIO_23) -> GPIO_4
              => Chgt SerialNMEA TXD1      (GPIO_22) -> GPIO_2 (Not Used)

    - 1.2.0 - Accueil du projet 'https://github.com/espressif/arduino-esp32/tree/master/libraries/SD'
    - 1.2.1 - 1st test with 'listdir()' (levels = 0), 'readFile()' and 'appendFile()' methods ;-))
              => Warning: 'file.path()' in 'listdir()' method  not supported
                          => Pas de recursion sur les sous-repertoires

    - 1.2.2 - Accueil de la classe 'SDCard' pour la gestion de la caret memoire
            - Support des methodes pour le test:
              - init:                         Initialisation
              - end:                          Terminaison (permet une reprise apres les erreurs de la SD Card)
              - printInfos:                   Informations (type et taille)
              - listDir <dir>:                Liste d'un repertoire donne (ie. /, /PILOT, etc.)
              - readFile <file>:              Lecture et impression d'un fichier donne (ie. /PILOT/GpsPilot.txt)
              - appendFile <file> <patterns>: Concatenation d'un fichier donne avec des patterns constituant une ligne (ie. /LOG/log.txt ajout d'une ligne)

            - Accueil de la gestion de la Led 'LED_SDCARD' presentant les acces a la SD Card
              => Allumage de l'entree dans une methode d'acces a la SD Card avec armement du timer 'TIMER_SDCARD_ACCES'
                 pour une presenttaion durant 'DURATION_TIMER_SDCARD_ACCES'
              => Si erreur au retour de la methode d'acces a la SD Card, celle-ci est presentee durant 'DURATION_TIMER_SDCARD_ERROR'
                 associee au timer 'TIMER_SDCARD_ERROR'
                 => Priorite a la presentation de l'erreur SD Card
                    => Inhibition de la presentation des autres erreurs

    - 1.2.3 - Ajout de la cause de 'reboot' pour une eventuelle discrimination (+ test de 'esp_restart' method ;-)

    - 1.2.4 - Concatenation des trames TLV dans '/FRAME/GpsFrames.txt' a l'image de l'Arduino UNO avec l'entete PROMPT,
              l'apparition de l'etat 'GESTION_STATE_NOT_CONNECTED' ("#Not connected") et nouvelles infos ("New infos...")
            - L'arborescence retenue est:
              * /FRAME - Fichier 'GpsFrames.txt'
              * /GPX   - Fichier 'GpsPilot.gpx'
              * /LOG   - Fichiers logs
              * /PILOT - Fichier 'GpsPilot.txt' lu et analyse en priorite @ au fichier 'GpsPilot.gpx'
              * /STAT  - Fichiers statistiques

    - 1.2.5 - Add sub command ESP32 'disable_timer' and 'enable_timer'
              => Ok @ Led flash ;-)
              => Utilisation en entree/sortie des methodes startActivity()/stopActivity()
                 => Toujours le probleme d'instabilite dans la gestion de la SD Card ?!..

    - 1.3.0 - Abandon du projet 'https://github.com/espressif/arduino-esp32/tree/master/libraries/SD'
              au profit de '.../Arduino15/packages/esp32/hardware/esp32/1.0.4/libraries/SD' constitue des 5 fichiers:
                -rwx------+ 1 Sara RUDEL None  2423  2 oct.   2019 SD.cpp
                -rwx------+ 1 Sara RUDEL None  1241  2 oct.   2019 SD.h
                -rwx------+ 1 Sara RUDEL None   799  2 oct.   2019 sd_defines.h
                -rwx------+ 1 Sara RUDEL None 20277  2 oct.   2019 sd_diskio.cpp
                -rwx------+ 1 Sara RUDEL None  1085  2 oct.   2019 sd_diskio.h
                -rwx------+ 1 Sara RUDEL None  4938  2 oct.   2019 sd_diskio_crc.c

              avec les differences indiquees dans 'README.txt'
              => Recopie locale du projet pour ajout de traces, de compteurs d'erreurs , etc. ;-)

            - Changement 'STATE_LED_SDCARD' (reserve) en 'STATE_LED_YELLOW'
            - Non activite sur la Led YELLOW a l'armement du timer 'TIMER_ACTIVITY_EXEC_SYNTH'
              => La Led YELLOW indique maintenant l'activite sur la SD Card (1") et 'FIFO/Tx Play' (100 mS)
            - L'etat 'STATE_LED_RED' est toujours dedie aux erreurs de gestion de la SD Card

    - 1.3.3 - Add 'getFileLine(const char *i__file)' + Extraction du type 'I' suivi de DDMMYY dans la 1st trame GPS valide trouvee
              => Trame GPS commencant par "AA" + test de la checksum au moyen de 'SerialNMEA::calculChecksum()'

    - 1.3.4 - TODO: Implementation de 'SDCard::preparing()'...
            - TODO: Prise en compte du bouton rotatif +/- associe a des prompts (1-male, 2-male, ..., etc.)

    - 1.3.6 - Accueil nouvelle symbologie @ GPS
              => Reprise du code de 'GpsPilot' @ gestion des 1, 2 et 3 flash en mode connecte
                 => Application en mode non connecte pour presenter la progression des receptions ou non des trames UART GPS ;-)
                    => 1 flash -> Pas de reception de trames UART GPS durant xxx secondes
                    => 2 flash -> Reception ou non des trames '$GPGSV' avec nombre de satelittes egal a 0
                    => 3 flash -> Reception des trames '$GPGSV' avec nombre de satelittes > 0 et < 4 (pas de geolocalisation)
                       => Si nombre de satelittes >= 4 -> la geolocalisation est effective -> Passage en mode connecte ;-)
                 => Application en mode connecte pour presenter les 3 etats suivants:
                    => 1 flash -> Parcours dans le sens normal sur le trace enregistre dans la carte SD
                    => 2 flash -> Parcours dans le sens inverse sur le trace enregistre dans la carte SD
                    => 3 flash -> Enregistrement du trace suivi dans le cas ou aucune trace n'a ete lu de la SD Card ou
                                  qu'une action a ete faite dans ce sens

    - 1.3.7 - Fix: 'duration_wait_1st_cnx' est trouve anormalement a 0 (non systematique ;-)
              => Cf. 'Statistics.cpp' file
            - Ajout gestion bouton d'inhibition des operations sur la SDCard a l'image de l'UNO sur la cle USB

    - 1.3.8 - Acquisition par interruption sur front montant de la sortie "Pulse" du module GPS Microstack L80
              => Maj du compteur de statistiques 'general.duration_pulse' representant le temps en secondes comptabilise
                 => Les pulses sont generees des lors que le temps de la trame 'GPRMC' est fourni avec xxx.000 Sec ;-)
              => TODO: Permet d'avoir un cadencement a la seconde.000 Sec pres ;-)
                       => Non utilise

    - 1.3.9 - Ajout test de la longueur du fichier NAME_OF_FILE_GPS_FRAMES ecrite @ trame GPS
              => Permet de detecter une "non ecriture" suite a un probleme de SDCard non "vu"
                 apres une operation d'ecriture

    - 1.3.10 - Suite a l'acueil de l'amplificateur audio HW-104 pour une diffusion sur haut-parleurs 4 ou 8 Ohms:
               => Accueil de la classe "Menus"
               => Implementation du reglage du volume
                  => Test avec 'tx_other 06000e' (niveau 0x0e = 14 @ [0x00, 0x01, 0x1F] ([0, 1, ..., 31])
                     => TODO: Diffusion de "Bonjour le monde" apres la prise en compte de la commande ?!..

    - 1.4.2x - Preparation utilisation du module SerialMP3Player 'WT2003S'
              => Ok pour l'emission des commandes et les detections de debut/fin de diffusion sous It ;-)
              => Ko avec detection intempestive du changement d'état de 'PIN_WT2003S_BUSY' en l'absence de toute demande de diffusion ;-(
                 => TBC: Compteurs incoherents ('Play In Progress' @ 'Play Start')
                 => WT2003S:
                         Play Start        [18]     Play Start        [29]
                         Play End          [17]     Play End          [28]
                         Play In Progress  [18]     Play In Progress  [13]

             - Manque le numero du jour dans la semaine comme:
                     "8 prompts [03/0700_nous_sommes_le.mp3 03/0011_11.mp3 03/0304_avril.mp3 03/2023_2023.mp3
                                 03/0702_il_est_bientot.mp3 03/0518_18_heures.mp3
                                 03/0202_mardi.mp3 => Manque "deuxieme" ?!..
                                 03/0403_heure_d_ete.mp3] synthetizedsynt
                     => Peu de temps apres, synthese correcte comme:
                     " 11 prompts [03/0700_nous_sommes_le.mp3 03/0011_11.mp3 03/0304_avril.mp3 03/2023_2023.mp3
                                   03/0701_il_est.mp3 03/0518_18_heures.mp3 03/2049_passe.mp3
                                   03/0102_deuxieme.mp3 03/0202_mardi.mp3 03/0402_du_mois.mp3
                                   03/0403_heure_d_ete.mp3] synthetized
                     => Pb pour les 2 minutes qui precedent une heure pleine (HH:58:00 et HH:59:00 => a verifier)
                        => Pourtant la trace "Epoch UNIX GMT [1681549080] [2023/04/15 10:58:00 (#3/#5 Samedi) (Heure d'été)]"
                           est correcte ?!..

                => Fix (deplacement de 'l__flg_synth_num_day = false' ;-)

              - Ajout parametre optionnel dans 'SerialMP3Player::update()' pour empiler des evenements comme
                le "Play Start", "Play End", etc. permettant ainsi leur traitement comme des reponses emises
                par le module Serial MP3 Player

              - Determination a l'initialisation de l'utilisation du KT403A ou WT2003S
                => Permet de faire cohabiter les 2 logiques de traitement sans directive de compilation
                   => Les evenements et donnees en transmission dans la TX/Play sont translates dans le cas d'un WT2003S
                   => Les evenements et donnees en reception dans la Rx sont adaptes suivants le module detecte

              - Prise en compte des trames GPRMC 'V' mais correctement constituees comme 'GPRMC,123407.086,V,,,,,0.00,0.00,240423,,,N*43'
                => Permet de synthetiser la date et heure
                   => Remarque: les mS ne sont pas egales a 0 ;-)

            - Accueil de 'PIN_4800_9600_BAUDS' pour la selection de la vitesse UART du module GPS
              => 0: 4800 bauds (mise a la masse) et 1: 9600 bauds (en l'air grace a la pullup)
                 => Passage de la vitesse dans le constructeur 'SerialNMEA::SerialNMEA(int i__speed_uart)' 

            - Translation des commandes de diffusion pour le module SerialMP3Player 'WT2003S' si en service

    - 1.4.3x - 1st version avec le module SerialMP3Player 'WT2003S' versus 'KT403A'
               => TODO: - Sequence d'attente a l'initialisation a ameliorer
                       - Expiration des timers de famine "Courte famine sur FIFO/Tx Play" et "Longue famine sur FIFO/Tx Play"
                       - Compteur de reponses non attendues != 0 (ie. Err: Resp. Not Expected   [5])

             - Lecture du fichier trace '/PILOT/GpsPilot.txt' et de tous les autres /PILOT/GpsPilot*.txt
               => Preparation du suivi de trace aux alentours d'une trace deja effectuee ;-)

             - Reintegration de la lecture d'une trace pre-enregistree (/PILOT/GpsPilot.txt ou /PILOT/GpsPilot*.txt) lue
               a partir de la SDCard...
               => Accueil de la classe 'GpsPilot' definie dans les 2 fichiers 'FileGpsPilot.cpp' / 'FileGpsPilot.h'
                  => Cf. 'FileGpsPilot->getFileLines()' method

             - Definitions des villes pour la synthese "aux alentours..." (liaison supportee comme "aux alentours d'Allonnes" ou "aux alentours des Brevieres")
*/

#include <sstream>

#ifdef USE_SIMULATION
#include "ArduinoTypes.h"
#include "SerialPrint.h"
#endif

#include "Misc.h"

#ifndef USE_SIMULATION
#include "RotaryEncoder.h"
#endif

#include "SerialNMEA.h"
#include "SerialMP3Player.h"
#include "Timers.h"
#include "PromptsSynthesis.h"
#include "Pilot.h"
#include "Statistics.h"
#include "Errors.h"
#include "Plots.h"
#include "Menus.h"

#include "GpsCompass.h"

#define USE_TEST_DIRECT_EEPROM         0

#ifndef USE_SIMULATION
#if USE_TEST_DIRECT_EEPROM
#include "EEPROMClass.h"
#else
#include "GestionEEPROM.h"
#endif
#endif

#include "SDCard.h"
#include "WT2003S.h"

#include "FileGpsPilot.h"

#define COUNTER_FOR_10MS    20

#define PIN_BUTTON          32      // @KiCad: Definition of 'Button' pin for SDCard inhibition operations
#define PIN_GPS_PULSE       35      // @KiCad: Definition of pin 'Pulse' from GPS module
#define PIN_WT2003S_BUSY    34      // @KiCad: Definition of pin 'Busy' from WT2003S module

#define PIN_4800_9600_BAUDS 12      // @KiCad: Selection vitesse Module GPS (0: 4800 bauds 1: 9600 bauds)

#if USE_FORCE_CURRENT_COORD
boolean                     g__flg_force_coord_current = false;
COORD                       g__force_coord_current;
#endif

#if USE_FORCE_TIME
boolean                     g__flg_force_time_hour = false;
boolean                     g__flg_force_time_5mn  = false;
boolean                     g__flg_force_time_10mn = false;
#endif

// Simulation de déplacements
boolean                     g__simu_move_flg = false;   // Activation du mode simulation de déplacements

byte                        g__state_leds = STATE_NO_LEDS;

volatile int                g__duration_no_blocked = -1;
volatile boolean            g__flg_duration_no_blocked = false;

volatile unsigned long      g__counter_500uS = 0L;    // Compteur toutes les 500uS
volatile uint16_t           g__counter = 0;           // Compteur pour interruption

volatile byte               g__counter_for_10ms     = COUNTER_FOR_10MS;     // Valeur pivot pour la comptabilisation des 10 mS (valeur réajustée @ temps GPS)
volatile byte               g__counter_for_10ms_pre = 0;                    // Valeur pivot précédente (forçage recalibration)

volatile byte               g__counter_100mS = 0;     // Compteur pour la comptabilisation des 100 mS

volatile boolean            g__flg_1Sec  = false;     // Détection à 1 Sec
volatile boolean            g__flg_100mS = false;     // Détection à 100 mS
volatile boolean            g__flg_10mS  = false;     // Détection à 10 mS

byte                        g__cpt_flash = (byte) - 1;

// Definitions for gestion
ST_GESTION                  g__gestion;

// Definitions for timer
#ifndef USE_SIMULATION
hw_timer_t                  *g__timer = NULL;
portMUX_TYPE                g__timerMux = portMUX_INITIALIZER_UNLOCKED;
#endif

// Definitions of the all classes
#ifndef USE_SIMULATION
RotaryEncoder               *g__rotary_encoder = NULL;
SDCard                      *g__sdcard = NULL;
#endif

SerialNMEA                  *g__serial_nmea = NULL;
SerialMP3Player             *g__serial_mp3_player = NULL;
Pilot                       *g__pilot = NULL;
Timers                      *g__timers = NULL;
Statistics                  *g__stats = NULL;
Errors                      *g__errors = NULL;
Plots                       *g__plots = NULL;
Menus                       *g__menus = NULL;
FileGpsPilot                *g__file_gpspilot = NULL;

#ifndef USE_SIMULATION
GestionEEPROM               *g__gest_eeprom = NULL;

#if USE_TEST_DIRECT_EEPROM
EEPROMClass                 *g__eeprom = NULL;
#endif
#endif

WT2003S                     *g__wt2003s = NULL;

#if USE_INCOMING_CMD
size_t                      g__count = 0;
char                        g__incoming_buff[132 + 1];   // +1 for '\0' terminal
boolean                     g__flg_wait_command = true;
boolean                     g__synth_prompts_iter = false;
#endif

ST_DATE_AND_TIME            g__dateAndTime;

/* Coordonnées de la position de départ ;-)
 * => TODO: Valeurs permettant de synthétiser "Dist [50m] au NNO"
 *          => Valeurs exactes: TODO @ EditGPX
 *          => Cf. trace GPS https://www.visugpx.com/pdePKdP6CU - Environs d'Elancourt @ trace 'Elancourt (2020-12-17)'
 *             => Waypoint "Point de départ" (lat="48.78494" lon="1.95851" ele="123.0")
 */
static const COORD          g__coord_starting_position =
{
  0L,                   // No duration
  48.784939,            // Latitude           (TODO Home @ EditGPX: 48.784491 degrees)
  1.958505,             // Longitude          (TODO Home @ EditGPX: 1.958672 degrees)
  125.0,                // Elevation          (TODO Home @ EditGPX: 123.0 meters) 
  0.0                   // Non significatif
};

#define DURATION_DEBOUNCING 2
boolean                     g__flg_get_button = false;
byte                        g__counter_button = DURATION_DEBOUNCING;        // xxx mS before get button
boolean                     g__flg_inh_sdcard_ope = false;

void delayNoBlocked(int i__duration)
{
  char l__buffer[80];

  sprintf(l__buffer, "delayNoBlocked(): Entering with %d mS...\n", i__duration);
  Serial.print(l__buffer);

  // Atomic init duration
#ifndef USE_SIMULATION
  noInterrupts();
#endif

  g__flg_duration_no_blocked = false;
  g__duration_no_blocked = i__duration;

#ifndef USE_SIMULATION
  interrupts();
#endif

  // Loop while duration not expired
  while (true) {
    // 'g__flg_duration_no_blocked' is coded on 8 bits (atomic)
    if (g__flg_duration_no_blocked == true) {
      g__flg_duration_no_blocked = false;       // Reset flag for next call
      break;
    }
  }

  sprintf(l__buffer, "delayNoBlocked(): Leave\n");
  Serial.print(l__buffer);
}

#ifndef USE_SIMULATION
// Methode appelee sous It (cf. 'IRAM_ATTR onTimer500uS()')
void treatmentAll_500uS(byte i__counter)
{
  g__counter_500uS++;

  if (i__counter & 1) {
    // 1 mS
    // Rotary encoder update
    if (g__rotary_encoder != NULL) {
      g__rotary_encoder->update();
    }
    // End: Rotary encoder update

    // Update duration for 'delayNoBlocked()' method
    if (g__duration_no_blocked >= 0) {
      g__duration_no_blocked--;
      if (g__duration_no_blocked < 0) {
        /*  Set 'g__flg_duration_no_blocked'
         *  => Reset by delayNoBlocked() method
         */
        g__flg_duration_no_blocked = true;
      }
    }
    // End: Update duration for 'delayNoBlocked()' method
  }
  else {
    // 500 uS
    // Serial MP3 Player update
    if (g__serial_mp3_player != NULL) {
      g__serial_mp3_player->update();
    }
    // End: Serial MP3 Player update

    // Serial NMEA update (9600 bauds)
    if (g__serial_nmea != NULL) {
      g__serial_nmea->update();
    }
    // End: Serial NMEA update (9600 bauds)
  }
}

/* ISR routine with the code in IRAM
   - Called all the 500 uS for Leds gestion, Rotary Encoder, Serial NMEA (9600 bauds), Serial MP3 Player (9600 bauds), etc.
*/
void IRAM_ATTR onTimer500uS()
{
  portENTER_CRITICAL_ISR(&g__timerMux);

  if ((g__counter % g__counter_for_10ms) == 0) {
    // 20 * 500 uS = 10 mS
    g__flg_10mS = true;
  }

  if (g__counter++ >= (10 * g__counter_for_10ms)) {
    // 200 * 500 uS = 100 mS
    g__counter = 0;
    g__flg_100mS = true;
  }

  // Passage de 'g__counter' pour discriminer les 500 uS des 1 mS
  treatmentAll_500uS(g__counter);

  portEXIT_CRITICAL_ISR(&g__timerMux);
}

/* ISR routine with the code in IRAM
   - Called on Gps pulse pin __/--
*/
void IRAM_ATTR doIsrRisingInputPulse()
{
  g__stats->setGpsPulse();
}

/* ISR routine with the code in IRAM
   - Called on WT2003S busy pin __/--
*/
void IRAM_ATTR doIsrRisingInputWT2003SBusy()
{
  g__wt2003s->setPlayStart();
  g__wt2003s->incCptPlayStart();
  g__wt2003s->incCptPlayInProgress();

  g__serial_mp3_player->update(WT2003S_CMD_PLAY_START);

  attachInterrupt(PIN_WT2003S_BUSY, doIsrFallingInputWT2003SBusy, FALLING);   // Attach PIN_WT2003S_BUSY --\__ to 'doIsrFallingInputWT2003SBusy' isr method
}

/* ISR routine with the code in IRAM
   - Called on WT2003S busy pin --\__
*/
void IRAM_ATTR doIsrFallingInputWT2003SBusy()
{
  g__wt2003s->setPlayEnd();
  g__wt2003s->incCptPlayEnd();

  g__serial_mp3_player->update(WT2003S_CMD_PLAY_END);

  attachInterrupt(PIN_WT2003S_BUSY, doIsrRisingInputWT2003SBusy, RISING);     // Attach PIN_WT2003S_BUSY   __/-- to 'doIsrRisingInputWT2003SBusy' isr method
}

boolean callback_cmd_tx_play(CMD_OPCODE i__event, uint16_t i__data)
{
  char l__buffer[80];

  sprintf(l__buffer, "\tcallback Tx/PLAY (0x%02x, 0x%04x)\n", i__event, i__data);
  Serial.print(l__buffer);

  g__serial_mp3_player->setAllPromptsPlayed(false);

  return false;
}

boolean callback_cmd_tx_other(CMD_OPCODE i__event, uint16_t i__data)
{
  char l__buffer[80];

  sprintf(l__buffer, "\tcallback Tx/OTHER (0x%02x, 0x%04x)\n", i__event, i__data);
  Serial.print(l__buffer);

  return false;
}

boolean callback_cmd_rx(CMD_OPCODE i__event, uint16_t i__data)
{
  char l__buffer[80];

  sprintf(l__buffer, "\tcallback Rx (0x%02x, 0x%04x)\n", i__event, i__data);
  Serial.print(l__buffer);

  /*  Lancement d'une commande 'QUERY_TRACK_NUMBER' sur le 'ACK_OK' reçu
      => TODO: 'ACK_OK' sur les commandes TX/Other non encore supporté
  */
  if (i__event == STATUS_ACK_OK) {
    /*  TBC: Attente forfaitaire de 250 mS pour un retour correct du #Track number
        N'est utilisé que pour la synchroniqation avec la fin de diffusion de certains prompts, notamment à l'init.
        => Impacte l'exécution "temps réel" puisque même les traitements sous ITs ne sont pas faits ainsi que ceux
           dans la boucle 'loop()' (pas de test de fin de timer, ni de scrutation du buffer UART, etc ;-)
           => A surveiller: Réception des trames GPS à 4800 baud reçues toutes les 5 Sec. (~2 mS / caractère)
    */
    Serial.print("\tSTATUS_ACK_OK: Awaiting 250 mS...\n");
    delayNoBlocked(250);

    if (g__serial_mp3_player->getLastTxOtherOpcode() == OPCODE_NONE) {
      g__serial_mp3_player->writeFifoTx(TX_OTHER, g__command_query_track_number, sizeof(g__command_query_track_number));
      g__serial_mp3_player->setCommandSended(TX_OTHER, true);
    }
  }

  if (i__event == STATUS_FILE_END) {
    /*  Parcours des prompts synchronisés 'normal' et 'advert'
        => Message "Warning" pour s'assurer du bon parcours avec l'index attendu/non attendu ;-)
    */
    size_t l__idx_prompt = IDX_PROMPT_NORMAL;
    for (l__idx_prompt = IDX_PROMPT_NORMAL; l__idx_prompt < IDX_PROMPT_MAX; l__idx_prompt++) {
      uint16_t l__num_prompt_to_play = g__serial_mp3_player->getNumPromptToPlay((ENUM_IDX_PROMPT)l__idx_prompt);

      /* Bug20220723-Famine-Demarrage: Abandon du test de comparaison des Ids pour la diffusion successive des prompts a l'initialisation (cf. 'GpsPilot')
       * => La trace "End of #%d - 0x%04x prompt played and expected (idx #%d)" peut ne pas etre significative ;-)
      */
      uint16_t l__data = g__serial_mp3_player->getIdxOfPromptPlayed((ENUM_IDX_PROMPT)l__idx_prompt);

      if (g__serial_mp3_player->getIdxOfPromptPlayed((ENUM_IDX_PROMPT)l__idx_prompt) != (uint16_t)-1) {
        sprintf(l__buffer, "\tEnd of #%d - 0x%04x prompt played and expected (idx #%d)\n", l__num_prompt_to_play, l__num_prompt_to_play, l__data);
        Serial.print(l__buffer);

        // Séquencement suivant le #num prompt
        byte *l__commands;
        size_t l__size = 0;

        switch (l__num_prompt_to_play) {
          case NUM_PROMPT_HELLO_WORLD:
            sprintf(l__buffer, "\tEnd of 'NUM_PROMPT_HELLO_WORLD\n");
            Serial.print(l__buffer);

            // Enchainement avec 'NUM_PROMPT_JE_SUIS__FEMALE_BIS'
            l__size = buildCommandsPromptsGeneral(BASE_JE_SUIS__FEMALE_BIS, &l__commands);

            // Début de la séquence d'initialisation...
            g__gestion.sub_state = GESTION_SUB_STATE_INIT_IN_PROGRESS;

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          case NUM_PROMPT_JE_SUIS__FEMALE_BIS:
            sprintf(l__buffer, "\tEnd of 'NUM_PROMPT_JE_SUIS__FEMALE_BIS\n");
            Serial.print(l__buffer);

            if (g__gestion.state == GESTION_STATE_CONNECTED) {
              g__serial_mp3_player->clearNumAndIdxPromptToPlay();

              // Insertion de "Etablissement du signal GPS"
              l__size = buildCommandsPromptsGeneral(BASE_GPS, &l__commands);
            }
            else {
              // Enchainement avec 'NUM_PLEASE_WAIT' si 'non connected'
              l__size = buildCommandsPromptsGeneral(BASE_PLEASE_WAIT_GPS, &l__commands);
            }

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          case NUM_GPS:
          case NUM_RECOVERY_GPS:
            sprintf(l__buffer, "\tEnd of '%s'\n",
              (l__num_prompt_to_play == NUM_GPS) ? "NUM_GPS" : "NUM_RECOVERY_GPS");
            Serial.print(l__buffer);

            if (l__num_prompt_to_play == NUM_RECOVERY_GPS) {
              // Update error if only reestablishment
              g__errors->update(ENUM_ERROR_NBR_REETABLISHMENTS);
            }

            // Protection de rediffusion des messages à l'établissement du signal GPS
            g__serial_nmea->isMulipleOfHours();       // Dans le cas des HH:00:SS
            g__serial_nmea->isMulipleOf5Minutes();    // Dans le cas des multiples de "5 minutes"
            g__serial_nmea->isNewMinute();            // Dans le cas des changements de minutes
            // Fin: Protection de rediffusion des messages à l'établissement du signal GPS

            //  1! Diffusion de 'BASE_FLAT_STONE_THROW_INTO_WATER' si time-out 'TIMER_MISSING_USB_END' constaté
            if (g__gestion.flg_err_missing_usb_end == true) {
              l__size = buildCommandsPromptsGeneral(BASE_FLAT_STONE_THROW_INTO_WATER, &l__commands);
              g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);

              g__gestion.flg_err_missing_usb_end = false;
            }

            // Synthèse de la date et heure complète avec écriture dans le FIFO/Tx Play
            synthesisDateTime(SYNTH_DATE_TIME_ALL);

            // Synthèse de "vous êtes à ..." toutes les heures
            synthesisDistanceCapAndElevation(SYNTH_POSITION, g__pilot->getDistMode());

            // Synthèse de "Tracé à suivre sur..." (cf. 'execTreatmentsWithSynthesisPrompts' method)
            g__plots->setMakeSynthProperties(true);

            // Fin de la séquence d'initialisation => Autorisation à exécuter 'execTreatmentsWithSynthesisPrompts()'
            g__gestion.sub_state = l__num_prompt_to_play == NUM_GPS ? GESTION_SUB_STATE_INIT_DONE : GESTION_SUB_STATE_INIT_DONE2;

            // Terminaison de la synchronisation des prompts
            g__serial_mp3_player->clearNumAndIdxPromptToPlay();
            g__serial_mp3_player->clearFctCallback();
            g__serial_mp3_player->clearSynchroPromptsExpected();

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          case NUM_PLEASE_WAIT_GPS:
          case NUM_PLEASE_WAIT_RECOVERY_GPS:
            sprintf(l__buffer, "\tEnd of '%s' (retry: %d)\n",
              (l__num_prompt_to_play == NUM_PLEASE_WAIT_GPS) ? "NUM_PLEASE_WAIT_GPS" : "NUM_PLEASE_WAIT_RECOVERY_GPS",
              g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY));
            Serial.print(l__buffer);

            // Update statistics and errors
            g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY);
            g__stats->resetGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT);
            g__errors->update(ENUM_ERROR_NBR_RETRY);

            // Enchainement avec 'BASE_INTERSTELLAR_THEME' (test 'connected' durant la diffusion)
            l__size = buildCommandsPromptsGeneral(BASE_INTERSTELLAR_THEME, &l__commands);

            if (g__timers->isInUse(TIMER_ADVERT)) {
              g__timers->stop(TIMER_ADVERT);
            }
            g__timers->start(TIMER_ADVERT, DURATION_TIMER_ADVERT, &callback_advert);

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          case NUM_LOSS_GPS:
            sprintf(l__buffer, "\tEnd of 'NUM_LOSS_GPS'\n");
            Serial.print(l__buffer);

            // Diffusion de 'BASE_PLEASE_WAIT_RECOVERY_GPS' tant que 'non connecté' (reprise depuis le prompt 'BASE_PLEASE_WAIT_RECOVERY_GPS')
            l__size = buildCommandsPromptsGeneral(BASE_PLEASE_WAIT_RECOVERY_GPS, &l__commands);

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          case NUM_INTERSTELLAR_THEME:
            /*  TBC: Depuis la version '1.7.12'  (voire avant ;-), absence de fin de 'NUM_INTERSTELLAR_THEME' ?!..
             *       => Si d'aventure, fin de de diffusion => Ok => Comme avant ;-))
             *       => Pas de reprise de la diffusion ;-(
             *          => Si 'nbr_of_retry_advert' (incrémenté par timer) >= 4 => Forçage rediffusion
             */
            sprintf(l__buffer, "\tEnd of 'NUM_INTERSTELLAR_THEME' (retry: %d)\n", g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY));
            Serial.print(l__buffer);

            /*  Diffusion de 'BASE_PLEASE_WAIT_GPS' si jamais été connecté (ou 'nbr_of_establishments' == 0)
                => Sinon, diffusion de 'BASE_PLEASE_WAIT_RECOVERY_GPS'
            */
            l__size = buildCommandsPromptsGeneral(
                        g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS) == 0 ? BASE_PLEASE_WAIT_GPS : BASE_PLEASE_WAIT_RECOVERY_GPS,
                        &l__commands);

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          case NUM_ADVERT_WAIT:
            sprintf(l__buffer, "\tEnd of 'NUM_ADVERT_WAIT' (retry: %d)\n", g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT));
            Serial.print(l__buffer);

            // 'Advert' terminé => Demande de l'index du prompt interrompu à la terminaison de l'advert
            g__serial_mp3_player->setLastTxPlayAdvert(false);
            g__serial_mp3_player->setCommandSended(TX_OTHER, true);
            break;

          case NUM_SORRY_WAIT_AGAIN:
            sprintf(l__buffer, "\tEnd of 'NUM_SORRY_WAIT_AGAIN' (retry: %d)\n", g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY));
            Serial.print(l__buffer);

            // Update statistics and errors
            g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY);
            g__stats->resetGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT);  // Retour à "Veuillez toujours patienter"
            g__errors->update(ENUM_ERROR_NBR_RETRY);

            // Enchainement avec 'BASE_INTERSTELLAR_THEME' (test 'connected' durant la diffusion)
            l__size = buildCommandsPromptsGeneral(BASE_INTERSTELLAR_THEME, &l__commands);

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;
            
          case NUM_OUPS_FEMALE:
            sprintf(l__buffer, "\tEnd of 'NUM_OUPS_FEMALE'\n");
            Serial.print(l__buffer);

            // Prompt terminé
            g__serial_mp3_player->setLastTxPlay(false);
            break;

          default:
            sprintf(l__buffer, "\tEnd of unknown prompt [%d]\n", l__num_prompt_to_play);
            Serial.print(l__buffer);

            g__errors->update(ENUM_ERROR_UNKNOWN_PROMPT);

            /*  'Oups' played si fin de message non attendu ;-)
                => Warning: Bouclage infini possible
                   => TODO: Add 'case NUM_OUPS_FEMALE:' or limit at 1! 'Oups' @ as fin de diffusion
            */
            l__size = buildCommandsPromptsGeneral(BASE_OUPS_FEMALE, &l__commands);
            break;
        }
        // End: Séquencement suivant le #num prompt

        /*  Warning: Pas d'écriture dans la FIDO/Tx Play si 'NUM_GPS' ou 'NUM_RECOVERY_GPS'
         *           car déjà faite du fait de la synthèse de Date/Time complète
         */
        if (l__num_prompt_to_play != NUM_GPS && l__num_prompt_to_play != NUM_RECOVERY_GPS) {
          g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
        }

        g__serial_mp3_player->setCommandSended(TX_PLAY, true);    // Force à jouer tous les prompts "empilés"
      }
      else {
        sprintf(l__buffer, "Warning: #%d: End of #%d - 0x%04x prompt played and not expected (idx #%d)\n",
                l__idx_prompt, l__num_prompt_to_play, l__num_prompt_to_play, i__data);
        Serial.print(l__buffer);
      }
    }
  }

  return false;
}
#endif

/*  Expiration de 'TIMER_WAIT_FIFO_TX_PLAY'
 *  => Aucune action car c'est 'isInUse(TIMER_WAIT_FIFO_TX_PLAY)' qui est utilisée ;-)
*/
void callback_wait_fifo_tx_play()
{
#if 0
  char l__buffer[80];

  sprintf(l__buffer, "callback_wait_fifo_tx_play(): Entering...\n");
  Serial.print(l__buffer);
#endif
}

/*  Expiration de 'TIMER_WAIT_FIFO_TX_OTHER'
 *  => Aucune action car c'est 'isInUse(TIMER_WAIT_FIFO_TX_OTHER)' qui est utilisée ;-)
*/
void callback_wait_fifo_tx_other()
{
#if 0
  char l__buffer[80];

  sprintf(l__buffer, "callback_wait_fifo_tx_other(): Entering...\n");
  Serial.print(l__buffer);
#endif
}

void getBootReasonMessage()
{
    esp_reset_reason_t reset_reason = esp_reset_reason();

    switch (reset_reason) {
    case ESP_RST_UNKNOWN:
        Serial.println("ESP_RST_UNKNOWN: Reset reason can not be determined");
        break;
    case ESP_RST_POWERON:
        Serial.println("ESP_RST_POWERON: Reset due to power-on event");
        break;
    case ESP_RST_EXT:
        Serial.println("ESP_RST_EXT: Reset by external pin (not applicable for ESP32)");
        break;
    case ESP_RST_SW:
        Serial.println("ESP_RST_SW: Software reset via esp_restart");
        break;
    case ESP_RST_PANIC:
        Serial.println("ESP_RST_PANIC: Software reset due to exception/panic");
        break;
    case ESP_RST_INT_WDT:
        Serial.println("ESP_RST_INT_WDT: Reset (software or hardware) due to interrupt watchdog");
        break;
    case ESP_RST_TASK_WDT:
        Serial.println("ESP_RST_TASK_WDT: Reset due to task watchdog");
        break;
    case ESP_RST_WDT:
        Serial.println("ESP_RST_WDT: Reset due to other watchdogs");
        break;
    case ESP_RST_DEEPSLEEP:
        Serial.println("ESP_RST_DEEPSLEEP: Reset after exiting deep sleep mode");
        break;
    case ESP_RST_BROWNOUT:
        Serial.println("ESP_RST_BROWNOUT: Brownout reset (software or hardware)");
        break;
    case ESP_RST_SDIO:
        Serial.println("ESP_RST_SDIO: Reset over SDIO");
        break;
    default:
        Serial.printf("Reset reason unknown [%d]\n", reset_reason);
        break;
    }

    if (reset_reason == ESP_RST_DEEPSLEEP) {
        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

        switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            Serial.println("ESP_SLEEP_WAKEUP_UNDEFINED: In case of deep sleep: reset was not caused by exit from deep sleep");
            break;
        case ESP_SLEEP_WAKEUP_ALL:
            Serial.println("ESP_SLEEP_WAKEUP_ALL: Not a wakeup cause: used to disable all wakeup sources with esp_sleep_disable_wakeup_source");
            break;
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("ESP_SLEEP_WAKEUP_EXT0: Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("ESP_SLEEP_WAKEUP_EXT1: Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("ESP_SLEEP_WAKEUP_TIMER: Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("ESP_SLEEP_WAKEUP_TOUCHPAD: Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("ESP_SLEEP_WAKEUP_ULP: Wakeup caused by ULP program");
            break;
        case ESP_SLEEP_WAKEUP_GPIO:
            Serial.println("ESP_SLEEP_WAKEUP_GPIO: Wakeup caused by GPIO (light sleep only)");
            break;
        case ESP_SLEEP_WAKEUP_UART:
            Serial.println("ESP_SLEEP_WAKEUP_UART: Wakeup caused by UART (light sleep only)");
            break;
        default:
            Serial.printf("ESP_RST_DEEPSLEEP: wakeup reason unknown [%d]\n", wakeup_reason);
            break;
        }
    }

    Serial.println();
}

void disableTimer()
{
  timerAlarmDisable(g__timer);                        // Desabling this timer
}

void enableTimer()
{
  timerAlarmWrite(g__timer, 500, true);               // Generate an interrupt each mS (500 uS) + reload
  timerAlarmEnable(g__timer);                         // Enabling this timer
}

void setup()
{
  char l__buffer[80];

#ifndef USE_SIMULATION
  // Initialize digital pin LEDs as an output.
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_SDCARD, OUTPUT);

  // Initialize PIN_BUTTON
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Initialize PIN_GPS_PULSE
  pinMode(PIN_GPS_PULSE, INPUT_PULLUP);

  // Initialize PIN_WT2003S_BUSY
  pinMode(PIN_WT2003S_BUSY, INPUT_PULLUP);

  // Initialize PIN_4800_9600_BAUDS
  pinMode(PIN_4800_9600_BAUDS, INPUT_PULLUP);

  /* Test the LEDs on before processing
   * - Allumage des 3 Leds
   * - Attente de 1 Sec.
   * - Extinction de la Led Red après 500 mS
   * - Extinction de la Led Yellow après 1 Sec.
   * - Extinction de la Led Green après 1.5 Sec
   */
#if USE_INVERSE_LEDS
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_SDCARD, LOW);
#else
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_SDCARD, HIGH);
#endif

  /* TODO: Passage à 500 mS pour accélérer la séquence et notamment pour être prêt à recevoir
   *       les données de l'UNO qui transmet le contenu du fichier 'GpsPilot.txt'
   *       => Pas constate au 2022/07/17 ;-)
   */
  delay(1000);

#if USE_INVERSE_LEDS
  digitalWrite(LED_RED, HIGH);
  delay(500);
  digitalWrite(LED_YELLOW, HIGH);
  delay(500);
  digitalWrite(LED_GREEN, HIGH);
  delay(500);
  digitalWrite(LED_SDCARD, HIGH);
#else
  digitalWrite(LED_RED, LOW);
  delay(500);
  digitalWrite(LED_YELLOW, LOW);
  delay(500);
  digitalWrite(LED_GREEN, LOW);
  delay(500);
  digitalWrite(LED_SDCARD, LOW);
#endif
  // End: Test the LEDs on before processing

  Serial.begin(115200);
#endif

  Serial.print(PROMPT);
  getBootReasonMessage();

  // Verification of the size of some base types
  sprintf(l__buffer, "sizeof 'boolean'  [%d] bytes\n", sizeof(boolean));
  Serial.print(l__buffer);
  sprintf(l__buffer, "sizeof 'byte'     [%d] bytes\n", sizeof(byte));
  Serial.print(l__buffer);
  sprintf(l__buffer, "sizeof 'int'      [%d] bytes\n", sizeof(int));
  Serial.print(l__buffer);
  sprintf(l__buffer, "sizeof 'long'     [%d] bytes\n", sizeof(long));
  Serial.print(l__buffer);
  sprintf(l__buffer, "sizeof 'uint16_t' [%d] bytes\n", sizeof(uint16_t));
  Serial.print(l__buffer);
  sprintf(l__buffer, "sizeof 'uint32_t' [%d] bytes\n", sizeof(uint32_t));
  Serial.print(l__buffer);
  sprintf(l__buffer, "sizeof 'float'    [%d] bytes\n", sizeof(float));
  Serial.print(l__buffer);

#ifndef USE_SIMULATION
#if USE_TEST_DIRECT_EEPROM
  // Test direct de l'EEPROM
  g__eeprom = new EEPROMClass("eeprom", EEPROM_SIZE);
  g__eeprom->begin(EEPROM_SIZE);

  sprintf(l__buffer, "Size of EEPROM        [%d] bytes\n", g__eeprom->length());  
  Serial.print(l__buffer);
  sprintf(l__buffer, "Useful size of EEPROM [%d] bytes\n", g__eeprom->useful_length());  
  Serial.print(l__buffer);
  // Fin: Test direct de l'EEPROM
#endif
#endif

  memset(&g__gestion, '\0', sizeof(ST_GESTION));

  g__gestion.state_pre  = GESTION_STATE_UNKNOWN;
  g__gestion.state      = GESTION_STATE_UNKNOWN;
  g__gestion.sub_state  = GESTION_SUB_STATE_UNKNOWN;
  g__gestion.sub_state2 = GESTION_SUB_STATE2_UNKNOWN;

  g__state_leds = STATE_NO_LEDS;

  // Instanciation of the all classes
#ifndef USE_SIMULATION
  g__rotary_encoder = new RotaryEncoder();
  g__sdcard = new SDCard();
  g__file_gpspilot = new FileGpsPilot();
#endif

  // Selection vitesse de transmission @ PIN_4800_9600_BAUDS
  int l__speed_uart = (digitalRead(PIN_4800_9600_BAUDS) == 0) ? 4800 : 9600;
  g__serial_nmea = new SerialNMEA(l__speed_uart);

  g__serial_mp3_player = new SerialMP3Player();
  g__pilot = new Pilot();

  g__menus = new Menus();

#ifndef USE_SIMULATION
  g__gest_eeprom = new GestionEEPROM("eeprom", EEPROM_SIZE);

  if (g__serial_mp3_player != NULL) {
    g__serial_mp3_player->setFctCallback(&callback_cmd_rx);
    g__serial_mp3_player->setFctCallback(TX_PLAY, &callback_cmd_tx_play);
    g__serial_mp3_player->setFctCallback(TX_OTHER, &callback_cmd_tx_other);
  }
#endif

  g__timers = new Timers();

  // Warning: La classe 'WT2003S' utilise les timers (test des definitions des opcodes supportes)
  g__wt2003s = new WT2003S();

#ifndef USE_SIMULATION
  // Init timer reload all the 1 mS
  g__timer = timerBegin(0, 80, true);                   // Init. timer #0 with a prescaler of 80 (80 MHz)
  timerAttachInterrupt(g__timer, &onTimer500uS, true);  // Automatically reload upon generating the interrupt
  timerAlarmWrite(g__timer, 500, true);                 // Generate an interrupt each mS (500 uS) + reload
  timerAlarmEnable(g__timer);                           // Enabling this timer

  attachInterrupt(PIN_GPS_PULSE, doIsrRisingInputPulse, RISING);  // Attach PIN_GPS_PULSE __/-- to 'doIsrRisingInputPulse' isr method

  attachInterrupt(PIN_WT2003S_BUSY, doIsrRisingInputWT2003SBusy, RISING);     // Attach PIN_WT2003S_BUSY   __/-- to 'doIsrRisingInputWT2003SBusy' isr method
#endif

  // Statistiques générales (gestion, synthèses, etc.)
  g__stats = new Statistics();

  // Errors
  g__errors = new Errors();

  // Tracés
  g__plots = new Plots();

#ifndef USE_SIMULATION
  // Initialize of the module KT403A ('Reset Player' -> 'Select Source Device' (by automate) -> 'Prompt' diffusion (by automate))
  g__serial_mp3_player->writeFifoTx(TX_OTHER, g__command_reset_player, sizeof(g__command_reset_player));

  delay(100);   // Warning: Attente forfaitaire de 100 mS; sinon pas d'emission sur Tx ?!..

  // Launch the 'Reset Player' command
  g__serial_mp3_player->setCommandSended(TX_OTHER, true);
  // End: Initialize of the module KT403A ('Reset Player' -> 'Select Source Device' (by automate) -> 'Prompt' diffusion (by automate))

  /* Passage a l'etat 'WT2003S_STATE_SENT_RESET_KT403A'
   * => Si le module WT2003S est connecte, celui-ci repondra avec 'WT2003S_RSP_KO' qui l'identifira @ KT403A ;
   */
   g__wt2003s->setAndSaveState(WT2003S_STATE_SENT_RESET_KT403A);
#endif

  /*  Armement timer pour la présentation des erreurs
   *  => Celui-ci sera arrêté durant la phase d'acquisition du tracé
   *  => et redémarré à la fin
   */
  g__timers->start(TIMER_FOR_ERROR_1, DURATION_TIMER_FOR_ERROR_1, &callback_activity_error_1);

  // Init 'Starting position' and distance + cap from this position
  g__pilot->initCoordStartingPosition(&g__coord_starting_position);
  g__pilot->setDistMode(SYNTH_DIST_FROM_STARTING_POS);

  g__flg_force_coord_current = false;
  g__pilot->initCoordToNoMean(&g__force_coord_current);

#if 0
  // TEST 'nan'
  COORD l__coord;
  g__pilot->initCoordToNoMean(&l__coord);
  
  uint32_t l__dist = g__pilot->getDistanceTo(&l__coord);
  float    l__cap  = g__pilot->getCapTo(&l__coord);

  char l__buffer[32];
  sprintf(l__buffer, "l__dist [%d] l__cap [", l__dist);
  Serial.print(l__buffer);
  Serial.print(l__cap, 1);
  sprintf(l__buffer, "]\n");
  Serial.print(l__buffer);

  // Test d'un 'nan'
  if (g__pilot->getLat(COORD_CURRENT) != g__pilot->getLat(COORD_CURRENT)) {
    sprintf(l__buffer, "Lat. current 'NaN' [");
    Serial.print(l__buffer);
    Serial.print(g__pilot->getLat(COORD_CURRENT), 1);
    sprintf(l__buffer, "]\n");
    Serial.print(l__buffer);  
  }
  else {
    Serial.print("Lat. current isn't 'NaN'\n");
  }
  // End: TEST 'nan'
#endif

  //  Initialisation de la SDCard avec 5 "retry" si erreur
#ifndef USE_SIMULATION
  Serial.println("Initialization of SDCard...");
  g__sdcard->init(5);
#endif

  // Force the 'g__gestion' stats in simulation
#ifdef USE_SIMULATION
  g__gestion.state      = GESTION_STATE_CONNECTED;
  g__gestion.sub_state  = GESTION_SUB_STATE_INIT_DONE;
#endif
}

// Delegation to 'g__errors->callback_activity_error_1()
void callback_activity_error_1()
{
  g__errors->activity_error_1();
}

// Delegation to 'g__errors->callback_activity_error_2()
void callback_activity_error_2()
{
  g__errors->activity_error_2();
}

// Delegation to 'g__errors->callback_activity_error_3()
void callback_activity_error_3()
{
  g__errors->activity_error_3();
}

/* Treatment called all 10 mS

*/
void treatmentAll_10mS()
{
  g__flg_10mS = false;

  // Timers update all the 10 mS
  if (g__timers != NULL) {
    g__timers->update();
  }
  // End: Timers update all the 10 mS

  /*  Calcul d'une ligne dans la table des "adjust plots"
   *  => Permet de répartir l'ensemble du traitement relativement lourd
   *     durant les 5 secondes qui séparent chaque nouvelle acquisition ;-)
   *     => 5 Sec / 10 mS = 500 lignes possibles (une centaine dans la pratique)
   */
  if (g__plots->calculOfAdjustPlotLine() == true) {
    // All treatment in 'calculOfAdjustPlotLine()' method...
  }
  // Fin: Calcul d'une ligne dans la table des "adjust plots"

  // Calcul de la position 'enregistrée'/'mémorisée' la plus proche de la position courante
  if (g__plots->getNbrPositions() > 0) {
    g__plots->calculOfPositions();
    // All treatment in 'calculOfPositions()' method...
  }
  // Fin: Calcul de la position 'enregistrée'/'mémorisée' la plus proche de la position courante

  // Calcul de la position de la commune la plus proche de la position courante
  if (g__plots->getNbrPosCommunes() > 0) {
    g__plots->calculOfPosCommunes();
    // All treatment in 'calculOfPosCommunes()' method...
  }
  // Fin: Calcul de la position de la commune la plus proche de la position courante
}

/* Treatment called all 100 mS

*/
void treatmentAll_100mS()
{
  g__flg_100mS = false;

  // Increment for detection of 1 Sec
  if (g__counter_100mS++ >= 10) {
    g__flg_1Sec = true;
  }

  if (g__gestion.flg_detect_module_gps == false) {
    // Timer réarmés automatiquement
    g__cpt_flash++;

    // LED_GREEN: Flash de 100 mS __/--\____ toutes les secondes si NOT_CONNECTED (sinon --\__/---)
    if (g__gestion.state == GESTION_STATE_CONNECTED) {
      // 1! flash dans les cas
      if (g__cpt_flash == 0) {
        g__state_leds &= ~STATE_LED_GREEN;    // g__cpt_flash = 0 (--\__)
      }
      else {
        g__state_leds |= STATE_LED_GREEN;     // g__cpt_flash = 1, 2, 3, ..., 10 (__/--)
      }

      // 2 flash si detection du sens inverse sur le trace
      if (g__plots->getDetectSensInverse()) {
        if (g__cpt_flash == 0 || g__cpt_flash == 2) {
          g__state_leds &= ~STATE_LED_GREEN;  // g__cpt_flash = 0, 2 (--\__)
        }
      }

      // 3 flash si enregistrement du trace suivi
      if (g__gestion.flg_save_current_positions == true) {
        if (g__cpt_flash == 0 || g__cpt_flash == 2 || g__cpt_flash == 4) {
          g__state_leds &= ~STATE_LED_GREEN;  // g__cpt_flash = 0, 2, 4 (--\__)
        }        
      }

      // Provision: xxx flash pour d'autres modes
    }
    else {
      // 1 flash si aucune reception du module GPS ou si 0 satellite
      if (g__cpt_flash == 0) {
        g__state_leds |= STATE_LED_GREEN;     // g__cpt_flash = 0 (__/--)
      }
      else {
        g__state_leds &= ~STATE_LED_GREEN;    // g__cpt_flash = 1, 2, 3, ..., 10 (--\__)
      }

      /* 2 flash si reception de 0, 1, 2 ou 3 satellites
       * => Permet de discriminer avec pas de reception de trame 'GPGSV'
       */
      if (g__serial_nmea->getGpsReceptionState() == GPS_RECEPTION_0_SATELLITE
       || g__serial_nmea->getGpsReceptionState() == GPS_RECEPTION_1_3_SATELLITES) {
        if (g__cpt_flash == 0 || g__cpt_flash == 2) {
          g__state_leds |= STATE_LED_GREEN;   // g__cpt_flash = 0, 2 (__/--)
        }
      }

      /* 3 flash si reception de 4 stalellites ou plus
       * => Ne doit pas etre constate car passage en mode "connecte"
       */
      if (g__serial_nmea->getGpsReceptionState() == GPS_RECEPTION_4_MORE_SATELLITES) {
        if (g__cpt_flash == 0 || g__cpt_flash == 2 || g__cpt_flash == 4) {
          g__state_leds |= STATE_LED_GREEN;   // g__cpt_flash = 0, 2, 4 (__/--)
        }
      }
    }

    if (g__cpt_flash >= 10) {
      // End of 1 Sec.
      g__cpt_flash = (byte)-1;  // Car incrémentation avant tests
    }
  }
  // Fin: Gestion du flash LED_GREEN en mode [non] connecte + ajout d'un flash si sens inverse detecte

  // Detection "connecte" <-> "non connecte"
  if (g__gestion.state_pre == GESTION_STATE_CONNECTED && g__gestion.state == GESTION_STATE_NOT_CONNECTED) {
      Serial.print("Not connected...\n");

      g__sdcard->appendGpsFrame("#Not connected...\n");   
  }

  // Fix correct 'nbr_of_establishments' and 'duration_wait_1st_cnx' ;-)
  if (g__gestion.state_pre != GESTION_STATE_CONNECTED && g__gestion.state == GESTION_STATE_CONNECTED) {
      Serial.print("Connected\n");

      // Update statistics
      g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS);

      // TODO: Forcage en attendant la lecture de la trace... 
      g__gestion.sub_state = GESTION_SUB_STATE_INIT_DONE;
  }
  // Fin: Detection "connecte" <-> "non connecte"

  g__gestion.state_pre = g__gestion.state;

  // Gestion of button
  if (g__flg_get_button == false) {
    if (digitalRead(PIN_BUTTON)) {
      if (g__counter_button > 0) {
        g__counter_button--;          // Awaiting for get button (awaiting __/--)
      }
      else {
        g__flg_get_button = true;     // Ready for get button
      }
    }
  }
  else {
    if (digitalRead(PIN_BUTTON) == 0) {
        g__counter_button++;          // Awaiting --\__     
    }
    if (g__counter_button >= DURATION_DEBOUNCING) {
      g__flg_get_button = false;
      g__state_leds ^= STATE_LED_SDCARD;

      g__flg_inh_sdcard_ope = (g__state_leds & STATE_LED_SDCARD) ? true : false;

      if (g__flg_inh_sdcard_ope == true) {
        // Inhibition sdcard operation
        Serial.print("Inhibition sdcard operation\n");

        // Marquage dans la SDCard
        g__sdcard->appendGpsFrame("#Stop recording...\n", true);
      }
    }
  }
  // End: Gestion of button
}

// Definition des methodes 'callback'
void callback_disconnect()
{
  byte *l__commands;
  size_t l__size = 0;

  // Update statistics and errors
  g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_LOSS);
  g__errors->update(ENUM_ERROR_LOSS_GPS);

  g__gestion.state     = GESTION_STATE_NOT_CONNECTED;
  g__gestion.sub_state = GESTION_SUB_STATE_INIT_RETRY;

#ifndef USE_SIMULATION
  g__serial_mp3_player->setFctCallback(&callback_cmd_rx);
  g__serial_mp3_player->setFctCallback(TX_PLAY, &callback_cmd_tx_play);
  g__serial_mp3_player->setFctCallback(TX_OTHER, &callback_cmd_tx_other);
#endif

  // Diffusion: "Perte du signal GPS"
  l__size = buildCommandsPromptsGeneral(BASE_LOSS_GPS, &l__commands);

#ifndef USE_SIMULATION
  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
  g__serial_mp3_player->setCommandSended(TX_PLAY, true);
#endif

  // Arret time-out et activité "Exec Synth"
  g__timers->stop(TIMER_EXEC_SYNTH);
  g__timers->stop(TIMER_ACTIVITY_EXEC_SYNTH);

#ifndef USE_SIMULATION
  // Update statistics:
  // Add duration before detection
  g__stats->addGenDurationNotConnected(DURATION_TIMER_CONNECT / 100);

  // GPS ou of order
  g__stats->setGpsAvailable(false);
#endif
}

void callback_error_plot_infos()
{
  // g__state_leds &= ~STATE_LED_RED;
}

void callback_exec_synth()
{
  char l__buffer[80];

  /*  Force 'sub_state' to 'GESTION_SUB_STATE_INIT_DONE'
   *  => TODO: Explication ?!..
   */
  g__gestion.sub_state = GESTION_SUB_STATE_INIT_DONE;

#ifndef USE_SIMULATION
  g__serial_mp3_player->setFctCallback(&callback_cmd_rx);
  g__serial_mp3_player->setFctCallback(TX_PLAY, &callback_cmd_tx_play);
  g__serial_mp3_player->setFctCallback(TX_OTHER, &callback_cmd_tx_other);
#endif

  sprintf(l__buffer, "callback_exec_synth(): Force to 'GESTION_SUB_STATE_INIT_DONE'\n");
  Serial.print(l__buffer);
}

void callback_activity_exec_synth()
{
  if (g__sdcard->isInUse() == false) {
    g__state_leds &= ~STATE_LED_YELLOW;
  }
}

void callback_activity_fifo_tx_play()
{
  if (g__sdcard->isInUse() == false && g__timers->isInUse(TIMER_SDCARD_ACCES) == false) {
    g__state_leds ^= STATE_LED_YELLOW;
  }
}

void callback_short_famine_fifo_tx_play()
{
  byte *l__commands;

#ifndef USE_SIMULATION     // 'l__size' not used
  size_t l__size = 0;
#endif

  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_SHORT_FAMINE_FIFO_TX_PLAY expired\n");

  //  Diffusion: Prompt pour supprimer la famine
#ifndef USE_SIMULATION
#if USE_FLAT_STONE_THROW_INTO_WATER
  l__size = buildCommandsPromptsGeneral(BASE_FLAT_STONE_THROW_INTO_WATER, &l__commands);
#else
  l__size = buildCommandsPromptsGeneral(BASE_TEST_MESSAGE, &l__commands);
#endif

  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
  g__serial_mp3_player->setCommandSended(TX_PLAY, true);
#else
#if USE_FLAT_STONE_THROW_INTO_WATER     // 'l__size' not used
  buildCommandsPromptsGeneral(BASE_FLAT_STONE_THROW_INTO_WATER, &l__commands);
#else
  buildCommandsPromptsGeneral(BASE_TEST_MESSAGE, &l__commands);
#endif
#endif

  // Comptage erreur
  g__errors->update(ENUM_ERROR_SHORT_FAMINE);
}

void callback_very_long_famine_fifo_tx_play()
{
  // Comptage erreur
  g__errors->update(ENUM_ERROR_VERY_LONG_FAMINE);

  g__errors->setVeryLongFamine(true);
}

void callback_long_famine_fifo_tx_play()
{
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_LONG_FAMINE_FIFO_TX_PLAY expired\n");

  // Comptage erreur
  g__errors->update(ENUM_ERROR_LONG_FAMINE);

#ifndef USE_SIMULATION
  g__serial_mp3_player->purgeFIFOTxPlay();
#endif

  g__timers->start(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY, DURATION_TIMER_VERY_LONG_FAMINE, &callback_very_long_famine_fifo_tx_play);

  byte *l__commands;

#ifndef USE_SIMULATION   // 'l__size' not used
  size_t l__size = buildCommandsPromptsGeneral(BASE_SHOCK_IMPACT_METALLIC, &l__commands);
  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
#else
  buildCommandsPromptsGeneral(BASE_SHOCK_IMPACT_METALLIC, &l__commands);
#endif
}

void callback_advert()
{
  /*  Au 4th retry ('NUM_INTERSTELLAR_THEME' doit toujours être en diffusion ;-)
   *  => remplacement de "Veuillez toujours patienter" par "Desolé, veuillez encore patienter"
    */
#ifndef USE_SIMULATION
  g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT);
  uint16_t l__counter = g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT);

  byte *l__commands;

  size_t l__size = 0;

  /*  Diffusion: "Veuillez toujours patienter" ("Désolé, veuillez toujours patienter")
   *  dans 'NUM_INTERSTELLAR_THEME' (qui interrompt 'NUM_INTERSTELLAR_THEME')
    */
  l__size = buildCommandsPromptsGeneral((l__counter <= INDEX_FOR_BASCULE) ? BASE_ADVERT_WAIT : BASE_SORRY_WAIT_AGAIN, &l__commands);
  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
  g__serial_mp3_player->setCommandSended(TX_PLAY, true);
#endif

  // Reload time-out 'TIMER_ADVERT'
  g__timers->start(TIMER_ADVERT, DURATION_TIMER_ADVERT, &callback_advert);
}

#ifndef USE_SIMULATION
void callback_timer_button()
{
  char l__buffer[80];
  sprintf(l__buffer, "callback_timer_button(): %d press button\n", g__rotary_encoder->getNbrOfPressButton());
  Serial.print(l__buffer);

  g__rotary_encoder->setPressButtonAction(g__rotary_encoder->getNbrOfPressButton());
  
  g__rotary_encoder->resetNbrOfPressButton();
}

void callback_timer_wait_button()
{
  Serial.printf("Encoder position => End of awaiting of menu %d\n", g__rotary_encoder->getNumMenu());
}

void callback_timer_new_menu()
{
  Serial.printf("Encoder position => Confirm menu %d\n", g__rotary_encoder->getNumMenu());

  /* - TODO: Synthese du libelle du menu
   * - Armement timer pour l'attente de l'appui bouton (Duree multiple de 10 mS)
  */
  g__timers->start(TIMER_WAIT_BUTTON, (g__rotary_encoder->getMenuParamDurationButton() / 10), &callback_timer_wait_button);
}
#endif

void callback_sdcard_retry_init()
{
  // Suite du traitement dans la classe 'SDCard'
  g__sdcard->callback_sdcard_retry_init_more();
}

void callback_end_sdcard_acces() {
  g__state_leds &= ~STATE_LED_YELLOW;
}

void callback_end_sdcard_error() {
  g__state_leds &= ~STATE_LED_RED;
}

void callback_sdcard_init_error()
{
  g__state_leds ^= STATE_LED_RED;

  g__timers->start(TIMER_SDCARD_INIT_ERROR, DURATION_TIMER_SDCARD_INIT_ERROR, &callback_sdcard_init_error);
}

/* Absence reception de trames GPS
 * => Retour a l'etat 'GPS_NO_RECEPTION'
 */
void callback_exec_gps_reception_state()
{
  g__serial_nmea->setGpsReceptionState(GPS_NO_RECEPTION);
}
// End: Definition des methodes 'callback'

void synthesisDateTime(ENUM_SYNTH_DATE_TIME_MODES i__mode)
{
  // Application de l'heure été/hiver
  ST_DATE_AND_TIME l__dateAndTime_presentation;
  memcpy(&l__dateAndTime_presentation, &g__dateAndTime, sizeof(ST_DATE_AND_TIME));
  applySommerWinterHour(&l__dateAndTime_presentation);

  byte      *l__commands = NULL;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

#ifndef USE_SIMULATION
  size_t l__size = buildCommandsPromptsDateTime(&l__dateAndTime_presentation, &l__commands, i__mode, &l__durations, &l__nbr_durations);

  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
  buildCommandsPromptsDateTime(&l__dateAndTime_presentation, &l__commands, i__mode, &l__durations, &l__nbr_durations);
#endif
}

ENUM_SYNTH_DISTANCE_TYPES synthesisDistance(uint32_t i__distance, ENUM_SYNTH_POSITION_MODES i__pos_mode, ENUM_SYNTH_DISTANCE_MODES i__dist_mode)
{
  byte      *l__commands;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

  ENUM_SYNTH_DISTANCE_TYPES l__dist_type = SYNTH_DIST_UNKNOWN_TYPE;

#ifndef USE_SIMULATION
  size_t l__size = buildCommandsPromptsDistance(i__distance, &l__dist_type, &l__commands, i__pos_mode, i__dist_mode, &l__durations, &l__nbr_durations);

  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
  buildCommandsPromptsDistance(i__distance, &l__dist_type, &l__commands, i__pos_mode, i__dist_mode, &l__durations, &l__nbr_durations);
#endif

  return l__dist_type;
}

void synthesisCap(float i__cap)
{
  byte      *l__commands;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

#ifndef USE_SIMULATION
  size_t l__size = buildCommandsPromptsCap(i__cap, &l__commands, &l__durations, &l__nbr_durations);

  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
  buildCommandsPromptsCap(i__cap, &l__commands, &l__durations, &l__nbr_durations);
#endif
}

void synthesisCapHours(float i__cap)
{
  byte      *l__commands;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

#ifndef USE_SIMULATION
  size_t l__size = buildCommandsPromptsCapHours(i__cap, &l__commands, &l__durations, &l__nbr_durations);

  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
  buildCommandsPromptsCapHours(i__cap, &l__commands, &l__durations, &l__nbr_durations);
#endif
}

void synthesisElevation(float i__ele_current, float i__ele_ref, ENUM_SYNTH_ELE_MODES i__ele_mode)
{
  byte      *l__commands;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

#ifndef USE_SIMULATION
  size_t l__size = buildCommandsPromptsElevation(i__ele_current, i__ele_ref, &l__commands, i__ele_mode, &l__durations, &l__nbr_durations);

  g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
  buildCommandsPromptsElevation(i__ele_current, i__ele_ref, &l__commands, i__ele_mode, &l__durations, &l__nbr_durations);
#endif
}

void synthesisDistanceCapAndElevation(ENUM_SYNTH_POSITION_MODES i__pos_mode, ENUM_SYNTH_DISTANCE_MODES i__dist_mode)
{
  char l__buffer[80];

  // Distance et Cap (si pas 'SYNTH_DIST_VERY_NEAR')
  uint32_t l__distance = 0L;
  
  if (i__dist_mode == SYNTH_DIST_FROM_STARTING_POS) {
    l__distance = g__pilot->getDistanceToStartingPos();
  }
  else if (i__dist_mode == SYNTH_DIST_FROM_POS_SAVE) {
    l__distance = g__pilot->getDistanceToLastPosSave();
  }
  else {
    /* Ne doit jamais arriver
     * => Force return
     */
    sprintf(l__buffer, "Error: synthesisDistanceCapAndElevation(): #0: Wrong dist_mode [%d]\n", i__dist_mode);
    Serial.print(l__buffer);

    return;
  }

  ENUM_SYNTH_DISTANCE_TYPES l__dist_type = synthesisDistance(l__distance, i__pos_mode, i__dist_mode);

  sprintf(l__buffer, "Distance [%d] m (dist_type [%d] pos_mode [%d] dist_mode [%d])\n", l__distance, l__dist_type, i__pos_mode, i__dist_mode);
  Serial.print(l__buffer);

  float l__cap = 0.0;
  float l__ele = 0.0;

  if (i__dist_mode == SYNTH_DIST_FROM_STARTING_POS) {
    l__cap = g__pilot->getCapToStartingPos();
    l__ele = g__pilot->getEleToStartingPos();
  }
  else if (i__dist_mode == SYNTH_DIST_FROM_POS_SAVE) {
    l__cap = g__pilot->getCapToLastPosSave();
    l__ele = g__pilot->getEleToLastPosSave();
  }
  else {
    /* Ne doit jamais arriver
     * => Force return
     */
    sprintf(l__buffer, "Error: synthesisDistanceCapAndElevation(): #1: Wrong dist_mode [%d]\n", i__dist_mode);
    Serial.print(l__buffer);

    return;
  }

  if (l__dist_type == SYNTH_DIST_NEAR || l__dist_type == SYNTH_DIST_DISTANT || l__dist_type == SYNTH_DIST_VERY_DISTANT) {
    sprintf(l__buffer, "Cap to join the position [");
    Serial.print(l__buffer);
    Serial.print(l__cap, 1);
    sprintf(l__buffer, "] degrees\n");
    Serial.print(l__buffer);

    synthesisCap(l__cap);

    /*  Get the current cap for calcul @ 'Starting Position' or 'Last Position Save'
     *  if the speed is greater of 1 KmH
     */
    float l__speedKmH = g__pilot->getSpeedKmH(COORD_CURRENT);
    if (l__speedKmH >= 1.0 ) {
      float l__current_cap = g__pilot->getCap(COORD_CURRENT);
      sprintf(l__buffer, "Current Cap [");
      Serial.print(l__buffer);
      Serial.print(l__current_cap, 1);
      sprintf(l__buffer, "] degrees\n");
      Serial.print(l__buffer);

      // Cap relatif: Diff from current cap to cap position in the range [0:360.0[ degrees
      float l__cap_diff = (l__cap + (360.0 - l__current_cap));
      if (l__cap_diff > 360.0) {
        l__cap_diff -= 360.0;
      }
      sprintf(l__buffer, "Cap to join (relatif) [");
      Serial.print(l__buffer);
      Serial.print(l__cap_diff, 1);
      sprintf(l__buffer, "] degrees\n");
      Serial.print(l__buffer);

      synthesisCapHours(l__cap_diff);
    }
    else {
      sprintf(l__buffer, "No cap to join (relatif) [Speed: ");
      Serial.print(l__buffer);
      Serial.print(l__speedKmH, 1);
      sprintf(l__buffer, "] KmH\n");
      Serial.print(l__buffer);
    }
  }

  /*  Elevation synthesis
   *  - Absolute if |l__current_ele - l__ele| < ELEVATION_ROUND_FLOAT
   *  - Relative signed if |l__current_ele - l__ele| >= ELEVATION_ROUND_FLOAT
   */
  float l__current_ele = g__pilot->getEle(COORD_CURRENT);
  sprintf(l__buffer, "Current Elevation [");
  Serial.print(l__buffer);
  Serial.print(l__current_ele, 1);
  sprintf(l__buffer, "] meters\n");
  Serial.print(l__buffer);

  sprintf(l__buffer, "Elevation to join [");
  Serial.print(l__buffer);
  Serial.print(l__ele, 1);
  sprintf(l__buffer, "] meters\n");
  Serial.print(l__buffer);

  float l__diff_ele = (l__current_ele - l__ele);
  sprintf(l__buffer, "Elevation diff [");
  Serial.print(l__buffer);
  Serial.print(l__diff_ele, 1);
  sprintf(l__buffer, "] meters\n");
  Serial.print(l__buffer);

  if (l__diff_ele < 0.0) {
    l__diff_ele = -l__diff_ele;
  }

  ENUM_SYNTH_ELE_MODES l__ele_mode = (l__diff_ele < ELEVATION_ROUND_FLOAT) ? SYNTH_ELE_ABSOLUTE : SYNTH_ELE_RELATIVE;

  synthesisElevation(l__current_ele, l__ele, l__ele_mode);
}

/* Test si le Cap horaire est égal à "midi" (retourne 'true') si vitesse >= 1 KmH
 * => Si vitesse < 1 KmH; retoune 'false'
 */
boolean isCapHourMidday(float i__cap)
{
  boolean l__flg_rtn = false;   // A priori, pas "midi"

  COORD l__coord_current;
  g__plots->getCoordCurrent(&l__coord_current);

  if (l__coord_current.speedKmH >= 1.0) {
    Serial.print("isCapHourMidday(): Speed [");
    Serial.print(l__coord_current.speedKmH, 1);
    Serial.print("] KmH\n");

    // Cap relatif: Diff from current cap to cap position in the range [0:360.0[ degrees
    float l__cap_diff = (i__cap + (360.0 - l__coord_current.cap));
    if (l__cap_diff > 360.0) {
      l__cap_diff -= 360.0;
    }

    Serial.print("isCapHourMidday(): Cap [");
    Serial.print(l__cap_diff, 1);
    Serial.print("] degrees\n");

    if ((l__cap_diff >= THRESHOLD_CAP_MIDDAY && l__cap_diff <= CAP_MAX)
     || (l__cap_diff >= CAP_MIN && l__cap_diff < THRESHOLD_CAP_13H))
    {
      Serial.print(" => No synthesis of Cap Hour\n");

      l__flg_rtn = true;    // Cap horaire à "midi"
    }
  }

  return l__flg_rtn;
}

void synthesisOfCaps(float i__cap, boolean i__flg_synth_midday = false)
{
  COORD l__coord_current;
  g__plots->getCoordCurrent(&l__coord_current);

  if (l__coord_current.speedKmH < 1.0) {
    // Cap in [North, ..., South, ...]
    Serial.print("synthesisOfCaps(): Speed [");
    Serial.print(l__coord_current.speedKmH, 1);
    Serial.print("] KmH\n");

    synthesisCap(i__cap);     // Synthèse du Cap magnétique
  }
  else {
    // Cap relatif: Diff from current cap to cap position in the range [0:360.0[ degrees
    float l__cap_diff = (i__cap + (360.0 - l__coord_current.cap));
    if (l__cap_diff > 360.0) {
      l__cap_diff -= 360.0;
    }

    Serial.print("synthesisOfCaps(): Cap diff. [");
    Serial.print(l__cap_diff, 1);
    Serial.print("] degrees\n");

    if (i__flg_synth_midday == true) {
      // Synthèse de "à midi" autorisée"
      synthesisCapHours(l__cap_diff);   // Synthèse du Cap horaire
    }
    else {
      if ((l__cap_diff >= THRESHOLD_CAP_MIDDAY && l__cap_diff <= CAP_MAX)
       || (l__cap_diff >= CAP_MIN && l__cap_diff < THRESHOLD_CAP_13H))
      {
        Serial.print(" => No synthesis of '12 H'\n");
      }
      else {
        synthesisCapHours(l__cap_diff);   // Synthèse du Cap horaire
      }
    }
  }
}

/* Synthèse de la distance au tracé avec toutes ses variantes
 * => Par défaut, aucune restriction suivant le placement @ au tracé
 */
boolean synthesisDistanceToPlot(ST_RESULTS *i__results, boolean i__flg_plot_1 = true, boolean i__flg_plot_2 = true, boolean i__flg_plot_3 = true)
{
  char l__buffer[128];
  boolean l__flg_synth_rtn = false;

  Serial.print("synthesisDistanceToPlot():\n");

  g__plots->printResults(l__buffer, i__results);

  byte      *l__commands;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

  if (i__results->type_synth == TYPE_SYNTH_YOU_HAVE_ARRIVED) {
    // Si distance restante < 50 M => Synthèse du [presque]
#ifndef USE_SIMULATION
    size_t l__size = buildCommandsPromptsYouHaveArrived(false, &l__commands, &l__durations, &l__nbr_durations);   // Jamais de [presque] ;-)
    g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
    buildCommandsPromptsYouHaveArrived(false, &l__commands, &l__durations, &l__nbr_durations);   // Jamais de [presque] ;-)
#endif

    l__flg_synth_rtn = true;
  }
  else if (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3) {
    // Calcul du cap horaire avant pour déterminer si synthèse ou non
    if (isCapHourMidday(i__results->cap) == false) {
      // Synthèse du cap (non "à midi") et de la distance ("Bifurquez à ... et continuez sur ... M")
#ifndef USE_SIMULATION
      size_t l__size = buildCommandsPromptsBranchOfNow(&l__commands, &l__durations, &l__nbr_durations);
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
      buildCommandsPromptsBranchOfNow(&l__commands, &l__durations, &l__nbr_durations);
#endif

      synthesisOfCaps(i__results->cap);     // Pas de synthèse de "à midi"

#ifndef USE_SIMULATION
      l__size = buildCommandsPromptsAndContinueOn(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
      buildCommandsPromptsAndContinueOn(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
#endif
    }
    else {
      // Synthèse de la distance uniquement ("Continuez sur ... M")
#ifndef USE_SIMULATION
      size_t l__size = buildCommandsPromptsContinueOn(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);      
#else
      buildCommandsPromptsContinueOn(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
#endif

      // "Continuez sur ... M" instead of "Bifurquez à 12 H et continuez sur ... M"
      sprintf(l__buffer, "\t=> Continuez sur [%d] (no Cap Hour)\n", i__results->distance);
      Serial.print(l__buffer);
    }

    l__flg_synth_rtn = true;
  }
  else {
    /* Passage de 'i__results->type_synth' dans 'buildCommandsPromptsDistToPlots()' pour la synthèse asynchrone
     * et permettre de diffuser "Préparez vous à bifurquer..." dans le cas 'i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2'
     */
    boolean l__flg_more_synth = false;

    size_t l__size = buildCommandsPromptsDistToPlots(i__results, &l__commands, &l__durations, &l__nbr_durations, &l__flg_more_synth);

#ifndef USE_SIMULATION
    if (l__size != 0) {
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
    }
#endif

    if (l__flg_more_synth == true) {
      /*  Test si synthèse d'un prompt faite ('l__size' != 0') et 'l__flg_more_synth == true'
       *  => La synthèse d'une distance/cap doit toujours être précédée d'un prompt ;-)
       */
      if (l__size == 0) {
#ifndef USE_SIMULATION
        Serial.print("synthesisDistanceToPlot(): Error: 'l__size' == 0' and 'l__flg_more_synth == true'\n");
#endif
      }

      // Synthèse de la distance à suivre et du cap suivant le type
      if (i__results->type_synth == TYPE_SYNTH_THE_PLOT_IS) {
        // Synthèse de la distance et du cap ("Le tracé est à ... M à ...")
        synthesisOfCaps(i__results->cap, true);     // Synthèse de "à midi" autorisée

        l__flg_synth_rtn = true;
      }
      else if (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1) {
        // Synthèse de la distance et du cap ("Continuez sur ... M à ...")
#ifndef USE_SIMULATION
        size_t l__size = buildCommandsPromptsContinueOn(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
        g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
        buildCommandsPromptsContinueOn(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
#endif

        synthesisOfCaps(i__results->cap, false);     // Pas de synthèse de "à midi"

        l__flg_synth_rtn = true;
      }
      else if (i__results->type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2) {
        if (isCapHourMidday(i__results->cap) == false) {
          // Synthèse de la distance et du cap (non "à midi") ("Préparez vous à bifurquez à ... M à ...")
#ifndef USE_SIMULATION
          size_t l__size = buildCommandsPromptsPrepareYourselves(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
        
          g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
          buildCommandsPromptsPrepareYourselves(i__results->distance, &l__commands, &l__durations, &l__nbr_durations);
#endif

          synthesisOfCaps(i__results->cap);    // Pas de synthèse de "à midi"

          l__flg_synth_rtn = true;
        }
        else {
          // No trace of prompts "Préparez vous à bifurquez à ... M à 12 H"
          Serial.print("\t=> No synthesis (Cap Hour at midday)\n");          
        }
      }
    }
  }

  return l__flg_synth_rtn;
}

void synthesisPlotsProperties()
{
  byte      *l__commands;
  uint16_t  *l__durations = NULL;
  size_t    l__nbr_durations = 0;

  boolean l__flg_about = false;
  uint32_t l__remaining_distance = g__plots->getRemainingDistance();
  uint32_t l__total_distance     = g__plots->getTotalDistance();

  uint32_t l__distance = (UINT_MAX);

  if (l__remaining_distance > l__total_distance) {
    // Prise en compte de la 'distance totale' si 'distance restante' non significative (à l'init notamment)
    l__distance = l__total_distance;
  }
  else {
    // Prise en compte de la 'distance restante' <= 'distance totale' qui est tjs significative ;-)
    l__distance = l__remaining_distance;

    // Récupération des caractéristiques de la position @ tracé
    ST_RESULTS l__results;
    boolean l__flg_results = g__plots->getResultsSave(&l__results);

    if (l__flg_results == true && l__results.type_synth == TYPE_SYNTH_THE_PLOT_IS) {
      // Ajout du prompt "environ" si à coté du tracé
      l__flg_about = true;
    }
  }

  /* Pas de synthèse de la distance restance si ... arrivée à la fin
   * => '<= DISTANCE_50M' correspond au type de synthèse 'TYPE_SYNTH_YOU_HAVE_ARRIVED' si sur le tracé
  */
  if (l__distance > DISTANCE_50M) {
#ifndef USE_SIMULATION
    size_t l__size = buildCommandsPromptsPlotsProperties(l__distance, l__flg_about, g__plots->getPlotsDirection(), &l__commands, &l__durations, &l__nbr_durations);
    g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
    buildCommandsPromptsPlotsProperties(l__distance, l__flg_about, g__plots->getPlotsDirection(), &l__commands, &l__durations, &l__nbr_durations);
#endif
  }
}

boolean synthesisPosRecordedOrMemorized()
{
  boolean l__flg_rtn = false;

  Serial.print("synthesisPosRecordedOrMemorized(): Entering...\n");

  ST_COORD_POSITION l__st_coord_position;

  if (g__timers->isInUse(TIMER_TYPE_SYNTH_POSITION) == false
   && g__plots->getPositionSelected(&l__st_coord_position) == true
   && (l__st_coord_position.type == TYPE_POSITION_RECORDED || l__st_coord_position.type == TYPE_POSITION_MEMORIZED))
  {
    g__plots->toStringPositions(false, true, false);

    /* Synthèse si distance inférieure à 500 M
     * => A confirmer/infirmer sur le terrain
     */
    if (l__st_coord_position.distance <= DISTANCE_500M) {
      byte      *l__commands = NULL;
      uint16_t  *l__durations = NULL;
      size_t    l__nbr_durations = 0;

      int l__nbr_positions = 0;
      if (l__st_coord_position.type == TYPE_POSITION_RECORDED) {
        l__nbr_positions = g__plots->getNbrPositionsRecorded();
      }
      else if (l__st_coord_position.type == TYPE_POSITION_MEMORIZED) {
        l__nbr_positions = g__plots->getNbrPositionsMemorized();
      }

      boolean l__flg_synth_cap = false;

#ifndef USE_SIMULATION
      size_t l__size = buildCommandsPromptsPosition(&l__st_coord_position, l__nbr_positions, &l__flg_synth_cap, &l__commands, &l__durations, &l__nbr_durations);

      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
      buildCommandsPromptsPosition(&l__st_coord_position, l__nbr_positions, &l__flg_synth_cap, &l__commands, &l__durations, &l__nbr_durations);
#endif

      if (l__flg_synth_cap == true) {
        // Calcul du cap à suivre pour rejoindre la position 'enregistrée' / 'mémorisée'
        COORD l__coord_position;
        g__pilot->initCoordToNoMean(&l__coord_position);
        l__coord_position.lat = l__st_coord_position.lat;
        l__coord_position.lon = l__st_coord_position.lon;
        l__coord_position.ele = l__st_coord_position.ele;
      
        float l__cap = g__pilot->getCapTo(&l__coord_position);
        // Fin: Calcul du cap à suivre pour rejoindre la position 'enregistrée' / 'mémorisée'

        synthesisOfCaps(l__cap, true);    // "à midi" autorisé
      }
      
      // Protection répétition avant l'expiration de 'DURATION_TIMER_TYPE_SYNTH_POSITION'
      g__timers->start(TIMER_TYPE_SYNTH_POSITION, DURATION_TIMER_TYPE_SYNTH_POSITION, NULL);

      l__flg_rtn = true;
    }
  }

  return l__flg_rtn;
}

boolean synthesisPosCommune()
{
  boolean l__flg_rtn = false;

  Serial.print("synthesisPosCommune(): Entering...\n");

  ST_COORD_POSITION l__st_coord_position;

  if (g__timers->isInUse(TIMER_TYPE_SYNTH_POS_COMMUNE) == false
   && g__plots->getPosCommunesSelected(&l__st_coord_position) == true)
  {
    g__plots->toStringPosCommunes(false, true);

    byte      *l__commands = NULL;
    uint16_t  *l__durations = NULL;
    size_t    l__nbr_durations = 0;

    boolean l__flg_synth_cap = false;

#ifndef USE_SIMULATION
    size_t l__size = buildCommandsPromptsPosCommune(&l__st_coord_position, &l__flg_synth_cap, &l__commands, &l__durations, &l__nbr_durations);

    g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
#else
    buildCommandsPromptsPosCommune(&l__st_coord_position, &l__flg_synth_cap, &l__commands, &l__durations, &l__nbr_durations);
#endif

    if (l__flg_synth_cap == true) {
      // Calcul du cap à suivre pour rejoindre la position de la commune sélectionnée
      COORD l__coord_position;
      g__pilot->initCoordToNoMean(&l__coord_position);
      l__coord_position.lat = l__st_coord_position.lat;
      l__coord_position.lon = l__st_coord_position.lon;
      l__coord_position.ele = l__st_coord_position.ele;
      
      float l__cap = g__pilot->getCapTo(&l__coord_position);
      // Fin: Calcul du cap à suivre pour rejoindre la position de la commune sélectionnée

      synthesisOfCaps(l__cap, true);    // "à midi" autorisé
    }

    // Protection répétition avant l'expiration de 'DURATION_TIMER_TYPE_SYNTH_POS_COMMUNE'
    g__timers->start(TIMER_TYPE_SYNTH_POS_COMMUNE, DURATION_TIMER_TYPE_SYNTH_POS_COMMUNE, NULL);

    l__flg_rtn = true;
  }

  return l__flg_rtn;
}

/* Treatments with synthesis prompts (normaly: mode 'connected' enabled)
*/
void execTreatmentsWithSynthesisPrompts()
{
  char l__buffer[80];

  sprintf(l__buffer, "execTreatmentsWithSynthesisPrompts():\n");
  Serial.print(l__buffer);

  boolean l__flg_isMulipleOfHours = false;
  boolean l__flg_isMulipleOf5Minutes = false;

  long    l__add_timer_duration = 0L;     // Ajout éventuel d'une durée timer

  // 1 - Synthèse Date/Time complète à chaque heure (SYNTH_DATE_TIME_ALL)
#if USE_FORCE_TIME
  l__flg_isMulipleOfHours = (g__serial_nmea->isMulipleOfHours() == true) || (g__flg_force_time_hour == true);
#else
  l__flg_isMulipleOfHours = g__serial_nmea->isMulipleOfHours();
#endif

  if (l__flg_isMulipleOfHours == true) {
    g__serial_mp3_player->incAndGetNumSequence(TX_PLAY);    // Increment 'num_sequence' for retry this prompts suite

    synthesisDateTime(SYNTH_DATE_TIME_ALL);

    // Synthèse de "vous êtes à ..." toutes les heures
    synthesisDistanceCapAndElevation(SYNTH_POSITION, g__pilot->getDistMode());

    // Synthèse de "Tracé à suivre sur...", ...
    g__plots->setMakeSynthProperties(true);
    l__add_timer_duration = 30 * 100L;

#if USE_FORCE_TIME
    g__flg_force_time_hour = false;
#endif
  }

  // ou toutes les 5 minutes exactes sauf HH:00 (SYNTH_TIME)
#if USE_FORCE_TIME
  l__flg_isMulipleOf5Minutes = (g__serial_nmea->isMulipleOf5Minutes() == true) || (g__flg_force_time_5mn == true);
#else
  l__flg_isMulipleOf5Minutes = g__serial_nmea->isMulipleOf5Minutes();
#endif

  if (l__flg_isMulipleOf5Minutes == true) {
    g__serial_mp3_player->incAndGetNumSequence(TX_PLAY);    // Increment 'num_sequence' for retry this prompts suite

    synthesisDateTime(SYNTH_TIME);

    // Pas de synthèse de "vous êtes à ..." toutes les 5 minutes
    synthesisDistanceCapAndElevation(SYNTH_NO_POSITION, g__pilot->getDistMode());

    // Synthèse de "Tracé à suivre sur...", ...
    g__plots->setMakeSynthProperties(true);
    l__add_timer_duration = 15 * 100L;

#if USE_FORCE_TIME
    g__flg_force_time_5mn = false;
#endif
  }

  // Récupération des caractéristiques de la position @ tracé
  ST_RESULTS l__results;
  boolean l__flg_results = g__plots->getResultsSave(&l__results);

  /* Synthèse des propriétés des adjuts plots
   * - Distance calculée dans 'Plots::adjustmentCapAndDistanceMore()'
   * - Sens "Aller"/"Retour" déteminée dans 'Plots::adjustmentCapAndDistanceMore()'
   */
  if (g__plots->isMakeSynthProperties()) {
    // Pas de synthèse si "Arrivé à la fin du tracé"
    if (g__plots->getPlotsDirection() != PLOTS_DIR_NO_MEAN && l__results.type_synth != TYPE_SYNTH_YOU_HAVE_ARRIVED) {

      const char *l__text = "Unknown";
      switch (g__plots->getPlotsDirection()) {
      case PLOTS_DIR_GO:
        l__text = "Aller";
        break;
      case PLOTS_DIR_RETURN:
        l__text = "Retour";
        break;
      default:
        break;
      }

      Serial.print("Synthesis results:\n");
      sprintf(l__buffer, "\tType synth [#%d] Idx [#%d] Dist [%d] M\n",
        l__results.type_synth, l__results.idx, l__results.distance);
      Serial.print(l__buffer);

      sprintf(l__buffer, "\t=> Dist: remaining [%d] on plots [%d] total [%d] M Sens [%s]\n",
        g__plots->getRemainingDistance(), g__plots->getDistanceOnPlots(), g__plots->getTotalDistance(), l__text);
      Serial.print(l__buffer);

      synthesisPlotsProperties();
    }

    g__plots->setMakeSynthProperties(false);
  }
  // Fin: Synthèse des propriétés des adjuts plots

  // Synthèse synchrone et asynchrone de la distance au tracé avec toutes ses variantes
  boolean l__flg_synth_to_make = false;     // A priori, pas de synthèse...
  boolean l__flg_synth_done = false;        // Synthèse faite

  /* Détermination des restrictions "temporelles" suite à la synthèse synchrone
   * - Report de diffusion du même type de message
   *   => Avec en supplément 'l__add_timer_duration' à tous les autres timers suivant l'origine de la diffusion:
   *      - 30" dans le cas des heures
   *      - 15" dans le cas des 5 mn
   *      -  5" dans le cas asynchrone
   */
  if (l__flg_results == true) {
    // Pas de synthèse si déjà faite depuis "peu de temps"
    if (l__results.type_synth == TYPE_SYNTH_THE_PLOT_IS && g__timers->isInUse(TIMER_TYPE_SYNTH_THE_PLOT_IS) == true) {
      l__flg_synth_to_make = false;
    }
    else if (l__results.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1 && g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1) == true) {
      l__flg_synth_to_make = false;
    }
    else if (l__results.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2 && g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2) == true) {
      l__flg_synth_to_make = false;
    }
    else if (l__results.type_synth == TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3 && g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3) == true) {
      l__flg_synth_to_make = false;
    }
    else if (l__results.type_synth == TYPE_SYNTH_YOU_HAVE_ARRIVED && g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED) == true) {
      l__flg_synth_to_make = false;
    }
    else {
      l__flg_synth_to_make = true;
    }
  }
  // Fin: Détermination des restrictions "temporelles" suite à la synthèse synchrone
  
  // Test changement de minutes
  // TODO: Pas de synthèse de 'synthesisDistanceToPlot' et 'synthesisPosRecordedOrMemorized' si 'Hour' ou '5 mn'
#ifndef USE_SIMULATION   // Call to 'isNewMinute()'
  if (g__serial_nmea->isNewMinute() == true || l__flg_isMulipleOfHours == true || l__flg_isMulipleOf5Minutes == true)
#else
  if (l__flg_isMulipleOfHours == true || l__flg_isMulipleOf5Minutes == true)
#endif
  {
    if (l__flg_synth_to_make == true) {
#ifndef USE_SIMULATION
      // Pas d'increment de la séquence dans les cas de changement d'heure ou multiple de 5 mn
      if (l__flg_isMulipleOfHours == false && l__flg_isMulipleOf5Minutes == false) {
        g__serial_mp3_player->incAndGetNumSequence(TX_PLAY);    // Increment 'num_sequence' for retry this prompts suite
      }
#endif

      sprintf(l__buffer, "Call 'synthesisDistanceToPlot()' in Synchrone: Type [%d]\n", l__results.type_synth);
      Serial.print(l__buffer);

      l__flg_synth_done = synthesisDistanceToPlot(&l__results);
    }

    // Traces if Hour and 5 mn + 'flg_dist_to_track_too_large'
    sprintf(l__buffer, "Muliple of Hours [%d] of 5Minutes [%d] Dist. Track [%d]\n",
      l__flg_isMulipleOfHours, l__flg_isMulipleOf5Minutes, l__results.flg_dist_to_track_too_large);
    Serial.print(l__buffer);

    if (l__flg_isMulipleOfHours == false && l__flg_isMulipleOf5Minutes == false) {  
      // Synthèse de la position 'enregistrée' / 'mémorisée' toutes les minutes sauf multiple de 5 mn
      synthesisPosRecordedOrMemorized();

      if (g__plots->isAvailable() == false || l__results.flg_dist_to_track_too_large == true) {
        /* Synthèse de la position de la commune toutes les minutes si pas de tracé disponible
         * ou si la distance au tracé est "trop grande"
         */
        Serial.print("\tCall 'synthesisPosCommune()' #1\n");
        synthesisPosCommune();
      }
    }
    else {
      // Synthèse de la position de la commune toutes les 5 mn et toutes les heures
      Serial.print("\tCall 'synthesisPosCommune()' #2\n");
      synthesisPosCommune();
    }
  }
  else {
    /*  Synthèse asynchrone dès sur le tracé avec toutes ses variantes avec appel à 'synthesisDistanceToPlot()' dans les cas suivants
     *  - Si timer associé à 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1' non en cours et si de nouveau sur le tracé
     *  - Si timer associé à 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2' non en cours
     *  - Si timer associé à 'TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3' non en cours
     *  
     *  Remarque: Dans tous les cas, Set/Reset de 'g__flg_inh_synth_dist_to_plot' pour ne pas synthétiser la distance sur ou au tracé
     *            => Ne sera synthétisé que le message: "Vous êtes de nouveau sur le tracé..." (cf. 'buildCommandsPromptsDistToPlots()' method)
     */
    if (l__flg_synth_to_make == true) {
#ifndef USE_SIMULATION
      g__serial_mp3_player->incAndGetNumSequence(TX_PLAY);    // Increment 'num_sequence' for retry this prompts suite
#endif

      sprintf(l__buffer, "Call 'synthesisDistanceToPlot()' in Asynchrone: Type [%d]\n", l__results.type_synth);
      Serial.print(l__buffer);

      // Synthèse avec une éventuelle restriction; à savoir: Distance sur ou au tracé non synthétisée pour éviter les répétions toutes les "xxx" Sec.
      g__flg_inh_synth_dist_to_plot = true;

      // Etat de la synthèse effective après les restrictions éventuelles
      l__flg_synth_done = synthesisDistanceToPlot(&l__results);

      g__flg_inh_synth_dist_to_plot = false;    // Annulation de la restriction   

      // Supplément 'l__add_timer_duration' de 5" (cas asynchrone)
      l__add_timer_duration = 5 * 100L;
    }
  }

  // Armement des timers associés si synthèse réalisée
  if (l__flg_synth_done == true) {
    // Armement de tous les timers @ 'l__add_timer_duration'
    if (l__add_timer_duration > 0L) {
      g__timers->start(TIMER_TYPE_SYNTH_THE_PLOT_IS, l__add_timer_duration, NULL);
      g__timers->start(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1, l__add_timer_duration, NULL);
      g__timers->start(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2, l__add_timer_duration, NULL);
      g__timers->start(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3, l__add_timer_duration, NULL);
      g__timers->start(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED, l__add_timer_duration, NULL);
    }

    // Armement/Complément suivant le type de synthèse
    switch (l__results.type_synth) {
    case TYPE_SYNTH_THE_PLOT_IS:
      if (g__timers->isInUse(TIMER_TYPE_SYNTH_THE_PLOT_IS) == false) {
        g__timers->start(TIMER_TYPE_SYNTH_THE_PLOT_IS, DURATION_TIMER_TYPE_SYNTH_THE_PLOT_IS, NULL);
      }
      else {
        g__timers->addDuration(TIMER_TYPE_SYNTH_THE_PLOT_IS, DURATION_TIMER_TYPE_SYNTH_THE_PLOT_IS);
      }
      break;
    case TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1:
      if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1) == false) {
        g__timers->start(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1, DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1, NULL);
      }
      else {
        g__timers->addDuration(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1, DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_1);
      }
      break;
    case TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2:
      if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2) == false) {
        g__timers->start(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2, DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2, NULL);
      }
      else {
        g__timers->addDuration(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2, DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_2);
      }
      break;
    case TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3:
      if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3) == false) {
        g__timers->start(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3, DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3, NULL);
      }
      else {
        g__timers->addDuration(TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3, DURATION_TIMER_TYPE_SYNTH_YOU_ARE_ON_THE_PLOT_3);
      }
      break;
    case TYPE_SYNTH_YOU_HAVE_ARRIVED:
      if (g__timers->isInUse(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED) == false) {
        g__timers->start(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED, DURATION_TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED, NULL);
      }
      else {
        g__timers->addDuration(TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED, DURATION_TIMER_TYPE_SYNTH_YOU_HAVE_ARRIVED);
      }
      break;
    default:
      break;
    }
  }
  // Fin: Armement des timers associés si synthèse réalisée
  // Fin: Synthèse synchrone et asynchrone de la distance au tracé avec toutes ses variantes

  // Other treatments with synthesis prompts

  /*  Call to 'setCommandSended(true)' method if all prompts played
      => No stopping the diffusion in progress ;-)

      => Remarques: Si tous les prompts ont été joués
                    => Force à jouer ceux de la synthèse Date/Time (return true)
                    => Sinon, ces derniers seront joués à la suite du prompt en cours de diffusion
                       et du ou des prompts de la FIFO/Tx PLAY (return false)
                    => Permet de construire une liste de prompts qui seront joués après que les
                       précédents aient été diffusés ;-)
  */
#ifndef USE_SIMULATION
  if (g__serial_mp3_player->isAllPromptsPlayed()) {
    g__serial_mp3_player->setCommandSended(TX_PLAY, true);
  }
#endif
}

/*  Calibration du temps interne:
    - Si Le temps interne est inférieur au temps GPS => Diminution @ COUNTER_FOR_10MS
      => 1st range:   [ -5, -4, -3, -2, -1, +1, +2, +3, +4,  +5, ...,  +9, +10] (normal:  RANGE_COEFF: 1)
         => IT 500 uS [ 15, 16, 17, 18, 19, 21, 22, 23, 24,  25, ...,  29,  30] à comparer à COUNTER_FOR_10MS = 20
      => 2nd range:   [-10, -8, -6, -4, -2, +2, +4, +6, +8, +10, ..., +18, +20] (dynamic: RANGE_COEFF: 2)
         => IT 500 uS [ 10, 12, 14, 16, 18, 22, 24, 26, 28,  30, ...,  38,  40] à comparer à COUNTER_FOR_10MS = 20

    - Aucun impact sur le Date/Time réellement synthétisé puisque issu du temps GPS en mode 'CONNECTED'
*/
void internalTimeCalibration()
{
#ifdef USE_SIMULATION   // Skip 'internalTimeCalibration()'
  Serial.print("internalTimeCalibration(): Entering...");
#else
#define RANGE_COEFF         2

  char l__buffer[80];

  long l__diff = g__stats->getDiffDurationInternalToGps();

  l__buffer[0] = '\0';

  if (l__diff == 0L) {
    sprintf(l__buffer, "internalTimeCalibration(): diff [%ld] Current [%d]\n", l__diff, g__counter_for_10ms);
  }
  else {
    byte l__counter_for_10ms = COUNTER_FOR_10MS;      // Valeur pivot

    if (l__diff <= -60L || l__diff >= 60L) {
      /*  Différence de plus de 1 mn
       *  => Trop long à rattraper (le temps interne a dérivé de plus de 1 mn @ nouvelle heure GPS ;-)
       *     => Recopie du temps GPS dans le temps interne 'duration_internal'
       *        => Sans impact sur 'duration_not_connected' car incrémenté
       */
      char l__buffer_bis[16];
      convertDurationToString(l__buffer_bis, l__diff, true);
      sprintf(l__buffer, "internalTimeCalibration(): Big diff [%ld] [%s]\n", l__diff, l__buffer_bis);
      Serial.print(l__buffer);

#ifndef USE_SIMULATION
      convertDurationToString(l__buffer_bis, g__stats->getGenDurationFromGps(), false);
      sprintf(l__buffer, "\t-> Init. with GPS duration [%ld] [%s]\n", g__stats->getGenDurationFromGps(), l__buffer_bis);
      Serial.print(l__buffer);

      g__stats->copyGenEpochCurrentToDurationInternal();
#endif
    }
    else if (l__diff <= -5L) {
      /*  Le temps GPS a pris de l'avance sur le temps interne
       *  => Limitation volontaire de l'accélaration du temps interne @ au ralentissement
       */
      l__counter_for_10ms -= (5 * RANGE_COEFF);
    }
    else if (l__diff >= 10L) {
      /*  Le temps GPS a pris du retard sur le temps interne
       *  => C'est le cas en mode 'NOT CONNECTED'
       *     => Autorisation a ralentir plus fortement (@ accélération -> 5") le temps interne (-> 10")
       */
      l__counter_for_10ms += (10 * RANGE_COEFF);
    }
    else {
      l__counter_for_10ms += (byte)(l__diff * RANGE_COEFF);
    }

    if (l__counter_for_10ms != g__counter_for_10ms_pre) {
      // Warning: 'g__counter_for_10ms' est lu sous ITs (cf. 'onTimer500uS()' method)
#ifndef USE_SIMULATION
      noInterrupts();
#endif

      g__counter_for_10ms = l__counter_for_10ms;

#ifndef USE_SIMULATION
      interrupts();
#endif

      sprintf(l__buffer, "internalTimeCalibration(): diff [%ld] Pre [%d] => Current [%d]\n",
              l__diff, g__counter_for_10ms_pre, g__counter_for_10ms);

      g__counter_for_10ms_pre = g__counter_for_10ms;
    }
    else {
      sprintf(l__buffer, "internalTimeCalibration(): diff [%ld] Pre [%d] == Current [%d]\n",
              l__diff, g__counter_for_10ms_pre, g__counter_for_10ms);
    }
    Serial.print(l__buffer);
  }

  // Update statistics (#samples, current, min, avg and max) of 'g__counter_for_10ms'
  g__stats->updateGestionCounterFor10ms(g__counter_for_10ms);
#endif
}

/* Delegation de la nouvelle position a la classe 'Menus'
 * => Si en mode connected; Le prompt d'attente en musique ('NUM_INTERSTELLAR_THEME' en cours de diffusion) sera) {
 *    stoppe ainsi que le timer 'TIMER_ADVERT'
 * => Si en mode non connecte; la diffusion en cours sera stoppe, la FIFO/Tx Play sera purgee pour que les prompts
 *    eventuellement empiles n'interagissent avec la gestion du menu
 * => Sinon; Ignore
*/
void gestionMenus(ST_GESTION_MENUS *io__st_gest_menus)
{
  switch (io__st_gest_menus->menu_action) {
  case MENU_ACTION_NEW_MENU:
    g__menus->updateVolumeLevel(io__st_gest_menus);
    break;
  default:
    break;
  }
}

void loop()
{
  char l__buffer[132];

  // Treatment all the 10 mS
  if (g__flg_10mS == true) {
    treatmentAll_10mS();
  }

  // Treatment all the 100 mS
  if (g__flg_100mS == true) {
    treatmentAll_100mS();
  }

  if (g__flg_1Sec == true) {
    g__flg_1Sec = false;
    g__counter_100mS = 0;

    g__stats->incGenDurationInternal();

    if (g__gestion.state == GESTION_STATE_NOT_CONNECTED) {
      g__stats->incGenDurationNotConnected();
    }

    // Update the General QOS
    g__stats->setGenQos();
  }

#ifndef USE_SIMULATION
  // Rotary encoder gestion in background
  if (g__rotary_encoder->isEncoderPosition()) {
    noInterrupts();
    int l__encoder_position = g__rotary_encoder->updateEncoderPosition();
    interrupts();

    // Remarque: L'appel à 'g__rotary_encoder->isNewNumMenu()' repositionne a "pas de nouveau menu" ;-)
    boolean l__flg_new_menu = g__rotary_encoder->isNewNumMenu();

    sprintf(l__buffer, "Encoder position [%c%d] Sens [%c] Button [%d] Menu [%d] (%d)\n",
            (l__encoder_position >= 0) ? '+' : '-',
            (l__encoder_position >= 0) ? l__encoder_position : -l__encoder_position,
            (g__rotary_encoder->getSensRotation()) == false ? '-' : '+',
            g__rotary_encoder->stateEncoderButton(),
            g__rotary_encoder->getNumMenu(),
            l__flg_new_menu);

    Serial.print(l__buffer);

    if (l__flg_new_menu) {
      Serial.printf("Encoder position => New menu [%d]\n", g__rotary_encoder->getNumMenu());

      if (g__timers->isInUse(TIMER_ADVERT)) {
        // Arrêt du timer diffusion 'advert'
        g__timers->stop(TIMER_ADVERT);
      }

      // Purge des messages eventuellement a diffuser
      if (g__serial_mp3_player->isAllPromptsPlayed() == false) {
        Serial.print("\t=> Purge FIFO Tx/Play...\n");
        g__serial_mp3_player->purgeFIFOTxPlay();
      }

      // Passage dans la classe 'Menus' pour determiner la nouvelle valeur du 'Volume Level"
      ST_GESTION_MENUS l__st_gest_menus;
      l__st_gest_menus.flg_sens     = g__rotary_encoder->getSensRotation();
      l__st_gest_menus.menu_action  = MENU_ACTION_NEW_MENU;
      l__st_gest_menus.menu_value   = g__rotary_encoder->getNumMenu();
      l__st_gest_menus.volume_level = g__serial_mp3_player->getValueEqualizer();
      gestionMenus(&l__st_gest_menus);

      // Arret du message en cours avec la diffusion de "BIG_RATCHET_HALF_TURN"
      byte *l__commands;
      size_t l__size = 0;
      l__size = buildCommandsPromptsGeneral(BASE_BIG_RATCHET_HALF_TURN, &l__commands);
      g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
      g__serial_mp3_player->setCommandSended(TX_PLAY, true);

      if (g__timers->isInUse(TIMER_NEW_MENU)) {
        g__timers->stop(TIMER_NEW_MENU);
      }

      // Relance du timer (expiration correspondante au dernier nouveau menu) (Duree multiple de 10 mS)
      g__timers->start(TIMER_NEW_MENU, (g__rotary_encoder->getMenuParamDurationIdle() / 10), &callback_timer_new_menu);
    }

    // Support des appuis/relachés dans les xxx mS
    if (g__rotary_encoder->stateEncoderButton() == true) {
      if (g__timers->isInUse(TIMER_BUTTON) == false) {
        g__timers->start(TIMER_BUTTON, DURATION_TIMER_BUTTON, &callback_timer_button);
      }
      g__rotary_encoder->incNbrOfPressButton();
    }
    // Fin: Support des appuis/relachés dans les xxx mS
  }
  // End: Rotary encoder gestion in background
#endif

#ifndef USE_SIMULATION
  /* Gestion of button action
   * => Prise en compte si la vitesse de déplacement est < 0.75 KmH
   */
  if (g__timers->isInUse(TIMER_BUTTON) == false) {
    ENUM_BUTTON_ACTION l__num_button_action = g__rotary_encoder->getPressButtonAction();

    // Pour la prise en compte à l'arrêt @ 'speedKmH'
    COORD l__coord_current;
    g__plots->getCoordCurrent(&l__coord_current);

    if (l__num_button_action == ENUM_BUTTON_ACTION_1) {
      if (g__gestion.state == GESTION_STATE_CONNECTED && g__stats->isMovementInProgress() == false) {   // Vitesses GPS ou calculée <= 0.75 KmH
        /*  Rediffusion de la dernière sequence vocale
         *  => Ne sera faite que lorsque tous les prompts auront été diffusés
         *     ou immédiatement si la FIFO/Tx Play est vide
         */
        g__gestion.flg_retry_last_sequence = true;
      }

      Serial.print("Force the Last position saved\n");
      g__gestion.flg_force_last_pos_saved = true;

      /*  Enregistrement des positions courantes dans les plots records (suivant critères ;-)
       *  => Remplacement des valeurs téléchargées
       */
      Serial.print("Save the current positions\n");
      g__gestion.flg_save_current_positions = true;
    }
    else if (l__num_button_action == ENUM_BUTTON_ACTION_2 && g__stats->isMovementInProgress() == false) {   // Vitesses GPS ou calculée <= 0.75 KmH
      byte      *l__commands;
      uint16_t  *l__durations = NULL;
      size_t    l__nbr_durations = 0;

      /* - Action d'enregistrement du tracé si 'g__plots->getPlotsDirection() == PLOTS_DIR_NO_MEAN'
       * - Sinon, action d'inversion du tracé déjà enregistré
       */
      if (g__plots->getPlotsDirection() == PLOTS_DIR_NO_MEAN) {
        g__serial_mp3_player->incAndGetNumSequence(TX_PLAY);    // Increment 'num_sequence' for retry this prompts suite

        size_t l__size = buildCommandsPromptsBuildingOfPlots(&l__commands, &l__durations, &l__nbr_durations);
        g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
      }

      if (g__gestion.flg_save_current_positions == true) {
        // Arrêt de l'enregistrement des positions courantes et 'plot records' valid
        g__gestion.flg_save_current_positions = false;
        g__plots->setAvailable();
      }

      // Lancement du calcul des inverses des 'plots' si disponibles
      if (g__plots->isAvailable() == true) {
        if (g__plots->getPlotsDirection() == PLOTS_DIR_GO || g__plots->getPlotsDirection() == PLOTS_DIR_RETURN) {
          // Synthèse de "Inversion du sens du tracé"
          size_t l__size = buildCommandsPromptsChangeSensOfPlots(&l__commands, &l__durations, &l__nbr_durations);
          g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size, l__durations, l__nbr_durations);
        }
      
        g__plots->inversePlotsRecord();
      }
    }
  }
  // End: Gestion of button action
#endif

  // Mémorisation de la position 'SYNTH_DIST_FROM_POS_SAVE'
  if (g__gestion.state == GESTION_STATE_CONNECTED) {
    switch (g__gestion.sub_state2) {
    case GESTION_SUB_STATE2_UNKNOWN:
      {
        COORD l__coord;
        g__pilot->initCoordToNoMean(&l__coord);

        if (g__plots->isAvailable() == false || g__gestion.flg_force_last_pos_saved == true) {
          /*  Acquisition de la position actuelle
           *  => Car pas de 'Starting Position' ou appui button durant le téléchargement du tracé
           *  => Fait 1! fois à l'établissement GPS (et non au rétablissement ;-)
           */
          l__coord.lat = g__pilot->getLat(COORD_CURRENT);
          l__coord.lon = g__pilot->getLon(COORD_CURRENT);
          l__coord.ele = g__pilot->getEle(COORD_CURRENT);

          // Recopie dans la dernière position enregistrée
          g__pilot->initCoordLastPosSave(&l__coord);
          g__pilot->setDistMode(SYNTH_DIST_FROM_POS_SAVE);

          Serial.print("Last position saved:\n");
          Serial.print("\tLat.  [");
          Serial.print(l__coord.lat, 6);
          Serial.print("] degrees\n");
          Serial.print("\tLon.  [");
          Serial.print(l__coord.lon, 6);
          Serial.print("] degrees\n");
          Serial.print("\tEle.  [");
          Serial.print(l__coord.ele, 0);
          Serial.print("] M\n");
        }
      }

      g__gestion.sub_state2 = GESTION_SUB_STATE2_CURRENT_POSITION_SAVED;
      break;

    case GESTION_SUB_STATE2_CURRENT_POSITION_SAVED:
      //Serial.print("Warning: Last position already saved:\n");
      break;

    default:
      sprintf(l__buffer, "Error: 'state2' [%d] unknown\n", g__gestion.sub_state2);
      Serial.print(l__buffer);
      break;
    }
  }
  // Fin: Mémorisation de la position 'SYNTH_DIST_FROM_POS_SAVE'

  // Serial NMEA gestion in background
  if (g__serial_nmea->isFrameNMEAAvailable()) {
#if USE_PRINT_GPS_VALUES
    char l__buffer_bis[72];
    if (g__serial_nmea->getFrameNMEA().length() > 100) {       // Pour la place de "[...] (Cks [0xHH] Ok)"
      memset(l__buffer_bis, '\0', sizeof(l__buffer_bis));
      strncpy(l__buffer_bis, g__serial_nmea->getFrameNMEA().c_str(), (sizeof(l__buffer_bis) - 1));
      sprintf(l__buffer, "NMEA [%s...", l__buffer_bis);
    }
    else {
      sprintf(l__buffer, "NMEA [%s", g__serial_nmea->getFrameNMEA().c_str());
    }
#endif

    // Test des checksum lue et calculee, print de la trame NMEA non decodee et extraction des infos attendues
    unsigned char l__cks_calculated = 0x00;
    unsigned char l__cks_expected = 0x00;
    boolean l__cks_rtn = g__serial_nmea->calculChecksumNMEA((char *)g__serial_nmea->getFrameNMEA().c_str(), &l__cks_calculated, &l__cks_expected);

#if USE_PRINT_GPS_VALUES
    sprintf(&l__buffer[strlen(l__buffer)], "] (Cks [0x%02x] %s)\n",
            l__cks_calculated,
            (l__cks_rtn == true ? "Ok" : "Ko"));
    Serial.print(l__buffer);
#endif

    if (l__cks_rtn == true) {
      g__serial_nmea->extractInfosNMEA((char *)g__serial_nmea->getFrameNMEA().c_str());
    }
  }
  // Fin: Test des checksum lue et calculee, print de la trame NMEA non decodee et extraction des infos attendues

  if (g__serial_nmea->isFrameTLVAvailable()) {

#ifdef USE_SIMULATION
    // Force indisponibilitee trame GPS
    g__serial_nmea->setFrameAvailable(false);
#endif

    boolean l__flg_rearm_timer_gps = false;

    // Test des checksum lue et calculée + print de la trame non décodée
    unsigned char l__cks_calculated = 0xff;
    boolean l__cks_rtn = g__serial_nmea->calculChecksum((char *)g__serial_nmea->getFrameTLV().c_str(), &l__cks_calculated);

    /* Print with suppression of '\r' and '\n' terminal
     * => TODO: Limiter la recopie à la taille de 'l__buffer' (origine des redémarrages ;-)
     *    => Passer par un 1st buffer de taille raisonable (40 bytes pour la copie terminée par "...")
     */
    char l__buffer_bis[40];
    if (g__serial_nmea->getFrameTLV().length() > 100) {       // Pour la place de "[...] (Cks [0xHH] Ok)"
      memset(l__buffer_bis, '\0', sizeof(l__buffer_bis));
      strncpy(l__buffer_bis, g__serial_nmea->getFrameTLV().c_str(), (sizeof(l__buffer_bis) - 1));
      sprintf(l__buffer, "GPS [%s...", l__buffer_bis);
    }
    else {
      sprintf(l__buffer, "GPS [%s", g__serial_nmea->getFrameTLV().c_str());
    }
    char *l__pattern = NULL;
    if ((l__pattern = strrchr(l__buffer, '\n')) != NULL) *l__pattern = '\0';
    if ((l__pattern = strrchr(l__buffer, '\r')) != NULL) *l__pattern = '\0';
    sprintf(&l__buffer[strlen(l__buffer)], "] (Cks [0x%02x] %s)\n",
            l__cks_calculated,
            (l__cks_rtn == true ? "Ok" : "Ko"));
    Serial.print(l__buffer);

    // Print infos si ckecksum correcte
    if (l__cks_rtn == true) {
#ifndef USE_SIMULATION
      String l__message = "";
#else
      std::string l__message = "";
#endif

#ifdef USE_SIMULATION
      boolean l__rtn = g__serial_nmea->extractGpsTLVInfos(l__message, g__simu_move_flg, g__force_date, g__force_time);
#else
      boolean l__rtn = g__serial_nmea->extractGpsTLVInfos(l__message, g__simu_move_flg);
#endif

      sprintf(l__buffer, "extractGpsTLVInfos(%d): Rtn [%d] [%s]\n", g__simu_move_flg, l__rtn, l__message.c_str());
      Serial.print(l__buffer);

      if (g__serial_nmea->isGpsTLVInfosValid()) {
        char l__t_date_time[80];
        long l__epoch = g__serial_nmea->getUnixTimeGMT(l__t_date_time, &g__dateAndTime);
        sprintf(l__buffer, "\tEpoch UNIX GMT [%ld] [%s]\n", l__epoch, l__t_date_time);

        // TODO: Ajout dans 'execTreatmentsWithSynthesisPrompts()' d'une temporisation via la FIFO/Tx Play si prompt en cours
        // Treatments with synthesis prompts with a new valid frame received
        if (g__gestion.sub_state == GESTION_SUB_STATE_INIT_DONE || g__gestion.sub_state == GESTION_SUB_STATE_INIT_DONE2) {
          // Prise de l'epoch interne pour la détermination de la durée max entre la fin de 2 prompts en mode connecté
          g__serial_mp3_player->setModeConnected(true);
          g__serial_mp3_player->setDurationInternal(g__stats->getGenDurationInternal());
          // Fin: Prise de l'epoch interne pour la détermination de la durée max entre la fin de 2 prompts en mode connecté

          /*  Armement time-out de 'DURATION_TIMER_EXEC_SYNTH' permettant de détecter qu'en mode connecté,
              aucune synthèse n'est effectuée suite à un [r]établissement du signal GPS
          */
          if (g__timers->isInUse(TIMER_EXEC_SYNTH)) {
            g__timers->restart(TIMER_EXEC_SYNTH, DURATION_TIMER_EXEC_SYNTH);
          }
          else {
            g__timers->start(TIMER_EXEC_SYNTH, DURATION_TIMER_EXEC_SYNTH, &callback_exec_synth);
          }

          g__timers->start(TIMER_ACTIVITY_EXEC_SYNTH, DURATION_TIMER_ACTIVITY_EXEC_SYNTH, &callback_activity_exec_synth);
          // Pas d'activite sur la Led YELLOW
          //g__state_leds |= STATE_LED_YELLOW;

          execTreatmentsWithSynthesisPrompts();

          // Set the current coordinates
          COORD l__coord_current;

#if USE_FORCE_CURRENT_COORD
          if (g__flg_force_coord_current == false) {
#endif
          // Recopie de la position courante figée entre 2 acquisitions
          l__coord_current.lat      = g__pilot->getLat(COORD_CURRENT);
          l__coord_current.lon      = g__pilot->getLon(COORD_CURRENT);
          l__coord_current.cap      = g__pilot->getCap(COORD_CURRENT);
          l__coord_current.ele      = g__pilot->getEle(COORD_CURRENT);
          l__coord_current.speedKmH = g__pilot->getSpeedKmH(COORD_CURRENT);


#if USE_FORCE_CURRENT_COORD
          }
          else {
            Serial.print("Force the current coord\n");

            /* Comparison with the coordinates forced and if already identical
             * => Only the value Lat and Lon are tested (position with direction and speed @ plots ;-)
             */
            if (l__coord_current.lat == g__force_coord_current.lat
             && l__coord_current.lon == g__force_coord_current.lon)
            {
              Serial.print("=> Lat and Lon values already forced...\n");
            }

            // Forçage avec toutes les valeurs (Ele, Cap and Speed)...
            memcpy(&l__coord_current, &g__force_coord_current, sizeof(COORD));
          }
#endif

          // Marquage du timestamp interne (maj indépendamment des infos GPS ;-) 
          l__coord_current.duration = g__stats->getGenDurationInternal();

#if USE_FORCE_CURRENT_COORD
          g__plots->setCoordCurrent(&l__coord_current, g__flg_force_coord_current);
#else
          g__plots->setCoordCurrent(&l__coord_current);
#endif
        }
        else {
          g__serial_mp3_player->setModeConnected(false);
        }

        // Update statistics
        if (g__stats->getGenEpochInit() == 0L) {
          g__stats->setGenEpochInit(l__epoch);
        }
        else {
          g__stats->setGenEpochCurrent(l__epoch);
        }

        l__flg_rearm_timer_gps = true;

        /*  Recalibration du temps interne @GPS sur les trames correctes
         *  => Sinon, absence de calibration; le temps interne reste figé et sera
         *     ajusté dès les prochaines trames GPS correctes ;-)
         */
        internalTimeCalibration();

        // Update statistics
        g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_GOOD_FRAMES);
      }
      else {
        sprintf(l__buffer, "Error: GPS infos invalid (missing or wrong infos)\n");
        g__serial_nmea->setGpsTLVInfosValid(false);

        // Update statistics
        g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_INVALID_INFOS);
        g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES);
      }
    }
    else {
      g__serial_nmea->setGpsTLVInfosValid(false);
      sprintf(l__buffer, "Error: GPS infos invalid (not connected or wrong cks)\n");

      // Update statistics
      g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_WRONG_CKS);
      g__stats->incGpsCounter(ENUM_STATS_GPS_NBR_OF_ERROR_FRAMES);
    }

    Serial.print(l__buffer);

    if (l__flg_rearm_timer_gps == true) {
      /*  Rearmement timer gps qui pilote l'état "connecté/non connecté" avec la Led Green
             => L'armement du timer fera passer à l'état "connecté" immédiatement
             => L'expiration du timer fera passer à l'état "non connecté" (hystérésis @ durée du timer;-)
      */
      g__gestion.state = GESTION_STATE_CONNECTED;

      if (g__timers->isInUse(TIMER_CONNECT)) {
        g__timers->restart(TIMER_CONNECT, DURATION_TIMER_CONNECT);
      }
      else {
        g__timers->start(TIMER_CONNECT, DURATION_TIMER_CONNECT, &callback_disconnect);
      }

      // Arrêt de 'NUM_INTERSTELLAR_THEME' si en cours avec la diffusion de "[r]établissement du signal GPS"
      if (g__serial_mp3_player->getNumPromptToPlay(IDX_PROMPT_NORMAL) == NUM_INTERSTELLAR_THEME) {
        Serial.print("Stop the 'NUM_INTERSTELLAR_THEME' prompt\n");

        // Arrêt du timer diffusion 'advert'
        g__timers->stop(TIMER_ADVERT);

        byte *l__commands;
        size_t l__size = 0;

        //  Diffusion: "[r]etablissement du signal GPS" suivant le nombre d'etablssement du signal GPS
        l__size = buildCommandsPromptsGeneral(g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_ETABLISHMENTS) == 1 ? BASE_GPS : BASE_RECOVERY_GPS, &l__commands); 
        g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);
        g__serial_mp3_player->setCommandSended(TX_PLAY, true);

        if (g__plots->getNbrPositionsMemorized() == 0) {
          /* Lecture de l'eeprom à chaque "[r]établissement du signal GPS" et si aucune position mémorisée
           * => Postions dans l'ordre chronologique [-(NBR_COORD_IN_EEPROM - 1), -(NBR_COORD_IN_EEPROM - 2), ..., -1, 0]
           *    => Soit de la plus ancienne (NBR_COORD_IN_EEPROM - 1) à la plus récente (0)
           *    => Le 'range' permet de synthétiser l'ordre chronologique ;-)
          */
          Serial.print("Read the positions memorized...\n");

#ifndef USE_SIMULATION
          bool l__flg_rtn = false;
          int n = 0;
          for (n = (NBR_COORD_IN_EEPROM - 1); n >= 0; n--) {
            ST_COORD_POSITION l__pos;

            l__pos.type  = TYPE_POSITION_MEMORIZED;

            timerAlarmDisable(g__timer);
            l__flg_rtn = g__gest_eeprom->pop(&l__pos.idx, &l__pos.lat, &l__pos.lon, &l__pos.ele, &l__pos.range, -n);
            timerAlarmEnable(g__timer);

            if (l__flg_rtn == true) {
              // La position la plus ancienne est considérée comme la 1st (ordre [0, 1, ..., (NBR_COORD_IN_EEPROM - 1])
              l__pos.range = (uint8_t)((NBR_COORD_IN_EEPROM - 1) - n);

              l__flg_rtn = g__plots->addPosition(&l__pos);
              sprintf(l__buffer, "\tAdd position #%d (%s)\n", n, (l__flg_rtn == true) ? "Ok" : "Ko");
              Serial.print(l__buffer);              
            }
            else {
              sprintf(l__buffer, "No position memmorized for #%d\n", n);
              Serial.print(l__buffer);              
            }
          }

          g__plots->toStringPositions(true);
#endif
        }
        else {
          sprintf(l__buffer, "%d positions already memorized\n", g__plots->getNbrPositionsMemorized());
          Serial.print(l__buffer);
        }
        // Fin: Lecture de l'eeprom à chaque "[r]établissement du signal GPS" et si aucune position mémorisée
      }

      // Update statistics
      g__stats->setGpsAvailable(true);
    }
  } // if (g__serial_nmea->isFrameAvailable())
  // End: Serial NMEA gestion in background

#ifndef USE_SIMULATION
  // Serial MP3 Player gestion in background
  if (g__serial_mp3_player->isFrameAvailable()) {
   Serial.println(g__serial_mp3_player->getFrameRx());

    g__serial_mp3_player->exec_automate(TREATMENT_RX);

    // Set to unavailable frame (the frame received is treated ;-)
    g__serial_mp3_player->setUnvailableFrame();
  }

  /*  Attente de la lecture de la FIFO/Tx Play
   *  Si le timer 'TIMER_WAIT_FIFO_TX_PLAY' est en cours d'exécution
   *  => Pas d'appel à 'isCommandToSend(TX_PLAY)' correspondant au temps d'attente
   *     avant la prochaine lecture de la FIFO/Tx Play
  */
  if (g__timers->isInUse(TIMER_WAIT_FIFO_TX_PLAY) == false) {
    if (g__serial_mp3_player->isCommandToSend(TX_PLAY)) {
      Serial.println(g__serial_mp3_player->getFrameTx(TX_PLAY));

      g__serial_mp3_player->exec_automate(TREATMENT_TX_PLAY);
    }
  }
  // Fin: Attente de la lecture de la FIFO/Tx Play

  /*  Attente de la lecture de la FIFO/Tx Other
   *  Si le timer 'TIMER_WAIT_FIFO_TX_OTHER' est en cours d'exécution
   *  => Pas d'appel à 'isCommandToSend(TX_OTHER)' correspondant au temps d'attente
   *     avant la prochaine lecture de la FIFO/Tx Other
  */
  if (g__timers->isInUse(TIMER_WAIT_FIFO_TX_OTHER) == false) {
    if (g__serial_mp3_player->isCommandToSend(TX_OTHER)) {
      Serial.println(g__serial_mp3_player->getFrameTx(TX_OTHER));

      g__serial_mp3_player->exec_automate(TREATMENT_TX_OTHER);
    }
  }
  // Fin: Attente de la lecture de la FIFO/Tx Other
  // End: Serial MP3 Player gestion in background
#endif

#ifndef USE_SIMULATION
  /*  Report sur la Led YELLOW (flash de 100 mS) durant la famine de "FIFO/Tx Play"
      qui ne doit pas perdurer entrainant l'abscence de synthèse sauf si 'advert' en cours d'activité ;-)
      => Priorité au report de l'activité du traitement de la famine de "FIFO/Tx Play"
      => Si 'advert' en cours d'activité, report sur la Led YELLOW (flash de 100 mS)
         => Fix: S'appuyer sur 'GESTION_SUB_STATE_INIT_IN_PROGRESS' et 'GESTION_SUB_STATE_INIT_RETRY'
                 pour ne pas utiliser 'g__timers->isInUse(TIMER_ADVERT)' car l'insertion d'une publicité
                 sera peut-être utiliser dans les menus avec ou sans synchronisation ;-)
  */
  // Test de la famine de la FIFO Tx/Play uniquement si [r]établissement GPS
  if (g__serial_mp3_player->isAllPromptsPlayed() == false && (g__gestion.sub_state == GESTION_SUB_STATE_INIT_DONE || g__gestion.sub_state == GESTION_SUB_STATE_INIT_DONE2)) {
    long l__prompts_duration = g__timers->getPromptsDuration();

    if (g__timers->isInUse(TIMER_LONG_FAMINE_FIFO_TX_PLAY) == false) {
#if USE_REAL_PROMPTS_DURATION_FROM_TIME
      long l__const_duration   = DURATION_TIMER_SHORT_FAMINE;
      long l__duration         = (l__const_duration + 2L * l__prompts_duration);    // Marge d'un facteur 2 pour la durée des prompts

      sprintf(l__buffer, "Set 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY': [%ld + 2 * %ld] mS -> [%ld] mS\n",
        10L * l__const_duration, 10L * l__prompts_duration, 10L * l__duration);
#else
      /* TODO: Test si valeur nulle
       *       => Initialisation avec une valeur minimale
       *       => Utile uniquement pour les tests de coupure de diffusion en cours avec le timer associé...
       */
      long l__duration         = (150L * l__prompts_duration) / 100L;               // Enveloppe time-out de +50% 
      sprintf(l__buffer, "Set 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY': [%ld] mS -> [%ld] mS\n",
        10L * l__prompts_duration, 10L * l__duration);
#endif
      Serial.print(l__buffer);

      sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_SHORT_FAMINE_FIFO_TX_PLAY started [%ld] mS\n", (l__duration * 10));
      Serial.print(l__buffer);

      g__timers->start(TIMER_SHORT_FAMINE_FIFO_TX_PLAY, l__duration, &callback_short_famine_fifo_tx_play);
      g__timers->clrPromptsDuration();
      g__timers->clrNumPromptPlayed();
    }
    /*  Cas où de nouvelles diffusions de prompts ont été écrites dans la FIFO/Tx Play en mode "connected"
     *  et le timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY' est en cours d'exécution
     *  => TODO: Too many 'g__timers->addDuration()' calls at the initialization ?!..
     */
#if 0
    else if (g__timers->getNumPromptPlayed() && l__prompts_duration != 0L) {
      /*  'TIMER_SHORT_FAMINE_FIFO_TX_PLAY' in progress
       *  => Add the new value not null at the current value
       */
#if USE_REAL_PROMPTS_DURATION_FROM_TIME
      long l__duration = (2L * l__prompts_duration);            // Marge d'un facteur 2 pour la durée des prompts
#else
      long l__duration = (150L * l__prompts_duration) / 100L;   // Enveloppe time-out de +50% 
#endif
      g__timers->addDuration(TIMER_SHORT_FAMINE_FIFO_TX_PLAY, l__duration);
    }
#endif

    g__timers->start(TIMER_LONG_FAMINE_FIFO_TX_PLAY, DURATION_TIMER_LONG_FAMINE, &callback_long_famine_fifo_tx_play);
  }
  // Fin: Test de la famine de la FIFO Tx/Play uniquement si [r]établissement GPS
#endif

#ifndef USE_SIMULATION
  // Activité sur la FIFO Tx/Play non totalement lue
  if (g__serial_mp3_player->isAllPromptsPlayed() == false || g__gestion.sub_state == GESTION_SUB_STATE_INIT_IN_PROGRESS || g__gestion.sub_state == GESTION_SUB_STATE_INIT_RETRY) {
    if (g__timers->isInUse(TIMER_ACTIVITY_FIFO_TX_PLAY) == false) {
      if (g__stats->getGpsCounter(ENUM_STATS_GPS_NBR_OF_RETRY_ADVERT) < INDEX_FOR_BASCULE) {
        // Activité sur la Led YELLOW jusqu'à l'avant dernier "Veuillez toujours patienter"
        g__timers->start(TIMER_ACTIVITY_FIFO_TX_PLAY, DURATION_TIMER_FIFO_TX_PLAY, &callback_activity_fifo_tx_play);
      }
      else {
        // Forcçage extinction de la Led YELLOW
        if (g__sdcard->isInUse() == false && g__timers->isInUse(TIMER_SDCARD_ACCES) == false) {
          g__state_leds &= ~STATE_LED_YELLOW;
        }
      }
    }      
  }
  else {
    /*  La FIFO Tx/Play est totalement lue (pas de famine)
     *  => Arrêt des timers d'activité si en cours d'utilisation
     *  => Extinction de la Led YELLOW si timer d'activité 'TIMER_EXEC_SYNTH' en cours d'utilisation
     *  => Raz durée des prompts
     */
    if (g__timers->isInUse(TIMER_ACTIVITY_FIFO_TX_PLAY))     g__timers->stop(TIMER_ACTIVITY_FIFO_TX_PLAY);
    if (g__timers->isInUse(TIMER_SHORT_FAMINE_FIFO_TX_PLAY)) {
      Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_SHORT_FAMINE_FIFO_TX_PLAY stopped\n");
      g__timers->stop(TIMER_SHORT_FAMINE_FIFO_TX_PLAY);
    }
    if (g__timers->isInUse(TIMER_LONG_FAMINE_FIFO_TX_PLAY))  g__timers->stop(TIMER_LONG_FAMINE_FIFO_TX_PLAY);

    g__errors->setVeryLongFamine(false);    // Effacement de la présentation de l'erreur

    if (g__timers->isInUse(TIMER_EXEC_SYNTH) == false) {
      // Aucune activité famine reportée sur la Led Yellow
      if (g__sdcard->isInUse() == false && g__timers->isInUse(TIMER_SDCARD_ACCES) == false) {
        g__state_leds &= ~STATE_LED_YELLOW;
      }
    }

    if (g__gestion.flg_retry_last_sequence == true) {
      /*  Redifusion de la dernière séquence au moyen de la maj de 'idx_tx_read' de la FIFO/Tx Play
       *  au niveau du dernier évènement 'CMD_NUM_SEQ XX YY'
       */
      g__serial_mp3_player->reInitTxReadForRetry(TX_PLAY);
      g__gestion.flg_retry_last_sequence = false;

      /* Mémorisation de la position actuelle avec un 'range' global [0, 1, ..., N]
       * => TODO: Accueillir une structure de l'image de l'eeprom 
       */
      static uint8_t g__eeprom_coord_range = (uint8_t)-1;

      COORD l__coord_current;
      g__plots->getCoordCurrent(&l__coord_current);

      timerAlarmDisable(g__timer);
      g__gest_eeprom->push(l__coord_current.lat, l__coord_current.lon, l__coord_current.ele, ++g__eeprom_coord_range);
      timerAlarmEnable(g__timer);
      // Mémorisation de la position actuelle avec un 'range' global
    }
  }
  // Fin: Activité sur la FIFO Tx/Play non totalement lue
#endif

  // Test all the timers
  g__timers->test();
  // End: Test all the timers

#ifndef USE_SIMULATION
  // Allumage fixe de la Led Red sur 'TIMER_LONG_FAMINE_FIFO_TX_PLAY'
  if (g__errors->isVeryLongFamine() == true) {
    // g__state_leds |= STATE_LED_RED;
  }

  // Allumage fugitif de la Led Red sur 'TIMER_WT2003S_RESP_ERROR'
  if (g__timers->isInUse(TIMER_WT2003S_RESP_ERROR)) {
    g__state_leds |= STATE_LED_RED;    
  }
  else {
    /* Remarque: Extinction meme si une autre cause presente un allumage
     *           => TODO: A corriger d'une maniere generale avec une logique associee
     *                    comme la gestion d'un compteur de cause...
     */
    g__state_leds &= ~STATE_LED_RED;
  }
  // Fin: Allumage fugitif de la Led Red sur 'TIMER_WT2003S_RESP_ERROR'

  // Recopie physique des états Leds
#if USE_INVERSE_LEDS
  digitalWrite(LED_RED,    (g__state_leds & STATE_LED_RED)    ? LOW : HIGH);
  digitalWrite(LED_YELLOW, (g__state_leds & STATE_LED_YELLOW) ? LOW : HIGH);
  digitalWrite(LED_GREEN,  (g__state_leds & STATE_LED_GREEN)  ? LOW : HIGH);
  digitalWrite(LED_SDCARD, (g__state_leds & STATE_LED_SDCARD) ? LOW : HIGH);
#else
  digitalWrite(LED_RED,    (g__state_leds & STATE_LED_RED)    ? HIGH : LOW);
  digitalWrite(LED_YELLOW, (g__state_leds & STATE_LED_YELLOW) ? HIGH : LOW);
  digitalWrite(LED_GREEN,  (g__state_leds & STATE_LED_GREEN)  ? HIGH : LOW);
  digitalWrite(LED_SDCARD, (g__state_leds & STATE_LED_SDCARD) ? HIGH : LOW);
#endif
#endif

#if USE_INCOMING_CMD
extern void synthesisPromptsIter(byte *i__commands, size_t i__size);

  // Test de synthese de la date complete toutes les minutes
  if (g__synth_prompts_iter == true) {
    if (g__serial_nmea->isNewMinute() == true) {
      // Synthesis date and time...
      // Application de l'heure été/hiver
      ST_DATE_AND_TIME l__dateAndTime_presentation;
      memcpy(&l__dateAndTime_presentation, &g__dateAndTime, sizeof(ST_DATE_AND_TIME));
      applySommerWinterHour(&l__dateAndTime_presentation);

      byte     *l__commands = NULL;
      uint16_t *l__durations = NULL;
      size_t    l__nbr_durations = 0;

      size_t l__size = buildCommandsPromptsDateTime(&l__dateAndTime_presentation, &l__commands, SYNTH_DATE_TIME_ALL, &l__durations, &l__nbr_durations);

      synthesisPromptsIter(l__commands, l__size);
    }
    else {
      synthesisPromptsIter(NULL, 0);
    }
  }
#endif

  // TODO: A supprimer car etats m.a.j. directement sous It
  g__wt2003s->isPlayInProgress();

  g__wt2003s->treatmentResponse();

#if USE_INCOMING_CMD
  gestionOfCommands();
#endif
}

#if USE_INCOMING_CMD
void synthesisPrompts(byte *i__commands, size_t i__size)
{
  std::ostringstream l__out;
  hexDump(l__out, (const char *)i__commands, i__size);
  Serial.print(l__out.str().c_str());
  Serial.print("\n");

  int l__num_command = 0;
  size_t l__size = 0;
  for (l__size = 0; l__size < i__size; l__size += 3, l__num_command++) {
    // Test @ opcode du KT403A (synthese prompt dans un repertoire)
    if (*(i__commands + l__size) == 0x14) {
      // Construction du numero 'string' du fichier a synthetiser
      char l__file_name[5+1];
      memset(l__file_name, '\0', sizeof(l__file_name));

      // Abandon du repertoire + MSB:LSB sur 12 bits
      uint16_t l__num_file_in_root = 256 * (*(i__commands + l__size + 1) & 0x0F) + *(i__commands + l__size + 2);
      l__num_file_in_root += 7000;    // Offset de translation base sonore KT403A -> WT2003S
      sprintf(l__file_name, "%05d", l__num_file_in_root);

      Serial.printf("Command #%d: File name [\"%s\"]\n", l__num_command, l__file_name);

      byte l__hexa_datas[8];
      memset(l__hexa_datas, '\0', sizeof(l__hexa_datas));
      l__hexa_datas[0] = WT2003S_SD_PLAY_FILE_IN_ROOT;
      memcpy(&l__hexa_datas[1], l__file_name, 5);

      char l__buffer[16];
      size_t n = 0;
      for (n = 0; n < 6; n++) {
        sprintf(l__buffer, ((n == 0) ? ">>> [0x%02x" : "%02x"), l__hexa_datas[n]);
        Serial.print(l__buffer);
      }
      sprintf(l__buffer, "] (%d bytes)\n", n);
      Serial.print(l__buffer);

      // Write to 'WT2003S'...
      g__serial_mp3_player->writeToWT2003S(l__hexa_datas, n);

      // Awaiting 'Play Start' __/--
      g__wt2003s->waitPlayStart();

      // Awaiting 'Play End' __/--
      g__wt2003s->waitPlayEnd();
    }
    else {
      Serial.printf("Unknown opcode [0x%02x] != [0x%02x]\n", *(i__commands + l__size), 0x14);      
    }
  }
}

/* Version iterative de 'synthesisPrompts()' pouvant etre appelee en fond de tache
 * Warning: Methode a abandonner car incompatible avec la gestion par l'automate ;-)
 *          => Emission des commandes non veritablement synchrones
 *             => Mauvais fonctionnement des commandes / reponses attendu par l'automate  
 *
 *          => Utilisation de la mise en FIFO de la Tx/Play des que validee avec le WT2003S
*/
void synthesisPromptsIter(byte *i__commands, size_t i__size)
{
  static byte   *g__synth_prompts_commands = NULL;
  static size_t  g__synth_prompts_size_total = 0;
  static size_t  g__synth_prompts_size = 0;
  static int     g__synth_prompts_num_command = 0;

  if (i__commands != NULL) {
    g__synth_prompts_commands = i__commands;
    g__synth_prompts_size_total = i__size;
    g__synth_prompts_size = 0;
    g__synth_prompts_num_command = 0;
  }

  if (g__wt2003s->isPlayInProgress() == true) {
    // Diffusion en cours => Attente de sa fin...
    return;
  }

  if (g__synth_prompts_size < g__synth_prompts_size_total) {
    // Test @ opcode du KT403A (synthese prompt dans un repertoire)
    if (*(g__synth_prompts_commands + g__synth_prompts_size) == 0x14) {
      // Construction du numero 'string' du fichier a synthetiser
      char l__file_name[5+1];
      memset(l__file_name, '\0', sizeof(l__file_name));

      // Abandon du repertoire + MSB:LSB sur 12 bits
      uint16_t l__num_file_in_root = 256 * (*(g__synth_prompts_commands + g__synth_prompts_size + 1) & 0x0F) + *(g__synth_prompts_commands + g__synth_prompts_size + 2);
      l__num_file_in_root += 7000;    // Offset de translation base sonore KT403A -> WT2003S
      sprintf(l__file_name, "%05d", l__num_file_in_root);

      Serial.printf("Command #%d: File name [\"%s\"]\n", g__synth_prompts_num_command, l__file_name);

      byte l__hexa_datas[8];
      memset(l__hexa_datas, '\0', sizeof(l__hexa_datas));
      l__hexa_datas[0] = WT2003S_SD_PLAY_FILE_IN_ROOT;
      memcpy(&l__hexa_datas[1], l__file_name, 5);

      char l__buffer[16];
      size_t n = 0;
      for (n = 0; n < 6; n++) {
        sprintf(l__buffer, ((n == 0) ? ">>> [0x%02x" : "%02x"), l__hexa_datas[n]);
        Serial.print(l__buffer);
      }
      sprintf(l__buffer, "] (%d bytes)\n", n);
      Serial.print(l__buffer);

      // Write to 'WT2003S'...
      g__serial_mp3_player->writeToWT2003S(l__hexa_datas, n);

#if 1 // Test without avaiting...
      // Awaiting 'Play Start' __/--
      g__wt2003s->waitPlayStart();
#endif

      // Preparation prochaine synthese
      g__synth_prompts_size += 3;
      g__synth_prompts_num_command += 1;
    }
    else {
      Serial.printf("Unknown opcode [0x%02x] != [0x%02x]\n", *(g__synth_prompts_commands + g__synth_prompts_size), 0x14);      
    }
  }
}

void gestionOfCommands()
{
  char l__cmd_result[sizeof(g__incoming_buff)];

  // Gestion of commands
  if (Serial.available() > 0) {
    // Read the incoming byte
    int incomingByte = Serial.read();

    if (incomingByte == '\n') {
      g__incoming_buff[g__count] = '\0';
      g__count = 0;

      Serial.print(">>> [");
      Serial.print(g__incoming_buff);
      sprintf(l__cmd_result, "] (length: %d)\n", strlen(g__incoming_buff));
      Serial.print(l__cmd_result);

      // Interpretation of command
#if USE_STATS_SERIAL_NMEA
      if (!strcmp(g__incoming_buff, "Serial NMEA stats")) {
#ifndef USE_SIMULATION
        sprintf(l__cmd_result, ">>> Serial NMEA: Nbr Frames [%d] Errors [%d/%d/%d/%d/%d] Spare Min [%d]\n",
                g__serial_nmea->getNbrFramesNMEA(),
                g__serial_nmea->getErrFifo(0),    // La trame NMEA precedente n'a pas ete extraite
                g__serial_nmea->getErrFifo(1),    // Saturation FIFO => Reinit 'spare_min'
                g__serial_nmea->getErrCks(),
                g__serial_nmea->getErrNMEA(),
                g__serial_nmea->getErrMissingField(),
                g__serial_nmea->getSpareMin());

        Serial.print(l__cmd_result);

        g__serial_nmea->hexDumpFifoRx();
#else
        Serial.println("Warning: Command not simulated");
#endif
      }
      else
#endif
        if (!strcmp(g__incoming_buff, "Plots stats")) {
          g__plots->toString();
          g__plots->toStringAdjust();
          g__plots->toStringWaypoints();
          g__plots->toStringSegments();
          g__plots->toStringPositions(true);

#if USE_POS_COMMUNES        
          g__plots->toStringPosCommunes(false);
#endif
        }

#if USE_FORCE_CURRENT_COORD
        else if (!strncmp(g__incoming_buff, "ForceCoord", strlen("ForceCoord"))) {
          /*  Extraction of coord[+cap] in degrees (float xxx.y) and Cap optional
           *  ex: ForceCoord 48.78453 1.958657 [17.4]
           *  
           *  Remarque: La prise en compte s'effectuera au moment de l'acquisition
           *            des coordonnées réelles
           */
          strcpy(l__cmd_result, g__incoming_buff);
          float l__lat = 0.0;
          float l__lon = 0.0;
          float l__cap = 0.0;         // Init au "Nord" par défaut (optionnel dans la commande "ForceCoord")
          float l__speed_kmh = 4.0;   // Init à une randonnée "normale" par défaut (optionnel dans la commande "ForceCoord")
     
          initToNaN(&l__lat);
          initToNaN(&l__lon);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "ForceCoord" command
          l__pattern = strtok(NULL, " ");
          if (l__pattern != NULL) {
            l__lat = (float)strtod(l__pattern, NULL);
            l__pattern = strtok(NULL, " ");
            if (l__pattern != NULL) {
              l__lon = (float)strtod(l__pattern, NULL);
              l__pattern = strtok(NULL, " ");
              if (l__pattern != NULL) {
                l__cap = (float)strtod(l__pattern, NULL);
                l__pattern = strtok(NULL, " ");
                if (l__pattern != NULL) {
                  l__speed_kmh = (float)strtod(l__pattern, NULL);
                }
              }
            }
          }
          Serial.print("Force coord: Lat [");
          Serial.print(l__lat, 6);
          Serial.print("] Lon [");
          Serial.print(l__lon, 6);
          Serial.print("] Cap [");
          Serial.print(l__cap, 1);
          Serial.print("] Speed [");
          Serial.print(l__speed_kmh, 1);
          Serial.print("] KmH\n");

          g__force_coord_current.lat      = l__lat;
          g__force_coord_current.lon      = l__lon;
          g__force_coord_current.cap      = l__cap;
          g__force_coord_current.ele      = 0.0;            // Elevation not significative
          g__force_coord_current.speedKmH = l__speed_kmh;   // > 1.0 KmH for significative cap

          g__flg_force_coord_current = true;
        }
#endif
#if USE_FORCE_TIME
        else if (!strncmp(g__incoming_buff, "ForceTime", strlen("ForceTime"))) {
          // ForceTime {hour | 10mn | 5mn}
          boolean l__flg_pattern = true;
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "ForceTime" command
          l__pattern = strtok(NULL, " ");
          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "hour")) {
              g__flg_force_time_hour = true;
            }
            else if (!strcmp(l__pattern, "10mn")) {
              g__flg_force_time_10mn = true;
            }
            else if (!strcmp(l__pattern, "5mn")) {
              g__flg_force_time_5mn = true;
            }
            else {
              l__flg_pattern = false;
            }
          }
          Serial.print("Force time: [");
          Serial.print(l__pattern);
          if (l__flg_pattern == true) {
            sprintf(l__cmd_result, "] [%d] [%d] [%d]\n", g__flg_force_time_hour, g__flg_force_time_10mn, g__flg_force_time_5mn);
            Serial.print(l__cmd_result);
          }
          else {
            Serial.print("] Not supported\n");
          }
        }
#endif
        else if (!strncmp(g__incoming_buff, "SimuMove", strlen("SimuMove"))) {
          /* - SimuMove stop/start      # Arrêt/Lancement de la simulation
           * - SimuMove dd speed_kmh    # Lancement de la simulation à partir du segment #dd à la vitesse de xx KmH
           * - SimuMove clearStats      # Raz des statistiques
           */
          strcpy(l__cmd_result, g__incoming_buff);

          boolean l__flg_valid_command = false;
          int l__num_segment = 0;
          float l__speed_kmh = 0.0;
          initToNaN(&l__speed_kmh);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SimuMove" command
          l__pattern = strtok(NULL, " ");
          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "stop")) {
              g__plots->stopSimuMove();
              Serial.print("SimuMove: stop\n");
              l__flg_valid_command = true;
            }
            else if (!strcmp(l__pattern, "start")) {
              g__plots->startSimuMove();
              Serial.print("SimuMove: start\n");
              l__flg_valid_command = true;
            }
            else if (!strcmp(l__pattern, "clearStats")) {
#ifndef USE_SIMULATION
              g__stats->clearCineticInfos();
              Serial.print("SimuMove: clearStats\n");
              l__flg_valid_command = true;
#else
              Serial.println("Warning: Command not simulated");
#endif
            }
            else {
              l__num_segment = (int)strtol(l__pattern, NULL, 10);
              l__pattern = strtok(NULL, " ");
              if (l__pattern != NULL) {
                l__speed_kmh = (float)strtod(l__pattern, NULL);

                Serial.print("SimuMove:");
                if (l__num_segment >= 0 && l__speed_kmh == l__speed_kmh && l__speed_kmh >= 0.0) {
                  // 2 paramètres définis (SimuMove dd speed_kmh)
                  printf(l__cmd_result, "#%d ", l__num_segment);
                  Serial.print(l__cmd_result);
                  Serial.print(" Speed [");
                  Serial.print(l__speed_kmh, 1);
                  Serial.print("] KmH\n");

                  if (g__plots->isAvailable() == true && l__num_segment < g__plots->getNbrOfPlotsAdjust()) {
                    g__plots->setSimuMoveParameters(l__num_segment, l__speed_kmh);
                    g__simu_move_flg = true;

                    l__flg_valid_command = true;
                  }
                  else {
                    Serial.print("Err: Adjust Plots not available or #num_plot too large\n");
                  }
                }
              }
            }
          }  

          if (l__flg_valid_command == false) {
            // Aucun paramètre défini, correct ou 'adjust' plots non disponible
            Serial.print(" Warning: Command a/o parameter(s) not accepted\n");
          }
        }
        // End: SimuMove

        else if (!strncmp(g__incoming_buff, "ForcePurgeTx", strlen("ForcePurgeTx"))) {
#ifndef USE_SIMULATION
          // ForcePurgeTx without parameter
          sprintf(l__cmd_result, "Force Purge Tx: [%d]\n", g__serial_mp3_player->isAllPromptsPlayed());
          Serial.print(l__cmd_result);

          if (g__serial_mp3_player->isAllPromptsPlayed() == false) {
            Serial.print("\t=> Purge FIFO Tx/Play...\n");

            g__serial_mp3_player->purgeFIFOTxPlay();
          }
#else
          Serial.println("Warning: Command not simulated");
#endif
        }

#if USE_TEST_DIRECT_EEPROM
        /* Test direct de l'EEPROM
         * - begin:  Accès aux xxx bytes < EEPROM_SIZE
         * - size:   Taille défine et utile de l'eeprom
         * - read:   Lecture d'un byte à une adresse donnée
         * - write:  Ecriture d'un byte à une adresse donnée
         * - commit: "Commit" des valeurs nouvellement écrites
         * - end:    Fin de l'accès à l'eeprom (un "commit" est exécuté ;-)
         * - dump:   "Dump" de l'eeprom sur la longueur utile
         */
        else if (!strncmp(g__incoming_buff, "EEPROM", strlen("EEPROM"))) {
#ifndef USE_SIMULATION
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "EEPROM" command
          l__pattern = strtok(NULL, " ");

          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "begin")) {
              l__pattern = strtok(NULL, " ");    // Size
              if (l__pattern != NULL) {
                size_t l__size = (size_t)strtol(l__pattern, NULL, 10);

                timerAlarmDisable(g__timer);
                bool l__flg_rtn = g__eeprom->begin(l__size);
                timerAlarmEnable(g__timer);
              
                sprintf(l__cmd_result, "\t-> Begin size [%d] => Return [%d]\n", l__size, l__flg_rtn);
                Serial.print(l__cmd_result);
              }
            }
            else if (!strcmp(l__pattern, "size")) {
              timerAlarmDisable(g__timer);
              size_t l__length = g__eeprom->length();
              size_t l__useful_length = g__eeprom->useful_length();
              timerAlarmEnable(g__timer);

              sprintf(l__cmd_result, "\t-> Length => Return [%d]\n", l__length);
              Serial.print(l__cmd_result);
              sprintf(l__cmd_result, "\t-> Useful length => Return [%d]\n", l__useful_length);
              Serial.print(l__cmd_result);
            }
            else if (!strcmp(l__pattern, "read")) {
              l__pattern = strtok(NULL, " ");    // Address
              if (l__pattern != NULL) {
                int l__address = (int)strtol(l__pattern, NULL, 16);

                timerAlarmDisable(g__timer);
                uint8_t l__value = g__eeprom->readByte(l__address);
                timerAlarmEnable(g__timer);

                sprintf(l__cmd_result, "\t-> Read address [0x%x] Value [0x%x]\n", l__address, l__value);
                Serial.print(l__cmd_result);
              }
            }
            else if (!strcmp(l__pattern, "write")) {
              l__pattern = strtok(NULL, " ");    // Address
              if (l__pattern != NULL) {
                int l__address = (int)strtol(l__pattern, NULL, 16);
                l__pattern = strtok(NULL, " ");    // Value
                if (l__pattern != NULL) {
                  int l__value = (int)strtol(l__pattern, NULL, 16);
                  sprintf(l__cmd_result, "\t-> Write address [0x%x] Value [0x%x]\n", l__address, l__value);
                  Serial.print(l__cmd_result);

                  timerAlarmDisable(g__timer);
                  bool l__flg_rtn = g__eeprom->writeByte(l__address, l__value);
                  timerAlarmEnable(g__timer);

                  sprintf(l__cmd_result, "\t\t=> Return [%d]\n", l__flg_rtn);
                  Serial.print(l__cmd_result);
                }
              }
            }
            else if (!strcmp(l__pattern, "dump")) {
              timerAlarmDisable(g__timer);
              size_t l__useful_length = g__eeprom->useful_length();
              timerAlarmEnable(g__timer);

              if (l__useful_length != 0) {
                sprintf(l__cmd_result, "\t-> Content of EEPROM (%d useful bytes)\n", l__useful_length);
                Serial.print(l__cmd_result);

                // TODO: Use 'EEPROMClass::readBytes(int address, void* value, size_t maxLen)' method
                uint8_t *l__values = (uint8_t*)malloc(l__useful_length);
                size_t n = 0;
                for (n = 0; n < l__useful_length; n++) {
                  timerAlarmDisable(g__timer);
                  *(l__values + n) = g__eeprom->readByte((int)n);
                  timerAlarmEnable(g__timer);
                }

                std::ostringstream l__out;
                hexDump(l__out, (const char *)l__values, l__useful_length);
                Serial.print(l__out.str().c_str());
                Serial.print("\n");

                free(l__values);
              }
              else {
                Serial.print("No acces to EEPROM\n");
              }
            }
            else if (!strcmp(l__pattern, "commit")) {
              timerAlarmDisable(g__timer);
              bool l__flg_rtn = g__eeprom->commit();
              timerAlarmEnable(g__timer);

              sprintf(l__cmd_result, "\t-> Commit => Return [%d]\n", l__flg_rtn);
              Serial.print(l__cmd_result);
            }
            else if (!strcmp(l__pattern, "end")) {
              timerAlarmDisable(g__timer);
              bool l__flg_rtn = g__eeprom->end();
              timerAlarmEnable(g__timer);

              sprintf(l__cmd_result, "\t-> End => Return [%d]\n", l__flg_rtn);
              Serial.print(l__cmd_result);
            }
            else {
              Serial.print("Invalid sub command\n");
            }
          }
          else {
            Serial.print("No sub command found\n");
          }
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
        // Fin: Test direct de l'EEPROM
#else
        /* Test de l'EEPROM via la classe 'GestionEEPROM'
         * - size:   Taille défine et utile de l'eeprom
         * - read:   Lecture d'un byte à une adresse donnée
         * - write:  Ecriture d'un byte à une adresse donnée   (Warning: Intégrité de l'eeprom)
         * - end:    Fin de l'accès à l'eeprom (un "commit" est exécuté ;-)
         * - dump:   "Dump" de l'eeprom sur la longueur utile
         * - string: "String" du contenu de l'eeprom (interprétation du contenu)
         */
        else if (!strncmp(g__incoming_buff, "EEPROM", strlen("EEPROM"))) {
#ifndef USE_SIMULATION
#if 0
          static GestionEEPROM *g__gest_eeprom = NULL;

          if (g__gest_eeprom == NULL) {
            g__gest_eeprom = new GestionEEPROM("eeprom", EEPROM_SIZE);
          }
#endif
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "EEPROM" command
          l__pattern = strtok(NULL, " ");

          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "init")) {
              l__pattern = strtok(NULL, " ");    // Version "x.y.z"

              char const *l__version = "0.0.0";
              if (l__pattern != NULL) {
                l__version = l__pattern;
              }

              timerAlarmDisable(g__timer);
              bool l__init = g__gest_eeprom->init(l__version);
              timerAlarmEnable(g__timer);

              sprintf(l__cmd_result, "\t-> Return [%d]\n", l__init);
              Serial.print(l__cmd_result);
            }
            else if (!strcmp(l__pattern, "size")) {
              timerAlarmDisable(g__timer);
              size_t l__length = g__gest_eeprom->length();
              timerAlarmEnable(g__timer);

              sprintf(l__cmd_result, "\t-> Length => Return [%d]\n", l__length);
              Serial.print(l__cmd_result);
            }
            else if (!strcmp(l__pattern, "read")) {
              l__pattern = strtok(NULL, " ");    // Address
              if (l__pattern != NULL) {
                int l__address = (int)strtol(l__pattern, NULL, 16);

                timerAlarmDisable(g__timer);
                uint8_t l__value = g__gest_eeprom->readByte(l__address);
                timerAlarmEnable(g__timer);

                sprintf(l__cmd_result, "\t-> Read address [0x%x] Value [0x%x]\n", l__address, l__value);
                Serial.print(l__cmd_result);
              }
            }
            else if (!strcmp(l__pattern, "write")) {
              l__pattern = strtok(NULL, " ");    // Address
              if (l__pattern != NULL) {
                int l__address = (int)strtol(l__pattern, NULL, 16);
                l__pattern = strtok(NULL, " ");    // Value
                if (l__pattern != NULL) {
                  int l__value = (int)strtol(l__pattern, NULL, 16);
                  sprintf(l__cmd_result, "\t-> Write address [0x%x] Value [0x%x]\n", l__address, l__value);
                  Serial.print(l__cmd_result);

                  timerAlarmDisable(g__timer);
                  bool l__flg_rtn = g__gest_eeprom->writeByte(l__address, l__value);
                  timerAlarmEnable(g__timer);

                  sprintf(l__cmd_result, "\t\t=> Return [%d]\n", l__flg_rtn);
                  Serial.print(l__cmd_result);
                }
              }
            }
            else if (!strcmp(l__pattern, "push")) {
              // Extract: EEPROM push <lat> <lon> <ele>
              float l__lat = 0.0;
              float l__lon = 0.0;
              float l__ele = 0.0;
              uint8_t l__range = 0;

              initToNaN(&l__lat);
              initToNaN(&l__lon);
              initToNaN(&l__ele);

              l__pattern = strtok(NULL, " ");    // Lat
              if (l__pattern != NULL) {
                l__lat = (float)strtod(l__pattern, NULL);

                l__pattern = strtok(NULL, " ");    // Lon
                if (l__pattern != NULL) {
                  l__lon = (float)strtod(l__pattern, NULL);

                  l__pattern = strtok(NULL, " ");    // Ele
                  if (l__pattern != NULL) {
                    l__ele = (float)strtod(l__pattern, NULL);

                    l__pattern = strtok(NULL, " ");    // Range
                    if (l__pattern != NULL) {
                      l__range = (uint8_t)strtol(l__pattern, NULL, 10);
                    }
                  }
                }
              }

              Serial.print(">>> Lat [");
              Serial.print(l__lat, 6);
              Serial.print("] Lon [");
              Serial.print(l__lon, 6);
              Serial.print("] Ele [");
              Serial.print(l__ele, 1);
              Serial.print("] Range [");
              Serial.print(l__range);
              Serial.print("]...\n");

              timerAlarmDisable(g__timer);
              bool l__flg_rtn = g__gest_eeprom->push(l__lat, l__lon, l__ele, l__range);
              timerAlarmEnable(g__timer);

              Serial.print("\t\t=> Return [");
              Serial.print(l__flg_rtn);
              Serial.print("]\n");
            }
            else if (!strcmp(l__pattern, "pop")) {
              // Read the last Lat, Lon and Ele values
              int   l__pos = 0;
              l__pattern = strtok(NULL, " ");    // Pos
                if (l__pattern != NULL) {
                  l__pos = (int)strtol(l__pattern, NULL, 10);
              }

              uint8_t l__idx = -1;
              float l__lat = 0.0;
              float l__lon = 0.0;
              float l__ele = 0.0;
              uint8_t l__range = (uint8_t)-1;

              initToNaN(&l__lat);
              initToNaN(&l__lon);
              initToNaN(&l__ele);

              sprintf(l__cmd_result, ">>> Pos [%d]\n", l__pos);
              Serial.print(l__cmd_result);

              timerAlarmDisable(g__timer);
              bool l__flg_rtn = g__gest_eeprom->pop((int *)&l__idx, &l__lat, &l__lon, &l__ele, &l__range, l__pos);
              timerAlarmEnable(g__timer);

              if (l__flg_rtn == true) {
                Serial.print("\t\t=> Return Idx [");
                Serial.print(l__idx);
                Serial.print("] Lat [");
                Serial.print(l__lat, 6);
                Serial.print("] Lon [");
                Serial.print(l__lon, 6);
                Serial.print("] Ele [");
                Serial.print(l__ele, 1);
                Serial.print("] Range [");
                Serial.print(l__range, 1);
                Serial.print("] Rtn [");
                Serial.print(l__flg_rtn);
                Serial.print("]\n");
              }
              else {
                Serial.print("\t\t=> No infos:");
                Serial.print(" Rtn [");
                Serial.print(l__flg_rtn);
                Serial.print("]\n");
              }
            }
            else if (!strcmp(l__pattern, "inc_idx")) {
              // Read the increment
              int l__inc = 0;
              l__pattern = strtok(NULL, " ");    // Increment en hexa
                if (l__pattern != NULL) {
                  l__inc = (int)strtol(l__pattern, NULL, 16);
              }

              sprintf(l__cmd_result, ">>> Inc Idx [%d]\n", l__inc);
              Serial.print(l__cmd_result);

              timerAlarmDisable(g__timer);
              bool l__flg_rtn = g__gest_eeprom->inc_idx(l__inc);
              timerAlarmEnable(g__timer);

              Serial.print("\t\t=> Return [");
              Serial.print(l__flg_rtn);
              Serial.print("]\n");
            }
            else if (!strcmp(l__pattern, "dump")) {
              std::ostringstream l__out;
 
              timerAlarmDisable(g__timer);
              g__gest_eeprom->toDump(l__out);
              timerAlarmEnable(g__timer);

              Serial.print(l__out.str().c_str());
              Serial.print("\n");
            }
            else if (!strcmp(l__pattern, "print")) {
              timerAlarmDisable(g__timer);
              g__gest_eeprom->printValues();
              timerAlarmEnable(g__timer);
            }
            else if (!strcmp(l__pattern, "end")) {
              if (g__gest_eeprom != NULL) {
                timerAlarmDisable(g__timer);
                delete g__gest_eeprom;
                timerAlarmEnable(g__timer);
              }

              g__gest_eeprom = NULL;    // End of 'eeprom' instance
            }
            else {
              Serial.print("Invalid sub command\n");
            }
          }
          else {
            Serial.print("No sub command found\n");
          }
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
        // Fin: Test de l'EEPROM via la classe 'GestionEEPROM'
#endif
        // Test ESP32 commands
        else if (!strncmp(g__incoming_buff, "ESP32", strlen("ESP32"))) {
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "ESP32" command
          l__pattern = strtok(NULL, " ");                   // Get the sub-command
          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "restart")) {           // 'restart' sub-command
              // Software reboot
              esp_restart();
              //ESP.restart();
            }
            else if (!strcmp(l__pattern, "reset_reason")) {      // 'reset_reason' sub-command
              // Raison du 'reboot'
              getBootReasonMessage();
            }
            else if (!strcmp(l__pattern, "disable_timer")) {      // 'disable_timer' sub-command
              timerAlarmDisable(g__timer);                        // Desabling this timer
            }
            else if (!strcmp(l__pattern, "enable_timer")) {       // 'enable_timer' sub-command
              timerAlarmWrite(g__timer, 500, true);               // Generate an interrupt each mS (500 uS) + reload
              timerAlarmEnable(g__timer);                         // Enabling this timer
            }
            else {
              Serial.print("No sub command found\n");
            }
          }
        }
        // Fin: ESP32 commands

        else if (!strncmp(g__incoming_buff, "setMenuParameters", strlen("setMenuParameters"))) {
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "setMenuParameters" command

          bool l__flg_params = false;
          int l__nbr_increments = 0;
          int l__duration_idle = 0;
          int l__duration_button = 0;

          l__pattern = strtok(NULL, " ");                   // Get the 'nbr_increments'
          if (l__pattern != NULL) {
            l__nbr_increments = (int)strtol(l__pattern, NULL, 10);

            l__pattern = strtok(NULL, " ");                 // Get 'duration_idle'
            if (l__pattern != NULL) {
              l__duration_idle = (int)strtol(l__pattern, NULL, 10);

              l__pattern = strtok(NULL, " ");               // Get 'duration_button'
              if (l__pattern != NULL) {
                l__duration_button = (int)strtol(l__pattern, NULL, 10);

                Serial.printf("\t=> Call setMenuParameters(%d, %d, %d)\n", l__nbr_increments, l__duration_idle, l__duration_button);
                l__flg_params = true;
              }     
            }
          }

          if (l__flg_params) {
            g__rotary_encoder->setMenuParameters(l__nbr_increments, l__duration_idle, l__duration_button);
          }
          else {
            Serial.print("\t=> setMenuParameters(): Missing parameter(s)\n");
          }
        }

        /* Test de la SDCard
         * o Classe 'SDCard'
         * - init:                         Initialisation
         * - end:                          Terminaison (permet une reprise apres les erreurs de la SD Card)
         * - setInhAppendGpsFrame <flg>:   Set/Reset inhibition of appending in 'GpsFrames.txt' (<flg> optional - true by default)
         * - printInfos:                   Informations (type et taille)
         * - listdir <dir>:                Liste d'un repertoire donne (ie. /, /PILOT, etc.)
         * - exists <path>:                Existence d'un repertoire ou d'un fichier
         * - readFile <file>:              Lecture et impression d'un fichier donne (ie. /PILOT/GpsPilot.txt)
         * - getFileLine <file> <nbr_lines>: Lectures successives d'une ligne d'un fichier
         * - appendFile <file> <patterns>: Concatenation d'un fichier donne avec des patterns constituant une ligne (ie. /LOG/log.txt ajout d'une ligne)
         * - renameFile <path_from> <path_to>: Renommage d'un repertoire ou d'un fichier
         * - deleteFile <file>:            Suppression d'un fichier

         * o Classe 'FileGpsPilot'
         * - getFileLines [<file>]: Ouverture, lecture et analyse des enregistrements de 'file' si non NULL; sinon 'NAME_OF_FILE_GPS_PILOT'
         */
        else if (!strncmp(g__incoming_buff, "SDCard", strlen("SDCard"))) {
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SDCard" command
          l__pattern = strtok(NULL, " ");                   // Get the sub-command
          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "init")) {              // 'init' sub-command
              bool l__flg_rtn = g__sdcard->init();
              Serial.printf("\t=> init(): [%s]\n", (l__flg_rtn == true) ? "Ok" : "Ko");
            }
            else if (!strcmp(l__pattern, "end")) {              // 'end' sub-command
              g__sdcard->end();
              Serial.printf("\t=> end(): No rtn code\n");
            }
            else if (!strcmp(l__pattern, "printInfos")) {   // 'printInfos' sub-command
              bool l__flg_rtn = g__sdcard->printInfos();
              Serial.printf("\t=> printInfos(): [%s]\n", (l__flg_rtn == true) ? "Ok" : "Ko");
            }
            else if (!strcmp(l__pattern, "listDir")) {      // 'listDir' sub-command
              l__pattern = strtok(NULL, " ");               // Get the directory name
              const char *l__dir = "/";                     // Root by default
              if (l__pattern != NULL) {
                l__dir = l__pattern;
              }
              bool l__flg_rtn = g__sdcard->listDir(l__dir);
              Serial.printf("\t=> listDir(%s): [%s]\n", l__dir, (l__flg_rtn == true) ? "Ok" : "Ko");
            }
            else if (!strcmp(l__pattern, "exists")) {        // 'exists' sub-command
              l__pattern = strtok(NULL, " ");                // Get the path name
              if (l__pattern != NULL) {
                const char *l__path = l__pattern;
                bool l__flg_rtn = g__sdcard->exists(l__path);
                Serial.printf("\t=> exists(%s): [%s]\n", l__path, (l__flg_rtn == true) ? "Ok" : "Ko");              
              }
              else {
                Serial.printf("\t=> exists(): Missing file name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "readFile")) {      // 'readFile' sub-command
              l__pattern = strtok(NULL, " ");                // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;
                bool l__flg_rtn = g__sdcard->readFile(l__file);
                Serial.printf("\t=> readFile(%s): [%s]\n", l__file, (l__flg_rtn == true) ? "Ok" : "Ko");              
              }
              else {
                Serial.printf("\t=> readFile(): Missing file name\n");
              }
            }
            else if (!strcmp(l__pattern, "getFileLine")) {   // 'getFileLine' sub-command
              l__pattern = strtok(NULL, " ");                // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;

                l__pattern = strtok(NULL, " ");               // Get the nbr of lines
                if (l__pattern != NULL) {
                  int l__nbr_lines = (int)strtol(l__pattern, NULL, 10);

                  for (int n = 0; n < l__nbr_lines; n++) {
                    String l__line = "";

                    bool l__flg_rtn = g__sdcard->getFileLine(
                      (n == 0) ? l__file : NULL,                  // Passage du nom du fichier sur la 1st demande
                      l__line,
                      (n == (l__nbr_lines - 1)) ? true : false);  // Cloture du fichier sur la derniere demande

                    Serial.printf("\t=> #%d: getFileLine(%s): [%s] [%s]\n", n, l__file, (l__flg_rtn == true) ? "Ok" : "Ko", l__line.c_str());

                    // Si trame GPS, extraction du type 'I' suivi de DDMMYY (pour le renommage du fichier 'GpsFrames.txt')
                    if (!strncmp(l__line.c_str(), "AA", 2)) {
                      unsigned char l__cks_calculated = 0xff;
                      boolean l__cks_rtn = g__serial_nmea->calculChecksum((char *)l__line.c_str(), &l__cks_calculated);

                      Serial.printf("\t\t=> Cks [0x%02x] %s\n", l__cks_calculated, (l__cks_rtn == true ? "Ok" : "Ko"));

                      if (l__cks_rtn == true) {
                        char *l__pattern = strchr(l__line.c_str(), 'I');  // Type 'I' (date "I6DDMMYY" - 6 char)
                        if (l__pattern != NULL) {
                          char l__t_date[6+1];
                          memset(l__t_date, '\0', sizeof(l__t_date));
                          strncpy(l__t_date, l__pattern + 2, 6);          // Skip type and length fields

                          Serial.printf("\t\t\t=> Date [%s]\n", l__t_date);
                        }
                        else {
                          Serial.printf("\t\t\t=> No Date found\n");
                        }
                      }
                    }
                  }
                }
                else {
                  Serial.printf("\t=> getFileLine(): Missing nbr lines\n");
                }
              }
              else {
                Serial.printf("\t=> getFileLine(): Missing file name\n");
              }
            }
            else if (!strcmp(l__pattern, "appendFile")) {     // 'appendFile' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;
                String l__line = "";
                l__pattern = strtok(NULL, " ");               // Get the line
                while (l__pattern != NULL) {
                  l__line += l__pattern;
                  l__pattern = strtok(NULL, " ");             // Cont'd ...
                  if (l__pattern != NULL) {
                    l__line += " ";                           // ... with ' ' separator
                  }
                }
                l__line += "\n";                              // LF terminal for each line

                bool l__flg_rtn = g__sdcard->appendFile(l__file, l__line.c_str());
                Serial.printf("\t=> appendFile([%s], [%s]): [%s]\n", l__file, l__line.c_str(), (l__flg_rtn == true) ? "Ok" : "Ko");              
              }
              else {
                Serial.printf("\t=> appendFile(): Missing file name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "renameFile")) {     // 'renameFile' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the 1st path name
              if (l__pattern != NULL) {
                const char *l__path_from = l__pattern;
                l__pattern = strtok(NULL, " ");               // Get the 2nd path name
                if (l__pattern != NULL) {
                  const char *l__path_to = l__pattern;

                  bool l__flg_rtn = g__sdcard->renameFile(l__path_from, l__path_to);
                  Serial.printf("\t=> renamedFile([%s], [%s]): [%s]\n", l__path_from, l__path_to, (l__flg_rtn == true) ? "Ok" : "Ko");
                }
                else {
                  Serial.printf("\t=> renameFile(): Missing 2nd path name\n");                              
                }
              }
              else {
                Serial.printf("\t=> renameFile(): Missing 1st path name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "deleteFile")) {     // 'deleteFile' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the file name
              if (l__pattern != NULL) {
                const char *l__file = l__pattern;

                bool l__flg_rtn = g__sdcard->deleteFile(l__file);
                Serial.printf("\t=> deleteFile(%s): [%s]\n", l__file, (l__flg_rtn == true) ? "Ok" : "Ko");
              }
              else {
                Serial.printf("\t=> deleteFile(): Missing file name\n");                              
              }
            }
            else if (!strcmp(l__pattern, "setInhAppendGpsFrame")) {      // 'setInhAppendGpsFrame' sub-command
              l__pattern = strtok(NULL, " ");                            // Get the flag (true/false pattern)
              if (l__pattern != NULL) {
                if (!strcmp(l__pattern, "false")) {
                  g__sdcard->setInhAppendGpsFrame(false);
                }
                else {
                  g__sdcard->setInhAppendGpsFrame(true);
                }   
              }
              else {
                g__sdcard->setInhAppendGpsFrame(true);
              }
            }
            else if (!strcmp(l__pattern, "getFileLines")) {   // 'getFileLines' sub-command
              char *l__file = NULL;
              l__pattern = strtok(NULL, " ");                 // Get the file name if exists
              if (l__pattern != NULL) {
                l__file = l__pattern;
              }
              g__file_gpspilot->getFileLines(l__file, true);
            }
          }
          else {
            Serial.printf("Invalid sub-command (%s)\n", l__pattern);
          }
        }
        // Test de la SDCard

#if USE_FORCE_NO_GPS
        // Pilotage de l'etat de forcage de la reception GPS
        else if (!strncmp(g__incoming_buff, "ForceNoGps", strlen("ForceNoGps"))) {
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "ForceNoGps" command
          l__pattern = strtok(NULL, " ");                   // Get the sub-command
          const char *l__text = "false";
          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "true")) {
              g__serial_nmea->setFlgForceNoGps(true);
              l__text = "true";
            }
            else {
              g__serial_nmea->setFlgForceNoGps(false);
            }   
          }
          Serial.printf("\t=> setFlgForceNoGps(%s)\n", l__text);
        }
        // Fin: Pilotage de l'etat de forcage de la reception GPS
#endif
        // Test menu with the Rotary Encoder + Button
        else if (!strncmp(g__incoming_buff, "TestMenu", strlen("TestMenu"))) {
          strcpy(l__cmd_result, g__incoming_buff);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "TestMenu" command
          l__pattern = strtok(NULL, " ");                   // Get the sub-command
          if (l__pattern != NULL) {
            if (!strcmp(l__pattern, "force_in_progress")) {   // 'force_in_progress' sub-command
              l__pattern = strtok(NULL, " ");                 // Get the flag (true/false pattern)
              const char *l__text = "false";
              if (l__pattern != NULL) {
                if (!strcmp(l__pattern, "true")) {
                  g__menus->setFlgInProgress(true);
                  l__text = "true";
                }
                else {
                  g__menus->setFlgInProgress(false);
                }   
              }
              Serial.printf("\t=> setFlgInProgress(%s)\n", l__text);
            }            
          }
        }
        // End: Test menu with the Rotary Encoder + Button

        else if (!strcmp(g__incoming_buff, "Serial MP3 dump FIFO/Rx")) {
#ifndef USE_SIMULATION
          g__serial_mp3_player->hexDumpFifoRx();
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
        else if (!strcmp(g__incoming_buff, "Serial MP3 dump FIFO/Tx")) {
#ifndef USE_SIMULATION
          g__serial_mp3_player->hexDumpFifoTx();
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
        else if (!strcmp(g__incoming_buff, "WT2003S dump FIFO/Rx")) {
#ifndef USE_SIMULATION
          g__wt2003s->hexDumpFifoRx();
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
        else if (!strcmp(g__incoming_buff, "Stats")) {
#ifndef USE_SIMULATION
          g__stats->printAll();
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
        else if (!strncmp(g__incoming_buff, "Errors", strlen("Errors"))) {
          ENUM_ERRORS l__errors[NBR_MAX_ERRORS];
          memset(l__errors, '\0', sizeof(l__errors));
          strcpy(l__cmd_result, g__incoming_buff);
          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "Errors" command

          byte l__idx = 0;
          l__pattern = strtok(NULL, " ");     // 1st error
          while (l__pattern != NULL && l__idx < NBR_MAX_ERRORS) {
            l__errors[l__idx++] = (ENUM_ERRORS)strtol(l__pattern, NULL, 0);
            l__pattern = strtok(NULL, " ");   // Next error
          }

          if (l__idx != 0) {
            sprintf(l__cmd_result, ">>> %d errors:\n", l__idx);
          }
          else {
            sprintf(l__cmd_result, ">>> No errors (clear all)\n");
            g__errors->clear();
          }
          Serial.print(l__cmd_result);

          byte n = 0;
          for (n = 0; n < l__idx; n++) {
            sprintf(l__cmd_result, "\t#%d: [%d - 0x%02x]\n", n, l__errors[n], l__errors[n]);
            Serial.print(l__cmd_result);

            sprintf(l__cmd_result, ">>> update(0x%02x) error\n", l__errors[n]);
            Serial.print(l__cmd_result);

            g__errors->update(l__errors[n]);
          }

          // Présentation dans les 3 secondes
          g__timers->stop(TIMER_FOR_ERROR_1);
          g__timers->stop(TIMER_FOR_ERROR_2);
          g__timers->stop(TIMER_FOR_ERROR_3);
          g__timers->start(TIMER_FOR_ERROR_1, (3 * 100L), &callback_activity_error_1);
          g__state_leds &= ~STATE_LED_RED;
        }
        else if (!strncmp(g__incoming_buff, "SynthesisDist", strlen("SynthesisDist"))) {
          // Extraction of #0: distance in meters, #1: 'ENUM_SYNTH_POSITION_MODES' and #2: 'ENUM_SYNTH_DISTANCE_MODES' 
          strcpy(l__cmd_result, g__incoming_buff);

          uint32_t l__distance = 0;
          ENUM_SYNTH_POSITION_MODES l__pos_mode  = SYNTH_POSITION;                // Synthèse de la position ...
          ENUM_SYNTH_DISTANCE_MODES l__dist_mode = SYNTH_DIST_FROM_STARTING_POS;  // ... de départ

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SynthesisDist" command
          byte l__idx = 0;
          l__pattern = strtok(NULL, " ");
          while (l__pattern != NULL && l__idx < 3) {
            switch (l__idx) {
            case 0:
              l__distance = (uint32_t)strtol(l__pattern, NULL, 10);
              break;
            case 1:
              l__pos_mode  = (ENUM_SYNTH_POSITION_MODES)strtol(l__pattern, NULL, 10);
              break;
            case 2:
              l__dist_mode = (ENUM_SYNTH_DISTANCE_MODES)strtol(l__pattern, NULL, 10);
              break;
            default:
              break;
            }
            l__pattern = strtok(NULL, " ");   // Next field
            l__idx++;
          }
          sprintf(l__cmd_result, ">>> Synthesis distance Ori. [%d] Pos. [%d] Dist [%d] m\n",
            l__dist_mode, l__pos_mode, l__distance);
          Serial.print(l__cmd_result);

          synthesisDistance(l__distance, l__pos_mode, l__dist_mode);
        }
        else if (!strncmp(g__incoming_buff, "SynthesisCapHours", strlen("SynthesisCapHours"))) {
          // Extraction of cap in degrees (float xxx.y)
          strcpy(l__cmd_result, g__incoming_buff);

          float l__cap = 0;

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SynthesisCapHours" command
          l__pattern = strtok(NULL, " ");
          l__cap = (float)strtod(l__pattern, NULL);

          sprintf(l__cmd_result, ">>> Synthesis Cap Hours [");
          Serial.print(l__cmd_result);
          Serial.print(l__cap, 1);
          sprintf(l__cmd_result, "] degrees\n");
          Serial.print(l__cmd_result);

          synthesisCapHours(l__cap);
        }
        else if (!strncmp(g__incoming_buff, "SynthesisCap", strlen("SynthesisCap"))) {
          // Extraction of cap in degrees (float xxx.y)
          strcpy(l__cmd_result, g__incoming_buff);

          float l__cap = 0;

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SynthesisCap" command
          l__pattern = strtok(NULL, " ");
          l__cap = (float)strtod(l__pattern, NULL);

          sprintf(l__cmd_result, ">>> Synthesis Cap [");
          Serial.print(l__cmd_result);
          Serial.print(l__cap, 1);
          sprintf(l__cmd_result, "] degrees\n");
          Serial.print(l__cmd_result);

          synthesisCap(l__cap);
        }
        else if (!strncmp(g__incoming_buff, "SynthesisElevation", strlen("SynthesisElevation"))) {
          // Extraction of #0: current and #1: reference
          strcpy(l__cmd_result, g__incoming_buff);

          float l__ele_current; initToNaN(&l__ele_current);
          float l__ele_ref;     initToNaN(&l__ele_ref);

          ENUM_SYNTH_ELE_MODES l__ele_mode = SYNTH_NO_ELEVATION;

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SynthesisElevation" command
          l__pattern = strtok(NULL, " ");
          if (l__pattern != NULL) {
              l__ele_current = (float)strtod(l__pattern, NULL);
              l__pattern = strtok(NULL, " ");
              if (l__pattern != NULL) {
                l__ele_ref = (float)strtod(l__pattern, NULL);

                l__pattern = strtok(NULL, " ");
                if (l__pattern != NULL) {
                  l__ele_mode = (ENUM_SYNTH_ELE_MODES)strtol(l__pattern, NULL, 0);
                }
              }
          }

          sprintf(l__cmd_result, ">>> Synthesis Elevation Current [");
          Serial.print(l__cmd_result);
          Serial.print(l__ele_current, 1);
          sprintf(l__cmd_result, "] Ref. [");
          Serial.print(l__cmd_result);
          Serial.print(l__ele_ref, 1);
          sprintf(l__cmd_result, "]\n");
          Serial.print(l__cmd_result);

          synthesisElevation(l__ele_current, l__ele_ref, l__ele_mode);
        }
        else if (!strncmp(g__incoming_buff, "SetAndSynthesisDistToStartingPos", strlen("SetAndSynthesisDistToStartingPos"))) {
          // Extraction of #0: latitude in decimal degres and #1: longitude in decimal degres
          strcpy(l__cmd_result, g__incoming_buff);

          float l__lat; initToNaN(&l__lat);
          float l__lon; initToNaN(&l__lon);

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "SetAndSynthesisDistToStartingPos" command
          l__pattern = strtok(NULL, " ");
          if (l__pattern != NULL) {
              l__lat = (float)strtod(l__pattern, NULL);
              l__pattern = strtok(NULL, " ");
              if (l__pattern != NULL) {
                l__lon = (float)strtod(l__pattern, NULL);
              }
          }

          sprintf(l__cmd_result, ">>> Set and Synthesis Distance Lat [");
          Serial.print(l__cmd_result);
          Serial.print(l__lat, 6);
          sprintf(l__cmd_result, "] Lon [");
          Serial.print(l__cmd_result);
          Serial.print(l__lon, 6);
          sprintf(l__cmd_result, "]\n");
          Serial.print(l__cmd_result);

          COORD l__coord;
          g__pilot->initCoordToNoMean(&l__coord);
          l__coord.lat = l__lat;
          l__coord.lon = l__lon;
          g__pilot->initCoordToCurrent(&l__coord);

          uint32_t l__distance = g__pilot->getDistanceToStartingPos();
          sprintf(l__cmd_result, ">>> Distance [%d] m\n", l__distance);
          Serial.print(l__cmd_result);

          synthesisDistance(l__distance, SYNTH_POSITION, SYNTH_DIST_FROM_STARTING_POS);

          float l__cap = g__pilot->getCapToStartingPos();
          sprintf(l__cmd_result, ">>> Cap [");
          Serial.print(l__cmd_result);
          Serial.print(l__cap, 1);
          sprintf(l__cmd_result, "] degrees\n");
          Serial.print(l__cmd_result);

          synthesisCap(l__cap);
        }
        else if (!strncmp(g__incoming_buff, "TestStringToNumeric", strlen("TestStringToNumeric"))) {
          strcpy(l__cmd_result, g__incoming_buff);

          int   l__int_value = 0;
          float l__float_value; initToNaN(&l__float_value);
          char *l__endPtr = NULL;

          char *l__pattern = strtok(l__cmd_result, " ");    // Skip "TestStringToNumeric" command
          l__pattern = strtok(NULL, " ");
          if (l__pattern != NULL) {
              l__int_value = (int)strtol(l__pattern, &l__endPtr, 10);
              if (*l__endPtr != '\0' || *l__pattern == '\0') {
                Serial.print("EINVAL with 'int'\n");
              }
              l__pattern = strtok(NULL, " ");
              if (l__pattern != NULL) {
                l__float_value = (float)strtod(l__pattern, &l__endPtr);
                if (*l__endPtr != '\0' || *l__pattern == '\0') {
                  Serial.print("EINVAL with 'float'\n");
                }
              }
          }

          sprintf(l__cmd_result, ">>> TestStringToNumeric int [%d] float [", l__int_value);
          Serial.print(l__cmd_result);
          Serial.print(l__float_value, 6);
          sprintf(l__cmd_result, "]\n");
          Serial.print(l__cmd_result);
        }
        else {
#ifndef USE_SIMULATION
          // Extraction "mp3 datas in hexa-ascii"
          char *l__pattern = strchr(g__incoming_buff, ' ');

          if (l__pattern != NULL && !strncmp(g__incoming_buff, "mp3", 3)) {
            byte l__hexa_datas[32];
            memset(l__hexa_datas, '\0', sizeof(l__hexa_datas));

            char *l__datas = l__pattern + 1;
            sprintf(l__cmd_result, ">>> Command MP3 Player with [%s]\n", l__datas);
            Serial.print(l__cmd_result);
            if ((strlen(l__datas) % 2) == 0) {
              boolean l__err = true;
              size_t n = 0;
              size_t m = 0;
              for (n = 0; n < strlen(l__datas); n += 2, m++) {
                char l__char = *(l__datas + n);
                byte l__hexa = 0x00;
                l__err = convAscii2Hexa(l__char, &l__hexa);
                if (l__err == false) {
                  l__char = *(l__datas + n + 1);
                  l__hexa <<= 4;
                  l__err = convAscii2Hexa(l__char, &l__hexa);

                  if (l__err == false) {
                    l__hexa_datas[m] = l__hexa;
                  }
                }
                if (l__err == true) {
                  sprintf(l__cmd_result, ">>> Err: Invalid char (not hexa) (%c)\n", l__char);
                  Serial.print(l__cmd_result);
                  break;
                }
              }
              if (l__err == false) {
                for (n = 0; n < m; n++) {
                  if (n == 0) {
                    sprintf(l__cmd_result, ">>> [0x%02x", l__hexa_datas[n]);
                  }
                  else {
                    sprintf(l__cmd_result, "%02x", l__hexa_datas[n]);
                  }
                  Serial.print(l__cmd_result);

                  // Send to 'Serial MP3 Player'
                  g__serial_mp3_player->write(l__hexa_datas[n]);
                  delayMicroseconds(1100);    // Awaiting 1.1 mS between each byte (9600 bauds => 10 * 104 uS = 1.04 mS)
                }
                sprintf(l__cmd_result, "] (%d bytes)", n);
                Serial.print(l__cmd_result);

                // Calcul de la checksum
                uint16_t l__cks = g__serial_mp3_player->checksum((char *)l__hexa_datas, n);
                sprintf(l__cmd_result, " Cks [0x%x]\n", l__cks);
                Serial.print(l__cmd_result);

                // Pas de passage dans l'automate (permet une analyse des échanges) jusqu'à une commande 'tx_play' ou 'tx_other'
                g__serial_mp3_player->setFlgSkipAutomate(true);
              }
            }
            else {
              sprintf(l__cmd_result, ">>> Err: Datas not multiple of 2\n");
              Serial.print(l__cmd_result);
            }
          }
          else if (!strncmp(g__incoming_buff, "tx_play", 7) || !strncmp(g__incoming_buff, "tx_other", 8)) {
            // Passage dans l'automate
            g__serial_mp3_player->setFlgSkipAutomate(false);

            if (l__pattern == NULL) {
              // No datas -> Force 'setCommandSended(true)'
              if (!strncmp(g__incoming_buff, "tx_play", 7)) {
                g__serial_mp3_player->setCommandSended(TX_PLAY, true);
              }
              else if (!strncmp(g__incoming_buff, "tx_other", 8)) {
                g__serial_mp3_player->setCommandSended(TX_OTHER, true);
              }
            }
            else {
              byte l__hexa_datas[32];
              memset(l__hexa_datas, '\0', sizeof(l__hexa_datas));

              char *l__datas = l__pattern + 1;
              sprintf(l__cmd_result, ">>> Command %s with [%s]\n", g__incoming_buff, l__datas);
              Serial.print(l__cmd_result);
              if ((strlen(l__datas) % 2) == 0) {
                boolean l__err = true;
                size_t n = 0;
                size_t m = 0;
                for (n = 0; n < strlen(l__datas); n += 2, m++) {
                  char l__char = *(l__datas + n);
                  byte l__hexa = 0x00;
                  l__err = convAscii2Hexa(l__char, &l__hexa);
                  if (l__err == false) {
                    l__char = *(l__datas + n + 1);
                    l__hexa <<= 4;
                    l__err = convAscii2Hexa(l__char, &l__hexa);

                    if (l__err == false) {
                      l__hexa_datas[m] = l__hexa;
                    }
                  }
                  if (l__err == true) {
                    sprintf(l__cmd_result, ">>> Err: Invalid char (not hexa) (%c)\n", l__char);
                    Serial.print(l__cmd_result);
                    break;
                  }
                }
                if (l__err == false) {
                  for (n = 0; n < m; n++) {
                    if (n == 0) {
                      sprintf(l__cmd_result, ">>> [0x%02x", l__hexa_datas[n]);
                    }
                    else {
                      sprintf(l__cmd_result, "%02x", l__hexa_datas[n]);
                    }
                    Serial.print(l__cmd_result);
                  }
                  sprintf(l__cmd_result, "] (%d bytes)\n", n);
                  Serial.print(l__cmd_result);

                  // Write into FIFO/Tx and set frame to send
                  if (!strncmp(g__incoming_buff, "tx_play", 7)) {
                    g__serial_mp3_player->writeFifoTx(TX_PLAY, l__hexa_datas, n);
                    g__serial_mp3_player->setCommandSended(TX_PLAY, true);
                  }
                  else if (!strncmp(g__incoming_buff, "tx_other", 8)) {
                    g__serial_mp3_player->writeFifoTx(TX_OTHER, l__hexa_datas, n);
                    g__serial_mp3_player->setCommandSended(TX_OTHER, true);
                  }
                }
              }
              else {
                sprintf(l__cmd_result, ">>> Err: Datas not multiple of 2\n");
                Serial.print(l__cmd_result);
              }
            }
          }
          // WT2003S
          else if (!strncmp(g__incoming_buff, "WT2003S", 7)) {
            // Warning: 'l__pattern' pointe sur le caractere ' ' ;-)
            if (l__pattern != NULL) {
              if (!strcmp(l__pattern + 1, "date_time") || !strcmp(l__pattern + 1, "date") || !strcmp(l__pattern + 1, "time")) {
                // Synthesis date and time, date only eor time only...
                // Application de l'heure été/hiver
                ST_DATE_AND_TIME l__dateAndTime_presentation;
                memcpy(&l__dateAndTime_presentation, &g__dateAndTime, sizeof(ST_DATE_AND_TIME));
                applySommerWinterHour(&l__dateAndTime_presentation);

                byte     *l__commands = NULL;
                uint16_t *l__durations = NULL;
                size_t    l__nbr_durations = 0;
                size_t    l__size = 0;

                ENUM_SYNTH_DATE_TIME_MODES l__synth_mode = SYNTH_DATE_TIME_ALL;   // By default
                if (!strcmp(l__pattern + 1, "date")) {
                  l__synth_mode = SYNTH_DATE;
                }
                else if (!strcmp(l__pattern + 1, "time")) {
                  l__synth_mode = SYNTH_TIME;
                }

                l__size = buildCommandsPromptsDateTime(&l__dateAndTime_presentation, &l__commands, l__synth_mode, &l__durations, &l__nbr_durations);

                // Synthese synchrone des ('l__size' / 3) prompts de 'l__commands'
                synthesisPrompts(l__commands, l__size);

                // Preparation synthese iterative
                g__synth_prompts_iter = true;
              }
              else {
                // Send command...
              byte l__hexa_datas[32];
              memset(l__hexa_datas, '\0', sizeof(l__hexa_datas));

              char *l__datas = l__pattern + 1;
              sprintf(l__cmd_result, ">>> Command %s with [%s]\n", g__incoming_buff, l__datas);
              Serial.print(l__cmd_result);

              boolean l__flg_uppercase = false;   // Aiguillage 'g__wt2003s->update()' / 'g__serial_mp3_player->writeToWT2003S()'

              if ((strlen(l__datas) % 2) == 0) {
                boolean l__err = true;
                size_t n = 0;
                size_t m = 0;
                for (n = 0; n < strlen(l__datas); n += 2, m++) {
                  char l__char = *(l__datas + n);

                  // Test si minuscule/majuscule
                  if (l__char >= 'A' && l__char <= 'F') {
                    l__flg_uppercase = true;  
                  }

                  byte l__hexa = 0x00;
                  l__err = convAscii2Hexa(l__char, &l__hexa);
                  if (l__err == false) {
                    l__char = *(l__datas + n + 1);
                    l__hexa <<= 4;
                    l__err = convAscii2Hexa(l__char, &l__hexa);

                    if (l__err == false) {
                      l__hexa_datas[m] = l__hexa;
                    }
                  }
                  if (l__err == true) {
                    sprintf(l__cmd_result, ">>> Err: Invalid char (not hexa) (%c)\n", l__char);
                    Serial.print(l__cmd_result);
                    break;
                  }
                }
                if (l__err == false) {
                  for (n = 0; n < m; n++) {
                    if (n == 0) {
                      sprintf(l__cmd_result, ">>> [0x%02x", l__hexa_datas[n]);
                    }
                    else {
                      sprintf(l__cmd_result, "%02x", l__hexa_datas[n]);
                    }
                    Serial.print(l__cmd_result);
                  }
                  sprintf(l__cmd_result, "] (%d bytes)\n", n);
                  Serial.print(l__cmd_result);

                  if (l__flg_uppercase == true) {
                    // Simulation 'update' @ Write...
                    boolean l__rtn_update = false;
                    int m = 0;
                    for (m = 0; m < n; m++) {                    
                      l__rtn_update = g__wt2003s->update(l__hexa_datas[m], true);
                      if (l__rtn_update == false) {
                        Serial.printf("\tWT2003S::update(0x%2X): Error\n", l__hexa_datas[m]);

                        g__timers->start(TIMER_WT2003S_RESP_ERROR, DURATION_TIMER_WT2003S_RESP_ERROR, NULL);
                      }
                    }
                  }
                  else {
                    // Write to 'WT2003S'...
                    g__serial_mp3_player->writeToWT2003S(l__hexa_datas, n);
                  }
                }
              }
              else {
                sprintf(l__cmd_result, ">>> Err: Datas not multiple of 2\n");
                Serial.print(l__cmd_result);
              }
            }
            }
          }
          // Fin: WT2003S
          else {
            sprintf(l__cmd_result, ">>> Err: Unknown command [%s]\n", g__incoming_buff);
            Serial.print(l__cmd_result);
          }
#else
          Serial.println("Warning: Command not simulated");
#endif
        }
#if 0
      {
        sprintf(l__cmd_result, ">>> Err: Unknown command [%s]\n", g__incoming_buff);
        Serial.print(l__cmd_result);
      }
#endif

      g__flg_wait_command = true;
    }
    else if (g__count < sizeof(g__incoming_buff)) {
      g__incoming_buff[g__count++] = (char)incomingByte;
    }
    else {
      sprintf(l__cmd_result, ">>> Buffer overflow (%d >= %d)\n", g__count, sizeof(g__incoming_buff));
      Serial.print(l__cmd_result);
      g__count = 0;

      g__flg_wait_command = true;
    }
  }
  else if (g__flg_wait_command == true) {
    Serial.println(">>> Type the command...");
    g__count = 0;
    g__incoming_buff[g__count] = '\0';
    g__flg_wait_command = false;
  }
}

void getSimuMovePosition(COORD *io__coord_current)
{
  // Ajout du temps de la demande et délégation dans le module 'Plots'
  long l__duration = g__stats->getGenDurationInternal();
  g__plots->getSimuMovePosition(io__coord_current, l__duration);
}
#endif
