// $Id: SerialMP3Player.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __SERIAL_MP3_PLAYER__
#define __SERIAL_MP3_PLAYER__

#include "Misc.h"

// List of constant commands ('Reset Player', 'Select Source Device', 'Prompt', etc.) to write into FIFO/Tx Play and Other
extern byte g__command_reset_player[3];
extern byte g__command_select_source_device[3];
extern byte g__command_prompt[3];

extern byte g__command_query_track_number[3];
extern byte g__command_stop_music[3];
// End: List of constant commands ('Reset Player', 'Select Source Device', 'Prompt', etc.) to write into FIFO/Tx Play and Other

#define USE_STATS_SERIAL_MP3_PLAYER       1

#define RXD2             16   // @KiCad: Reception des trames du lecteur MP3 à 9600 bauds
#define TXD2             17   // @KiCad: Emission des trames vers le lecteur MP3 à 9600 bauds

#define NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX        256
#define NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX        256

#define CONST_LENGTH                              6

// Définitions des position dans la trame de commande (10 bytes)
typedef enum {
  CMD_START = 0,    // [0]: Start
  CMD_VERSION,      // [1]: Version
  CMD_LEN,          // [2]: Length position
  CMD_COMMAND,      // [3]: Command
  CMD_FEEDBACK,     // [4]: Feedback
  CMD_DATA_MSB,     // [5]: Data Hi
  CMD_DATA_LSB,     // [6]: Data Lo
  CMD_CKS_MSB,      // [7]: Checksum Hi
  CMD_CKS_LSB,      // [8]: Checksum Lo
  CMD_STOP,         // [9]: Stop
  CMD_LENGTH        // Length of the command
} POS_OF_CMD_DATA;

// Définitions des position dans la trame de réponse (10 bytes)
typedef enum {
  RSP_START = 0,    // [0]: Start
  RSP_VERSION,      // [1]: Version
  RSP_LEN,          // [2]: Length position
  RSP_STATUS,       // [3]: Status
  RSP_FEEDBACK,     // [4]: Feedback
  RSP_DATA_MSB,     // [5]: Data Hi
  RSP_DATA_LSB,     // [6]: Data Lo
  RSP_CKS_MSB,      // [7]: Checksum Hi
  RSP_CKS_LSB,      // [8]: Checksum Lo
  RSP_STOP,         // [9]: Stop
  RSP_LENGTH        // Length of the response
} POS_OF_RSP_DATA;

// Constantes pour la communication avec les modules KT403A et WT2003S
typedef enum {
  COMMAND_BYTE_START   = 0x7e,    // KT403A et WT2003S
  COMMAND_BYTE_STOP    = 0xef,    // KT403A et WT2003S
  COMMAND_BYTE_VERSION = 0xff     // KT403A uniquement
} CONST_COMMAND_BYTE;

// Constantes pour la commande setEqualizer()
typedef enum {
  EQUALIZER_NORMAL = 0x00,
  EQUALIZER_POP,
  EQUALIZER_ROCK,
  EQUALIZER_JAZZ,
  EQUALIZER_CLASSIC,
  EQUALIZER_BASS
} CONST_EQUALIZER;

// Constantes pour la commande selectSourceDevice()
typedef enum {
  DEVICE_UDISK = 0x01,
  DEVICE_SDCARD,
  DEVICE_AUX,       // Not used
  DEVICE_PC,        // Debug only
  DEVICE_FLASH,
  DEVICE_SLEEP
} CONST_DEVICE;

