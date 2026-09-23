// $Id: SerialMP3Player.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifdef USE_SIMULATION
#include <stdio.h>
#include <string.h>

// Include files for simulation
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"

#else
#include <Arduino.h>
#include <HardwareSerial.h>
#endif

#include <sstream>

#include "SerialMP3Player.h"
#include "Misc.h"
#include "PromptsSynthesis.h"
#include "Timers.h"
#include "Errors.h"
#include "WT2003S.h"

extern void callback_wait_fifo_tx_play();
extern void callback_wait_fifo_tx_other();

#ifndef USE_SIMULATION
HardwareSerial            g__serialMP3Player(2);
#endif

static long               g__duration_short_famine = 0L;

/* Responses of the module KT403A treated as events for the automate
 *
 * Remarques: - La seule erreur est 0x40 = "STATUS_ERR_FILE"
 *            - Les évènements peuvent être reçus d'une manière non solicitée (asynchrone)
 *              ou attendues (synchrone) suite une demande
 *            - Les évènement inclus également ceux faisant l'objet d'émission
 */
static const ST_STATUS_KT403A g__event[25] =
{
  { "RESET_PLAYER",                      RESET_PLAYER,                      false },
  { "SELECT_SOURCE_DEVICE",              SELECT_SOURCE_DEVICE,              false },

  { "SELECT_SONG_IN_MP3_DIRECTORY",      SELECT_SONG_IN_MP3_DIRECTORY,      false },
  { "SELECT_SOURCE_FOLDER_AND_SONG",     SELECT_SOURCE_FOLDER_AND_SONG,     false },
  { "SELECT_SOURCE_BIG_FOLDER_AND_SONG", SELECT_SOURCE_BIG_FOLDER_AND_SONG, false },
  { "INSERT_SONG_FROM_ADVERT_DIRECTORY", INSERT_SONG_FROM_ADVERT_DIRECTORY, false },

  { "SET_VOLUME_LEVEL",                  SET_VOLUME_LEVEL, false },
  { "SET_EQUALIZER",                     SET_EQUALIZER,    false },

  { "STATUS_TF_INSERT",  STATUS_TF_INSERT,    false },    // TF Card was inserted (unsolicited)
  { "STATUS_TF_REMOVE",  STATUS_TF_REMOVE,    false },    // TF card was removed (unsolicited)
  { "STATUS_FILE_END",   STATUS_FILE_END,     false },    // Track/file has ended (unsolicited)
  { "STATUS_INIT",       STATUS_INIT,         false },    // Initialization complete (unsolicited)
  { "STATUS_ERR_FILE",   STATUS_ERR_FILE,     true  },    // Error file not found
  { "STATUS_ACK_OK",     STATUS_ACK_OK,       false },    // Message acknowledged ok
  { "STATUS_STATUS",     QUERY_DEVICE,        false },    // Current status
  { "STATUS_VOLUME",     QUERY_VOLUME,        false },    // Current volume level
  { "STATUS_EQUALIZER",  QUERY_EQUALIZER,     false },    // Equalizer status
  { "STATUS_TOT_FILES",  QUERY_FOLDER_FILES,  false },    // TF Total file count
  { "STATUS_PLAYING",    QUERY_TRACK_NUMBER,  false },    // Current track playing
  { "STATUS_FLDR_FILES", STATUS_FLDR_FILES,   false },    // Total number of files in the folder
  { "STATUS_TOT_FLDR",   QUERY_TOTAL_FOLDERS, false },    // Total number of folders

  { "WT2003S_OK",             WT2003S_OK,             false },    // Reponse Ok du WT2003S
  { "WT2003S_KO",             WT2003S_KO,             false },    // Reponse Ko du WT2003S
  { "WT2003S_CMD_PLAY_START", WT2003S_CMD_PLAY_START, false },    // Play Start du WT2003S
  { "WT2003S_CMD_PLAY_END",   WT2003S_CMD_PLAY_END,   false },    // Play End du WT2003S
};

static const ST_STATES g__states[STATE_NUMBER] =
{
  { "STATE_UNKNOWN",                  STATE_UNKNOWN                  },

  // Initialization of KT403A module
  { "STATE_WAIT_RSP_RESET_ACK_OK",    STATE_WAIT_RSP_RESET_ACK_OK    },
  { "STATE_WAIT_RSP_RESET_INIT",      STATE_WAIT_RSP_RESET_INIT      },
  { "STATE_WAIT_RSP_DEVICE_ACK_OK",   STATE_WAIT_RSP_DEVICE_ACK_OK   },

  // Synchro. pour 'Set Equalizer', 'Set Volume Level' et 'Query Volume'
  { "STATE_WAIT_SET_EQUALIZER_ACK_OK",    STATE_WAIT_SET_EQUALIZER_ACK_OK },
  { "STATE_WAIT_SET_VOLUME_LEVEL_ACK_OK", STATE_WAIT_SET_VOLUME_LEVEL_ACK_OK },
  { "STATE_WAIT_QUERY_VOLUME_ACK_OK",     STATE_WAIT_QUERY_VOLUME_ACK_OK },

  // SDcard removed/inserted
  { "STATE_TF_REMOVE",                STATE_TF_REMOVE                },
  { "STATE_TF_INSERT",                STATE_TF_INSERT                },

  { "STATE_IDLE",                     STATE_IDLE                     }
};

// List of constant commands ('Reset Player', 'Select Source Device', etc.) to write into FIFO/Tx Other
byte g__command_reset_player[3]         = { RESET_PLAYER,                 0x00, 0x00 };             // Reset the MP3 Player
byte g__command_select_source_device[3] = { SELECT_SOURCE_DEVICE,         0x00, DEVICE_SDCARD };    // Select the SDCard Device
byte g__command_prompt[3]               = { SELECT_SONG_IN_MP3_DIRECTORY, 0x00, 0x00 };             // Play the prompt with 'SELECT_SONG_IN_MP3_DIRECTORY' (Hello World...)

byte g__command_set_equalizer_x05[3]    = { SET_EQUALIZER,                0x00, 0x05 };             // Set Equalizer at 'EQUALIZER_BASS'

// Suite a l'accueil de la diffusion sur HP -> Reduction du niveau sonore de 26/31 a 10/31 ;-)
#if 0
byte g__command_set_volume_level_x1a[3] = { SET_VOLUME_LEVEL,             0x00, 0x1a };             // Set Volume Level to 26/31: Value in the range [0x00, 0x01, 0x1F] ([0, 1, ..., 31])
#else
byte g__command_set_volume_level_x1a[3] = { SET_VOLUME_LEVEL,             0x00, 0x0a };             // Set Volume Level to 10/31: Value in the range [0x00, 0x01, 0x1F] ([0, 1, ..., 31])
#endif
// Fin: Suite a l'accueil de la diffusion sur HP -> Reduction du niveau sonore de 26/31 a 10/31 ;-)

byte g__command_query_volume_level[3]   = { QUERY_VOLUME,                 0x00, 0x00 };             // Query Volume Level

byte g__command_query_track_number[3]   = { QUERY_TRACK_NUMBER,           0x00, 0x00 };             // Query the track number in progress
byte g__command_stop_music[3]           = { STOP_MUSIC,                   0x00, 0x00 };             // Stop music
// End: List of constant commands ('Reset Player', 'Select Source Device', etc.) to write into FIFO/Tx Play and Other

SerialMP3Player::SerialMP3Player() :
  flg_module_initialized(false),
  idx_rx_write_it(0), idx_rx_read_it(0), frame_rx_available_it(false), flg_rx_received_ignore_it(false),
  frame_rx_str(""), flg_callback(false), fct_callback(NULL),
  flg_all_prompts_played(false), nbr_call_purge_fifo_tx_play(0), nbr_exec_purge_fifo_tx_play(0),
  flg_connected(false), duration_internal_previous(0L), duration_internal_current(0L), max_duration_between_prompts(0L)
{
  Serial.println("SerialMP3Player::SerialMP3Player()");

#ifndef USE_SIMULATION
  pinMode(RXD2, INPUT_PULLUP);
  g__serialMP3Player.begin(9600, SERIAL_8N1, RXD2, TXD2);
#endif

  memset(frame_rx_it, 0xff, sizeof(frame_rx_it));     // Pour disciminer plus facilement les reponses du WT2003S

  fifo_tx[TX_PLAY].idx_tx_write    = 0;
  fifo_tx[TX_PLAY].idx_tx_read     = 0;
  fifo_tx[TX_PLAY].flg_tx_to_send  = false;
  fifo_tx[TX_PLAY].frame_tx_str    = "";
  fifo_tx[TX_PLAY].flg_callback    = false;
  fifo_tx[TX_PLAY].fct_callback    = NULL;

  fifo_tx[TX_PLAY].num_sequence_pre = (uint16_t)-1;
  fifo_tx[TX_PLAY].num_sequence     = 0;

  fifo_tx[TX_OTHER].idx_tx_write   = 0;
  fifo_tx[TX_OTHER].idx_tx_read    = 0;
  fifo_tx[TX_OTHER].flg_tx_to_send = false;
  fifo_tx[TX_OTHER].frame_tx_str   = "";
  fifo_tx[TX_OTHER].flg_callback   = false;
  fifo_tx[TX_OTHER].fct_callback   = NULL;

  fifo_tx[TX_OTHER].num_sequence_pre = (uint16_t)-1;
  fifo_tx[TX_OTHER].num_sequence     = 0;

  memset(fifo_tx[TX_PLAY].frame_tx, '\0', sizeof(fifo_tx[TX_PLAY].frame_tx));
  memset(fifo_tx[TX_OTHER].frame_tx, '\0', sizeof(fifo_tx[TX_OTHER].frame_tx));

  // Initialization of automate
  automate.flg_skip_automate = false;
  automate.current_state     = STATE_UNKNOWN;
  automate.event_previous    = INVALID_OPCODE;
  automate.event             = INVALID_OPCODE;
  automate.data              = 0;

  clearSynchroPromptsExpected();

  automate.seq_error         = false;

#if USE_STATS_SERIAL_MP3_PLAYER
  nbr_frames_rx = 0;
  err_fifo_0 = 0;
  err_fifo_1 = 0;
  err_retry = 0;
  spare_min = INT_MAX;
  err_frame_rx = 0;
  err_cks = 0;
#endif
}

