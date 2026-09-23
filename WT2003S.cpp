
#include <sstream>

#include <Arduino.h>

#include "Misc.h"
#include "PromptsSynthesis.h"
#include "SerialMP3Player.h"
#include "Timers.h"
#include "WT2003S.h"
#include "WT2003S_Def.h"    // 1! inclusion dans le projet pour les definitions des variables globales

WT2003S::WT2003S() : flg_in_service_in_progress(false), flg_in_service(false), flg_prompts_pump_in_progress(false), num_prompt_pump(0),
                     volume_level((byte)-1),
                     flg_play_start(false), flg_play_end(false), flg_play_in_progress(false),
                     cpt_play_start(0), cpt_play_end(0), cpt_play_in_progress(0)
{
  Serial.println("WT2003S::WT2003S()");

  // Raz des statistiques
  memset(&stats, '\0', sizeof(stats));

  /* Test and print the opcode definitions of responses:
   * - If 'g__wt2003s_response[n] == true && g__wt2003s_size_response[n] != 0'       => Opcode supported => Ok => Print
   *   => Error if 'g__wt2003s_size_response[n] > 9'
   * - Else if 'g__wt2003s_response[n] == true  && g__wt2003s_size_response[n] == 0' => Error of definition => Ko => Print + 'Internal error'
   * - Else if 'g__wt2003s_response[n] == false && g__wt2003s_size_response[n] != 0' => Error of definition => Ko => Print + 'Internal error'
   * - Else if 'g__wt2003s_response[n] == false && g__wt2003s_size_response[n] == 0' => Opcode not supported => Ok => Print
   * - Else => Other case => Print + 'Internal error'
   */
  Serial.print("Response defined by the WT2003S:\n");
  int l__resp_num = 0;
  boolean l__flg_err = true;

  size_t n = 0;
  for (n = 0; n < 256; n++) {
    if (g__wt2003s_response[n] == true && g__wt2003s_size_response[n] != 0) {
      if (g__wt2003s_size_response[n] > 9) {
        // Defined but wrong bytes defined
        Serial.printf("\t\t#%2d: [0x%02X] supported with a wrong [%d] bytes defined\n", l__resp_num++,
          n, g__wt2003s_size_response[n]);

        stats.respReceivedInternalErr++;
        g__timers->start(TIMER_WT2003S_RESP_ERROR, DURATION_TIMER_WT2003S_RESP_ERROR, NULL);
      }
      else {
        // Correctly defined
        Serial.printf("\t#%2d: [0x%02X] supported with [%d] bytes expected\n", l__resp_num++,
          n, g__wt2003s_size_response[n]);

        l__flg_err = false;
      }
    }
    else if (g__wt2003s_response[n] == true && g__wt2003s_size_response[n] == 0) {
      Serial.printf("\t\t#%2d: #1: [0x%02X] incorrectly defined [%d] and with [%d] bytes\n", l__resp_num++,
        n, g__wt2003s_response[n], g__wt2003s_size_response[n]);

      stats.respReceivedInternalErr++;
      g__timers->start(TIMER_WT2003S_RESP_ERROR, DURATION_TIMER_WT2003S_RESP_ERROR, NULL);
    }
    else if (g__wt2003s_response[n] == false && g__wt2003s_size_response[n] != 0) {
      Serial.printf("\t\t#%2d: #2: [0x%02X] incorrectly defined [%d] and with [%d] bytes\n", l__resp_num++,
        n, g__wt2003s_response[n], g__wt2003s_size_response[n]);

      stats.respReceivedInternalErr++;
      g__timers->start(TIMER_WT2003S_RESP_ERROR, DURATION_TIMER_WT2003S_RESP_ERROR, NULL);
    }
    else if (g__wt2003s_response[n] == false && g__wt2003s_size_response[n] == 0) {
#if 0 // No trace
      // Correctly defined but not supported
      Serial.printf("\t#%2d: [0x%02X] not supported\n", l__resp_num++, n);
#else
      l__resp_num++;
#endif

      l__flg_err = false;
    }
    else {
      Serial.printf("\t\t#%2d: #2: [0x%02X] other case [%d] and with [%d] bytes\n", l__resp_num++,
        n, g__wt2003s_response[n], g__wt2003s_size_response[n]);

      stats.respReceivedInternalErr++;
      g__timers->start(TIMER_WT2003S_RESP_ERROR, DURATION_TIMER_WT2003S_RESP_ERROR, NULL);      
    }
  }

  if (l__flg_err == true) {
    Serial.print("=> One or more definition(s) in error\n");
  }
  // Test and print the opcode definitions of responses:

  // Reset the FIFO
  memset(&fifo, '\0', sizeof(fifo));                // All members to 0 ...
  for (n = 0; n < NBR_RESPONSES; n++) {
    fifo.response[n].opcode = WT2003S_NO_MEAN;
    fifo.response[n].size   = -1;                   // Nombre de donnees inconnue
    memset(fifo.response[n].datas, 0xff, sizeof(fifo.response[0].datas));   // Pour le dump ;-)
  }

  memset(&st_state, '\0', sizeof(st_state));
}

