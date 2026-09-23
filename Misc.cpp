// $Id: Misc.cpp,v 1.1.1.1 2025/01/17 12:31:27 administrateur Exp $

#ifndef USE_SIMULATION
#include <Arduino.h>
#else
#include "../ArduinoTypes.h"
#include "../SerialPrint.h"
#endif

#include "Misc.h"
#include "Statistics.h"

#include "WT2003S.h"

static byte                 g__monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

ST_FOR_SOMMER_TIME_CHANGE   g__st_for_sommer_time_change = {
  false,
  {
    0, 0, 0,
    0, 0, 0,
    0
  },
  {
    0, 0, 0,
    0, 0, 0,
    0
  },
  { 
    { JEUDI,    "Jeudi" },
    { VENDREDI, "Vendredi" },
    { SAMEDI,   "Samedi" },
    { DIMANCHE, "Dimanche" },
    { LUNDI,    "Lundi" },
    { MARDI,    "Mardi" },
    { MERCREDI, "Mercredi" }
  }
};

void initToNaN(float *o__value)
{
  memset(o__value, 0xff, sizeof(float));
}

int convAscHexa2Int(char i__val)
{
	char l__buff[2];
	l__buff[0] = i__val;
	l__buff[1] = '\0';

	return (int)strtol(l__buff, NULL, 16);
}

boolean convAscii2Hexa(char i__char, byte *io__hexa)
{
  boolean l__err = false;

  if (i__char >= '0' && i__char <= '9') {
    *io__hexa |= (i__char - '0');
  }
  else if (i__char >= 'a' && i__char <= 'f') {
    *io__hexa |= (i__char - 'a' + 0xa);
  }
  else if (i__char >= 'A' && i__char <= 'F') {
    *io__hexa |= (i__char - 'A' + 0xa);
  }
  else {
    l__err = true;
  }

  return l__err;
}

void convertFloatToString(char *o__buffer, float i__value, int i__nbr_dec)
{
  boolean l__flg_neg = false;
  if (i__value < 0.0) {
    i__value = -i__value;
    l__flg_neg = true;
  }

  size_t n = 0;
  byte l__digit;
  char l__digits[8];
  memset(l__digits, '\0', sizeof(l__digits));

  // Add epsilon for round
  i__value += 0.0005;

  if (l__flg_neg == true) {
    l__digits[n++] = '-';
  }

  boolean l__flg_zero = false;
  l__digit = (byte)(((uint16_t)i__value % 1000) / 100);
  if (l__digit) {
    l__digits[n++] = '0' + l__digit;
    l__flg_zero = false;
  }
  else {
    l__flg_zero = true;
  }
  l__digit = (byte)(((uint16_t)i__value % 100) / 10);
  if (l__flg_zero == false || l__digit) {
    l__digits[n++] = '0' + l__digit;
    l__flg_zero = false;
  }
  else {
    l__flg_zero = true;
  }
  l__digit = (byte)(((uint16_t)i__value % 10)); l__digits[n++] = '0' + l__digit;
  l__digits[n++] = '.';

  if (i__nbr_dec > 0) {
    l__digit = (byte)(((uint16_t)(i__value * 10.0)) % 10); l__digits[n++] = '0' + l__digit;
  }
  if (i__nbr_dec > 1) {
    l__digit = (byte)(((uint16_t)(i__value * 100.0)) % 10); l__digits[n++] = '0' + l__digit;
  }

  strcpy(o__buffer, l__digits);
}

/*! \brief hexDump
 * Dump hexadecimal limited at i__nbr_bytes if > 0
 *
 * Output format as Unix (hexdump -C)
 *
 * Example:
 * 00000000  30 31 32 33 34 35 36 37  38 39 41 42 43 44 45 0a  |0123456789ABCDE.|
 * 00000060  30 31 32 33 34 35 36 37  38 39 41 0a              |0123456789A.|
 * 0000006c
 */