SerialMP3Player::~SerialMP3Player()
{
  Serial.println("SerialMP3Player::~SerialMP3Player()");
}

#if USE_STATS_SERIAL_MP3_PLAYER
unsigned int SerialMP3Player::getErrFifo(int i__num_err) const
{
  switch(i__num_err) {
  case 0: return err_fifo_0;
  case 1: return err_fifo_1;
  case 2: return err_retry;
  default: return (unsigned int)-1;
  }
}
#endif

/*  update() method with members suffixed by '_it'
 *
 *  Mise à jour de la FIFO de réception sous interruption avec notamment
 *  'idx_rx_read_it' et 'idx_rx_write_it' en accès concurent
 */
void SerialMP3Player::update(char i__char)
{
#ifndef USE_SIMULATION
#if USE_STATS_SERIAL_MP3_PLAYER
  if (frame_rx_available_it == true) {
    // La trame précédente n'a pas été extraite ;-(
    err_fifo_0++;
  }
#endif
  
  if (g__serialMP3Player.available() > 0 || i__char != INVALID_OPCODE) {
    /* Prise en compte en priorite de l'argument si valide
     * => A noter que 'update()' est appelee sous It depuis les 3 methodes
     *    'IRAM_ATTR onTimer500uS()' -> 'treatmentAll_500uS()', 'IRAM_ATTR doIsrRisingInputWT2003SBusy()'
     *    et 'IRAM_ATTR doIsrFallingInputWT2003SBusy()'
     */
    char l__char = (i__char != INVALID_OPCODE) ? i__char : char(g__serialMP3Player.read());

#if 0   // Traces impossibles: Core Dump wdt cpu1...
    // Conversion Hexa-ASCII
    char l__hexa_ascii_msb = convHexa2Ascii((l__char >> 4) &0xF);
    char l__hexa_ascii_lsb = convHexa2Ascii(l__char &0xF);
    char l__buffer[8];
    sprintf(l__buffer, "0x%c%c\n", l__hexa_ascii_msb, l__hexa_ascii_lsb);
    Serial.print(l__buffer);
#endif

    frame_rx_it[idx_rx_write_it] = l__char;
    idx_rx_write_it = (idx_rx_write_it + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX;

    int l__diff = (idx_rx_write_it - idx_rx_read_it);

#if USE_STATS_SERIAL_MP3_PLAYER
    // Test de l'état de la FIFO circulaire: Si 'idx_rx_write' atteint 'idx_rx_read' => Saturation FIFO
    if (l__diff == 0) {
      err_fifo_1++;
    }
    else if (l__diff > 0) {
      int l__spare = (NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX - l__diff);
      spare_min = l__spare < spare_min ? l__spare : spare_min;
    }
    else {
      int l__spare = -l__diff;
      spare_min = l__spare < spare_min ? l__spare : spare_min;
    }
    // End: Test de l'état de la FIFO circulaire
#endif

    /* Trame disponible ?
     *  - 'idx_rx_read' doit être inférieur d'au moins 'RSP_LENGTH' positions à 'idx_rx_write' (au modulo 'NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX' près)
     *  - Si le cas, déplacement et arrêt jusqu'au 'COMMAND_BYTE_START'
     *  - Recopie des 'RSP_LENGTH' bytes et test d'intégrité ('COMMAND_BYTE_STOP' terminal, checksum, etc.)
    */
    if (l__diff < 0) {
      l__diff = (NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX - idx_rx_read_it + idx_rx_write_it);
    }

    /* Traitement specifique pour le WT2003S
     * => TODO: A faire que si le module WT2003S a ete detecte ou suite au RESET du KT403A
     *          qui determinera quel module Serial MP3 Player connecte ;-)
     */
    if (l__diff > 0) {
      /* Passage dans 'WT2003S::update()' pour le prelevement des bytes recus
       * => Le traitement specifique pour le KT403A ('l__diff >= RSP_LENGTH') ne sera jamais realise ;-)  
       */
      byte l__byte = frame_rx_it[idx_rx_read_it];

      if (g__wt2003s->update(l__byte) == false) {
        g__timers->start(TIMER_WT2003S_RESP_ERROR, DURATION_TIMER_WT2003S_RESP_ERROR, NULL); 
      }

      /* Consommation du byte dans tous les cas avec maj de 'l__diff'
       * => TODO: Cohabitation des 2 modules Serial MP3 Player ?!..
       */
      idx_rx_read_it = (idx_rx_read_it + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX;
      l__diff -= 1;
    }
    // Fin: Traitement specifique pour le WT2003S

    /* Traitement specifique pour le KT403A
     * => TODO: A faire que si le module KT403A a ete detecte
     */
    if (l__diff >= RSP_LENGTH) {
      unsigned int l__idx_rx_read = idx_rx_read_it;    // For retry

      int l__idx = 0;
      memset(frame_rx_received_it, '\0', sizeof(frame_rx_received_it));

      boolean l__flg_copy = false;
      while (idx_rx_read_it != idx_rx_write_it && l__idx < RSP_LENGTH) {
        if (l__flg_copy == false && frame_rx_it[idx_rx_read_it] == COMMAND_BYTE_START) {
          l__flg_copy = true;
        }
        if (l__flg_copy == true) {
          frame_rx_received_it[l__idx++] = frame_rx_it[idx_rx_read_it];
        }
        idx_rx_read_it = (idx_rx_read_it + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX;
      }

      if (l__idx == RSP_LENGTH) {
        nbr_frames_rx++;

        /* Ignore si cette trame est strictement identique à la précédente reçue
         *  - Limitée aux réponses 'STATUS_FILE_END'
         *  - Remarque: 'flg_rx_received_ignore_it' n'est pas utilisee
         *              => TODO: A supprimer ;-)
         */
        if (frame_rx_received_it[RSP_STATUS] != STATUS_FILE_END || strncmp(frame_rx_received_pre, frame_rx_received_it, sizeof(frame_rx_received_it))) {
          // Save the new frame not identical
          strncpy(frame_rx_received_pre, frame_rx_received_it, sizeof(frame_rx_received_it));

          flg_rx_received_ignore_it = false;
          frame_rx_available_it     = true;
        }
        else {
          flg_rx_received_ignore_it = true;
          frame_rx_available_it     = false;
        }
        // Fin: Ignore si cette trame est strictement identique à la précédente reçue
      }
      else {
        /* Retry car aucune copie n'a été effectuée
         * Remarques: - Ce mécanisme est mis en oeuvre sur la réponse à la commande 'Reset'
         *              où 1 ou plusieurs bytes 0x00 sont trouvés entre les 2 réponses ?!..
         *            - Le danger est de se synchroniser sur un byte 'COMMAND_BYTE_START' qui
         *              ferait partie de la réponse et donc de se désynchroniser en permanence
         *              sur des réponses non ntègres (pas de 'COMMAND_BYTE_STOP', checksum Ko, etc.)
         *              => TODO: A amméliorer ou au pire faire un 'Reset' du MP3 Player ;-)
        */ 
        idx_rx_read_it = l__idx_rx_read;
      }
    }
    // Fin: Trame disponible ?
    // Fin: Traitement specifique pour le KT403A
  }
#endif  // #ifndef USE_SIMULATION
}
// End: update() method with members suffixed by '_it'

void SerialMP3Player::write(byte i__value)
{
#ifndef USE_SIMULATION
  g__serialMP3Player.write(i__value);
#endif
}

boolean SerialMP3Player::isFrameAvailable()
{
#ifdef USE_SIMULATION
  return false;        // TODO: @false/true
#else
  // Remarque: "size(boolean) = 1", mais par prudence ;-)
#ifndef USE_SIMULATION
  noInterrupts();
#endif

  boolean l__frame_rx_available = frame_rx_available_it;

#ifndef USE_SIMULATION
  interrupts();
#endif

  // Ignore si trame non disponible
  if (l__frame_rx_available == false) {
    return false;
  }

  // Extraction des infos
  frame_rx_str = "\nRX ";

  // Copy 'frame_rx_received_it[]' to local buffer because concurent acces ;-)
  char l__frame_rx_received[sizeof(frame_rx_received_it)];

#ifndef USE_SIMULATION
  noInterrupts();
#endif

  memcpy(l__frame_rx_received, frame_rx_received_it, sizeof(l__frame_rx_received));

#ifndef USE_SIMULATION
  interrupts();
#endif

  size_t n = 0;
  for (n = 0; n < RSP_LENGTH; n++) {
    char l__value = l__frame_rx_received[n];

    // Conversion Hexa-ASCII
    frame_rx_str += convHexa2Ascii((l__value >> 4) &0xF);
    frame_rx_str += convHexa2Ascii(l__value &0xF);
  }

  // Checksum of 'CONST_LENGTH' bytes from 'CONST_VERSION' position
  uint16_t l__cks_rec = 256 * l__frame_rx_received[RSP_CKS_MSB] + l__frame_rx_received[RSP_CKS_LSB];
  uint16_t l__cks_exp = checksum(&l__frame_rx_received[RSP_VERSION], CONST_LENGTH);

  // Check the fields
  boolean l__flg_integrity = false;
  if (l__frame_rx_received[RSP_START] == COMMAND_BYTE_START
   && l__frame_rx_received[RSP_VERSION] == COMMAND_BYTE_VERSION
   && l__frame_rx_received[RSP_LEN] == 6
   && l__frame_rx_received[RSP_STOP] == COMMAND_BYTE_STOP) {
    l__flg_integrity = true;
   }

  char l__buffer[80];
  sprintf(l__buffer, " (%s) (Cks Rec/Exp 0x%04x/0x%04x %s) ", (l__flg_integrity == true ? "Ok" : "Ko"), l__cks_rec, l__cks_exp,
    (l__cks_rec == l__cks_exp ? "Ok" : "Ko"));

  frame_rx_str += l__buffer;

  for (n = 0; n < sizeof(g__event)/sizeof(g__event[0]); n++) {
    if (g__event[n].value == l__frame_rx_received[RSP_STATUS] && g__event[n].text != NULL) {
      uint16_t l__data = 256 * l__frame_rx_received[RSP_DATA_MSB] + l__frame_rx_received[RSP_DATA_LSB];
      sprintf(l__buffer, " (%d - 0x%04x)", l__data, l__data);
      frame_rx_str += g__event[n].text;
      frame_rx_str += l__buffer;

      // Update this event with his data
      automate.event = (CMD_OPCODE)g__event[n].value;
      automate.data  = l__data;

      break;
    }
  }
  // End: Extraction des infos, calcul et comparaison de la checksum

  // If here, return always 'true' (available state read)
  return true;
#endif
}

#ifndef USE_SIMULATION
String SerialMP3Player::getFrameRx()
#else
std::string SerialMP3Player::getFrameRx()
#endif
{
  return frame_rx_str;
}

void SerialMP3Player::setUnvailableFrame()
{
  // Remarque: "size(boolean) = 1", mais par prudence ;-)
#ifndef USE_SIMULATION
  noInterrupts();
#endif

  frame_rx_available_it = false;

#ifndef USE_SIMULATION
  interrupts();
#endif
}

uint16_t SerialMP3Player::getNumSequence(FIFO_TX_NUM i__num)
{
  return (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].num_sequence : fifo_tx[TX_OTHER].num_sequence;
}

uint16_t SerialMP3Player::incAndGetNumSequence(FIFO_TX_NUM i__num)
{
  return (i__num == TX_PLAY) ? ++fifo_tx[TX_PLAY].num_sequence : ++fifo_tx[TX_OTHER].num_sequence;
}

void SerialMP3Player::writeFifoTx(FIFO_TX_NUM i__num, byte *i__value, size_t i__nbr_bytes, uint16_t *i__duration, size_t i__nbr_durations)
{
  char l__buffer[80];

  unsigned int l__idx_tx_write = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].idx_tx_write : fifo_tx[TX_OTHER].idx_tx_write;
  char *l__frame_tx = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].frame_tx : fifo_tx[TX_OTHER].frame_tx;

  size_t  l__idx_byte = 0;
  size_t  l__idx_duration = 0;
  long    l__total_duration = 0L;
  boolean l__flg_total_duration = false;

  if (i__num == TX_PLAY && fifo_tx[TX_PLAY].num_sequence_pre != fifo_tx[TX_PLAY].num_sequence) {
    // Update 'num_sequence' only for the FIFO/Tx Play
    *(l__frame_tx + l__idx_tx_write) = CMD_NUM_SEQ;
    l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
    *(l__frame_tx + l__idx_tx_write) = (fifo_tx[TX_PLAY].num_sequence / 256);
    l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
    *(l__frame_tx + l__idx_tx_write) = (fifo_tx[TX_PLAY].num_sequence % 256);
    l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;

    fifo_tx[TX_PLAY].num_sequence_pre = fifo_tx[TX_PLAY].num_sequence;

    // Suivie de 2 séquences 'CMD_NOT_DEFINED_1' et 'CMD_NOT_DEFINED_2' pour l'éventuelle redifussion du message
    int n = 0;
    for (n = 0; n < 2; n++) {
      *(l__frame_tx + l__idx_tx_write) = (n == 0) ? CMD_NOT_DEFINED_1 : CMD_NOT_DEFINED_2;
      l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
      *(l__frame_tx + l__idx_tx_write) = 0x00;
      l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
      *(l__frame_tx + l__idx_tx_write) = 0x00;
      l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
    }
  }

  for (l__idx_byte = 0; l__idx_byte < i__nbr_bytes; l__idx_byte++) {
    if (i__num == TX_OTHER) {
#if 0
      //  Ajout avant la commande OTHER d'un temps d'attente: (3 x bytes) = 1! commande
      if ((l__idx_byte % 3) == 0) {
        *(l__frame_tx + l__idx_tx_write) = CMD_TIME_WAIT;
        l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
        *(l__frame_tx + l__idx_tx_write) = (DURATION_TIMER_WAIT_FIFO_TX_OTHER / 256);
        l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
        *(l__frame_tx + l__idx_tx_write) = (DURATION_TIMER_WAIT_FIFO_TX_OTHER % 256);
        l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
      }
#endif
      *(l__frame_tx + l__idx_tx_write) = *(i__value + l__idx_byte);
      l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
    }
    else if (i__num == TX_PLAY) {
      *(l__frame_tx + l__idx_tx_write) = *(i__value + l__idx_byte);
      l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;

      /*  Ajout après la commande PLAY d'un temps d'attente: (3 x bytes) = 1! commande
       *  => Les durées des prompts définies dans la liste 'i__duration' sont éventuellement prises en compte
       *     => 'i__duration' peut être définie à NULL, auquel cas aucun ajout ne sera effectué (ie pour les prompts avec diffusion synchronisée)
      */
      if ((l__idx_byte % 3) == 2) {
        uint16_t l__duration = DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN;    // Durée minimale

        if (i__duration != NULL && i__nbr_durations > 0 && l__idx_duration < i__nbr_durations) {
          uint16_t l__duration_prompt = *(i__duration + l__idx_duration);   // Durée du prompt en multiple de 10 mS
          sprintf(l__buffer, "\t#%d/%d: [%d mS]\n", l__idx_duration, i__nbr_durations, 10 * l__duration_prompt);
          Serial.print(l__buffer);

          /*  Durée du prompt (multiple de 10 mS - ie. 50 for 500 mS)
           *  => Durée minimale si durée nulle du prompt (DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN)
           *  => Sinon, valeur réelle de la durée du prompt
           *     => TBC: A vérifier si la diffusion n'est pas "coupée" ;-)
           */
          if (l__duration_prompt != 0) {
            l__duration = l__duration_prompt;
          }

          //  Ajout de 'l__duration' + marge à la durée du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
          l__total_duration += (long)l__duration;
          l__flg_total_duration = true;

          l__idx_duration++;
        }

        *(l__frame_tx + l__idx_tx_write) = CMD_TIME_WAIT;
        l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
        *(l__frame_tx + l__idx_tx_write) = (l__duration / 256);
        l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
        *(l__frame_tx + l__idx_tx_write) = (l__duration % 256);
        l__idx_tx_write = (l__idx_tx_write + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
      }
    }
  }

  switch (i__num) {
  case TX_PLAY:
    fifo_tx[TX_PLAY].idx_tx_write = l__idx_tx_write;

    if (l__flg_total_duration == true) {
      g__timers->addPromptsDuration(l__total_duration);
    }

    break;
  case TX_OTHER:
    fifo_tx[TX_OTHER].idx_tx_write = l__idx_tx_write;
    break;
  default:
    break;
  }
}

void SerialMP3Player::setCommandSended(FIFO_TX_NUM i__num, boolean i__flg)
{
  switch (i__num) {
  case TX_PLAY:
    fifo_tx[TX_PLAY].flg_tx_to_send = i__flg;
    break;
  case TX_OTHER:
    fifo_tx[TX_OTHER].flg_tx_to_send = i__flg;
    break;
  default:
    break;
  }
}

#if USE_TIMER_DIFFUSION
void callback_diffusion()
{
  Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_DIFFUSION expired\n");
}
#endif

boolean SerialMP3Player::isCommandToSend(FIFO_TX_NUM i__num)
{
  char l__buffer[132];

  boolean l__flg_tx_to_send_pre = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].flg_tx_to_send_pre : fifo_tx[TX_OTHER].flg_tx_to_send_pre;
  boolean l__flg_tx_to_send = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].flg_tx_to_send : fifo_tx[TX_OTHER].flg_tx_to_send;
  unsigned int l__idx_tx_write = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].idx_tx_write : fifo_tx[TX_OTHER].idx_tx_write;
  unsigned int l__idx_tx_read  = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].idx_tx_read : fifo_tx[TX_OTHER].idx_tx_read;
  char *l__frame_tx = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].frame_tx : fifo_tx[TX_OTHER].frame_tx;
  char *l__frame_tx_to_send = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].frame_tx_to_send : fifo_tx[TX_OTHER].frame_tx_to_send;

  byte l__cmd_kt403a[3];
  memset(l__cmd_kt403a, '\0', sizeof(l__cmd_kt403a));

  // Trace si trame a emettre ou non
  if (l__flg_tx_to_send != l__flg_tx_to_send_pre) {
    Serial.printf("@@@ isCommandToSend(%d): flg_tx_to_send: Previous [%d] => Current [%d] => [%s]\n",
      i__num, l__flg_tx_to_send_pre, l__flg_tx_to_send, (l__flg_tx_to_send == true) ? "Frame to send" : "No frame to send");

    if (i__num == TX_PLAY) {
      fifo_tx[TX_PLAY].flg_tx_to_send_pre = l__flg_tx_to_send;
    }
    else {
      fifo_tx[TX_OTHER].flg_tx_to_send_pre = l__flg_tx_to_send;
    }
  }

  // Ignore si trame non à emettre
  if (l__flg_tx_to_send == false) {
    return false;
  }

  int l__diff = (l__idx_tx_write - l__idx_tx_read);

  if (l__diff < 0) {
    l__diff = (NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX - l__idx_tx_read + l__idx_tx_write);
  }

  if (l__diff >= 3) {
    memset(l__frame_tx_to_send, '\0', sizeof(fifo_tx[TX_PLAY].frame_tx_to_send));   // Size identical for the 2 FIFO/Tx Play and Other

    // Lecture des 3 bytes 'CMD_COMMAND', 'CMD_DATA_MSB' et 'CMD_DATA_LSB'
    *(l__frame_tx_to_send + CMD_START)    = COMMAND_BYTE_START;
    *(l__frame_tx_to_send + CMD_VERSION)  = COMMAND_BYTE_VERSION;
    *(l__frame_tx_to_send + CMD_LEN)      = 6;

    // Update event for automate
    automate.event = (CMD_OPCODE)*(l__frame_tx + l__idx_tx_read);

    *(l__frame_tx_to_send + CMD_COMMAND)  = *(l__frame_tx + l__idx_tx_read); l__idx_tx_read = (l__idx_tx_read + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
    *(l__frame_tx_to_send + CMD_FEEDBACK) = 1;
    *(l__frame_tx_to_send + CMD_DATA_MSB) = *(l__frame_tx + l__idx_tx_read); l__idx_tx_read = (l__idx_tx_read + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;
    *(l__frame_tx_to_send + CMD_DATA_LSB) = *(l__frame_tx + l__idx_tx_read); l__idx_tx_read = (l__idx_tx_read + 1) % NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX;

    // Sauvegarde des 3 bytes de commandes dans le cas du 'WT2003S'
    if (g__wt2003s->isInService() == true) {
      l__cmd_kt403a[0] = *(l__frame_tx_to_send + CMD_COMMAND);
      l__cmd_kt403a[1] = *(l__frame_tx_to_send + CMD_DATA_MSB);
      l__cmd_kt403a[2] = *(l__frame_tx_to_send + CMD_DATA_LSB);
    }

    // Checksum
    uint16_t l__cks = checksum(l__frame_tx_to_send + CMD_VERSION, CONST_LENGTH);
    *(l__frame_tx_to_send + CMD_CKS_MSB)  = ((l__cks >> 8) & 0xFF);
    *(l__frame_tx_to_send + CMD_CKS_LSB)  = (l__cks & 0xFF);

    *(l__frame_tx_to_send + CMD_STOP)     = COMMAND_BYTE_STOP;

    CMD_OPCODE l__event = (CMD_OPCODE)*(l__frame_tx_to_send + CMD_COMMAND);
    uint16_t   l__data  = 256 * (uint16_t)*(l__frame_tx_to_send + CMD_DATA_MSB) + (uint16_t)*(l__frame_tx_to_send + CMD_DATA_LSB);

    // Update 'read' index
    switch (i__num) {
    case TX_PLAY:
      fifo_tx[TX_PLAY].idx_tx_read = l__idx_tx_read;
      break;
    case TX_OTHER:
      fifo_tx[TX_OTHER].idx_tx_read = l__idx_tx_read;
      break;
    default:
      break;
    }

    if (l__event == CMD_NOT_DEFINED_1) {
      sprintf(l__buffer, "CMD_NOT_DEFINED_1 (0x%04x)\n", l__data);
      Serial.print(l__buffer);

      return false;
    }
    else if (l__event == CMD_NOT_DEFINED_2) {
      sprintf(l__buffer, "CMD_NOT_DEFINED_2 (0x%04x)\n", l__data);
      Serial.print(l__buffer);

      return false;
    }    
    else if (l__event == CMD_NUM_SEQ) {
      sprintf(l__buffer, "CMD_NUM_SEQ #%u (0x%04x)\n", l__data, l__data);
      Serial.print(l__buffer);

      return false;
    }
    else if (l__event == CMD_TIME_WAIT) {
      sprintf(l__buffer, "CMD_TIME_WAIT execution with %u (0x%04x) value\n", l__data, l__data);
      Serial.print(l__buffer);

      /*  Armement timer qui expirera dans (value x 10 mS) correspond au temps de la diffusion estimée ou réelle
       *  => Utilisé également pour l'enveloppe "time-out" de 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY' (cf. 'GpsPilot.ino')
       *  => Par expérience, ce temps peut être réduit car le module KT403A semmble posséder une bufferisation des commandes 
       *     => TBC
       *     => Si durée réelle, enchaînement "non fluide" des diffusions de prompts ;-(
       *        => D'où la division par 4 pour les attentes depuis la FIFO/Tx Play
       */
      if (i__num == TX_PLAY) {
#if USE_REAL_PROMPTS_DURATION_FROM_TIME
        long l__duration = (long)l__data;
#else
        long l__duration = (long)(l__data / 4);
#endif

        g__timers->start(TIMER_WAIT_FIFO_TX_PLAY, (long)l__duration, &callback_wait_fifo_tx_play);

#if USE_TIMER_DIFFUSION
        /*  Armement/Réarmement timer 'TIMER_DIFFUSION' qui expirera dans (value x 10 mS) correspond au temps de la diffusion estimée ou réelle x 2
         *  => Doit expirer après l'évènement 'STATUS_FILE_END'
         *     => Stoppé si 'STATUS_FILE_END'
         *     => Si expiration:
         *        => Empilement d'un pseudo évènement ~'STATUS_FILE_END' permettant d'éviter une courte et longue famine ;-)
         */
         if (g__timers->isInUse(TIMER_DIFFUSION)) {
            sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_DIFFUSION restarted [%d] mS\n", (l__data * 10 * 2));
            Serial.print(l__buffer);

            g__timers->restart(TIMER_DIFFUSION, (long)(l__data * 2));
         }
         else {
            sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- TIMER_DIFFUSION started [%d] mS\n", (l__data * 10 * 2));
            Serial.print(l__buffer);

            g__timers->start(TIMER_DIFFUSION, (long)(l__data * 2), &callback_diffusion);
         }
         //  Armement timer 'TIMER_DIFFUSION' qui expirera dans (value x 10 mS) correspond au temps de la diffusion estimée ou réelle x 2
#endif
      }
      else if (i__num == TX_OTHER) {
        g__timers->start(TIMER_WAIT_FIFO_TX_OTHER, (long)l__data, &callback_wait_fifo_tx_other);
      }

      /* Si (idx_tx_write == idx_tx_read), tous les prompts ont été joués et les commandes 'CMD_TIME_WAIT' dépilées
       *     => Affirmation de 'flg_all_prompts_played' (cf. isAllPromptsPlayed())
       */

      if (fifo_tx[TX_PLAY].idx_tx_write == fifo_tx[TX_PLAY].idx_tx_read) {
        flg_all_prompts_played = true;

        // Traces de l'évolution du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
        long l__duration_short_famine = g__timers->getDuration(TIMER_SHORT_FAMINE_FIFO_TX_PLAY);
        if (l__duration_short_famine >= 0L) {
          long l__diff = (l__duration_short_famine - g__duration_short_famine);
          sprintf(l__buffer, "\tEnd of prompt diffusion (CMD_TIME_WAIT) (flg_all_prompts_played: %d) (%ld mS) (Diff: %ld mS) (Total: %ld mS)",
            flg_all_prompts_played, 10L * l__duration_short_famine, 10L * l__diff, 10L * g__timers->getPromptsDurationTotal());
          Serial.print(l__buffer);

          if (g__timers->getPromptsDurationTotal() != 0L) {
            float l__percent = 0.0;
            l__percent = (100.0 * l__duration_short_famine) / g__timers->getPromptsDurationTotal();

            Serial.print(" (");
            Serial.print(l__percent, 1);
            Serial.print("%)\n");
          }
          else {
            Serial.print("\n");
          }

          /*  La FIFO/Tx Play est entièrement lue
           *  => Retour pour la prochaine gestion de 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
           */
          g__duration_short_famine = 0L;

          if (flg_all_prompts_played) {
            Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t--- End of all prompts diffusion\n");
          }
        }
        // Fin: Traces de l'évolution du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
      }
      else {
        flg_all_prompts_played = false;
      }

      return false;
    }

    // Build frame to send and sending if no 'CMD_TIME_WAIT' found
#ifndef USE_SIMULATION
    String l__frame_tx_str = "";
#else
    std::string l__frame_tx_str = "";
#endif
    switch (i__num) {
    case TX_PLAY:
      l__frame_tx_str += "\nTX_PLAY ";
      break;
    case TX_OTHER:
      l__frame_tx_str += "\nTX_OTHER ";
      break;
    default:
      break;
    }

    // Emission vers le module MP3
    if (g__wt2003s->isInService() == true) {
      /* Module WT2003S
       *  - Translation
       *  - Emission
       */
      byte l__cmd_wt2003s[16];
      memset(l__cmd_wt2003s, '\0', sizeof(l__cmd_wt2003s));

      size_t l__size = translatePromptsKT403AToWT2003S(l__cmd_kt403a, sizeof(l__cmd_kt403a), l__cmd_wt2003s, true);

      if (l__size != 0) {
        // Write to 'WT2003S'...
        writeToWT2003S(l__cmd_wt2003s, l__size);
      }
    }
    else {
      // Module KT403A
      size_t n = 0;
      for (n = 0; n < CMD_LENGTH; n++) {
        char l__value = *(l__frame_tx_to_send + n);

        // Conversion Hexa-ASCII
        l__frame_tx_str += convHexa2Ascii((l__value >> 4) & 0xF);
        l__frame_tx_str += convHexa2Ascii(l__value & 0xF);

        // Send to 'Serial MP3 Player'
#ifndef USE_SIMULATION
        g__serialMP3Player.write(l__value);
#endif

#ifndef USE_SIMULATION
        delayMicroseconds(1100);    // Awaiting 1.1 mS between each byte (9600 bauds => 10 * 104 uS = 1.04 mS)
#endif
      }
    }
    // Fin: Emission vers le module MP3

    boolean l__flg_prompts_in_mp3_dir = false;
    boolean l__flg_prompts_in_advert_dir = false;
    int l__idx = IDX_PROMPT_MAX;

    switch (i__num) {
    case TX_PLAY:
      fifo_tx[TX_PLAY].frame_tx_str = l__frame_tx_str;

      //  Enable the 'callback Rx' for ACK_OK with this 'SELECT_SONG_IN_MP3_DIRECTORY' and 'INSERT_SONG_FROM_ADVERT_DIRECTORY' prompts
      if (l__event == SELECT_SONG_IN_MP3_DIRECTORY) {
        if (l__data == NUM_PROMPT_HELLO_WORLD
         || l__data == NUM_PROMPT_JE_SUIS__FEMALE
         || l__data == NUM_PROMPT_JE_SUIS__FEMALE_BIS
         || l__data == NUM_PROMPT_JE_SUIS__MALE
         || l__data == NUM_PLEASE_WAIT_GPS
         || l__data == NUM_PLEASE_WAIT_RECOVERY_GPS
         || l__data == NUM_GPS
         || l__data == NUM_RECOVERY_GPS
         || l__data == NUM_LOSS_GPS
         || l__data == NUM_SORRY_WAIT_AGAIN
         || l__data == NUM_OUPS_FEMALE
         || l__data == NUM_INTERSTELLAR_THEME) {
          l__flg_prompts_in_mp3_dir = true;
        }
      }
      else if (l__event == INSERT_SONG_FROM_ADVERT_DIRECTORY) {
        if (l__data == NUM_ADVERT_WAIT) {
          l__flg_prompts_in_advert_dir = true;
        }
      }

      l__idx = (l__event == SELECT_SONG_IN_MP3_DIRECTORY) ? IDX_PROMPT_NORMAL : IDX_PROMPT_ADVERT;

      if (fifo_tx[TX_PLAY].fct_callback != NULL && (l__flg_prompts_in_mp3_dir == true || l__flg_prompts_in_advert_dir == true)) {
        fifo_tx[TX_PLAY].fct_callback(l__event, l__data);
        automate.t_num_prompt_to_play[l__idx] = l__data;

        flg_callback = true;
      }
      else {
        automate.t_num_prompt_to_play[l__idx] = (uint16_t)-1;
      }

      break;
    case TX_OTHER:
      fifo_tx[TX_OTHER].frame_tx_str = l__frame_tx_str;

#if 0
      if (fifo_tx[TX_OTHER].fct_callback != NULL && l__event == TODO) {
        fifo_tx[TX_OTHER].fct_callback(l__event, l__data);

        flg_callback = true;        // Enable the 'callback Rx' for ACK_OK with the 'TODO' event and 'data'
      }
#endif

      break;

    default:
      break;
    }
  }
  else if (l__diff == 0) {
    // Aucune commande n'a été émise
    return false;
  }

  /* Commande émise -> Effacement de 'frame_tx_to_send' qui sera reaffirmé
   *  "ponctuellement" ou par l'automate (fin de diffusion par exemple)
  */
  // Aucune commande a émettre
  switch (i__num) {
  case TX_PLAY:
    fifo_tx[TX_PLAY].flg_tx_to_send = false;
    break;
  case TX_OTHER:
    fifo_tx[TX_OTHER].flg_tx_to_send = false;
    break;
  default:
    break;
  }

  return true;
}

// Test si aucunue commande n'est en cours dans la FIFO/Tx (Play ou Other) 
boolean SerialMP3Player::isCommandInProgress(FIFO_TX_NUM i__num)
{
  unsigned int l__idx_tx_write = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].idx_tx_write : fifo_tx[TX_OTHER].idx_tx_write;
  unsigned int l__idx_tx_read  = (i__num == TX_PLAY) ? fifo_tx[TX_PLAY].idx_tx_read : fifo_tx[TX_OTHER].idx_tx_read;

  int l__diff = (l__idx_tx_write - l__idx_tx_read);

  return (l__diff == 0) ? false : true;
}

