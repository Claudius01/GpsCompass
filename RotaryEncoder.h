#ifndef __ROTARY_ENCODER__
#define __ROTARY_ENCODER__

#define ENCODER_CLK     27        // @KiCad
#define ENCODER_DATA    26        // @KiCad
#define ENCODER_BUTTON  25        // @KiCad

#define ENCODER_MAX_MENU    12

typedef enum {
  ENUM_BUTTON_NO_ACTION = 0,
  ENUM_BUTTON_ACTION_1,
  ENUM_BUTTON_ACTION_2,
  ENUM_BUTTON_ACTION_3
} ENUM_BUTTON_ACTION;

typedef struct {
  int nbr_increments;         // Nbr d'increments avant de passer au prochain/precedent menu
  int duration_idle;          // Duree en mS pour selectionner par inaction un menu (synthese de son libelle)
  int duration_button;        // Duree en mS d'attente appui bouton sur un menu selectionne
} ST_MENU_PARAMETERS;

class RotaryEncoder {
  private:
    byte      encoder_clk;
    byte      encoder_data;
    byte      encoder_button;

    boolean   flg_encoder_rotation;
    boolean   flg_encoder_button;       // true (false): Bouton (non) enfonce

    boolean   flg_sens_rotation;        // false: Sens anti-horaire (decrement) true: Sens horaire (increment) 
    int       encoder_position_pre;
    int       encoder_position;
    int       num_menu_pre;
    int       num_menu;                 // [-1, 0, 1, 2, ..., (ENCODER_MAX_MENU - 1)] (-1 pour "No mean")
    boolean   flg_new_num_menu;

    byte                nbr_of_press_button;
    ENUM_BUTTON_ACTION  num_button_action;

    ST_MENU_PARAMETERS  menus_params;

  public:
    RotaryEncoder();
    ~RotaryEncoder();

    void update();
    boolean isEncoderPosition() const { noInterrupts(); return flg_encoder_rotation; interrupts(); };
    boolean stateEncoderButton() const { noInterrupts(); return flg_encoder_button; interrupts(); };
    void clearNumMenu() { num_menu = -1; };
    int getNumMenu() { return num_menu; };
    boolean isNewNumMenu() { boolean l__flg = flg_new_num_menu; flg_new_num_menu = false; return l__flg; };

    int updateEncoderPosition();
    boolean getSensRotation() const { return flg_sens_rotation; };

    byte getNbrOfPressButton() const { return nbr_of_press_button; };
    void resetNbrOfPressButton() { nbr_of_press_button = 0; };
    void incNbrOfPressButton() { nbr_of_press_button++; };

    void setPressButtonAction(byte i__nbr_of_press_button);
    ENUM_BUTTON_ACTION getPressButtonAction();

    void setMenuParameters(int i__nbr_increments, int i__duration_idle, int i__duration_button) {
      menus_params.nbr_increments  = i__nbr_increments;
      menus_params.duration_idle   = i__duration_idle;
      menus_params.duration_button = i__duration_button;
    }
    int getMenuParamDurationIdle()   const { return menus_params.duration_idle; };
    int getMenuParamDurationButton() const { return menus_params.duration_button; };
};
#endif
