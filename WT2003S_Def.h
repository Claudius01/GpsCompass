#ifndef __WT2003S_DEF__
#define __WT2003S_DEF__

/* Tableau de reconnaissance des reponses reconnues
 * => Permet une optimisation car cette determination est faite sous It dans 'update()'
*/
static boolean g__wt2003s_response[256] = {
// 0x00  0x01   0x02   0x03   0x04   0x05   0x06   0x07   0x08   0x09   0x0A   0x0B   0x0C   0x0D   0x0E   0x0F
  true , true , true , false, false, true , false, false, false, false, false, false, false, false, false, false,   // 0x00, ..., 0x0F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x10, ..., 0x1F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x20, ..., 0x2F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x30, ..., 0x3F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x40, ..., 0x4F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x50, ..., 0x5F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x60, ..., 0x6F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x70, ..., 0x7F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x80, ..., 0x8F
  true , true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0x90, ..., 0x9F
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0xA0, ..., 0xAF
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0xB0, ..., 0xBF
  false, true , true , true,  false, true , true , false, false, true , true , true , false, false, false, false,   // 0xC0, ..., 0xCF
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0xD0, ..., 0xDF
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,   // 0xE0, ..., 0xEF
  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false    // 0xF0, ..., 0xFF
};

/* Tableau des longueurs des reponses reconnues
 * => Permet une optimisation car cette determination est faite sous It dans 'update()'
 * => Warning: La taille ne doit pas etre definie comme > 9
 *             => Cf. le membre "byte datas[8];  // Donnees suivant l'opcode sur une taille de ('size' - 1)"
 *                de la structure 'ST_RESPONSE'
*/
static byte g__wt2003s_size_response[256] = {
//0  1  2  3  4  5  6  7    8  9  A  B  C  D  E  F
  1, 1, 1, 0, 0, 1, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x00, ..., 0x0F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x10, ..., 0x1F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x20, ..., 0x2F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x30, ..., 0x3F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x40, ..., 0x4F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x50, ..., 0x5F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x60, ..., 0x6F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x70, ..., 0x7F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x80, ..., 0x8F
  1, 1, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0x90, ..., 0x9F
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0xA0, ..., 0xAF
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0xB0, ..., 0xBF
  0, 2, 2, 3, 0, 3, 3, 0,   0, 3, 2, 9, 0, 0, 0, 0,   // 0xC0, ..., 0xCF
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0xD0, ..., 0xDF
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   // 0xE0, ..., 0xEF
  0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0    // 0xF0, ..., 0xFF
};

static ST_WT2003S_CMD g__st_wt2003s_cmd[17] = {
  { WT2003S_SD_PLAY_INDEX_IN_ROOT,        "WT_CMD: SD: Play file index in root", TYPE_UINT16,   WT2003S_STATE_SENT_PLAY },
  { WT2003S_SD_PLAY_FILE_IN_ROOT,         "WT_CMD: SD: Play file in root", TYPE_CHAR,           WT2003S_STATE_SENT_PLAY },
  { WT2003S_SD_PLAY_INDEX_IN_FOLDER,      "WT_CMD: SD: Play index in folder", TYPE_UINT16,      WT2003S_STATE_SENT_PLAY },
  { WT2003S_PAUSE_OR_PLAY,                "WT_CMD: Set Pause ou Play", TYPE_NO_MEAN,            WT2003S_STATE_SENT_PAUSE },
  { WT2003S_STOP,                         "WT_CMD: Set Stop Play", TYPE_NO_MEAN,                WT2003S_STATE_SENT_STOP },
  { WT2003S_NEXT,                         "WT_CMD: Set Next Play", TYPE_NO_MEAN,                WT2003S_STATE_SENT_PLAY },
  { WT2003S_PREVIOUS,                     "WT_CMD: Set Previous Play", TYPE_NO_MEAN,            WT2003S_STATE_SENT_PLAY },
  { WT2003S_SET_VOLUME,                   "WT_CMD: Set Volume", TYPE_BYTE,                      WT2003S_STATE_SENT_SET_VOLUME },
  { WT2003S_SET_PLAYMODE,                 "WT_CMD: Set Play mode", TYPE_BYTE,                   WT2003S_STATE_NO_MEAN },
  { WT2003S_SET_CUTIN_MODE,               "WT_CMD: Set Cutin mode", TYPE_UINT16,                WT2003S_STATE_SENT_CUTIN_MODE },
  { WT2003S_GET_VOLUME,                   "WT_CMD: Get Volume", TYPE_NO_MEAN,                   WT2003S_STATE_SENT_GET_VOLUME },
  { WT2003S_GET_STATE,                    "WT_CMD: Get State", TYPE_NO_MEAN,                    WT2003S_STATE_SENT_STATE },
  { WT2003S_DISKSTATUS,                   "WT_CMD: Get Disk status", TYPE_NO_MEAN,              WT2003S_STATE_SENT_DISKSTATUS },
  { WT2003S_GET_SD_SONGCOUNT,             "WT_CMD: Get SD Song Count", TYPE_NO_MEAN,            WT2003S_STATE_SENT_GET_SD_SONGCOUNT },
  { WT2003S_GET_FILE_PLAYING,             "WT_CMD: Get file playing", TYPE_NO_MEAN,             WT2003S_STATE_SENT_GET_FILE_PLAYING },
  { WT2003S_GET_SONG_NAME_PLAYING,        "WT_CMD: Get Song name playing", TYPE_NO_MEAN,        WT2003S_STATE_SENT_GET_SONG_NAME_PLAYING },
  { WT2003S_NO_MEAN,                      "WT_CMD: No mean", TYPE_NO_MEAN,                      WT2003S_STATE_NO_MEAN }
};