#ifndef USE_SIMULATION
String SerialMP3Player::getFrameTx(FIFO_TX_NUM i__num)
#else
std::string SerialMP3Player::getFrameTx(FIFO_TX_NUM i__num)
#endif
{
  switch (i__num) {
  case TX_PLAY:
    return fifo_tx[TX_PLAY].frame_tx_str;
  case TX_OTHER:
    return fifo_tx[TX_OTHER].frame_tx_str;
    break;
  default:
    return "";
  }
}

void SerialMP3Player::reInitTxReadForRetry(FIFO_TX_NUM i__num)
{
#ifndef USE_SIMULATION
  char l__buffer[80];

  if (i__num == TX_PLAY) {
    /*  Recherche de la dernière séquence 'CMD_NUM_SEQ XX YY' suivie de:
     *  'NOP 00 00' + 'NOP 00 00' + '1st PLAY' + '1st TIME' + '2nd PLAY' + '2nd TIME' ...
     *  - Remplacement des 2 'NOP 00 00' + 'NOP 00 00' par une diffusion ddu message "Rediffusion" avec son time-out
     *  - Remplacement éventuel de la séquence correspondant au prompt "Message de test" par "Oups Male" pour ne pas
     *    créer un avertissement correspondant à l'expiration de 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
     *  - Ecriture de 'idx_tx_read' de la FIFO/Tx PLAY sur le 1st 'NOP 00 00'
     *  - Cumul des time-out lors de la recherche pour un armement de 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
     *  - Lancement de la diffusion...
     *
     *  Erreurs détectées:
     *   - Absence de 'CMD_NUM_SEQ XX YY' (bouclage de toute la FIFO) => Diffusion du prompt "Oups Male"
     *   - To be completed
     */

    hexDumpFifoTx();

    unsigned int l__idx_tx_write = fifo_tx[TX_PLAY].idx_tx_write;
    unsigned int l__idx_tx_read  = fifo_tx[TX_PLAY].idx_tx_read;
    char *l__frame_tx = fifo_tx[TX_PLAY].frame_tx;

    sprintf(l__buffer, "reInitTxReadForRetry(%d): r/w: [0x%02x/0x%02x]\n", i__num, l__idx_tx_read, l__idx_tx_write);
    Serial.print(l__buffer);

    boolean l__flg_break = false;
    boolean l__flg_found = false;
    size_t l__num_bytes = 0;

    char l__first_byte = 0;
    char l__second_byte = 0;
    unsigned int l__idx_tx_read_first_byte = 0;
    unsigned int l__idx_tx_read_second_byte = 0;

    long      l__total_duration = 0L;
    boolean   l__flg_total_duration = false;

    do {
      uint16_t  l__duration = 0;
      uint16_t  l__duration_prompt = 0;

      // Positionnement sur le dernier byte lu (ou écrit ;-)
      if (l__idx_tx_read > 0) {
        l__idx_tx_read--;
      }
      else {
        l__idx_tx_read = (NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX - 1);
      }

      char l__byte = *(l__frame_tx + l__idx_tx_read);

      switch (l__num_bytes % 3) {
      case 0:   // 2nd byte of command
        l__second_byte = l__byte;
        l__idx_tx_read_second_byte = l__idx_tx_read;
        break;

      case 1:   // 1st byte of command
        l__first_byte = l__byte;
        l__idx_tx_read_first_byte = l__idx_tx_read;
        break;

      case 2:   // Command
        if (l__byte == CMD_NOT_DEFINED_1) {
          sprintf(l__buffer, "\t#%d: 'CMD_NOT_DEFINED_1': [0x%02x] [0x%02x]\n", l__num_bytes, l__first_byte, l__second_byte);
          Serial.print(l__buffer);

          //  Remplacement par le prompt "Rediffusion du message" (BASE_RETRY_PROMPTS_DIFFUSION)
          *(l__frame_tx + l__idx_tx_read)             = g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].commands[0];
          *(l__frame_tx + l__idx_tx_read_first_byte)  = g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].commands[1];
          *(l__frame_tx + l__idx_tx_read_second_byte) = g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].commands[2];
        }
        else if (l__byte == CMD_NOT_DEFINED_2) {
          sprintf(l__buffer, "\t#%d: 'CMD_NOT_DEFINED_2': [0x%02x] [0x%02x]\n", l__num_bytes, l__first_byte, l__second_byte);
          Serial.print(l__buffer);

          //  Remplacement par le time-out du prompt "Rediffusion du message" (BASE_RETRY_PROMPTS_DIFFUSION)
          *(l__frame_tx + l__idx_tx_read)             = CMD_TIME_WAIT;

#if USE_REAL_PROMPTS_DURATION_FROM_TIME
          *(l__frame_tx + l__idx_tx_read_first_byte)  = (100 * g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].duration_from_time) / 256;
          *(l__frame_tx + l__idx_tx_read_second_byte) = (100 * g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].duration_from_time) % 256;