void hexDump(std::ostringstream &o_out, const char *i__bytes, const size_t i__nbr_bytes)
{
	size_t i = 0;
	size_t l__size_read = 0;
	char l__buffer[80];
	char l__buffer_pre[80];
	l__buffer_pre[0] = '\0';
	bool l__flg_skip = false;

	sprintf(l__buffer, "Dump of [%p] (%d bytes)\n", i__bytes, (int)i__nbr_bytes);
	o_out << l__buffer << (i__nbr_bytes ? "Address    0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F   0123456789ABCDEF\n" : "No data\n");

	while (l__size_read < i__nbr_bytes && (i__nbr_bytes == 0 || (i * 16) < i__nbr_bytes)) {
		std::string sb1;
		std::string sb2 = " |";

		sprintf(l__buffer, "%08x  ", i * 16);
		for (int j = 0; j < 16; j++) {
			if (l__size_read < i__nbr_bytes) {
				char value = *(i__bytes + l__size_read++);
				sprintf(&l__buffer[strlen(l__buffer)], "%02x%s", (unsigned char)value, (j != 7) ? " " : "  ");
				if (value >= ' ' && value < 0x7f) {
					char l__s_value[2];
					l__s_value[0] = value;
					l__s_value[1] = '\0';
					sb2.append(l__s_value);
				}
				else {
					sb2.append(".");
				}
			}
			else {
				if (j < 8) sb1.append(" ");
				for (; j < 16; j++) {
					sb1.append("   ");
				}
			}
		}

		// Dump buffer if...
		if (l__buffer_pre[0] == '\0'					// 1st dump
			|| strcmp(l__buffer_pre + 8, l__buffer + 8)	// Buffer different of the previous
			/* || l__size_read == i__nbr_bytes */)				// Last buffer (no comment for dump)
		{
			strcpy(l__buffer_pre, l__buffer);

			if (!(l__size_read % 16) && l__flg_skip == true) {
				// Forget the last identical buffer
				l__flg_skip = false;
			}

			sb2.append("|\n");

			o_out << l__buffer;
			o_out << sb1 << sb2;
		}
		else if (l__flg_skip == false) {				// No dump if 2nd, 3rd, etc. identical buffer
			l__flg_skip = true;

			o_out << "*\n";
		}

		i++;
	}

	// 1st offset value after the buffer
	sprintf(l__buffer, "%08x", l__size_read);
	o_out << l__buffer;
}


/* Convert broken time to calendar time (seconds since 1970)
 *
 * Warning: Date limite en 2038 ;-)
*/
long my_mktime(ST_TM *timeptr)
{
  int  year  = timeptr->tm_year + 1900;
  int  month = timeptr->tm_mon;
  int  i;
  long seconds;

  // Seconds from 1970 till 1 jan 00:00:00 this year
  seconds = (long)(year - 1970) * 60L * 60 * 24 * 365;  // seconds is a long

  // Add extra days for leap years
  for (i = 1970; i < year; i++) {
    if (LEAP_YEAR(i)) {
      seconds += 60L * 60 * 24;        // Add one day for each bisecstil year
    }
  }

    // Add days for this year
  for (i = 0; i < month; i++) {
    if (i == 1 && LEAP_YEAR(year)) { 
      seconds += 60L * 60 * 24 * 29;  // seconds is a long
    }
    else {
      seconds += (long)g__monthDays[i] * 60L * 60 * 24;  // seconds is a long
    }
  }

  seconds += (long)(timeptr->tm_mday - 1) * 60L * 60 * 24;  // seconds is a long
  seconds += (long)timeptr->tm_hour * 60L * 60;
  seconds += (long)timeptr->tm_min * 60L;
  seconds += (long)timeptr->tm_sec;

  return seconds;
}

/* Calcul du temps en secondes depuis le 01/01/1970 - 00:00:00
*/
long calculatedEpochTime(ST_DATE_AND_TIME *i__st_date_and_time)
{
    ST_TM l__st_tm;
    memset(&l__st_tm, '\0', sizeof(ST_TM));

    l__st_tm.tm_mday = i__st_date_and_time->day;
    l__st_tm.tm_mon  = i__st_date_and_time->month - 1;
    l__st_tm.tm_year = i__st_date_and_time->year + (2000 - 1900);
    l__st_tm.tm_hour = i__st_date_and_time->hours;
    l__st_tm.tm_min  = i__st_date_and_time->minutes;
    l__st_tm.tm_sec  = i__st_date_and_time->seconds;

    return my_mktime(&l__st_tm);
}

