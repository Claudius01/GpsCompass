
#ifndef __WT2003S__
#define __WT2003S__

#include <Arduino.h>

#define WT2003S_VOLUME_LEVEL_INIT         3     // Niveau du volume a l'initialisation

// Opcode (command and response)
typedef enum {
  WT2003S_NO_MEAN                         = 0xFF,
  WT2003S_RSP_OK                          = 0x00,
  WT2003S_RSP_KO                          = 0x01,
  WT2003S_RSP_KO_2                        = 0x02,
  WT2003S_CMD_UNKNOWN                     = 0x05,                 // Reponse a une commande inconnue
  WT2003S_PLAY_START                      = 0x90,                 // Pseudo opcode de reponse
  WT2003S_PLAY_END                        = 0x91,                 // Pseudo opcode de reponse
  WT2003S_SPIFLASH_PLAY_INDEX             = WT2003S_NO_MEAN,      // 0xA0 (Not Used)
  WT2003S_SD_PLAY_INDEX_IN_ROOT           = 0xA2,
  WT2003S_SD_PLAY_FILE_IN_ROOT            = 0xA3,
  WT2003S_SD_PLAY_INDEX_IN_FOLDER         = 0xA4,
  WT2003S_UDISK_PLAY_INDEX_IN_ROOT        = WT2003S_NO_MEAN,      // 0xA6 (Not Used)
  WT2003S_UDISK_PLAY_FILE_IN_ROOT         = WT2003S_NO_MEAN,      // 0xA7 (Not Used)
  WT2003S_UDISK_PLAY_INDEX_IN_FOLDER      = WT2003S_NO_MEAN,      // 0xA8 (Not Used)
  WT2003S_PAUSE_OR_PLAY                   = 0xAA,
  WT2003S_STOP                            = 0xAB,
  WT2003S_NEXT                            = 0xAC,
  WT2003S_PREVIOUS                        = 0xAD,
  WT2003S_SET_VOLUME                      = 0xAE,
  WT2003S_SET_PLAYMODE                    = 0xAF,
  WT2003S_SET_CUTIN_MODE                  = 0xB1,
  WT2003S_COPY_SDTOSPIFLASH               = WT2003S_NO_MEAN,      // 0xB3 (Not Used)
  WT2003S_COPY_UDISKTOSPIFLASH            = WT2003S_NO_MEAN,      // 0xB4 (Not Used)
  WT2003S_STORY_USERDATA                  = WT2003S_NO_MEAN,      // 0xB8 (Not Used)
  WT2003S_ISNEED_RETURNCODE               = WT2003S_NO_MEAN,      // 0xBA (Not Used)
  WT2003S_GET_VOLUME                      = 0xC1,
  WT2003S_GET_STATE                       = 0xC2,
  WT2003S_GET_SPIFLASH_SONGCOUNT          = WT2003S_NO_MEAN,      // 0xC3 (Not Used)
  WT2003S_GET_SD_SONGCOUNT                = 0xC5,
  WT2003S_GET_SD_SONGS_IN_FOLDER_COUNT    = 0xC6,
  WT2003S_GET_UDISK_SONGCOUNT             = WT2003S_NO_MEAN,      // 0xC7 (Not Used)
  WT2003S_GET_UDISK_SONGS_IN_FOLDER_COUNT = WT2003S_NO_MEAN,      // 0xC8 (Not Used)
  WT2003S_GET_FILE_PLAYING                = 0xC9,
  WT2003S_DISKSTATUS                      = 0xCA,
  WT2003S_GET_SONG_NAME_PLAYING           = 0xCB,
  WT2003S_GET_USERDATA                    = WT2003S_NO_MEAN,      // 0xCF (Not Used)
  WT2003S_SWITCH_WORKDATA                 = WT2003S_NO_MEAN,      // 0xD2 (Not Used)
} WT2003S_OPCODE;

// Etats de retour de 'WT2003S_GET_STATE'
typedef enum {
  WT2003S_PLAY_IN_PROGRESS     = 1,
  WT2003S_PLAY_NOT_IN_PROGRESS = 2,
  WT2003S_PLAY_IN_PAUSE        = 3
} WT2003S_STATES_PLAYING;