#else
          *(l__frame_tx + l__idx_tx_read_first_byte)  = (g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].duration_from_length / 10L) / 256;
          *(l__frame_tx + l__idx_tx_read_second_byte) = (g__prompts_general[BASE_RETRY_PROMPTS_DIFFUSION].duration_from_length / 10L) % 256;
#endif

          // Cumul des time-out pour le timer 'DURATION_TIMER_SHORT_FAMINE'
          // Durée du prompt en multiple de 10 mS
          l__duration = DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN;    // Durée minimale
          l__duration_prompt = 256 * (*(l__frame_tx + l__idx_tx_read_first_byte)) + (*(l__frame_tx + l__idx_tx_read_second_byte));

          /*  Durée du prompt (multiple de 10 mS - ie. 50 for 500 mS)
           *  => Durée minimale si durée nulle du prompt (DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN)
           *  => Sinon, valeur réelle de la durée du prompt
           *     => TBC: A vérifier si la diffusion n'est pas "coupée" ;-)
           */
          if (l__duration_prompt != 0) {
            l__duration = l__duration_prompt;
          }

          //  Ajout de 'l__duration' + marge à la durée du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
          l__total_duration += (long)l__duration;
          l__flg_total_duration = true;

          sprintf(l__buffer, "\t\t [%d mS] Total [%ld mS]\n", 10 * l__duration_prompt, 10L * l__total_duration);
          Serial.print(l__buffer);
          // Fin: Cumul des time-out pour le timer 'DURATION_TIMER_SHORT_FAMINE'
        }
        else if (l__byte == CMD_NUM_SEQ) {
          sprintf(l__buffer, "\t#%d: 'CMD_NUM_SEQ': [0x%02x] [0x%02x]\n", l__num_bytes, l__first_byte, l__second_byte);
          Serial.print(l__buffer);

          l__flg_break = true;
          l__flg_found = true;
        }
        else if (l__byte == CMD_TIME_WAIT) {
          sprintf(l__buffer, "\t#%d: 'CMD_TIME_WAIT': [0x%02x] [0x%02x]\n", l__num_bytes, l__first_byte, l__second_byte);
          Serial.print(l__buffer);

          // Cumul des time-out pour le timer 'DURATION_TIMER_SHORT_FAMINE'
          // Durée du prompt en multiple de 10 mS
          l__duration = DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN;    // Durée minimale
          l__duration_prompt = 256 * (*(l__frame_tx + l__idx_tx_read_first_byte)) + (*(l__frame_tx + l__idx_tx_read_second_byte));

          /*  Durée du prompt (multiple de 10 mS - ie. 50 for 500 mS)
           *  => Durée minimale si durée nulle du prompt (DURATION_TIMER_WAIT_FIFO_TX_PLAY_MIN)
           *  => Sinon, valeur réelle de la durée du prompt
           *     => TBC: A vérifier si la diffusion n'est pas "coupée" ;-)
           */
          if (l__duration_prompt != 0) {
            l__duration = l__duration_prompt;
          }

          //  Ajout de 'l__duration' + marge à la durée du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
          l__total_duration += (long)l__duration;
          l__flg_total_duration = true;

          sprintf(l__buffer, "\t\t [%d mS] Total [%ld mS]\n", 10 * l__duration_prompt, 10L * l__total_duration);
          Serial.print(l__buffer);
          // Fin: Cumul des time-out pour le timer 'DURATION_TIMER_SHORT_FAMINE'
        }
        else {
          sprintf(l__buffer, "\t#%d: 'PLAY': [0x%02x] [0x%02x] [0x%02x]\n", l__num_bytes, l__byte, l__first_byte, l__second_byte);
          Serial.print(l__buffer);

          /*  Remplacement du prompt "Message de test" par "Oups Male"
           *  => Le time-out n'est pas remplacé car < à celui déjà défini dans la FIFO/Tx Play
           */
#if USE_FLAT_STONE_THROW_INTO_WATER
          if (l__byte == g__prompts_general[BASE_FLAT_STONE_THROW_INTO_WATER].commands[0]
           && l__first_byte == g__prompts_general[BASE_FLAT_STONE_THROW_INTO_WATER].commands[1]
           && l__second_byte == g__prompts_general[BASE_FLAT_STONE_THROW_INTO_WATER].commands[2])
#else
          if (l__byte == g__prompts_general[BASE_TEST_MESSAGE].commands[0]
           && l__first_byte == g__prompts_general[BASE_TEST_MESSAGE].commands[1]
           && l__second_byte == g__prompts_general[BASE_TEST_MESSAGE].commands[2])
#endif
          {
            *(l__frame_tx + l__idx_tx_read)             = g__prompts_general[BASE_OUPS_MALE].commands[0];
            *(l__frame_tx + l__idx_tx_read_first_byte)  = g__prompts_general[BASE_OUPS_MALE].commands[1];
            *(l__frame_tx + l__idx_tx_read_second_byte) = g__prompts_general[BASE_OUPS_MALE].commands[2];
          }
        }
        break;

      default:
        break;
      }

      l__num_bytes++;
    }
    while (l__flg_break == false && l__idx_tx_read != l__idx_tx_write && l__num_bytes < NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX);

    if (l__flg_found == true) {
      sprintf(l__buffer, "\rRediffusion: r/w: [0x%02x/0x%02x]\n", l__idx_tx_read, l__idx_tx_write);
      Serial.print(l__buffer);

      if (l__flg_total_duration == true) {
        g__timers->addPromptsDuration(l__total_duration);
      }

      fifo_tx[TX_PLAY].idx_tx_read = l__idx_tx_read;

      // Force to play the all prompts
      setCommandSended(TX_PLAY, true);

      hexDumpFifoTx();
    }
  }
  else {
    sprintf(l__buffer, "reInitTxReadForRetry(%d): Ignored\n", i__num);
    Serial.print(l__buffer);
  }