WT2003S::~WT2003S()
{
  Serial.println("WT2003S::~WT2003S()");
}

// Private methods
size_t WT2003S::getNbrElementsCmdOrResp(ENUM_TYPE_CMD_RESP i__type) const
{
  size_t l__nbr_elements = 0;
  switch (i__type) {
  case TYPE_CMD:
    l__nbr_elements = sizeof(g__st_wt2003s_cmd)/sizeof(g__st_wt2003s_cmd[0]);
    break;

  case TYPE_RESP:
    l__nbr_elements = sizeof(g__st_wt2003s_resp)/sizeof(g__st_wt2003s_resp[0]);
    break;

  default:
    break;
  }

  return l__nbr_elements;
}

size_t WT2003S::getNbrElementsListStates() const
{
  return sizeof(g__st_wt2003s_list_states)/sizeof(g__st_wt2003s_list_states[0]);
}

boolean WT2003S::automate(WT2003S_OPCODE i__opcode, byte i__byte, uint16_t i__uint16_t, char *i__t_char)
{
  boolean l__rtn = true;

  const char *l__opcode_label         = getLabelOpcodeCmdOrResp(TYPE_RESP, i__opcode);
  const char *l__state_previous_label = getLabelState(st_state.previous);
  const char *l__state_current_label  = getLabelState(st_state.current);

  byte   l__cmd_for_config[8];
  size_t l__cmd_for_config_size = 0;
        
  Serial.printf("\t\t\tWT2003S: [%s] received in Previous [%s] => Current [%s]: ",
    l__opcode_label != NULL ? l__opcode_label : "UNKNOWN",
    l__state_previous_label != NULL ? l__state_previous_label : "UNKNOWN",
    l__state_current_label != NULL ? l__state_current_label : "UNKNOWN");

  switch (st_state.current) {
  case WT2003S_STATE_SENT_RESET_KT403A:
    if (i__opcode == WT2003S_RSP_KO) {
      Serial.print("WT2003S connected...");
      flg_in_service_in_progress = true;

      /* Continue avec: 
       * - Test du disque status qui doit en retour etre 'WT2003S_SD_CARD'
       * - Si Ok => Configuration du volume au minimum
       * 
       * Remarque: Non utilisation de la FIFO Tx/Other (en attendant l'implementation de la translation)
      */
      // Prepare command 'WT2003S_DISKSTATUS'...
      memset(l__cmd_for_config, '\0', sizeof(l__cmd_for_config));
      l__cmd_for_config[0] = WT2003S_DISKSTATUS;
      l__cmd_for_config_size = 1;
    }
    else {
      Serial.print("Not expected");
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      l__rtn = false;
    }
    setAndSaveState(WT2003S_STATE_IDLE);
    break;
  // End: case WT2003S_STATE_SENT_RESET_KT403A:

  case WT2003S_STATE_SENT_PLAY:
    if (i__opcode == WT2003S_RSP_OK) {
      Serial.print("Play Start acquited");
      setAndSaveState(WT2003S_STATE_WAIT_PLAY_START);
    }
    else {
      Serial.print("Not expected");
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_PLAY:

  case WT2003S_STATE_WAIT_PLAY_START:
    if (i__opcode == WT2003S_PLAY_START) {
      Serial.print("Play Start in progress...");
      setAndSaveState(WT2003S_STATE_WAIT_PLAY_END);
    }
    else {
      Serial.print("Not expected");
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_WAIT_PLAY_START:

  case WT2003S_STATE_WAIT_PLAY_END:
    if (i__opcode == WT2003S_PLAY_END) {
      Serial.print("Play End");
      setAndSaveState(WT2003S_STATE_IDLE);

      // Gestion de l'amorce des prompts d'accueil
      if (flg_prompts_pump_in_progress) {
        byte *l__commands;
        size_t l__size = 0;

        switch(num_prompt_pump) {
        case 0:
          Serial.print("\n\t\t\t\t=> End of 'NUM_PROMPT_HELLO_WORLD'\n");
          // Enchainement avec 'NUM_PROMPT_JE_SUIS__FEMALE_BIS'
          l__size = buildCommandsPromptsGeneral(BASE_JE_SUIS__FEMALE_BIS, &l__commands);
          break;

        case 1:
          Serial.print("\n\t\t\t\t=> End of 'NUM_PROMPT_JE_SUIS__FEMALE_BIS'\n");
          // Enchainement avec 'NUM_PLEASE_WAIT_GPS'
          l__size = buildCommandsPromptsGeneral(BASE_PLEASE_WAIT_GPS, &l__commands);
          break;

        case 2:
          Serial.print("\n\t\t\t\t=> End of 'NUM_PLEASE_WAIT_GPS'\n");
          // Enchainement avec 'BASE_INTERSTELLAR_THEME' (test 'connected' durant la diffusion)
          l__size = buildCommandsPromptsGeneral(BASE_INTERSTELLAR_THEME, &l__commands);
          break;

        case 3:
          Serial.print("\n\t\t\t\t=> End of 'NUM_INTERSTELLAR_THEME'\n");
          break;

        default:
          Serial.print("\n\t\t\t\t=> End of prompts sequence\n");
          flg_prompts_pump_in_progress = false;
          break;
        }

        if (flg_prompts_pump_in_progress) {
          g__serial_mp3_player->setLastTxPlay(false);                         // Prompt termine
          g__serial_mp3_player->writeFifoTx(TX_PLAY, l__commands, l__size);   // Empilement dans la FIFO/TX_PLAY
          g__serial_mp3_player->setCommandSended(TX_PLAY, true);              // Force a jouer tous les prompts empiles

          num_prompt_pump++;    // Next prompt...
        }
      }
      // Fin: Gestion de l'amorce des prompts d'accueil

      Serial.print("\n\t\t\t\t=> End of diffusion => Ready to play the next prompt...\n");

      g__serial_mp3_player->execStatusFileEnd();
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_WAIT_PLAY_END:

  case WT2003S_STATE_SENT_STOP:
    if (i__opcode == WT2003S_RSP_OK) {
      Serial.print("Play Stop acquited");
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      Serial.print("Not expected");
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_STOP:

  /* Warning: La pause acquitee arrete la diffusion avec apres la fin de cette diffusion sur reception 'Play End'
   *          => Passage en 'WT2003S_STATE_IDLE'
   *             => La relance entraine la reception inverse d'un 'Play Start' suivi d'un 'WT2003S_RSP_OK' ;-)
   *                => Reponse non attendu... dans l'etat 'WT2003S_STATE_IDLE'
   *          => De plus, l'emission d'une pause alors qu'aucune diffusion est en cours produit la rediffusion du dernier prompt
   *
   *          => Aucunne correction envisagee car sequence non utilisee en operationnel
   */
  case WT2003S_STATE_SENT_PAUSE:
    if (i__opcode == WT2003S_RSP_OK) {
      Serial.print("Pause acquited");
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      Serial.print("Not expected");
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_PAUSE:

  case WT2003S_STATE_SENT_CUTIN_MODE:
    if (i__opcode == WT2003S_RSP_OK) {
      Serial.print("Cutin Mode acquited");        // Prompt en cours de diffusion interrompu
      st_state.current = st_state.previous;       // Retour a l'etat precedent
    }
    else if (i__opcode == WT2003S_RSP_KO_2) {
      Serial.print("Cutin Mode not effective");   // Aucun prompt en cours de diffusion
      st_state.current = st_state.previous;       // Retour a l'etat precedent
    }
    else {
      Serial.print("Not expected");
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      l__rtn = false;
    }
    break;
    // End: case WT2003S_STATE_SENT_CUTIN_MODE:

  case WT2003S_STATE_SENT_SET_VOLUME:
    if (i__opcode == WT2003S_RSP_OK) {
      Serial.print("Set Volume Ok");
      st_state.current = st_state.previous;   // Retour a l'etat precedent

      if (flg_in_service_in_progress == true) {
        // Prepare command 'WT2003S_GET_VOLUME' (pour verification configuration)...
        memset(l__cmd_for_config, '\0', sizeof(l__cmd_for_config));
        l__cmd_for_config[0] = WT2003S_GET_VOLUME;         
        l__cmd_for_config_size = 1;          
      }
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
    // End: case WT2003S_STATE_SENT_SET_VOLUME:

  case WT2003S_STATE_SENT_GET_VOLUME:
    if (i__opcode == WT2003S_GET_VOLUME) {
      Serial.printf("Get Volume");
      if (i__byte != (byte)-1) {
        Serial.printf(" [%d]", i__byte);

        setValueVolumeLevel(i__byte);     // Update value of Volume Level

        if (flg_in_service_in_progress == true && getValueVolumeLevel() == WT2003S_VOLUME_LEVEL_INIT) {
          /* Le module est maintenant configure et pret a etre utilise avec le niveau de volume
           * egal a 'WT2003S_VOLUME_LEVEL_INIT' ou celui defini au cours du fonctionnement...
          */
          flg_in_service = true;
        }
      }
      else {
        Serial.printf(" [INVALID]");          // Valeur non passee a l'automate
        l__rtn = false;
      }
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_GET_VOLUME:

  case WT2003S_STATE_SENT_STATE:
    if (i__opcode == WT2003S_GET_STATE) {
      Serial.printf("Get State");
      if (i__byte != (byte)-1) {
        Serial.printf(" [%d] ", i__byte);
        switch (i__byte) {
        case WT2003S_PLAY_IN_PROGRESS:
          Serial.printf("(Play in progress)");
          break;
        case WT2003S_PLAY_NOT_IN_PROGRESS:
          Serial.printf("(Play not in progress)");
          break;
        case WT2003S_PLAY_IN_PAUSE:
          Serial.printf("(Play in pause)");
          break;
        default:
          Serial.printf("(Unknown)");
          break;
        }
      }
      else {
        Serial.printf(" [INVALID]");          // Valeur non passee a l'automate
        l__rtn = false;
      }
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_STATE:

  case WT2003S_STATE_SENT_DISKSTATUS:
    if (i__opcode == WT2003S_DISKSTATUS) {
      Serial.printf("Get Disk status");
      if (i__byte != (byte)-1) {
        Serial.printf(" [%d] (%s)", i__byte, (i__byte == WT2003S_SD_CARD) ? "SDCard" : "Other ?");

        if (i__byte == WT2003S_SD_CARD && flg_in_service_in_progress == true) {
          // Prepare command 'WT2003S_SET_VOLUME' + Niveau 'WT2003S_VOLUME_LEVEL_INIT'...
          memset(l__cmd_for_config, '\0', sizeof(l__cmd_for_config));
          l__cmd_for_config[0] = WT2003S_SET_VOLUME;
          l__cmd_for_config[1] = WT2003S_VOLUME_LEVEL_INIT;          
          l__cmd_for_config_size = 2;          
        }
      }
      else {
        Serial.printf(" [INVALID]");          // Valeur non passee a l'automate
        l__rtn = false;
      }
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_DISKSTATUS:

  /* Warning: Si diffusion en cours, 'GET_SD_SONGCOUNT' arrete celle-ci
   *          => Reponses non attendues ensuite...
   *             => Aucunne correction envisagee car sequence non utilisee en operationnel
   */
  case WT2003S_STATE_SENT_GET_SD_SONGCOUNT:
    if (i__opcode == WT2003S_GET_SD_SONGCOUNT) {
      Serial.printf("Get SD Song Count");
      if (i__uint16_t != (uint16_t)-1) {
        Serial.printf(" [%d]", i__uint16_t);
      }
      else {
        Serial.printf(" [INVALID]");          // Valeur non passee a l'automate
        l__rtn = false;
      }
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_GET_SD_SONGCOUNT:

  case WT2003S_STATE_SENT_GET_FILE_PLAYING:
    if (i__opcode == WT2003S_GET_FILE_PLAYING) {
      Serial.printf("Get File Playing");
      if (i__uint16_t != (uint16_t)-1) {
        Serial.printf(" [#%d]", i__uint16_t);
      }
      else {
        Serial.printf(" [INVALID]");          // Valeur non passee a l'automate
        l__rtn = false;
      }
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_GET_FILE_PLAYING:

  case WT2003S_STATE_SENT_GET_SONG_NAME_PLAYING:
    if (i__opcode == WT2003S_GET_SONG_NAME_PLAYING) {
      Serial.printf("Get File Name Playing");
      if (i__t_char != NULL) {
        Serial.printf("[\"%s\"]", i__t_char);
      }
      else {
        Serial.printf(" [INVALID]");          // Valeur non passee a l'automate
        l__rtn = false;
      }
      st_state.current = st_state.previous;   // Retour a l'etat precedent
    }
    else {
      stats.respReceivedNotExpected++;
      stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
      Serial.print("Not expected");
      l__rtn = false;
    }
    break;
  // End: case WT2003S_STATE_SENT_GET_SONG_NAME_PLAYING:

  default:
    Serial.print("Not expected");
    stats.respReceivedNotExpected++;
    stats.respReceivedAndValid--;       // Car a ete comptabilisee comme valide (respect du nombre total inchange)
    l__rtn = false;
    break;
  }

  Serial.print("\n");

  // New state
  l__state_current_label  = getLabelState(st_state.current);
  Serial.printf("\t\t\tWT2003S: New state current [%s] (%s)\n",
    l__state_current_label != NULL ? l__state_current_label : "UNKNOWN",
    (l__rtn == true) ? "Ok" : "Ko");

  if (l__rtn == true) {
    if (flg_in_service_in_progress == true) {
      if (l__cmd_for_config_size != 0) {
        Serial.print("\t\t\t\t\t\tWT2003S: Configuration in progress...\n");
 
        // Write to 'WT2003S'...
        g__serial_mp3_player->writeToWT2003S(l__cmd_for_config, l__cmd_for_config_size);
      }
      else if (flg_in_service == true) {
        Serial.print("\t\t\t\t\t\tWT2003S: Connected and configured ;-)\n");
        flg_in_service_in_progress = false;

        // Amorce de la diffusion des prompts d'accueil ('Hello World', ...)
        flg_prompts_pump_in_progress = true;
        g__serial_mp3_player->writeFifoTx(TX_PLAY,  g__command_prompt, sizeof(g__command_prompt));
        g__serial_mp3_player->setCommandSended(TX_PLAY, true);        // Launch the 'Prompt' diffusion command
      }
      else {
        Serial.print("\t\t\t\t\t\tWT2003S: Error in configuration in progress\n");
      }
    }
  }

  return l__rtn;
}
// End: Private methods

// Public methods
/* Method called under It
 * - But: Construire dans une FIFO les reponses completes et supportees du WT2003S
 * - Return: false: Opcode non supporte ou erreur interne
 *           true:  Reponse complete attendue ou en cours de construction
*/
boolean WT2003S::update(byte i__byte, boolean i__flg_trace)
{
  WT2003S_OPCODE l__opcode = WT2003S_NO_MEAN;
  byte l__size = (byte)-1;

  /* Prise en compte si:
   * - En attente d'un opcode
   * - Reponse supportee
   */
  // Test d'un nouvel 'opcode'
  if (fifo.response[fifo.idx_write].opcode == WT2003S_NO_MEAN && fifo.response[fifo.idx_write].size == (byte)-1) {
    stats.respReceivedTotal++;    // A priori, 'opcode' recu

    if (g__wt2003s_response[i__byte] == true) {
      // Opcode de la reponse supportee nouvellement construit
      l__opcode = (WT2003S_OPCODE)i__byte;
      fifo.response[fifo.idx_write].opcode = l__opcode;
      fifo.response[fifo.idx_write].size = 1;

      if (fifo.response[fifo.idx_write].size == g__wt2003s_size_response[l__opcode]) {
        // Preparation prochaine construction de la reponse... 
        fifo.idx_write = (fifo.idx_write + 1) % NBR_RESPONSES;

        // Test de saturation de la FIFO/Rx
        if (fifo.idx_write == fifo.idx_read) {
          if (i__flg_trace) {
            Serial.printf("\tWT2003S::update(0x%02X): Ko: #1: FIFO/Rx full (r/w: %d/%d)\n",
              l__opcode, fifo.idx_read, fifo.idx_write);
          }

          stats.nbrSaturationFifoRx++;
          return false;
        }
        // Fin: Test de saturation de la FIFO/Rx

        fifo.response[fifo.idx_write].opcode = WT2003S_NO_MEAN;
        fifo.response[fifo.idx_write].size = (byte)-1;
        memset(fifo.response[fifo.idx_write].datas, 0xff, sizeof(fifo.response[0].datas));

        if (i__flg_trace) Serial.printf("\tWT2003S::update(0x%02X): Ok: Resp. complete\n", i__byte);

        /*    Reponse complete recue et attendue (limitee a l'opcode)
         * ou Reponse partiellement recue (donnees a suivre)
        */
      }
      else {
        if (i__flg_trace) {
          Serial.printf("\tWT2003S::update(0x%02X): Ok: fifo.response[fifo.idx_write].size (%d) != g__wt2003s_size_response[l__opcode] (%d)\n",
            l__opcode, fifo.response[fifo.idx_write].size, g__wt2003s_size_response[l__opcode]);
        }

        /* La taille des donnee(s) associee(s) a l'opcode n'est pas atteinte
         * => Ignore ... continue 
         */
      }

      stats.respReceivedAndValid++;
      return true;
    }
    else {
      /* Opcode not supported
       * => Ignore et passage fugitif en erreur (erreur interne)
      */
      stats.respReceivedNotSupported++;
      return false;
    }
  }
  // Fin: Test d'un nouvel 'opcode'

  // Opcode de la reponse supportee deja construit
  l__opcode = fifo.response[fifo.idx_write].opcode;
  l__size   = fifo.response[fifo.idx_write].size;

  if (l__opcode != WT2003S_NO_MEAN && l__size != (byte)-1) {
    if (g__wt2003s_response[l__opcode] == true && l__size < g__wt2003s_size_response[l__opcode]) {
      // Donnee(s) associee(s) pour cet opcode
      fifo.response[fifo.idx_write].datas[l__size - 1] = i__byte;
      fifo.response[fifo.idx_write].size += 1;

      if (i__flg_trace) {
        Serial.printf("\tWT2003S::update(0x%02X): Resp. in build (%d < %d) bytes...\n",
          l__opcode, l__size, g__wt2003s_size_response[l__opcode]);
      }
    }

    if (fifo.response[fifo.idx_write].size == g__wt2003s_size_response[l__opcode]) {
      /* Le nombre de donnee(s) associee(s) pour cet opcode est atteint
       * => Preparation prochaine construction de la reponse... 
      */
      fifo.idx_write = (fifo.idx_write + 1) % NBR_RESPONSES;

      // Test de saturation de la FIFO/Rx
      if (fifo.idx_write == fifo.idx_read) {
        if (i__flg_trace) {
          Serial.printf("\tWT2003S::update(0x%02X): Ko: #2: FIFO/Rx full (r/w: %d/%d)\n",
            l__opcode, fifo.idx_read, fifo.idx_write);
        }

        stats.nbrSaturationFifoRx++;
        return false;
      }
      // Fin: Test de saturation de la FIFO/Rx

      fifo.response[fifo.idx_write].opcode = WT2003S_NO_MEAN;
      fifo.response[fifo.idx_write].size = (byte)-1;
      memset(fifo.response[fifo.idx_write].datas, 0xff, sizeof(fifo.response[0].datas));

      if (i__flg_trace) {
        Serial.printf("\tWT2003S::update(0x%02X): Resp. complete (%d == %d) bytes...\n",
          l__opcode, fifo.response[fifo.idx_write].size, g__wt2003s_size_response[l__opcode]);
      }
    }
    else if (fifo.response[fifo.idx_write].size > g__wt2003s_size_response[l__opcode]) {
      if (i__flg_trace) {
        Serial.printf("\tWT2003S::update(0x%02X): Ko: Too many bytes (%d > %d) bytes\n",
          l__opcode, fifo.response[fifo.idx_write].size, g__wt2003s_size_response[l__opcode]);
      }

      /* La taille des donnees depasse celle definie pour cet opcode
       * => Ignore et passage fugitif en erreur (erreur interne)
      */
      stats.respReceivedInternalErr++;
      return false;
    }
  }
  else {
    if (i__flg_trace) {
      Serial.printf("\tWT2003S::update(0x%02X): Ko: Opcode not supported [0x%02x != WT2003S_NO_MEAN && %d != (byte)-1]\n",
        l__opcode, l__opcode, l__size);
    }

    /* Opcode not supported
     * => Ignore et passage fugitif en erreur (erreur interne)
    */
    stats.respReceivedNotSupported++;
    return false;
  }

  if (i__flg_trace) Serial.printf("\tWT2003S::update(): Ok: End of method\n");

  return true;
}
// End: Method called under It

// Awaiting 'Play Start' __/--
void WT2003S::waitPlayStart()
{
  while (true) {
    interrupts();
    delayMicroseconds(1000);    // Awaiting 1 mS between each awaiting
    noInterrupts();

    if (isPlayStart() == true) {
      break;
    }
  }
  interrupts();
}

// Awaiting 'Play End' __/--
void WT2003S::waitPlayEnd()
{
  while (true) {
    noInterrupts();
    delayMicroseconds(1000);    // Awaiting 1 mS between each awaiting
    noInterrupts();

    if (isPlayEnd() == true) {
      break;
    }
  }
  interrupts();
}
      
boolean WT2003S::isPlayInProgress()
{
  noInterrupts();
  boolean l__play_in_progress = flg_play_in_progress;
  boolean l__play_start       = flg_play_start;
  boolean l__play_end         = flg_play_end;
  interrupts();

  if (l__play_in_progress == false && l__play_start == true) {
    Serial.print("WT2003S: Start of play...\n");
    flg_play_in_progress = true;
  }
  else if (l__play_in_progress == true && l__play_end == true) {
    Serial.print("WT2003S: End of play\n");
    flg_play_in_progress = false;
  }
#if 0
  // TBC: Verifier si pas de trace permanente ;-)
  else {
    Serial.print("isWT2003SPlayInProgress(): Unknown states\n");
    Serial.printf("\tplay_in_progress [%d]\n", flg_play_in_progress);
    Serial.printf("\tplay_start       [%d]\n", flg_play_start);
    Serial.printf("\tplay_end         [%d]\n", flg_play_end);
  }
#endif

  return flg_play_in_progress;
}

boolean WT2003S::treatmentResponse(boolean i__flg_trace)
{
  boolean l__rtn = false;

  noInterrupts();
  byte l__idx_write = fifo.idx_write;   // Protection maj du pointeur ;-)
  interrupts();

  while (fifo.idx_read != l__idx_write) {
    if (i__flg_trace) {
      Serial.printf("WT2003S: Response: Opcode [0x%02x]", fifo.response[fifo.idx_read].opcode);

      if (fifo.response[fifo.idx_read].size > 1) {
        Serial.print(" Datas [");
        int n = 0;
        for (n = 0; n < (int)(fifo.response[fifo.idx_read].size - 1); n++) {
          Serial.printf((n == 0) ? "%02x" : " %02x", fifo.response[fifo.idx_read].datas[n]);
        }
        Serial.printf("] (%d bytes)", n);
      }
      Serial.print("\n");
    }

    interpretCmdOrResp(TYPE_RESP, fifo.response[fifo.idx_read].opcode, fifo.response[fifo.idx_read].size, fifo.response[fifo.idx_read].datas);

    fifo.idx_read = (fifo.idx_read + 1) % NBR_RESPONSES;
    l__rtn = true;
  }

  return l__rtn;
}

boolean WT2003S::interpretCmdOrResp(ENUM_TYPE_CMD_RESP i__type, WT2003S_OPCODE i__opcode, byte i__size, byte *i__datas)
{
  boolean l__rtn = false;
  boolean l__flg_found = false;

  size_t l__nbr_elements = getNbrElementsCmdOrResp(i__type);
  if (l__nbr_elements == 0) {
    return false;
  }

  Serial.print("WT2003S: ");

  boolean l__flg_value_byte   = false;
  boolean l__flg_value_num    = false;
  boolean l__flg_value_t_char = false;

  byte     l__value_byte = 0;
  uint16_t l__value_num = 0;
  char     l__value_t_char[16];
  memset(l__value_t_char, '\0', sizeof(l__value_t_char));

  size_t l__idx = 0;
  for (l__idx = 0; l__idx < l__nbr_elements; l__idx++) {
    WT2003S_OPCODE l__opcode = (i__type == TYPE_CMD) ? g__st_wt2003s_cmd[l__idx].opcode : g__st_wt2003s_resp[l__idx].opcode;
    if (i__opcode == l__opcode) {
      l__flg_found = true;

      Serial.print((i__type == TYPE_CMD) ? g__st_wt2003s_cmd[l__idx].label : g__st_wt2003s_resp[l__idx].label);

      if (i__opcode == WT2003S_CMD_UNKNOWN) {
        stats.respReceivedUnknown++;
        stats.respReceivedAndValid--;     // Car a ete comptabilisee comme valide (respect du nombre total inchange)
        return false;
      }

      if (i__size > 1) {
        Serial.print(" Datas [");
        int n = 0;
        for (n = 0; n < (int)(i__size - 1); n++) {
          Serial.printf((n == 0) ? "%02x" : " %02x", i__datas[n]);
        }
        Serial.printf("] (%d bytes)", n);

        // Conversion char or numeric
        ENUM_TYPE_DATA l__type_data = (i__type == TYPE_CMD) ? g__st_wt2003s_cmd[l__idx].type_data : g__st_wt2003s_resp[l__idx].type_data;

        for (n = 0; n < (int)(i__size - 1); n++) {
          if (l__type_data == TYPE_BYTE) {
            l__value_byte = i__datas[n];      // Normalement (n == 0)
            l__flg_value_byte = true;
          }
          else if (l__type_data == TYPE_UINT16) {
            l__value_num *= 256;
            l__value_num += i__datas[n];      // Normalement (n == 0 || n == 1)
            l__flg_value_num = true;
          }
          else if (l__type_data == TYPE_CHAR) {
            char l__char = i__datas[n];       // n in range [0, 1, ..., (i__size - 2)]
            if (!isprint((int)l__char)) {
              // Caractere non imprimable
              l__char = '?';
            }
            l__value_t_char[strlen(l__value_t_char)] = l__char;
            l__flg_value_t_char = true;
          }
          else {
            // Mauvais type
            Serial.printf(" Wrong type data [%d]", l__type_data);
            break;
          }
        }

        switch (l__type_data) {
        case TYPE_BYTE:
          Serial.printf(" Value [%d] (0x%02x)", l__value_byte, l__value_byte);
          break;
        case TYPE_UINT16:
          Serial.printf(" Value [%d] (0x%04x)", l__value_num, l__value_num);
          break;
        case TYPE_CHAR:
          Serial.printf(" Value [\"%s\"]", l__value_t_char);
          break;
        default:
          // No print
          break;
        }
        // End: Conversion char or numeric
      }
      else {
        Serial.print(" No datas");
      }
      Serial.print("\n");

      // Automate
      if (i__type == TYPE_CMD) {
        st_state.previous = st_state.current;
        st_state.current  = g__st_wt2003s_cmd[l__idx].state;
      }
      else if (i__type == TYPE_RESP) {
        if (l__flg_value_byte == true) {
          automate(i__opcode, l__value_byte, (uint16_t)-1, NULL);
        }
        else if (l__flg_value_num == true) {
          automate(i__opcode, (byte)-1, l__value_num, NULL);
        }
        else if (l__flg_value_t_char == true) {
          automate(i__opcode, (byte)-1, (uint16_t)-1, l__value_t_char);
        }
        else {
          automate(i__opcode, (byte)-1, (uint16_t)-1, NULL);
        }
      }

      l__rtn = true;
      break;
    }
  }

  if (l__flg_found == false) {
    Serial.printf("Opcode %s [0x%02x] No found\n", (i__type == TYPE_CMD) ? "command" : "response", i__opcode);
    l__rtn = false;
  }

  return l__rtn;
}

const char *WT2003S::getLabelOpcodeCmdOrResp(ENUM_TYPE_CMD_RESP i__type, WT2003S_OPCODE i__opcode) const
{
  const char *l__label_opcode = NULL;
  size_t l__idx = 0;
  for (l__idx = 0; l__idx < getNbrElementsCmdOrResp(i__type); l__idx++) {
    WT2003S_OPCODE l__opcode = (i__type == TYPE_CMD) ? g__st_wt2003s_cmd[l__idx].opcode : g__st_wt2003s_resp[l__idx].opcode;
    if (i__opcode == l__opcode) {
      l__label_opcode = (i__type == TYPE_CMD) ? g__st_wt2003s_cmd[l__idx].label : g__st_wt2003s_resp[l__idx].label;
      break;
    }
  }

  return l__label_opcode;
}

const char *WT2003S::getLabelState(ENUM_WT2003S_STATE i__state) const
{
  const char *l__label_state = NULL;
  size_t l__idx = 0;
  for (l__idx = 0; l__idx < getNbrElementsListStates(); l__idx++) {
    if (i__state == g__st_wt2003s_list_states[l__idx].state) {
      l__label_state = g__st_wt2003s_list_states[l__idx].label;
      break;
    }
  }

  return l__label_state; 
}

void WT2003S::updateValueVolumeLevel(byte i__value)
{
  // TODO: Emission de la value vers le module au moyen de la commande 'WT2003S_SET_VOLUME'...
}

void WT2003S::hexDumpFifoRx()
{
  std::ostringstream l__out;

  Serial.printf("WT2003S: FIFO Rx Write/Read #0x%02X/#0x%02X\n", fifo.idx_write, fifo.idx_read);

  hexDump(l__out, (char *)fifo.response, NBR_RESPONSES * sizeof(ST_RESPONSE));
  Serial.print(l__out.str().c_str());
  Serial.print("\n");
}
// End: Public methods