// TODO: Add test of all fields
long buildGpsDateTime(const char i__date[], const char i__time[], char *o_date_time, ST_DATE_AND_TIME *o__dateAndTime)
{
	ST_DATE_AND_TIME l__dateAndTime;
	memset(&l__dateAndTime, '\0', sizeof(ST_DATE_AND_TIME));

	char l__t_wrk[3];
	memset(l__t_wrk, '\0', sizeof(l__t_wrk));

	// Set the fields Year, Month, Day, Hours, Minutes and Seconds
	// Year as 20XX
	strncpy(l__t_wrk, &i__date[4], 2);
	l__dateAndTime.year = (int)strtol(l__t_wrk, NULL, 10);

	// Month
	strncpy(l__t_wrk, &i__date[2], 2);
	l__dateAndTime.month = (char)strtol(l__t_wrk, NULL, 10);

	// Day
	strncpy(l__t_wrk, &i__date[0], 2);
	l__dateAndTime.day = (char)strtol(l__t_wrk, NULL, 10);

	// Hour
	strncpy(l__t_wrk, &i__time[0], 2);
	l__dateAndTime.hours = (char)strtol(l__t_wrk, NULL, 10);

	// Minutes
	strncpy(l__t_wrk, &i__time[2], 2);
	l__dateAndTime.minutes = (char)strtol(l__t_wrk, NULL, 10);

	// Seconds
	strncpy(l__t_wrk, &i__time[4], 2);
	l__dateAndTime.seconds = (char)strtol(l__t_wrk, NULL, 10);
	// End: Set the fields Year, Month, Day, Hours, Minutes and Seconds

  // Get the current epoch and save into datas
  long l__epoch = calculatedEpochTime(&l__dateAndTime);
  l__dateAndTime.epoch = l__epoch;

  // Calculation of day of the week
  l__dateAndTime.day_of_week = (char)((l__epoch / 86400L) % 7L);

  // Détermination changement d'heure été/hiver si pas déjà fait
  if (g__st_for_sommer_time_change.flg_available == false) {
    setSommerWinterTimeChange(&l__dateAndTime);
  }

  // Application heure été/hiver
  calcSommerTimeChange(&l__dateAndTime);

  memcpy(o__dateAndTime, &l__dateAndTime, sizeof(ST_DATE_AND_TIME));

  const char *l__sommer_winter = "Unknown";
  if (o__dateAndTime->sommer_winter == SOMMER) {
    l__sommer_winter = "Heure d'été";
  }
  else if (o__dateAndTime->sommer_winter == WINTER) {
    l__sommer_winter = "Heure d'hiver";
  }

  // Présentation de la date, heure, jour de la semaine avec application de l'heure d'été/hiver
  ST_DATE_AND_TIME l__dateAndTime_presentation;
  memcpy(&l__dateAndTime_presentation, o__dateAndTime, sizeof(ST_DATE_AND_TIME));
  applySommerWinterHour(&l__dateAndTime_presentation);

  sprintf(o_date_time, "%04d/%02d/%02d %02d:%02d:%02d (#%d/#%d %s) (%s)",
    2000 + l__dateAndTime_presentation.year, l__dateAndTime_presentation.month, l__dateAndTime_presentation.day,
    l__dateAndTime_presentation.hours, l__dateAndTime_presentation.minutes, l__dateAndTime_presentation.seconds,
    l__dateAndTime_presentation.nbr_days_before + 1,
    l__dateAndTime_presentation.nbr_days_before + 1 + l__dateAndTime_presentation.nbr_days_after,    
    g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime_presentation.day_of_week].name,
    l__sommer_winter);

	return l__epoch;
}