#endif
}

void SerialMP3Player::hexDumpFifoRx()
{
  char l__buffer[80];
  std::ostringstream l__out;

  sprintf(l__buffer, "FIFO Rx Write/Read #0x%02X/#0x%02X\n", idx_rx_write_it, idx_rx_read_it);
  Serial.print(l__buffer);

  hexDump(l__out, frame_rx_it, NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX);
  Serial.print(l__out.str().c_str());
  Serial.print("\n");
}

void SerialMP3Player::hexDumpFifoTx()
{
  char l__buffer[80];
  std::ostringstream l__out_play;

  sprintf(l__buffer, "FIFO Tx Play Write/Read #0x%02X/#0x%02X\n", fifo_tx[TX_PLAY].idx_tx_write, fifo_tx[TX_PLAY].idx_tx_read);
  Serial.print(l__buffer);

  hexDump(l__out_play, fifo_tx[TX_PLAY].frame_tx, NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX);
  Serial.print(l__out_play.str().c_str());
  Serial.print("\n");

  std::ostringstream l__out_other;
  sprintf(l__buffer, "FIFO Tx Other Write/Read #0x%02X/#0x%02X\n", fifo_tx[TX_OTHER].idx_tx_write, fifo_tx[TX_OTHER].idx_tx_read);
  Serial.print(l__buffer);

  hexDump(l__out_other, fifo_tx[TX_OTHER].frame_tx, NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX);
  Serial.print(l__out_other.str().c_str());
  Serial.print("\n");
}

