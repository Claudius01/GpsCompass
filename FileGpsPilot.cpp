
#include <Arduino.h>

#include "SDCard.h"
#include "Misc.h"
#include "Timers.h"
#include "FileGpsPilot.h"

FileGpsPilot::FileGpsPilot()
{
  Serial.println("FileGpsPilot::FileGpsPilot()");

  calculChecksumIncClearValues();
}

FileGpsPilot::~FileGpsPilot()
{
  Serial.println("FileGpsPilot::~FileGpsPilot()");
}

/* Ouverture, lecture et analyse des enregistrements
 * 
 */
bool FileGpsPilot::getFileLines(char *i__file_name, boolean i__flg_trace)
{
  boolean l__flg_rtn = true;

  File l__file;
  String l__line = "";
  String l__crlf = "";
  boolean l__flg_cks_inc = true;      // Calcul de la checksum incrementale

  g__sdcard->startActivity();

  l__file = SD.open((i__file_name != NULL) ? i__file_name : NAME_OF_FILE_GPS_PILOT);

  if (!l__file) {
    // Failed to open file for reading line
    Serial.printf("FileGpsPilot::getFileLines(): Failed to open file [%s]\n", (i__file_name != NULL) ? i__file_name : NAME_OF_FILE_GPS_PILOT);

    l__flg_rtn = false;
  }
  else {
    int n = 0;

    Serial.printf("FileGpsPilot::getFileLines(): [%s]...\n", (i__file_name != NULL) ? i__file_name : NAME_OF_FILE_GPS_PILOT);

    calculChecksumIncClearValues();

    while (l__file.available()) {
      int l__value = l__file.read();

      // Concatenation de tous les caracteres pour la checksum
      l__line.concat((char)l__value);

      if (l__value == '\n') {
        // Ligne complete avec les '\r' et '\n'...
        if (i__flg_trace) {
          // Suppression et trace des '\r' et '\n'
          char l__buffer[80];
          memset(l__buffer, '\0', sizeof(l__buffer));
          strncpy(l__buffer, l__line.c_str(), min(sizeof(l__buffer) - 1, l__line.length()));

          char *l__p_pattern_cr = strrchr(l__buffer, '\r');
          if (l__p_pattern_cr != NULL) {
            l__crlf.concat("<CR>");
          }
          char *l__p_pattern_lf = strrchr(l__buffer, '\n');
          if (l__p_pattern_lf != NULL) {
            l__crlf.concat("<LF>");
          }

          if (l__p_pattern_cr != NULL) {
            *l__p_pattern_cr = '\0';
          }
          if (l__p_pattern_lf != NULL) {
            *l__p_pattern_lf = '\0';
          }
          // Fin: Suppression et trace des '\r' et '\n'

          // Trace sans les '\r' et '\n'
          Serial.printf("#%04d: [%s%s]\n", n, l__buffer, l__crlf.c_str());

          l__crlf.clear();
        }

        if (!strncmp(l__line.c_str(), PROMPT_FRAME_CHECKSUM, strlen(PROMPT_FRAME_CHECKSUM))) {
          // ie. #Checksum [4057077149 4174]
          char *l__buff_dup = strdup(l__line.c_str());
          char *l__pattern = strtok(l__buff_dup, " ");
          if (l__pattern != NULL) {
            l__pattern = strtok(NULL, " ");
            if (l__pattern != NULL) {
              checksum_read = (uint32_t)strtoul(l__pattern + 1, NULL, 10);  // +1 because [4057077149 ... ('[')
              l__pattern = strtok(NULL, " ");
              if (l__pattern != NULL) {
                *(l__pattern + strlen(l__pattern) - 1) = '\0';      // because  ... 4174] (']')
                size_datas_read = (size_t)strtol(l__pattern, NULL, 10);
              }
            }
          }
          free(l__buff_dup);

          l__flg_cks_inc = false;     // Fin du calcul de la checksum incrementale
        }
        else if (l__flg_cks_inc == true) {
          // Checksum incrementale sans l'enregistrement "#Checksum..."
          calculChecksumInc((uint8_t *)l__line.c_str(), l__line.length());
        }

        l__line.clear();      // Prepare next line
        n++;
      }
    }

    // Checksum finale
    calculChecksumInc(NULL, 0, true);

    if (i__flg_trace) {
      Serial.printf("\tCks [%u] [0x%08x] Size [%d] (reading)\n", checksum_read, checksum_read, size_datas_read);
      Serial.printf("\tCks [%u] [0x%08x] Size [%d] (calculated)\n", checksum_calc, checksum_calc, size_datas_calc);
    }

    if (checksum_read != checksum_calc || size_datas_read != size_datas_calc) {
      Serial.printf("FileGpsPilot::getFileLines(): Wrong Cks r/c (read [%d] != calc [%d]) a/o Size (read [%d] != calc [%d])\n",
        checksum_read, checksum_calc, size_datas_read, size_datas_calc);
      
      l__flg_rtn = false;
    }

    l__file.close();
  }

  g__sdcard->stopActivity(l__flg_rtn);

  return l__flg_rtn;
}

// Private methods
void FileGpsPilot::calculChecksumInc(uint8_t *i__datas, size_t i__size, boolean i__flg_end)
{
  if (i__flg_end == false) {
    size_cumul += i__size;
    crc_tmp = cksum_inc(i__datas, i__size, crc_tmp, 0);
  }
  else {
    checksum_calc = cksum_inc(NULL, 0, crc_tmp, size_cumul);
    size_datas_calc = size_cumul;

    // Prepare for next call
    size_cumul = 0;
    crc_tmp = 0;
  }
}

void FileGpsPilot::calculChecksumIncClearValues()
{
  nbrOfRecords = 0;

  checksum_calc = (uint32_t)-1;
  size_datas_calc = 0;
  checksum_read = (uint32_t)-1;
  size_datas_read = 0;
  crc_tmp = 0;
  size_cumul = 0;
}
// End: Private methods
