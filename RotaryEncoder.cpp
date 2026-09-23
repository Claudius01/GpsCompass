
#include <Arduino.h>
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder() : encoder_clk(0x00), encoder_data(0x00), encoder_button(0x00), flg_encoder_rotation(false), flg_encoder_button(false),
                                 flg_sens_rotation(false),
                                 encoder_position_pre(0), encoder_position(0), num_menu_pre(-1), num_menu(-1), flg_new_num_menu(false),
                                 nbr_of_press_button(0), num_button_action(ENUM_BUTTON_NO_ACTION)
{
  Serial.println("RotaryEncoder::RotaryEncoder()");

  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DATA, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);

  menus_params.nbr_increments  = 3;         // Nbr d'increments avant de passer au prochain/precedent menu (synthese de son numero)
  menus_params.duration_idle   = 400;       // Duree en mS pour selectionner par inaction un menu (synthese de son libelle)
  menus_params.duration_button = 2000;      // Duree en mS d'attente appui bouton sur un menu selectionne pour confirmation
}

RotaryEncoder::~RotaryEncoder()
{
  Serial.println("RotaryEncoder::~RotaryEncoder()");
}

/*  Mise à jour des états de l'encodeur sous interruption
 */
void RotaryEncoder::update()
{
  // Acquisition CLK
  encoder_clk <<= 1;
  if (digitalRead(ENCODER_CLK)) {
    encoder_clk |= 0x01;
  }
  else {
    encoder_clk &= 0xfe;
  }

  // Acquisition DATA
  encoder_data <<= 1;
  if (digitalRead(ENCODER_DATA)) {
    encoder_data |= 0x01;
  }
  else {
    encoder_data &= 0xfe;
  }

  // Acquisition BUTTON
  encoder_button <<= 1;
  if (digitalRead(ENCODER_BUTTON)) {
    encoder_button |= 0x01;
  }
  else {
    encoder_button &= 0xfe;
  }

  if (flg_encoder_rotation == false) {
    // Resolution de l'etat de l'encodeur sur CLK --\__
    if (encoder_clk == 0xf0) {
      flg_encoder_rotation = true;
    }

    // Resolution de l'etat du bouton de l'encoder (bascule)
    if (flg_encoder_button == false && encoder_button == 0x00) {
      flg_encoder_button = true;
      flg_encoder_rotation = true;
    }
    else if (flg_encoder_button == true && encoder_button == 0xff) {
      flg_encoder_button = false;
      flg_encoder_rotation = true;
    }
  }
}

int RotaryEncoder::updateEncoderPosition()
{
    /* Etats de DATA de part et d'autre du front de CLK
     *  Remarque: Le traitement suite a l'acquisition de CLK sur --\__ semble plus stable que celui sur __/--
     *  
     *  Sens positif de rotation (DATA a 0 sur front descendant de CLK et a 1 sur front montant de CLK)
     *        7654 3210          7654 3210
     *  CLK:  ----\____ 0xf0     ____/---- 0x0f
     *  DATA: _____/--- 0x07     ___/----- 0x1f
     *  DATA: ______/-- 0x03     __/------ 0x3f
     *  DATA: _______/- 0x01     _/------- 0x7f
     *  DATA: _________ 0x00     --------- 0xff

     *  Sens negatif de rotation (DATA a 1 sur front descendant de CLK et a 0 sur front montant de CLK)
     *        7654 3210          7654 3210
     *  CLK:  ----\____ 0xf0     ____/---- 0x0f
     *  DATA: ___/----- 0x1f     _____/--- 0x07
     *  DATA: __/------ 0x3f     ______/-- 0x03
     *  DATA: _/------- 0x7f     _______/- 0x01
     *  DATA: --------- 0xff     _________ 0x00
     */
  if (encoder_clk == 0xf0) {
    // Update the position if CLK --\__
    switch(encoder_data) {
    case 0x07:
    case 0x03:
    case 0x01:
    case 0x00:
      flg_sens_rotation = false;  // Sens anti-horaire (decrement)
      encoder_position--;
    break;
    case 0x1F:
    case 0x3F:
    case 0x7F:
    case 0xFF:
      flg_sens_rotation = true;   // Sens horaire (increment)
      encoder_position++;
      break;
    default:
      break;
    }
  }

  // Test si rotation significative (deroulement des menus)
  if ((encoder_position - encoder_position_pre) >= menus_params.nbr_increments) {
    encoder_position_pre = encoder_position;

    num_menu = (num_menu + 1) % ENCODER_MAX_MENU;
  }
  else if ((encoder_position - encoder_position_pre) <= -menus_params.nbr_increments) {
    encoder_position_pre = encoder_position;

    num_menu--;
    if (num_menu < 0) {
      num_menu = (ENCODER_MAX_MENU - 1);
    }
  }
  if (num_menu_pre != num_menu) {
    flg_new_num_menu = true;
    num_menu_pre = num_menu;
  }
  // Fin: Test si rotation significative (deroulement des menus)

  flg_encoder_rotation = false;

  return encoder_position;
}

void RotaryEncoder::setPressButtonAction(byte i__nbr_of_press_button)
{
  switch(i__nbr_of_press_button) {
  case 1:
    num_button_action = ENUM_BUTTON_ACTION_1;
    break;
  case 2:
    num_button_action = ENUM_BUTTON_ACTION_2;
    break;
  case 3:
    num_button_action = ENUM_BUTTON_ACTION_3;
    break;
  default:
    break;
  }
}

ENUM_BUTTON_ACTION RotaryEncoder::getPressButtonAction()
{
  ENUM_BUTTON_ACTION l__num_button_action = num_button_action;
  num_button_action = ENUM_BUTTON_NO_ACTION;

  return l__num_button_action;
}
