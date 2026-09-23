// $Id: Pilot.h,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef __PILOT__
#define __PILOT__

#include "PromptsSynthesis.h"

#define EPSILON       0.0001      // Pour éviter les divisions par 0.0

typedef enum {
  COORD_NO_MEAN = 0,
  COORD_PRE,
  COORD_CURRENT,
  COORD_STARTING_POS,
  COORD_LAST_POS_SAVE,
  COORD_END_POS,
  COORD_NUMBER
} ENUM_COORD_NUM;

class Pilot {
  private:
    float                           nan;
    COORD                           coord[COORD_NUMBER];

    ENUM_SYNTH_DISTANCE_MODES       dist_mode;    // 'SYNTH_DIST_FROM_STARTING_POS' or 'SYNTH_DIST_FROM_POS_SAVE'

  public:
    Pilot();
    ~Pilot();

    void initCoordToNoMean(COORD *i__coord);

#if 0
    void setCoordCurrent(COORD *i__coord);
#endif

    void initCoordToCurrent(COORD *i__coord);
    void initCoordStartingPosition(const COORD *i__coord);
    void initCoordLastPosSave(COORD *i__coord);

    void  setDuration(ENUM_COORD_NUM i__num, long i__value) { if (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) coord[i__num].duration = i__value; };
    long  getDuration(ENUM_COORD_NUM i__num) const          { return (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) ? coord[i__num].duration : 0L; };

    void  setLat(ENUM_COORD_NUM i__num, float i__value) { if (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) coord[i__num].lat = i__value; };
    float getLat(ENUM_COORD_NUM i__num) const           { return (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) ? coord[i__num].lat : nan; };

    void  setLon(ENUM_COORD_NUM i__num, float i__value) { if (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) coord[i__num].lon = i__value; };
    float getLon(ENUM_COORD_NUM i__num) const           { return (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) ? coord[i__num].lon : nan; };

    void  setEle(ENUM_COORD_NUM i__num, float i__value) { if (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) coord[i__num].ele = i__value; };
    float getEle(ENUM_COORD_NUM i__num) const           { return (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) ? coord[i__num].ele : nan; };

    void  setCap(ENUM_COORD_NUM i__num, float i__value) { if (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) coord[i__num].cap = i__value; };
    float getCap(ENUM_COORD_NUM i__num) const           { return (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) ? coord[i__num].cap : nan; };

    void  setSpeedKmH(ENUM_COORD_NUM i__num, float i__value) { if (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) coord[i__num].speedKmH = i__value; };
    float getSpeedKmH(ENUM_COORD_NUM i__num) const           { return (i__num > COORD_NO_MEAN || i__num < COORD_NUMBER) ? coord[i__num].speedKmH : nan; };

    void                      setDistMode(ENUM_SYNTH_DISTANCE_MODES i__dist_mode) {dist_mode = i__dist_mode; };
    ENUM_SYNTH_DISTANCE_MODES getDistMode() { return dist_mode; };

    uint32_t getDistanceTo(COORD *i__coord);

#if 0
    uint32_t getDistanceFromTo(COORD *i__coord_from, COORD *i__coord_to);
#endif

    uint32_t getDistanceToStartingPos();
    uint32_t getDistanceToLastPosSave();

    float    getCapTo(COORD *i__coord);

#if 0
    float    getCapFromTo(COORD *i__coord_from, COORD *i__coord_to);
#endif

    float    getCapToStartingPos();
    float    getCapToLastPosSave();
    float    getEleToStartingPos();
    float    getEleToLastPosSave();
};

extern Pilot                  *g__pilot;
#endif