typedef enum {
  OPCODE_NONE =                           0x00,

  // Events in transmission
  PLAY_NEXT_TRACK =                       0x01,
  PLAY_PREVIOUS_TRACK =                   0x02,
  PLAY_TRACK_NUMBER =                     0x03,
  SET_VOLUME_UP =                         0x04,
  SET_VOLUME_DOWN =                       0x05,
  SET_VOLUME_LEVEL =                      0x06,
  SET_EQUALIZER =                         0x07,
  REPEAT_SINGLE_TRACK =                   0x08,
  SELECT_SOURCE_DEVICE =                  0x09,
  ENTER_SLEEP_MODE =                      0x0a,
  EXIT_SLEEP_MODE =                       0x0b,
  RESET_PLAYER =                          0x0c,
  PLAY_RESUME_MUSIC =                     0x0d,
  PAUSE_MUSIC =                           0x0e,
  SELECT_SOURCE_FOLDER_AND_SONG =         0x0f,
  LOOP_ALL_MUSIC =                        0x11,
  SELECT_SONG_IN_MP3_DIRECTORY =          0x12,
  INSERT_SONG_FROM_ADVERT_DIRECTORY =     0x13,
  SELECT_SOURCE_BIG_FOLDER_AND_SONG =     0x14,
  STOP_INSERTED_SONG_AND_RESUME_PLAYING = 0x15,
  STOP_MUSIC =                            0x16,
  LOOP_FOLDER =                           0x17,
  SHUFFLE_PLAY =                          0x18,
  REPEAT_CURRENT_TRACK =                  0x19,
  MUTE_SOUND =                            0x1a,
  //PLAY_TRACK_WITH_VOLUME =              0x22,     // Not operational
  SHUFFLE_FOLDER =                        0x28,
  // End: Events in transmission

  // Events in reception
  WT2003S_OK =                            0x00,
  WT2003S_KO =                            0x01,

  STATUS_TF_INSERT =                      0x3a,     // TF Card was inserted (unsolicited)
  STATUS_TF_REMOVE =                      0x3b,     // TF card was removed (unsolicited)
  STATUS_FILE_END  =                      0x3d,     // Track/file has ended (unsolicited)
  STATUS_INIT      =                      0x3f,     // Initialization complete (unsolicited)
  STATUS_ERR_FILE  =                      0x40,     // Error file not found
  STATUS_ACK_OK    =                      0x41,     // Message acknowledged ok
  QUERY_DEVICE =                          0x42,
  QUERY_VOLUME =                          0x43,
  QUERY_EQUALIZER =                       0x44,     // Not Operational ;-)
  QUERY_FOLDER_FILES =                    0x48,
  QUERY_TRACK_NUMBER =                    0x4c,
  STATUS_FLDR_FILES =                     0x4e,     // Total number of files in the folder
  QUERY_TOTAL_FOLDERS =                   0x4f,
  // End: Events in reception

  // Other events in transmission (Tx/Play)
  CMD_NUM_SEQ =                           0x80,     // Numéro de séquence pour une reprise de la diffusion
  CMD_NOT_DEFINED_1 =                     0x81,     // Pas encore défini #1 (gestion de la rediffusion)
  CMD_NOT_DEFINED_2 =                     0x82,     // Pas encore défini #2
  CMD_TIME_WAIT =                         0x83,     // Attente entre la diffusion de 2 prompts (10 mS * nnn)

  // Other events in reception (Rx)
  WT2003S_CMD_PLAY_START =                0x90,     // Pseudo evenement 'Play Start' du module WT2003S
  WT2003S_CMD_PLAY_END =                  0x91,     // Pseudo evenement 'Play End' du module WT2003S
  // End: Other events

  INVALID_OPCODE =                        0xff
} CMD_OPCODE;

typedef struct {
  const char  *text;
  char        value;
  boolean     flg_err;
} ST_STATUS_KT403A;

typedef struct {
  const char  *text;
  char        value;
} ST_STATES;

typedef enum {
  TX_PLAY = 0,
  TX_OTHER,
  NBR_FIFO_TX           // Nombre de FIFO/Tx
} FIFO_TX_NUM;