boolean setSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime)
{
  char l__buffer[80];
  ST_DATE_AND_TIME l__dateAndTime;
  boolean l__flg_available_sommer = false;
  boolean l__flg_available_winter = false;

  // Détermination du passage à l'heure d'été (dernier Dimanche de Mars)
  // Set at YEAR/03/01 - 00:00:00
  memset(&l__dateAndTime, '\0', sizeof(ST_DATE_AND_TIME));
  l__dateAndTime.year = i__dateAndTime->year;
  l__dateAndTime.month = 3;
  l__dateAndTime.day = 1;

  long l__epoch = calculatedEpochTime(&l__dateAndTime);

  // Calculation of day of the week
  l__dateAndTime.day_of_week = (char)((l__epoch / 86400L) % 7L);

  sprintf(l__buffer, "Epoch UNIX GMT [%ld] (%04d/%02d/%02d %02d:%02d:%02d [%s])\n",
    l__epoch,
    2000 + l__dateAndTime.year, l__dateAndTime.month, l__dateAndTime.day,
    l__dateAndTime.hours, l__dateAndTime.minutes, l__dateAndTime.seconds,
    g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime.day_of_week].name);

  Serial.print(l__buffer);

  // Parcours jusqu'à trouver le dernier Dimanche du mois
  byte n = 0;
  byte l__nbr_days = g__monthDays[(int)l__dateAndTime.month - 1];
  for (n = l__dateAndTime.day; n <= l__nbr_days; n++) {
    byte l__day_of_week = (byte)((l__epoch / 86400L) % 7L);

    if (l__day_of_week == DIMANCHE) {
      byte l__nbr_days_after = (l__nbr_days - n) / 7;

      if (l__nbr_days_after == 0) {
        g__st_for_sommer_time_change.st_date_sommer.day = n;
        g__st_for_sommer_time_change.st_date_sommer.month = l__dateAndTime.month;
        g__st_for_sommer_time_change.st_date_sommer.year = l__dateAndTime.year;
        g__st_for_sommer_time_change.st_date_sommer.day_of_week = l__day_of_week;
        g__st_for_sommer_time_change.st_date_sommer.epoch = l__epoch + 3600L;     // Change at 01:00:00

        l__flg_available_sommer = true;

        sprintf(l__buffer, "\tDernier %s du mois [%04d/%02d/%02d 01:00:00] (%ld)\n",
          g__st_for_sommer_time_change.st_days_in_week[(int)g__st_for_sommer_time_change.st_date_sommer.day_of_week].name,
          2000 + g__st_for_sommer_time_change.st_date_sommer.year,
          g__st_for_sommer_time_change.st_date_sommer.month,
          g__st_for_sommer_time_change.st_date_sommer.day,
          g__st_for_sommer_time_change.st_date_sommer.epoch);

        Serial.print(l__buffer);
        break;
      }
    }

    l__epoch += 86400L;
  }
  // Fin: Détermination du passage à l'heure d'été (dernier Dimanche de Mars)

  // Détermination du passage à l'heure d'hiver (dernier Dimanche d'octobre)
  // Set at YEAR/10/01 - 00:00:00
  memset(&l__dateAndTime, '\0', sizeof(ST_DATE_AND_TIME));
  l__dateAndTime.year = i__dateAndTime->year;
  l__dateAndTime.month = 10;
  l__dateAndTime.day = 1;

  l__epoch = calculatedEpochTime(&l__dateAndTime);

  // Calculation of day of the week
  l__dateAndTime.day_of_week = (char)((l__epoch / 86400L) % 7L);

  sprintf(l__buffer, "Epoch UNIX GMT [%ld] (%04d/%02d/%02d %02d:%02d:%02d [%s])\n",
    l__epoch,
    2000 + l__dateAndTime.year, l__dateAndTime.month, l__dateAndTime.day,
    l__dateAndTime.hours, l__dateAndTime.minutes, l__dateAndTime.seconds,
    g__st_for_sommer_time_change.st_days_in_week[(int)l__dateAndTime.day_of_week].name);

  Serial.print(l__buffer);

  // Parcours jusqu'à trouver le dernier Dimanche du mois
  n = 0;
  l__nbr_days = g__monthDays[(int)l__dateAndTime.month - 1];
  for (n = l__dateAndTime.day; n <= l__nbr_days; n++) {
    byte l__day_of_week = (byte)((l__epoch / 86400L) % 7L);

    if (l__day_of_week == DIMANCHE) {
      byte l__nbr_days_after = (l__nbr_days - n) / 7;

      if (l__nbr_days_after == 0) {
        g__st_for_sommer_time_change.st_date_winter.day = n;
        g__st_for_sommer_time_change.st_date_winter.month = l__dateAndTime.month;
        g__st_for_sommer_time_change.st_date_winter.year = l__dateAndTime.year;
        g__st_for_sommer_time_change.st_date_winter.day_of_week = l__day_of_week;
        g__st_for_sommer_time_change.st_date_winter.epoch = l__epoch + 3600L;     // Change at 01:00:00

        l__flg_available_winter = true;

        sprintf(l__buffer, "\tDernier %s du mois [%04d/%02d/%02d 01:00:00] (%ld)\n",
          g__st_for_sommer_time_change.st_days_in_week[(int)g__st_for_sommer_time_change.st_date_winter.day_of_week].name,
          2000 + g__st_for_sommer_time_change.st_date_winter.year,
          g__st_for_sommer_time_change.st_date_winter.month,
          g__st_for_sommer_time_change.st_date_winter.day,
          g__st_for_sommer_time_change.st_date_winter.epoch);

        Serial.print(l__buffer);
        break;
      }
    }

    l__epoch += 86400L;
  }
  // Fin: Détermination du passage à l'heure d'hiver (dernier Dimanche d'octobre)

  if (l__flg_available_sommer == true && l__flg_available_winter == true) {
    g__st_for_sommer_time_change.flg_available = true;
  }
  else {
    g__st_for_sommer_time_change.flg_available = false;
  }

  return g__st_for_sommer_time_change.flg_available;
}