/*  Calcul de la checksum à emettre ou reçue
 */
uint16_t SerialMP3Player::checksum(char *i__from, size_t i__len)
{
  int16_t l__cks = 0;
  for (size_t n = 0; n < i__len; n++) {
    l__cks += *(i__from + n);
  }

  return -l__cks;
}

/* Automate gérant les enchaînements des réponses attendues (synchrones) ou non attendues (asynchrone)
 * en fonction des commandes émises vers le module KT403A
 * 
 * Méthode appelée après 'getFrameRx()', 'getFrameTx(TX_PLAY)' et 'getFrameTx(TX_OTHER)' en s'appuyant
 * sur l'état courant avec détermination des erreurs de séquencement, les time-out, etc.
 * 
 * Gestion implémentées:
 * - Séquence d'initialisation avec diffusion du message d'accueil
 * - Séquencement des diffusions d'une manière synchrone via les FIFO/Rx, FIFO/Tx Play et FIFO/Tx Other
 * - Paramétrage du module KT403A (Volume, Equaliseur, Pause, Release, etc.)
 * - etc.
 */
void SerialMP3Player::setCurrentState(AUTOMATE_STATE i__current_state)
{
  automate.current_state = i__current_state;
}

void SerialMP3Player::clearFctCallback()
{
  fifo_tx[TX_PLAY].flg_callback = false;
  fifo_tx[TX_PLAY].fct_callback = NULL;

  fifo_tx[TX_PLAY].flg_callback = false;
  fifo_tx[TX_PLAY].fct_callback = NULL;

  flg_callback = false;
  fct_callback = NULL;
}

void SerialMP3Player::clearSynchroPromptsExpected()
{
  size_t n = 0;
  for (n = 0; n < IDX_PROMPT_MAX; n++) {
    automate.t_num_prompt_to_play[n]   = (uint16_t)-1;    // Numéro du prompt à jouer pour la synchro
    automate.t_idx_of_prompt_played[n] = (uint16_t)-1;    // Index du prompt joué pour la synchro
  }

  automate.flg_last_tx_play        = false;
  automate.flg_last_tx_play_advert = false;
  automate.last_tx_other_opcode    = OPCODE_NONE;
}

void SerialMP3Player::clearNumAndIdxPromptToPlay()
{
  clearSynchroPromptsExpected();
}

void SerialMP3Player::purgeFIFOTxPlay()
{
  Serial.print("purgeFIFOTxPlay(): Entering...\n");

  nbr_call_purge_fifo_tx_play++;

  if (flg_all_prompts_played == true) {
    Serial.print("\t=> All prompts played\n");
  }
  else {
    Serial.print("\t=> All prompts not yet played\n");

    nbr_exec_purge_fifo_tx_play++;

    /* - Effacement de 'flg_all_prompts_played'
     * - Remise des pointeurs 'R/W' à l'origine
     * - Arrêt des timers @ famine
    */
    flg_all_prompts_played = true;          // Forçage à "tous les prompts joués"

    /* Raz FIFO/Tx Play
     * => Tous les prompts empilés seront perdus ;-)
    */
    fifo_tx[TX_PLAY].idx_tx_write    = 0;
    fifo_tx[TX_PLAY].idx_tx_read     = 0;
    fifo_tx[TX_PLAY].flg_tx_to_send  = false;

    memset(fifo_tx[TX_PLAY].frame_tx, '\0', sizeof(fifo_tx[TX_PLAY].frame_tx));

    // Arrêt systématique des timers "famine"
    g__timers->stop(TIMER_SHORT_FAMINE_FIFO_TX_PLAY);
    g__timers->stop(TIMER_LONG_FAMINE_FIFO_TX_PLAY);
  }
}

// Set the duration internal for the calcul of the max duration between 2 prompts played ('STATUS_FILE_END' received)
void SerialMP3Player::setModeConnected(boolean i__flg) {
  flg_connected = i__flg;

  if (flg_connected == false) {
    duration_internal_previous = 0L;
  }
}

void SerialMP3Player::setDurationInternal(long i__value)
{
  // Update if value not equal to zero
  if (i__value != 0L) {
    duration_internal_current = i__value;
  }
}

