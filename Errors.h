// $Id: Errors.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __ERRORS__
#define __ERRORS__

#define NBR_MAX_ERRORS        16

/*  Definition of the all errors families coded on 12 bits
 *  - 8 bits MSB: Family  [0x00, 0x01, ..., 0xFF]
 *  - 4 bits LSB: Counter [0x0, 0x1, ..., 0xF] (non defined into 'ENUM_ERRORS' (cf. 'update()' method)
 */
typedef enum {
  ENUM_ERRORS_NONE = 0,

  ENUM_ERROR_STARTUP_GPS_MODULE = 0x01,

  /*  Famille 0xEXX:
   *  - [0xE10, ..., 0xE1F]: Nombre de prompts inconnus (limité à 16)
   *  - [0xE20, ..., 0xE2F]: Nombre de courtes famines de la FIFO/Tx Play (limité à 16)
   *  - [0xE30, ..., 0xE3F]: Nombre de longues famines de la FIFO/Tx Play (limité à 16)
   *  - [0xE40, ..., 0xE4F]: Nombre de très longues famines de la FIFO/Tx Play (limité à 16)
   */
  ENUM_ERROR_UNKNOWN_PROMPT     = 0xE1,
  ENUM_ERROR_SHORT_FAMINE       = 0xE2,
  ENUM_ERROR_LONG_FAMINE        = 0xE3,
  ENUM_ERROR_VERY_LONG_FAMINE   = 0xE4,

  /*  Famille 0xFXX:
   *  - [0xF10, ..., 0xF1F]: Nombre de pertes du signal GPS (limité à 16)
   *  - [0xF20, ..., 0xF2F]: Nombre de reprises en attendant le [r]établssement du signal GPS (limité à 16)
   *  - [0xF30, ..., 0xF3F]: Nombre de [r]établssement du signal GPS (limité à 16)
  */
  ENUM_ERROR_LOSS_GPS            = 0xF1,
  ENUM_ERROR_NBR_RETRY           = 0xF2,
  ENUM_ERROR_NBR_REETABLISHMENTS = 0xF3,

} ENUM_ERRORS;

typedef struct {
  ENUM_ERRORS   family;
  const char    *name_enum;
  const char    *text;
} ST_ERROR_TEXT;

typedef enum {
  GESTION_ERROR_NUMBER = 0,         // Présentation du nombre d'erreurs
  GESTION_ERROR_VALUE,              // Présentation des codes erreurs
  GESTION_ERROR_PAUSE               // Pause
} ENUM_GESTION_ERROR;

typedef struct {
  ENUM_GESTION_ERROR        type_visu;
  byte                      number;
  uint16_t                  value[NBR_MAX_ERRORS];
  unsigned int              counter[NBR_MAX_ERRORS];    // Comptabilisation des erreurs sans limite (ou presque ;-)
  byte                      last_index;
  uint16_t                  wrk_number;
} ST_GESTION_ERROR;

class Errors {
  private:
    ST_GESTION_ERROR            st_errors;

    boolean                     flg_very_long_famine;
    long                        min_duration_very_long_famine;
    long                        max_duration_very_long_famine;

    void add(uint16_t i__value);
    const char *getNameEnum(ENUM_ERRORS i__init_mask) const;
    const char *getText(ENUM_ERRORS i__init_mask) const;

  public:
    Errors();
    ~Errors();

    void activity_error_1();
    void activity_error_2();
    void activity_error_3();

    void clear();
    void update(ENUM_ERRORS i__family);

    void    setVeryLongFamine(boolean i__flg) { flg_very_long_famine = i__flg; };
    boolean isVeryLongFamine() { return flg_very_long_famine; };
    void    updateMinMaxDurationsOfVeryLongFamine(long i__duration);
    long    getMinDurationOfVeryLongFamine() const { return min_duration_very_long_famine; }
    long    getMaxDurationOfVeryLongFamine() const { return max_duration_very_long_famine; }

    void printAll(std::ostringstream &io__out);
};

extern Errors                   *g__errors;

extern  void                    callback_activity_error_1();
extern  void                    callback_activity_error_2();
extern  void                    callback_activity_error_3();

#endif