void calcSommerTimeChange(ST_DATE_AND_TIME *io__dateAndTime)
{
#if 0
    char l__buffer[80];
#endif

    io__dateAndTime->nbr_days_before = (io__dateAndTime->day - 1) / 7;            // Nombre du même jour avant dans le mois

    byte l__nbr_days = g__monthDays[(int)io__dateAndTime->month - 1];
    if (io__dateAndTime->month == 2 && LEAP_YEAR(io__dateAndTime->year)) {
      l__nbr_days = 29;   // 29 jours en février les années bissextiles
    }
    io__dateAndTime->nbr_days_after = (l__nbr_days - io__dateAndTime->day) / 7;   // Nombre du même jour après dans le mois

    /* Détermination heure été/hiver:
     *   - Du dernier Dimance d'Octobre au dernier Dimanche de Mars: Heure d'hiver
     *   - Du dernier Dimanche de Mars au dernier Dimance d'Octobre: Heure d'été
     */
    io__dateAndTime->sommer_winter = getSommerWinterTimeChange(io__dateAndTime);

#if 0
    sprintf(l__buffer, "calcSommerTimeChange(): %s (%d/%d) Sommer/Winter [%d]\n",
      g__st_for_sommer_time_change.st_days_in_week[(int)io__dateAndTime->day_of_week].name,
      io__dateAndTime->nbr_days_before + 1,
      io__dateAndTime->nbr_days_before + 1 + io__dateAndTime->nbr_days_after,
      io__dateAndTime->sommer_winter);

    Serial.print(l__buffer);
#endif
}

ENUM_SOMMER_WINTER getSommerWinterTimeChange(ST_DATE_AND_TIME *i__dateAndTime)
{
  ENUM_SOMMER_WINTER l__sommer_winter = NO_SOMMER_WINTER;

#if 0   // Version with month/day 00:00:00 test
  if (g__st_for_sommer_time_change.flg_available == true) {
    if (i__dateAndTime->month > 3 && i__dateAndTime->month < 10) {
      // Heure d'été pour les mois d'Avril, Mai, Juin, Juillet, Aout et Septembre
      l__sommer_winter = SOMMER;
    }
    else if (i__dateAndTime->month < 3 || i__dateAndTime->month > 10) {
      // Heure d'hiver pour les mois de Janvier, Février, Novembre et Décembre
      l__sommer_winter = WINTER;
    }
    else if (i__dateAndTime->month == 3) {
      /* Heure d'été pour le mois de Mars avant la date de basculement à 00:00:00 GMT
       *  => Remarque: Le basculement devrait s'effectuer à 01:00:00
       */
      l__sommer_winter = (i__dateAndTime->day < g__st_for_sommer_time_change.st_date_sommer.day) ? WINTER : SOMMER;
    }
    else if (i__dateAndTime->month == 10) {
      /* Heure d'hiver pour le mois d'Octobre après la date de basculement à 00:00:00 GMT
       *  => Remarque: Le basculement devrait s'effectuer à 01:00:00
       */
      l__sommer_winter = (i__dateAndTime->day < g__st_for_sommer_time_change.st_date_winter.day) ? SOMMER : WINTER;      
    }
  }
#else   // Version with epoch test
  if (i__dateAndTime->epoch <  (g__st_for_sommer_time_change.st_date_sommer.epoch)
   || i__dateAndTime->epoch >= (g__st_for_sommer_time_change.st_date_winter.epoch)) {
    l__sommer_winter = WINTER;
  }
  else if (i__dateAndTime->epoch >= (g__st_for_sommer_time_change.st_date_sommer.epoch)
        && i__dateAndTime->epoch <  (g__st_for_sommer_time_change.st_date_winter.epoch)) {
    l__sommer_winter = SOMMER;
  }
#endif

  return l__sommer_winter;
}