/* FIFO circulaires des caractères à emettre vers le module KT403A (commandes 'PLAY' et 'OTHER')
 *  Remarques: - Seuls les caractères 'CMD_COMMAND', 'CMD_DATA_MSB' et 'CMD_DATA_LSB' sont empilés
 *               Les autres sont constants ou déterminés comme la checksum
 *             - La 1st FIFO consignent les commandes 'PLAY' qui doivent être cadencées au fur et
 *               à mesure des diffusions
 *             - La 2nd FIFO consignent toutes les autres commandes pouvant être émises en // des
 *               commandes 'PLAY'; l'insertion des 'ADVERT' (publicités ;-) font également parties
 *               de ces commandes puisque jouées en // de la diffusion en cours
 *
 *  La gestion des pointeurs 'idx_tx_write', 'idx_tx_read', 'idx_rx_write' et 'idx_rx_read'
 *  de la ou des FIFOs repose sur les grands principes suivants; à savoir:
 *   - Incrémentation après écriture et lecture de la position courante
 *     => Les pointeurs sont initialisés à 0 à la création de la FIFO
 *   - Si égalité des 2 pointeurs écriture et lecture après une opération de lecture:
 *     => Toutes les données ont été lues
 *        => la FIFO est vide (cas normal :-)
 *   - Si le pointeur d'écriture rejoint le pointeur de lecture après une opération d'écriture:
 *     => la FIFO est pleine (cas d'erreur :-()
 *   - Vidage de la FIFO en faisant rejoindre le pointeur de lecture "vers" le pointeur d'écriture
 *     jusqu'à l'égalité des 2 pointeurs de lecture et d'écriture
 *   - La taille de la FIFO est une puissance de 2 pour optimiser les calculs modulo cette taille
 *     qui doit est suffisaement dimensionnée pour éviter sa saturation lors d'écritures sous It dans
 *     le cas par exemple de la réception d'une liaison "temps réel" et d'une lecture pour un traitement
 *     en fond de tâche en s'interdisant tout traitement "lourd" sour It, en particulier les tests de
 *     reconnaissance de pattern dans les trames reçues ;-)
 *     => Il va de soit que des mutex doivent être posés pour se protéger des accès concurents aux variables
 *        partagées lors de leurs lectures, écritures et tests qui ne seraient pas atomiques par rapport à
 *        leur taille définies sur 8, 16, 32 voire 64 bits
 */
typedef struct {
  unsigned int        idx_tx_write;
  unsigned int        idx_tx_read;
  char                frame_tx[NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_TX];

  boolean             flg_tx_to_send_pre;
  boolean             flg_tx_to_send;

#ifndef USE_SIMULATION
  String              frame_tx_str;
#else
  std::string         frame_tx_str;
#endif
  char                frame_tx_to_send[CMD_LENGTH];

  boolean             flg_callback;                             // If true, enable the call at the 'fct_callback' method
  boolean             (*fct_callback)(CMD_OPCODE, uint16_t);    // Callback function called via the FIFO/Tx

  uint16_t            num_sequence_pre;
  uint16_t            num_sequence;

} ST_FIFO_TX;

// Pilotage de l'automate @ à son appel de méthode
typedef enum {
  TREATMENT_TX_PLAY  = TX_PLAY,       // #0
  TREATMENT_TX_OTHER = TX_OTHER,      // #1
  TREATMENT_RX                        // #2
} AUTOMATE_PILOT;

/* States of automate
 *  Hormis lors de l'initialisation et la détection de la SDCard où un séquancement
 *  commandes/réponses est strictement vérifié, l'automate est au repos et à l'écoute d'une
 *  réponse asynchrone (ie. 'Fin de diffusion d'un prompt', résultat s'un "query', etc.)  
 */
typedef enum {
  STATE_UNKNOWN = 0,               // 0: Non initialisé

  // Initialization of KT403A module
  STATE_WAIT_RSP_RESET_ACK_OK,      // 1: Send RESET_PLAYER -> STATUS_ACK_OK expected
  STATE_WAIT_RSP_RESET_INIT,        // 2:                   -> STATUS_INIT expected
  STATE_WAIT_RSP_DEVICE_ACK_OK,     // 3: Send SELECT_SOURCE_DEVICE -> STATUS_ACK_OK expected

  // Synchro. pour 'Set Equalizer', 'Set Volume Level' et 'Query Volume'
  STATE_WAIT_SET_EQUALIZER_ACK_OK,      // 4: 
  STATE_WAIT_SET_VOLUME_LEVEL_ACK_OK,   // 5:
  STATE_WAIT_QUERY_VOLUME_ACK_OK,       // 6:

  // SDcard removed/inserted
  STATE_TF_REMOVE,                  // 7: SDCard removed
  STATE_TF_INSERT,                  // 8: SDCard inserted

  STATE_IDLE,                       // 9: Automate au repos (aucune commande envoyée et aucune réponse synchrone attendue)

  STATE_NUMBER                      // Nombre d'états de l'automate
} AUTOMATE_STATE;