/* Etats de retour de 'WT2003S_GET_DISKSTATUS' (champ de bits)
 *  0x01: SPI FLASH
 *  0x02: SD Card
 *  0x04: U Disk
 */
typedef enum {
  WT2003S_SPI_FLASH = 1,
  WT2003S_SD_CARD   = 2,      // <= Seul disque utilise dans le projet
  WT2003S_U_DISK    = 4
} WT2003S_DISK_STATUS;

/* FIFO des reponses a traiter
 * => Warning: Adapter la taille en fonction de la dynamique de traitement (erreur "FIFO/Rx")
 *    => Saturation avec 16 positions suite au test "WT2003S date_time" qui emet ~20 prompts
 *       sans lecture de la FIFO ;-))
 *       => TBC: Passage a 32 en exploitation sans erreur de saturation
 *               => Peut etre pas utile a l'usage
 *
 *       => Test avec 16 + "WT2003S date_time":
 *        ->  Audio (WT2003S):
 *        ->     Play Start          [69]
 *        ->     Play End            [69]
 *        ->     Play In Progress    [69]
 *        -> 
 *        ->     Commands   Total    [69]   => 69 commandes 'Play Start' avec 3 reponses (0x00, 0x90 et 0x91): (3 * 69) = 207 = (204 + 3)
 *        ->     Responses  Total    [206]  => 206 = 204 + 2
 *        ->     Responses  Ok       [204]
 *        ->     Err: Resp. Ko       [0]
 *        ->     Err: Resp. Internal [0]
 *        ->     Err: FIFO/Rx full   [2]    => 2 saturations ont fait perdre 3 reponses
 *        
 *       => Test avec 16 + "WT2003S date":
 *        ->  Audio (WT2003S):
 *        ->     Play Start          [15]
 *        ->     Play End            [15]
 *        ->     Play In Progress    [15]
 *        -> 
 *        ->     Commands   Total    [15]
 *        ->     Responses  Total    [46]     => (3 * 15) = 45 + 1 car commande Reset emise vers le KT403A 
 *        ->     Responses  Ok       [46]     => avec une reponse 0x01 si WT2003A ;-)
 *        ->     Err: Resp. Ko       [0]
 *        ->     Err: Resp. Internal [0]
 *        ->     Err: FIFO/Rx full   [0]
 */
#define NBR_RESPONSES       16

typedef struct {
  byte                  size;           // Nombre de donnees associees y compris l'opcode ou -1 si inconnue
  WT2003S_OPCODE        opcode;         // 'Opcode' 
  byte                  datas[8];       // Donnees suivant l'opcode sur une taille de ('size' - 1)
} ST_RESPONSE;

typedef struct {
  byte                  idx_read;
  byte                  idx_write;
  ST_RESPONSE           response[NBR_RESPONSES];
} FIFO_RESPONSES;

typedef enum {
  TYPE_CMD = 0,
  TYPE_RESP
} ENUM_TYPE_CMD_RESP;

typedef enum {
  TYPE_NO_MEAN = 0,
  TYPE_BYTE,
  TYPE_CHAR,
  TYPE_UINT16
} ENUM_TYPE_DATA;