void applySommerWinterHour(ST_DATE_AND_TIME *io__dateAndTime_presentation)
{
  byte l__nbr_hours = 0;
  if (io__dateAndTime_presentation->sommer_winter == WINTER) {
    l__nbr_hours += 1;

    // Update for statistics
    g__stats->setOffsetSeconds(3600);
  }
  else if (io__dateAndTime_presentation->sommer_winter == SOMMER) {
    l__nbr_hours += 2;

    // Update for statistics
    g__stats->setOffsetSeconds(2 * 3600);
  }

  io__dateAndTime_presentation->hours += l__nbr_hours;
  if (io__dateAndTime_presentation->hours > 23) {
    //  Passage à minuit => +1 sur les jours dans le mois et dans la semaine
    io__dateAndTime_presentation->hours %= 24;
    io__dateAndTime_presentation->day += 1;
    io__dateAndTime_presentation->day_of_week += 1;
    io__dateAndTime_presentation->day_of_week %= 7;

    // Test si changement de mois, voire d'année ;-)
    byte l__nbr_days = g__monthDays[(int)io__dateAndTime_presentation->month];

    // Années bissextiles
    if (io__dateAndTime_presentation->month == 2 && LEAP_YEAR(io__dateAndTime_presentation->year)) {
      l__nbr_days = 29;
    }

    if (io__dateAndTime_presentation->day > l__nbr_days) {
      io__dateAndTime_presentation->day = 1;
      io__dateAndTime_presentation->month += 1;

      if (io__dateAndTime_presentation->month > 12) {
        io__dateAndTime_presentation->month = 1;
        io__dateAndTime_presentation->year += 1;
      }
    }
  }
}

/* Conversion du nibble [0x00..0x0f] en un ASICI ['0', '1', ..., '9', 'A', .., 'F']
 * 
 */
char convHexa2Ascii(char i__value)
{
  char l__value = i__value & 0x0F;

  if (l__value <= 9) {
    return '0' + l__value;
  }
  else if (l__value >= 0x0A && l__value <= 0x0F) {
    return 'A' + (l__value - 0x0A);
  }
  else {
    return '?';
  }
}

void convertDurationToString(char *o__result, long i__duration, boolean i__flg_with_sign)
{
  const char *l__sign = "";

  if (i__flg_with_sign == true) {
    if (i__duration < 0L) {
      i__duration = -i__duration;
      l__sign = "-";
    }
    else {
      l__sign = "+";
    }
  }

  long l__hh = i__duration / 3600;
  int l__mm = (i__duration - 3600 * l__hh) / 60;
  int l__ss = i__duration % 60;

  if (l__hh > 0 ) {
    sprintf(o__result, "%s%ldH %02d' %02d\"", l__sign, l__hh, l__mm, l__ss);
  }
  else if (l__mm > 0 ) {
    sprintf(o__result, "%s%d' %02d\"", l__sign, l__mm, l__ss);
  }
  else {
    sprintf(o__result, "%s%d\"", l__sign, l__ss);
  }
}

// Conversion degres + minutes decimales en degres decimaux
float convertToDecimalDegres(char *i__t_value)
{
  char l__t_wrk[16];
  float l__value;

  initToNaN(&l__value);     // Init float to 'NaN'
  memset(l__t_wrk, '\0', sizeof(l__t_wrk));
  strncpy(l__t_wrk, i__t_value, sizeof(l__t_wrk) - 1);

  char *l__point = strchr(l__t_wrk, '.');
  if (l__point != NULL) {
    *l__point = '\0';

    int l__degres = (int)((float)strtod(l__t_wrk, NULL)) / 100;
    float l__minutes_dec_frac = (float)strtod(i__t_value + (l__point - l__t_wrk - 2), NULL);
    l__value = l__degres + (l__minutes_dec_frac / 60.0);
  }

  return l__value;
}