void SerialMP3Player::exec_automate(AUTOMATE_PILOT i__origin)
{
  if (automate.flg_skip_automate == true) {
    return;
  }

  // TODO: Optimization with constant strings definitions + direct acces @ index ;-)
  char l__buffer[132];
  const char *l__event_previous_text = NULL;
  const char *l__event_text = NULL;

  size_t n = 0;
  for (n = 0; n < sizeof(g__event)/sizeof(g__event[0]); n++) {
    if (g__event[n].text != NULL) {
      // Update @ 'event_previous'
      if (g__event[n].value == automate.event_previous) {
        l__event_previous_text = g__event[n].text;
      }

      // Update @ 'event'
      if (g__event[n].value == automate.event) {
        l__event_text = g__event[n].text;
      }

      // Break if the two 'event_previous' and 'event' event found (optimization)
      if (l__event_previous_text != NULL && l__event_text != NULL) {
        break;
      }
    }
  }

  if (l__event_previous_text == NULL) l__event_previous_text = "Unknown";
  if (l__event_text == NULL) l__event_text = "Unknown";

  const char *l__origin_text = "Unknown";
  if (i__origin == TREATMENT_TX_PLAY) {
    l__origin_text = "TX_PLAY";
  }
  else if (i__origin == TREATMENT_TX_OTHER) {
    l__origin_text = "TX_OTHER";
  }
  else if (i__origin == TREATMENT_RX) {
    l__origin_text = "RX";
  }
  // End: TODO: Optimization with constant strings definitions + direct acces @ index ;-)

  sprintf(l__buffer, "automate(%d): %s: Current state [%d] (%s)\n",
    i__origin, l__origin_text, automate.current_state, g__states[automate.current_state].text);
  Serial.print(l__buffer);

  sprintf(l__buffer, "\tPrevious event [0x%02x] (%s) -> [0x%02x] (%s)\n",
    automate.event_previous, l__event_previous_text, automate.event, l__event_text);
  Serial.print(l__buffer);

  // Resolution @ current state and this event received or transmitted
  switch (i__origin) {
  case TREATMENT_TX_PLAY:     // [TX_PLAY events
    switch (automate.event) {
    case SELECT_SONG_IN_MP3_DIRECTORY:
    case SELECT_SOURCE_FOLDER_AND_SONG:
    case SELECT_SOURCE_BIG_FOLDER_AND_SONG:
      automate.current_state = STATE_IDLE;
      automate.flg_last_tx_play = true;
      automate.flg_last_tx_play_advert = false;
      break;

    case INSERT_SONG_FROM_ADVERT_DIRECTORY:
      automate.current_state = STATE_IDLE;
      automate.flg_last_tx_play = true;
      automate.flg_last_tx_play_advert = true;
      break;

    default:
      sprintf(l__buffer, "TREATMENT_TX_PLAY: Error: Event [0x%02x] (%s) not expected\n", automate.event, l__event_text);
      Serial.print(l__buffer);
      // TODO: Set the error of automate progression ;-)
      break;
    }
    break;                    // TX_PLAY events]

  case TREATMENT_TX_OTHER:    // [TX_OTHER events
    switch (automate.event) {
    case RESET_PLAYER:
      automate.current_state = STATE_WAIT_RSP_RESET_ACK_OK;
      break;

    case SELECT_SOURCE_DEVICE:
      automate.current_state = STATE_WAIT_RSP_DEVICE_ACK_OK;
      break;

    // Event 'Other' sans traitement particulier ici
    case QUERY_TRACK_NUMBER:
      // Save 
      break;

    case SET_EQUALIZER:
      automate.current_state = STATE_WAIT_SET_EQUALIZER_ACK_OK;
      break;

    case SET_VOLUME_LEVEL:
      automate.current_state = STATE_WAIT_SET_VOLUME_LEVEL_ACK_OK;
      break;

    case QUERY_VOLUME:
      automate.current_state = STATE_WAIT_QUERY_VOLUME_ACK_OK;
      // Save the level value;
      break;

    case STOP_MUSIC:
      break;

    default:
      sprintf(l__buffer, "TREATMENT_TX_OTHER: Error: Event [0x%02x] (%s) not expected\n", automate.event, l__event_text);
      Serial.print(l__buffer);
      // TODO: Set the error of automate progression ;-)
      break;
    }
    break;                    // TX_OTHER events]

  case TREATMENT_RX:          // [RX events
    switch (automate.event) {
    case STATUS_TF_REMOVE:
      Serial.print("\tSDCard removed\n");
      automate.current_state = STATE_TF_REMOVE;
      break;

    case STATUS_TF_INSERT:
      Serial.print("\tSDCard inserted\n");
      automate.current_state = STATE_TF_INSERT;

      Serial.print("\tReinitialization of SDCard...\n");
      // Initialize of the module KT403A ('Reset Player' -> 'Select Source Device' (by automate) -> 'Prompt' diffusion (by automate))
      writeFifoTx(TX_OTHER, g__command_reset_player, sizeof(g__command_reset_player));

      // Launch the 'Reset Player' command
      setCommandSended(TX_OTHER, true);
      // End: Initialize of the module KT403A ('Reset Player' -> 'Select Source Device' (by automate) -> 'Prompt' diffusion (by automate))
      break;

    case STATUS_ACK_OK:
      switch (automate.current_state) {
      case STATE_IDLE:
        if (fct_callback != NULL && flg_callback == true) {
          fct_callback(automate.event, automate.data);
        }
        break;

      case STATE_WAIT_RSP_RESET_ACK_OK:
        automate.current_state = STATE_WAIT_RSP_RESET_INIT;
        break;

      case STATE_WAIT_RSP_DEVICE_ACK_OK:
        // Set Equalizer at the frame value
        configuration.equalizer = g__command_set_equalizer_x05[2];

        writeFifoTx(TX_OTHER, g__command_set_equalizer_x05, sizeof(g__command_set_equalizer_x05));          // Non testable with 'QUERY_EQUALIZER' ;-(
        setCommandSended(TX_OTHER, true);       // Launch the 'Set Equalizer'
        break;

      case STATE_WAIT_SET_EQUALIZER_ACK_OK:
        // Set Volume Level to 26/31
        writeFifoTx(TX_OTHER, g__command_set_volume_level_x1a, sizeof(g__command_set_volume_level_x1a));    // 2nd command testable with 'QUERY_VOLUME'
        setCommandSended(TX_OTHER, true);       // Launch the 'Set Volume Level' command
        break;

     case STATE_WAIT_SET_VOLUME_LEVEL_ACK_OK:
        // Get the volume level after the set volume ;-)
        writeFifoTx(TX_OTHER, g__command_query_volume_level, sizeof(g__command_query_volume_level));    // Verif of the 'Set Volume Level'
        setCommandSended(TX_OTHER, true);       // Launch the 'Query Volume Level' command
        break;

      default:
        sprintf(l__buffer, "STATUS_ACK_OK: Error: Current state [%d] (%s) not expected\n", automate.current_state, g__states[automate.current_state].text);
        Serial.print(l__buffer);
        break;
      }
      break;

    case STATUS_INIT:
      switch (automate.current_state) {
      case STATE_WAIT_RSP_RESET_INIT:
        writeFifoTx(TX_OTHER, g__command_select_source_device, sizeof(g__command_select_source_device));
        setCommandSended(TX_OTHER, true);       // Launch the 'Select Source Device' command
        break;

      default:
        sprintf(l__buffer, "STATUS_INIT: Error: Current state [%d] (%s) not expected\n", automate.current_state, g__states[automate.current_state].text);
        Serial.print(l__buffer);
        break;
      }
      break;

    case STATUS_FILE_END:

    {
      sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- STATUS_FILE_END received\n");
      Serial.print(l__buffer);

      /*  Diffusion éventuelle des prompts suivants qui ont été écrits dans la FIFO/Tx PLAY (idx_tx_write != idx_tx_read)
       *  => Permet d'empiler une multitude de prompts les uns derrière les autres et
       *     qui seront joués successivement sans être stoppés
       *  => Si (idx_tx_write == idx_tx_read), tous les prompts ont été joués
       *     => Affirmation de 'flg_all_prompts_played' (cf. isAllPromptsPlayed())
       */
      if (fifo_tx[TX_PLAY].idx_tx_write == fifo_tx[TX_PLAY].idx_tx_read) {
        flg_all_prompts_played = true;
        setCommandSended(TX_PLAY, false);
      }
      else {
        flg_all_prompts_played = false;
        setCommandSended(TX_PLAY, true);
      }

      // Durée max entre 2 'STATUS_FILE_END' en mode connecté (calcul si 'duration_internal_previous' != 0L)
      if (flg_connected == true) {
        if (duration_internal_previous != 0L) {
          long l__diff_duration = (duration_internal_current - duration_internal_previous);

          if (l__diff_duration > max_duration_between_prompts) {
            max_duration_between_prompts = l__diff_duration;      // New max value
          }
        }

        duration_internal_previous = duration_internal_current;
      }

      /* Prise de la durée de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' avant son arrêt
       * => Cette durée diminuée de 'DURATION_TIMER_VERY_LONG_FAMINE' doit correspondre à la durée
       *    du prompt diffusé après la purge de la FIDO Tx/Play reportée dans le min et max
       *    qui doivent être très proches au fur et à mesure des reprises ;-)
       */
      if (g__timers->isInUse(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY)) {
        long l__duration_very_long = g__timers->getDuration(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY);
        g__errors->updateMinMaxDurationsOfVeryLongFamine(l__duration_very_long);

        g__timers->stop(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY);
      }
      // Fin: Prise de la durée de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' avant son arrêt

      // Traces de l'évolution du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
      {
        long l__duration_short_famine = g__timers->getDuration(TIMER_SHORT_FAMINE_FIFO_TX_PLAY);
        size_t l__num_prompt_played = g__timers->getAncIncNumPromptPlayed();

        if (l__duration_short_famine >= 0L) {
          long l__diff = (l__duration_short_famine - g__duration_short_famine);

          // Comptabilisation de la durée totale des diffusions...
          if (l__diff < 0L) {
            g__timers->addPromptsDurationTotal(-l__diff);
          }

          sprintf(l__buffer, "\t#%d: End of prompt diffusion (flg_all_prompts_played: %d) (%ld mS)",
            l__num_prompt_played, flg_all_prompts_played, 10L * l__duration_short_famine);

          if (l__diff != 0L) {
            sprintf(&l__buffer[strlen(l__buffer)], " (Diff: %ld mS)", 10L * l__diff);
          }
          if (l__diff > 0L) {
            sprintf(&l__buffer[strlen(l__buffer)], " (init)");
          }

          Serial.println(l__buffer);

          g__duration_short_famine = l__duration_short_famine;
        }
      }
      // Fin: Traces de l'évolution du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'

      if (fct_callback != NULL && flg_callback == true) {
        fct_callback(automate.event, automate.data);

        /* Conservation de 'flg_callback' à 'true' pour le support de l'advert
         * => 2 prompts sont en fait en cours de diffusion :-)
         */
      }
      }

      break;

    case QUERY_TRACK_NUMBER:
      if (fct_callback != NULL && flg_callback == true) {
        sprintf(l__buffer, "\tTrack #%d - 0x%04x in progress...\n", automate.data, automate.data);
        Serial.print(l__buffer);

        // Limitation aux 2 seules origines de Tx/Play ('advert' or normal)
        if (automate.last_tx_other_opcode != OPCODE_NONE) {
          sprintf(l__buffer, "\tLast Opcode Tx/Other: 0x%02x\n", automate.last_tx_other_opcode);
          Serial.print(l__buffer);
        }
        else if (automate.flg_last_tx_play == true) {
          ENUM_IDX_PROMPT l__enum_idx_prompt = automate.flg_last_tx_play_advert == true ? IDX_PROMPT_ADVERT : IDX_PROMPT_NORMAL;
          automate.t_idx_of_prompt_played[l__enum_idx_prompt] = automate.data;

          sprintf(l__buffer, "\tLast Opcode Tx/Play: %d (0:Normal 1:Advert)\n", l__enum_idx_prompt);
          Serial.print(l__buffer);
        }
      }
 
      break;

    case QUERY_VOLUME:
      /*  TODO:
       *  - Ajouter un critère et
       *  - Lever l'erreur lorsque le menu "Set Volume Level" sera implémenté ;-)
        */
      if (automate.current_state == STATE_WAIT_QUERY_VOLUME_ACK_OK) {
        // Set Volume Level at the frame value
        configuration.volume_level = (byte)(automate.data & 0xff);

        /* Amorce de la suite des messages modifie suite a l'accueil de la gestion du volume
         * => Ajout d'un flag representant 'Module KT403A initialized' ;-)
         */
        if (flg_module_initialized == false) {
          flg_module_initialized = true;

          writeFifoTx(TX_PLAY,  g__command_prompt, sizeof(g__command_prompt));
          setCommandSended(TX_PLAY, true);        // Launch the 'Prompt' diffusion command

          Serial.print("\tModule KT403A initialized\n");

          automate.current_state = STATE_IDLE;
        }
      }
      else {
        sprintf(l__buffer, "TREATMENT_RX: QUERY_VOLUME: State [0x%02x] (%s) not expected\n",
          automate.current_state, g__states[automate.current_state].text);
        Serial.print(l__buffer);    
      }

      break;

    default:
      sprintf(l__buffer, "TREATMENT_RX: Error: Event [0x%02x] (%s) not expected\n", automate.event, l__event_text);
      Serial.print(l__buffer);
      // TODO: Set the error of automate progression ;-)
      break;
    }
    break;    // RX events]
      
  default:
    sprintf(l__buffer, "Error: Origin [%d] unknown\n", i__origin);
    Serial.print(l__buffer);
    // TODO: Set the error of automate progression ;-)
    break;
  }
  // End: Resolution @ current state and this event received

  sprintf(l__buffer, "\tNew current state [%d] (%s)\n", automate.current_state, g__states[automate.current_state].text);
  Serial.print(l__buffer);

  automate.event_previous = automate.event;
}

