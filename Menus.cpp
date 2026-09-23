
#include <Arduino.h>

#include "Misc.h"
#include "Menus.h"

Menus::Menus() : flg_in_progress(false)
{
  Serial.println("Menus::Menus()");
}

Menus::~Menus()
{
  Serial.println("Menus::~Menus()");
}

void Menus::updateVolumeLevel(ST_GESTION_MENUS *io__st_gest_menus)
{

}