/* Methode de translation des commandes de diffusion des prompts 'KT403A' => 'WT2003S'
 * - 1st byte: 0x12 (mp3/), 0x13 (advert/), 0x14 (directory in 2nd byte @ 4-bits MSB) 
 * - Num file : 2nd byte @ 4-bits LSB + 3rd byte => 12 bits [0..4095]
 * 
 * Return: - 0: Pas de translation effectue
 *           => 'o__command' non maj
 *         - Longueur de '1 byte de commande + nom du fichier sur 5 char' (6 bytes)
 *           => 'o__command' maj a passer a la methode 'SerialMP3Player::writeToWT2003S()'
 */
size_t translatePromptsKT403AToWT2003S(byte *i__command, size_t i__size, byte *o__command, boolean i__flg_trace)
{
  char l__buffer[80];
  memset(l__buffer, '\0', sizeof(l__buffer));
  size_t l__size = i__size;
  for (l__size = 0; l__size < i__size; l__size++) {
    sprintf(&l__buffer[strlen(l__buffer)], "%c%02x", (l__size == 0) ? '[' : ' ', *(i__command + l__size));
  }
  strcat(&l__buffer[strlen(l__buffer)], "]");

  if (i__flg_trace) {
    Serial.printf("translatePromptsKT403AToWT2003S(%s, [%d bytes])\n", l__buffer, i__size);
  }

  size_t l__o__size = 0;

  if (i__size == 3) {
    byte l__mode  = *i__command;
    byte l__dir = (byte)-1;

    char l__t_dir[8];
    memset(l__t_dir, '\0', sizeof(l__t_dir));

    int l__num = (256 * (*(i__command + 1) & 0x0F)) + (*(i__command + 2));
    uint16_t l__offset_translate = (uint16_t)-1;

    switch (l__mode) {
    case 0x12:
      strcpy(l__t_dir, "mp3");
      l__offset_translate = 27000;
      break;
    case 0x13:
      strcpy(l__t_dir, "advert");
      // Pas de translation car gestion avec l'index du fichier ;-)

      break;
    case 0x14:
      l__dir = ((*(i__command + 1) >> 4) & 0x0F);
      sprintf(l__t_dir, "%02d", l__dir);

      // Translation @ 'l__dir'
      switch (l__dir) {
      case 1:
      case 4:
        l__offset_translate = 0;
        break;
      case 2:
        l__offset_translate = 3000;
        break;
      case 3:
        l__offset_translate = 7000;
        break;
      case 5:
        l__offset_translate = 10000;
        break;
      case 6:
        l__offset_translate = 17000;
        break;
      case 7:
        l__offset_translate = 15000;
        break;
      case 8:
        l__offset_translate = 30000;
        break;
      case 9:
        l__offset_translate = 40000;
        break;
      case 14:
        l__offset_translate = 22000;
        break;
      case 15:
        l__offset_translate = 24000;
        break;
      default:
        break;
      }
      // Fin: Translation @ 'l__dir'

      break;
    default:
      l__num = (int)-1;
      break;
    }

    if (i__flg_trace) {
      Serial.printf("\tMode [0x%02x] Dir [\"%s\"] Num [0x%03x]\n", l__mode, l__t_dir, l__num);
    }

    if (l__offset_translate != (uint16_t)-1 && l__num != (int)-1) {
      // Name of file after translation
      // Abandon du repertoire + MSB:LSB sur 12 bits
      uint16_t l__num_file_in_root = l__num + l__offset_translate;
      char l__file_name[5+1];
      memset(l__file_name, '\0', sizeof(l__file_name));
      sprintf(l__file_name, "%05d", l__num_file_in_root);

      // Commande de diffusion translatee
      byte l__hexa_datas[1+5];      // 1 byte de commande + nom du fichier sur 5 char
      memset(l__hexa_datas, '\0', sizeof(l__hexa_datas));
      l__hexa_datas[0] = WT2003S_SD_PLAY_FILE_IN_ROOT;
      memcpy(&l__hexa_datas[1], l__file_name, 5);
      l__o__size = sizeof(l__hexa_datas);      

      memcpy(o__command, l__hexa_datas, l__o__size);
      // Fin: Commande de diffusion translatee

      if (i__flg_trace) {
        memset(l__buffer, '\0', sizeof(l__buffer));
        for (l__size = 0; l__size < l__o__size; l__size++) {
          if (l__size == 0) {
            // 1st byte de la commande (opcode)
            sprintf(&l__buffer[strlen(l__buffer)], "[0x%02x", *(o__command + l__size));
          }
          else {
            sprintf(&l__buffer[strlen(l__buffer)], " 0x%02x", *(o__command + l__size));            
          }
        }
        strcat(&l__buffer[strlen(l__buffer)], "]");
  
        Serial.printf("\t=> %s [%d bytes] (File [%s])\n", l__buffer, l__o__size, (o__command + 1));
      }
    }
    else {
      Serial.printf("\tError: No translation with %s (Dir [%d])\n", l__buffer, l__dir);      
    }
  }
  else {
    Serial.printf("\tError: Invalid size with %s (%d != 3)\n", l__buffer, i__size);
  }

  return l__o__size;
}