static ST_WT2003S_CMD g__st_wt2003s_resp[15] = {
  { WT2003S_RSP_OK,                       "WT_RSP: Ok", TYPE_NO_MEAN },
  { WT2003S_RSP_KO,                       "WT_RSP: Ko", TYPE_NO_MEAN },     // Reponse recue au Reset du KT403A (discrimination)
  { WT2003S_RSP_KO_2,                     "WT_RSP: Ko 2", TYPE_NO_MEAN },
  { WT2003S_CMD_UNKNOWN,                  "WT_RSP: Command Unknown", TYPE_NO_MEAN },
  { WT2003S_PLAY_START,                   "WT_RSP: Play Start", TYPE_NO_MEAN },
  { WT2003S_PLAY_END,                     "WT_RSP: Play End", TYPE_NO_MEAN },
  { WT2003S_GET_VOLUME,                   "WT_RSP: Volume", TYPE_BYTE },
  { WT2003S_GET_STATE,                    "WT_RSP: State", TYPE_BYTE },
  { WT2003S_GET_SD_SONGCOUNT,             "WT_RSP: SD: Songs count", TYPE_UINT16 },
  { WT2003S_GET_SD_SONGS_IN_FOLDER_COUNT, "WT_RSP: SD: Songs in folder count", TYPE_UINT16 },
  { WT2003S_GET_FILE_PLAYING,             "WT_RSP: File playing", TYPE_UINT16 },
  { WT2003S_DISKSTATUS,                   "WT_RSP: Disk status", TYPE_BYTE },
  { WT2003S_GET_SONG_NAME_PLAYING,        "WT_RSP: Song name playing", TYPE_CHAR },
  { WT2003S_NO_MEAN,                      "WT_RSP: No mean", TYPE_NO_MEAN }
};

static ST_WT2003S_LIST_STATE g__st_wt2003s_list_states[16] = {
  { WT2003S_STATE_IDLE,                          0, "State: Idle" },                  // Etat de repos
  { WT2003S_STATE_SENT_RESET_KT403A,             0, "State: Sent Reset KT403A" },     // Identification si 'WT2003S_RSP_KO' recu
  { WT2003S_STATE_SENT_PLAY,                   500, "State: Sent Play" },             // Timeout sur la reponse 'WT2003S_RSP_OK'
  { WT2003S_STATE_WAIT_PLAY_START,             500, "State: Wait Play Start" },       // Timeout sur la reponse 'WT2003S_PLAY_START'
  { WT2003S_STATE_WAIT_PLAY_END,                 0, "State: Play in progress" },      // Diffusion d'un prompt en cours
  { WT2003S_STATE_SENT_STOP,                   500, "State: Sent Stop" },             // Timeout sur la reponse 'WT2003S_RSP_OK'
  { WT2003S_STATE_SENT_PAUSE,                  500, "State: Sent Pause" },            // Timeout sur la reponse 'WT2003S_RSP_OK'
  { WT2003S_STATE_SENT_CUTIN_MODE,             500, "State: Sent Cutin Mode" },       // Timeout sur la reponse 'WT2003S_RSP_OK' ou 'WT2003S_RSP_KO_2'
  { WT2003S_STATE_SENT_SET_VOLUME,             500, "State: Sent Set Volume" },       // Timeout sur la reponse 'WT2003S_RSP_OK'
  { WT2003S_STATE_SENT_GET_VOLUME,             500, "State: Sent Get Volume" },       // Timeout sur la reponse 'WT2003S_GET_VOLUME' + datas
  { WT2003S_STATE_SENT_STATE,                  500, "State: Sent State" },            // Timeout sur la reponse 'WT2003S_GET_STATE' + datas
  { WT2003S_STATE_SENT_DISKSTATUS,             500, "State: Sent Disk Status" },      // Timeout sur la reponse 'WT2003S_DISKSTATUS' + datas
  { WT2003S_STATE_SENT_GET_SD_SONGCOUNT,       500, "State: Get SD Song Count" },     // Timeout sur la reponse 'WT2003S_GET_SD_SONGCOUNT' + datas
  { WT2003S_STATE_SENT_GET_FILE_PLAYING,       500, "State: Get File Playing" },      // Timeout sur la reponse 'GET_FILE_PLAYING' + datas
  { WT2003S_STATE_SENT_GET_SONG_NAME_PLAYING,  500, "State: Get Song Name Playing" }, // Timeout sur la reponse 'GET_SONG_NAME_PLAYING' + datas
  { WT2003S_STATE_NO_MEAN,                       0, "State: No Mean" }                // Etat non encore implemente
};
#endif