// Definitions for 't_num_prompt_to_play[]' and 't_idx_of_prompt_played[]'
typedef enum {
  IDX_PROMPT_NORMAL = 0,            // Rang pour mémoriser les prompts "normaux" (autres que 'advert')
  IDX_PROMPT_ADVERT,                // Rang pour mémoriser les prompts "advert"
  IDX_PROMPT_MAX
} ENUM_IDX_PROMPT;

typedef struct {
  boolean             flg_skip_automate;
  AUTOMATE_STATE      current_state;
  CMD_OPCODE          event_previous;
  CMD_OPCODE          event;
  uint16_t            data;

  // Attributes for synchronization with the end of prompt diffusion
  // Mémorisation type commande Tx/Play ('advert' or normal) ou Tx/Other pour connaître l'origine du 'ACK_OK'
  boolean             flg_last_tx_play;           // If last opcode via FIFO Tx/Play
  boolean             flg_last_tx_play_advert;    // If last opcode via FIFO Tx/Play and 'advert'
  CMD_OPCODE          last_tx_other_opcode;       // All opcodes via FIFO Tx/Other in // or not of the 'last_tx_play'
  
  uint16_t            t_num_prompt_to_play[IDX_PROMPT_MAX];     // Numéros du prompt à jouer
  uint16_t            t_idx_of_prompt_played[IDX_PROMPT_MAX];   // Index du prompt en cours de diffusion ou terminé

  boolean             seq_error;
} ST_AUTOMATE;

typedef struct {
  byte                equalizer;
  byte                volume_level;
} ST_CONFIGURATION;

class SerialMP3Player {
  private:

  boolean             flg_module_initialized;
  
  // FIFO circulaire des caractères recus du module KT403A
  // Membres m.a.j ou utilisés sous It (Suffixe '_it' pour indiquer une utilisation sous It)
  unsigned int        idx_rx_write_it;
  unsigned int        idx_rx_read_it;
  char                frame_rx_it[NBR_OF_CHAR_IN_MP3_PLAYER_FRAME_RX];
  boolean             frame_rx_available_it;
  boolean             flg_rx_received_ignore_it;
  char                frame_rx_received_it[10];
  // Fin: Membres m.a.j ou utilisés sous It

#ifndef USE_SIMULATION
  String              frame_rx_str;
#else
  std::string         frame_rx_str;
#endif
  char                frame_rx_received_pre[10];    // Previous frame received (pour le filtrage du bégaiement ;-)

  boolean             flg_callback;                 // If true, enable the call at the 'fct_callback' method
  boolean             (*fct_callback)(CMD_OPCODE i__event, uint16_t i__data);    // Callback function called via the FIFO/Rx

  // FIFO circulaires des caractères à emettre vers le module KT403A (commandes 'PLAY' et 'OTHER')
  ST_FIFO_TX          fifo_tx[NBR_FIFO_TX];

  // Attributs pour l'automate
  ST_AUTOMATE         automate;
  ST_CONFIGURATION    configuration;

  boolean             flg_all_prompts_played;
  unsigned int        nbr_call_purge_fifo_tx_play;
  unsigned int        nbr_exec_purge_fifo_tx_play;

  boolean             flg_connected;
  long                duration_internal_previous;
  long                duration_internal_current;
  long                max_duration_between_prompts;

#if USE_STATS_SERIAL_MP3_PLAYER
  unsigned int        nbr_frames_rx;

  unsigned int        err_fifo_0;
  unsigned int        err_fifo_1;
  unsigned int        err_retry;
  unsigned int        err_frame_rx;
  int                 spare_min;

  unsigned int        err_cks;
#endif

  public:
    SerialMP3Player();
    ~SerialMP3Player();

    // Method for FIFO/Rx gestion
    void update(char i__char = INVALID_OPCODE);
    boolean isFrameAvailable();
    void setUnvailableFrame();

#ifndef USE_SIMULATION
    String getFrameRx();
#else
    std::string getFrameRx();
#endif

    void hexDumpFifoRx();
    void setFctCallback(boolean (*i__fct_callback)(CMD_OPCODE i__event, uint16_t i__data)) { fct_callback = i__fct_callback; };
    void clearFctCallback();
    void clearSynchroPromptsExpected();
    //void setFlgCallback() { flg_callback = true; };