// Definition des etats
typedef enum {
  WT2003S_STATE_IDLE = 0,                     // Repos (aucun evenement attendu)
  WT2003S_STATE_NO_MEAN,                      // Etat en attente d'implementation
  WT2003S_STATE_SENT_RESET_KT403A,            // Reset du KT403A emis (pour discrimination entre les 2 modules)
  WT2003S_STATE_WAIT_RSP_KO,                  // WT2003S_RSP_KO attendu (module WT2003S detecte si dans l'etat 'WT2003S_STATE_SENT_RESET_KT403A', sinon erreur)
  WT2003S_STATE_WAIT_RSP_OK,                  // WT2003S_RSP_OK attendu (acquitement de certaines commandes)
  WT2003S_STATE_SENT_PLAY,                    // Diffusion d'un prompt emis
  WT2003S_STATE_WAIT_PLAY_START,              // WT2003S_PLAY_START attendu
  WT2003S_STATE_WAIT_PLAY_END,                // WT2003S_PLAY_END attendu
  WT2003S_STATE_SENT_STOP,                    // Arret d'un prompt en cours de diffusion
  WT2003S_STATE_SENT_PAUSE,                   // Pause/Reprise d'un prompt en cours de diffusion
  WT2003S_STATE_SENT_CUTIN_MODE,              // Insertion d'un prompt "advert"
  WT2003S_STATE_SENT_SET_VOLUME,              // WT2003S_SET_VOLUME emis
  WT2003S_STATE_SENT_GET_VOLUME,              // WT2003S_GET_VOLUME attendu avec la valeur du niveau du volume
  WT2003S_STATE_SENT_STATE,                   // WT2003S_GET_STATE emis
  WT2003S_STATE_WAIT_STATE,                   // WT2003S_GET_STATE attendu avec le l'etat
  WT2003S_STATE_SENT_DISKSTATUS,              // WT2003S_DISKSTATUS emis
  WT2003S_STATE_SENT_GET_SD_SONGCOUNT,        // WT2003S_GET_SD_SONGCOUNT emis
  WT2003S_STATE_SENT_GET_FILE_PLAYING,        // GET_FILE_PLAYING emis
  WT2003S_STATE_SENT_GET_SONG_NAME_PLAYING,   // GET_SONG_NAME_PLAYING emis

  WT2003S_STATE_NUMBER                        // Nombre d'etats
} ENUM_WT2003S_STATE;

// Structure de traitement des commandes
typedef struct {
  WT2003S_OPCODE        opcode;         // 'Opcode'
  const char            *label;         // Label de l'opcode de commande
  ENUM_TYPE_DATA        type_data;
  ENUM_WT2003S_STATE    state;          // Etat suite a la commande
} ST_WT2003S_CMD;

// Structure de traitement des reponses
typedef struct {
  WT2003S_OPCODE        opcode;         // 'Opcode'
  const char            *label;         // Label de l'opcode de commande
  ENUM_TYPE_DATA        type_data;
} ST_WT2003S_RESP;

/* Structures de traitement des etats en fonction des evenements recus
 * => Le WT2003S repond systematiquement a une commande ;-)
 *    => Chaque emission de commande fait l'objet de l'armement du timer defini dans la structure 'ST_WT2003S_LIST_STATE'
 *       => A son expiration => Erreur et passage a 'WT2003S_STATE_IDLE'
 *    => Un seul etat est memorise pour traitement suite a une commande
 *       => L'envoi de la commande se fait pour eviter la reception de plusieurs reponses non associables a la commande
 *          => ie. 'WT2003S_SET_CUTIN_MODE' durant une diffusion => 'WT2003S_STATE_WAIT_RSP_OK' ou 'WT2003S_STATE_WAIT_PLAY_END' attendus
 *    => Passage a 'WT2003S_STATE_IDLE' apres la reception de la (les) reponse(s) attendue(s)
 *       => Erreur si la reponse n'est pas attendue et passage a 'WT2003S_STATE_IDLE'
*/

typedef struct {
  ENUM_WT2003S_STATE    state;          // Etat
  long                  timeout;        // Pas d'armement si egal a 0L (ie. 'WT2003S_STATE_WAIT_PLAY_END')
  const char            *label;         // Libelle de l'etat
} ST_WT2003S_LIST_STATE;

// Memorisation des etats precedent et courant pour determiner les transitions valides / invalides
typedef struct {
  ENUM_WT2003S_STATE    previous;       // Etats precedents
  ENUM_WT2003S_STATE    current;        // Etats courants
} ST_WT2003S_STATE;
// Fin: Structures de traitement des etats en fonction des evenements recus

typedef struct {
    uint32_t            cmdSentTotal;               // Incremente sur commande emise ('opcode' [+ datas])
    uint32_t            respReceivedTotal;          // Incremente sur chaque response (egal a la somme des 5 'respReceived' suivants)
    uint32_t            respReceivedAndValid;       // Incremente sur chaque response supportee et attendue
    uint32_t            respReceivedNotSupported;   // Incremente sur chaque response non supportee
    uint32_t            respReceivedNotExpected;    // Incremente sur chaque response non attendue dans l'etat courrant
    uint32_t            respReceivedUnknown;        // Incremente sur chaque response inconnue
    uint32_t            respReceivedInternalErr;    // Erreurs internes
    uint32_t            nbrSaturationFifoRx;        // Incremente sur chaque saturation de la FIFO/Rx
} ST_WT2003S_STATS;

