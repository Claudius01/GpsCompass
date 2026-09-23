#ifndef __MENUS__
#define __MENUS__

class Menus {
  private:
    boolean          flg_in_progress;

  public:
    Menus();
    ~Menus();

    void setFlgInProgress(boolean i__flg) { flg_in_progress = i__flg; };
    boolean getFlgInProgress() const { return flg_in_progress; };

    void updateVolumeLevel(ST_GESTION_MENUS *io__st_gest_menus);
};
#endif