/* Définition et méthode de calcul incrémentale de la checksum d'un buffer identique à 'cksum' de UNIX
   => Warning: Binary content not supported (use the '\0' terminal ;-)
   => Utilisation:
      ...
      // Dans une boucle avec 'l__string' une extraction consecutive du buffer d'entree
      // Calcul sans 'l__size_total' qui ne sera utile qu'au calcul terminal
      uint32_t l__crc_tmp = xxx;    // Valeur a conserver
      l__size_total += strlen(l__strings[n]);
      cksum_inc((uint8_t *)l__string, strlen(l__string), &l__crc_tmp, 0);
      ...
      // Calcul terminal avec 'l__size_total'
      l__crc = cksum_inc(NULL, 0, &l__crc_tmp, l__size_total);
*/
static const uint32_t crctab[256] =
{ 
  0x00000000, 
  0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 
  0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 
  0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 
  0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f, 
  0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 
  0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 
  0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 
  0xa4ad16ea, 0xa06c0b5d, 0xd4326d90, 0xd0f37027, 0xddb056fe, 
  0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95, 
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 
  0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0, 
  0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 
  0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07, 
  0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 
  0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 
  0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba, 
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 
  0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 
  0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 
  0x94ea7b2a, 0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 0xc6bcf05f, 
  0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 
  0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 
  0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 
  0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629, 
  0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 
  0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 
  0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 
  0xeba91bbc, 0xef68060b, 0xd727bbb6, 0xd3e6a601, 0xdea580d8, 
  0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3, 
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 
  0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71, 
  0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 
  0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21, 
  0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 
  0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e, 0x18197087, 
  0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec, 
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 
  0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 
  0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 
  0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 0x89b8fd09, 
  0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 
  0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 
  0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4 
};

uint32_t cksum_inc(uint8_t *i__datas, size_t i__size, uint32_t i__crc_tmp, size_t i__size_cumul)
{
  uint32_t l__crc = 0;

  if (i__datas != NULL && i__size != 0) {
    uint32_t l__crc_tmp = i__crc_tmp;
    size_t bytes_read = i__size;
    uint8_t *l__cp = i__datas;

    // Datas
    while (bytes_read--) {
      l__crc_tmp = (l__crc_tmp << 8) ^ crctab[((l__crc_tmp >> 24) ^ *l__cp++) & 0xFF];
    }

    l__crc = l__crc_tmp;  // Return the intermediate checksum
  }
  else {
    l__crc = i__crc_tmp;

    // Length
    for (size_t length = i__size_cumul; length; length >>= 8) {
      l__crc = (l__crc << 8) ^ crctab[((l__crc >> 24) ^ length) & 0xFF];
    }

    l__crc = ~l__crc & 0xFFFFFFFF;  // Return the effective checksum
  }

  return l__crc;
}
// Fin: Définition et méthode de calcul inrémentale de la checksum d'un buffer identique à 'cksum' de UNIX