/* Gestion du module SerialMP3Player 'WT2003S'
 * - writeToWT2003S(): Emission directe d'un 'opcode' et ses 'datas'
 *   apres calcul de la checksum (ie. 7e len opcode data_0 data_1 ... data_n cks ef)
 */
void SerialMP3Player::writeToWT2003S(byte *i__value, size_t i__nbr_bytes, boolean i__flg_trace)
{
  char l__buffer[80];
  memset(l__buffer, '\0', sizeof(l__buffer));

  char l__frame_tx_to_send[16];
  char *l__ptr_frame_tx_to_send = l__frame_tx_to_send;
  memset(l__frame_tx_to_send, '\0', sizeof(l__frame_tx_to_send));

  char l__len = (i__nbr_bytes + 2);     // 'opcode' + 'datas' + cks + COMMAND_BYTE_STOP

  // Construction de la trame
  *l__ptr_frame_tx_to_send++ = COMMAND_BYTE_START;
  *l__ptr_frame_tx_to_send++ = l__len;

  char l__cks = l__len;                 // Checksum a partir de 'len'

  size_t n = 0;
  for (n = 0; n < i__nbr_bytes; n++) {
    *l__ptr_frame_tx_to_send++ = *i__value;
    l__cks += *i__value++;
  }

  *l__ptr_frame_tx_to_send++ = l__cks;
  *l__ptr_frame_tx_to_send = COMMAND_BYTE_STOP;
  // Fin: Construction de la trame

  // Emission de la trame
  if (i__flg_trace) {
    sprintf(l__buffer, "writeToWT2003S: [");
  }
  for (n = 0; n < ((size_t)l__len + 2); n++) {    // Add 'COMMAND_BYTE_START' et 'len'
    char l__value = l__frame_tx_to_send[n];

    // Send to 'Serial MP3 Player'
    g__serialMP3Player.write(l__value);

    if (i__flg_trace) {
      sprintf(&l__buffer[strlen(l__buffer)], (n == 0 ? "%02x" : " %02x"), l__value);
    }

    // Warning: No delay with the module WT2003S ;-)
    //delayMicroseconds(1100);    // Awaiting 1.1 mS between each byte (9600 bauds => 10 * 104 uS = 1.04 mS)
  }

  if (i__flg_trace) {
    sprintf(&l__buffer[strlen(l__buffer)], "]\n");
    Serial.print(l__buffer);
  }
  // Fin: Emission de la trame

  // Interpretation de la commande
  byte l__datas[8];
  memset(l__datas, 0xff, sizeof(l__datas));
  memcpy(l__datas, &l__frame_tx_to_send[3], min(i__nbr_bytes - 1, sizeof(l__datas)));
  g__wt2003s->interpretCmdOrResp(TYPE_CMD, (WT2003S_OPCODE)l__frame_tx_to_send[2], i__nbr_bytes, l__datas);

  // Maj des statistiques du module WT2003S
  g__wt2003s->incCmdSentTotal();
}

void SerialMP3Player::execStatusFileEnd() {
    char l__buffer[132];

      sprintf(l__buffer, "\t\t\t\t\t\t\t\t\t\t\t\t--- STATUS_FILE_END received\n");
      Serial.print(l__buffer);

      /*  Diffusion éventuelle des prompts suivants qui ont été écrits dans la FIFO/Tx PLAY (idx_tx_write != idx_tx_read)
       *  => Permet d'empiler une multitude de prompts les uns derrière les autres et
       *     qui seront joués successivement sans être stoppés
       *  => Si (idx_tx_write == idx_tx_read), tous les prompts ont été joués
       *     => Affirmation de 'flg_all_prompts_played' (cf. isAllPromptsPlayed())
       */
      if (fifo_tx[TX_PLAY].idx_tx_write == fifo_tx[TX_PLAY].idx_tx_read) {
        flg_all_prompts_played = true;
        setCommandSended(TX_PLAY, false);
      }
      else {
        flg_all_prompts_played = false;
        setCommandSended(TX_PLAY, true);
      }

      // Durée max entre 2 'STATUS_FILE_END' en mode connecté (calcul si 'duration_internal_previous' != 0L)
      if (flg_connected == true) {
        if (duration_internal_previous != 0L) {
          long l__diff_duration = (duration_internal_current - duration_internal_previous);

          if (l__diff_duration > max_duration_between_prompts) {
            max_duration_between_prompts = l__diff_duration;      // New max value
          }
        }

        duration_internal_previous = duration_internal_current;
      }

      /* Prise de la durée de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' avant son arrêt
       * => Cette durée diminuée de 'DURATION_TIMER_VERY_LONG_FAMINE' doit correspondre à la durée
       *    du prompt diffusé après la purge de la FIDO Tx/Play reportée dans le min et max
       *    qui doivent être très proches au fur et à mesure des reprises ;-)
       */
      if (g__timers->isInUse(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY)) {
        long l__duration_very_long = g__timers->getDuration(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY);
        g__errors->updateMinMaxDurationsOfVeryLongFamine(l__duration_very_long);

        g__timers->stop(TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY);
      }
      // Fin: Prise de la durée de 'TIMER_VERY_LONG_FAMINE_FIFO_TX_PLAY' avant son arrêt

      // Traces de l'évolution du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'
      {
        long l__duration_short_famine = g__timers->getDuration(TIMER_SHORT_FAMINE_FIFO_TX_PLAY);
        size_t l__num_prompt_played = g__timers->getAncIncNumPromptPlayed();

        if (l__duration_short_famine >= 0L) {
          long l__diff = (l__duration_short_famine - g__duration_short_famine);

          // Comptabilisation de la durée totale des diffusions...
          if (l__diff < 0L) {
            g__timers->addPromptsDurationTotal(-l__diff);
          }

          sprintf(l__buffer, "\t#%d: End of prompt diffusion (flg_all_prompts_played: %d) (%ld mS)",
            l__num_prompt_played, flg_all_prompts_played, 10L * l__duration_short_famine);

          if (l__diff != 0L) {
            sprintf(&l__buffer[strlen(l__buffer)], " (Diff: %ld mS)", 10L * l__diff);
          }
          if (l__diff > 0L) {
            sprintf(&l__buffer[strlen(l__buffer)], " (init)");
          }

          Serial.println(l__buffer);

          g__duration_short_famine = l__duration_short_famine;
        }
      }
      // Fin: Traces de l'évolution du timer 'TIMER_SHORT_FAMINE_FIFO_TX_PLAY'

      if (fct_callback != NULL && flg_callback == true) {
        fct_callback(automate.event, automate.data);

        /* Conservation de 'flg_callback' à 'true' pour le support de l'advert
         * => 2 prompts sont en fait en cours de diffusion :-)
         */
      }
}

// Fin:  Gestion du module SerialMP3Player 'WT2003S'