    // Method for FIFO/Tx gestion
    void write(byte i__value);                          // Ecriture sans passer par la FIFO/Tx

    // Ecriture en passant par la FIFO/Tx avec la liste ('i__duration' @ 'i__nb_durations') des durées des prompts à diffusion (NULL @ 0 by default)
    void writeFifoTx(FIFO_TX_NUM i__num, byte *i__value, size_t i__nbr_bytes, uint16_t *i__duration = NULL, size_t i__nbr_durations = 0);

    boolean isCommandToSend(FIFO_TX_NUM i__num);
    void setCommandSended(FIFO_TX_NUM i__num, boolean i__flg);

#ifndef USE_SIMULATION
    String getFrameTx(FIFO_TX_NUM i__num);
#else
    std::string getFrameTx(FIFO_TX_NUM i__num);
#endif

    void hexDumpFifoTx();
    boolean isCommandInProgress(FIFO_TX_NUM i__num);

    void setFctCallback(FIFO_TX_NUM i__num, boolean (*i__fct_callback)(CMD_OPCODE, uint16_t)) { fifo_tx[i__num].fct_callback = i__fct_callback; };

    // Synchronization with the end of prompt diffusion
    uint16_t getNumPromptToPlay(ENUM_IDX_PROMPT i__idx_prompt)   { return automate.t_num_prompt_to_play[i__idx_prompt]; };
    uint16_t getIdxOfPromptPlayed(ENUM_IDX_PROMPT i__idx_prompt) { return automate.t_idx_of_prompt_played[i__idx_prompt]; };

    boolean isLastTxPlay() { return automate.flg_last_tx_play; };
    boolean isLastTxPlayAdvert() { return automate.flg_last_tx_play_advert; };
    void setLastTxPlay(boolean i__flg) { automate.flg_last_tx_play = i__flg; };
    void setLastTxPlayAdvert(boolean i__flg) { automate.flg_last_tx_play_advert = i__flg; };
    CMD_OPCODE getLastTxOtherOpcode() { return automate.last_tx_other_opcode; };

    void clearNumAndIdxPromptToPlay();

    void setAllPromptsPlayed(boolean i__flg) { flg_all_prompts_played = i__flg; };
    boolean isAllPromptsPlayed() { return flg_all_prompts_played; };

    void purgeFIFOTxPlay();
    unsigned int getNbrCallPurgeFIFOTxPlay() const { return nbr_call_purge_fifo_tx_play; };
    unsigned int getNbrExecPurgeFIFOTxPlay() const { return nbr_exec_purge_fifo_tx_play; };

    void setModeConnected(boolean i__flg);
    void setDurationInternal(long i__value);
    long getMaxDurationBetweenPrompts() const { return max_duration_between_prompts; };
    
    uint16_t checksum(char *i__from, size_t i__len);

    void exec_automate(AUTOMATE_PILOT i__origin);
    void setCurrentState(AUTOMATE_STATE i__current_state);
    void setFlgSkipAutomate(boolean i__flg) { automate.flg_skip_automate = i__flg; };

    byte getValueEqualizer()   { return configuration.equalizer; };
    byte getValueVolumeLevel() { return configuration.volume_level; };

    uint16_t getNumSequence(FIFO_TX_NUM i__num);
    uint16_t incAndGetNumSequence(FIFO_TX_NUM i__num);

    void reInitTxReadForRetry(FIFO_TX_NUM i__num);

#if USE_STATS_SERIAL_MP3_PLAYER
    unsigned int getNbrFramesRx() const { return nbr_frames_rx; };
    unsigned int getErrFifo(int i__num_err) const;
    int getSpareMin() const { return spare_min; };
    unsigned int getErrCks() const { return err_cks; };
#endif

    /* Gestion du module SerialMP3Player 'WT2003S'
     * - writeToWT2003S(): Emission directe d'un 'opcode' et ses 'datas'
     *   apres calcul de la checksum (ie. 7e len opcode data_0 data_1 ... data_n cks ef)
    */
    void writeToWT2003S(byte *i__value, size_t i__nbr_bytes, boolean i__flg_trace = false);
    // Fin:  Gestion du module SerialMP3Player 'WT2003S'

    void execStatusFileEnd();
};

extern SerialMP3Player             *g__serial_mp3_player;
#endif