class WT2003S {
  private:
    boolean             flg_in_service_in_progress;
    boolean             flg_in_service;

    boolean             flg_prompts_pump_in_progress;     // Diffusion des prompts d'accueil
    byte                num_prompt_pump;                  // Numero de ce prompt d'accueil [0, 1, ...]

    byte                volume_level;

    boolean             flg_play_start;
    boolean             flg_play_end;
    boolean             flg_play_in_progress;

    uint16_t            cpt_play_start;
    uint16_t            cpt_play_end;
    uint16_t            cpt_play_in_progress;

    FIFO_RESPONSES      fifo;

    ST_WT2003S_STATE    st_state;

    ST_WT2003S_STATS    stats;

    size_t getNbrElementsCmdOrResp(ENUM_TYPE_CMD_RESP i__type) const;
    size_t getNbrElementsListStates() const;

    boolean automate(WT2003S_OPCODE i__opcode, byte i__byte, uint16_t i__uint16_t, char *i__t_char);

  public:
    WT2003S();
    ~WT2003S();

    boolean isInService() const { return flg_in_service; };

    // Methodes de suivi des transitions de 'PIN_WT2003S_BUSY'
    void setPlayStart()    { flg_play_start = true;  flg_play_end = false; };
    void setPlayEnd()      { flg_play_start = false; flg_play_end = true; };
    void waitPlayStart();
    void waitPlayEnd();
    boolean isPlayStart()  { return flg_play_start; };
    boolean isPlayEnd()    { return flg_play_end; };
    void incCptPlayStart() { cpt_play_start++; };
    void incCptPlayEnd()   { cpt_play_end++; };
    void incCptPlayInProgress()   { cpt_play_in_progress++; }; 

    uint16_t getCptPlayStart() const { return cpt_play_start; };
    uint16_t getCptPlayEnd() const { return cpt_play_end; };
    uint16_t getCptPlayInProgress() const { return cpt_play_in_progress; };

    boolean isPlayInProgress();    // Diffusion en cours ?

    boolean update(byte i__byte, boolean i__flg_trace = false);
    boolean treatmentResponse(boolean i__flg_trace = false);
    
    void     incCmdSentTotal()                      { stats.cmdSentTotal++; };
    uint32_t getCmdSentTotal()                const { return stats.cmdSentTotal; };
    uint32_t getRespReceivedTotal()           const { return stats.respReceivedTotal; };
    uint32_t getRespReceivedAndValid()        const { return stats.respReceivedAndValid; };
    uint32_t getRespReceivedNotSupported()    const { return stats.respReceivedNotSupported; };
    uint32_t getRespReceivedNotExpected()     const { return stats.respReceivedNotExpected; };
    uint32_t getRespReceivedUnknown()         const { return stats.respReceivedUnknown; };
    uint32_t getRespReceivedInternalErr()     const { return stats.respReceivedInternalErr; };
    uint32_t getNbrSaturationsFifoRx()        const { return stats.nbrSaturationFifoRx; };

    void setAndSaveState(ENUM_WT2003S_STATE i__state) { st_state.previous = st_state.current; st_state.current = i__state; };
    
    boolean interpretCmdOrResp(ENUM_TYPE_CMD_RESP i__type, WT2003S_OPCODE i__opcode, byte i__size, byte *i__datas);

    ENUM_WT2003S_STATE getStatePrevious() const { return st_state.previous; };
    ENUM_WT2003S_STATE getStateCurrent() const { return st_state.current; };

    const char *getLabelOpcodeCmdOrResp(ENUM_TYPE_CMD_RESP i__type, WT2003S_OPCODE i__opcode) const;
    const char *getLabelState(ENUM_WT2003S_STATE i__state) const;

    byte getValueVolumeLevel() const { return volume_level; };
    void setValueVolumeLevel(byte i__value) { volume_level = i__value; };
    void updateValueVolumeLevel(byte i__value);
 
    void hexDumpFifoRx();
};

extern WT2003S  *g__wt2003s;
#endif
