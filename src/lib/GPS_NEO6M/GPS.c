/**
 * @file GPS.c
 * @brief NEO-6M GPS Module Library Implementation
 * @version 1.0
 * @date 2026-05-01
 */

#include "GPS.h"
#include <string.h>

/* ========== NMEA Sentence Types ========== */

#define NMEA_GPGGA  "$GPGGA"
#define NMEA_GPRMC  "$GPRMC"

/* ========== Private Helpers ========== */

/**
 * @brief Parse float from NMEA field (ddmm.mmmm format)
 * @param str   pointer to field string (e.g., "1320.12345")
 * @param hemi  hemisphere ('N','S','E','W')
 * @return decimal degrees
 */
static float _parse_nmea_coord(const char* str, char hemi) {
    float raw;
    int   deg;
    float min;
    float result;

    if (str == NULL || str[0] == '\0') return 0.0f;

    /* convert string to float */
    raw = 0.0f;
    while (*str >= '0' && *str <= '9') {
        raw = raw * 10.0f + (float)(*str - '0');
        str++;
    }
    if (*str == '.') {
        float frac = 0.1f;
        str++;
        while (*str >= '0' && *str <= '9') {
            raw += (float)(*str - '0') * frac;
            frac *= 0.1f;
            str++;
        }
    }

    /* extract degrees and minutes */
    if (hemi == 'N' || hemi == 'S') {
        /* latitude: first 2 digits = degrees */
        deg = (int)(raw / 100.0f);
        min = raw - (float)(deg * 100);
        result = (float)deg + min / 60.0f;
        if (hemi == 'S') result = -result;
    } else {
        /* longitude: first 3 digits = degrees */
        deg = (int)(raw / 100.0f);
        min = raw - (float)(deg * 100);
        result = (float)deg + min / 60.0f;
        if (hemi == 'W') result = -result;
    }

    return result;
}

/**
 * @brief Parse integer from NMEA field
 */
static int32_t _parse_int(const char* str) {
    int32_t val = 0;
    if (str == NULL || *str == '\0') return 0;

    while (*str >= '0' && *str <= '9') {
        val = val * 10 + (int32_t)(*str - '0');
        str++;
    }
    return val;
}

/**
 * @brief Parse float from NMEA field
 */
static float _parse_float(const char* str) {
    float val = 0.0f;
    int neg = 0;

    if (str == NULL || *str == '\0') return 0.0f;
    if (*str == '-') { neg = 1; str++; }

    while (*str >= '0' && *str <= '9') {
        val = val * 10.0f + (float)(*str - '0');
        str++;
    }
    if (*str == '.') {
        float frac = 0.1f;
        str++;
        while (*str >= '0' && *str <= '9') {
            val += (float)(*str - '0') * frac;
            frac *= 0.1f;
            str++;
        }
    }

    return neg ? -val : val;
}

/**
 * @brief Split NMEA sentence by commas into fields
 * @param sentence full NMEA sentence
 * @param fields   output array of field pointers
 * @param max_fields max number of fields
 * @return number of fields found
 */
static uint8_t _split_nmea(char* sentence, char** fields, uint8_t max_fields) {
    uint8_t count = 0;
    char* p = sentence;

    fields[count++] = p;

    while (*p && count < max_fields) {
        if (*p == ',') {
            *p = '\0';           /* null-terminate current field */
            fields[count++] = p + 1;
        }
        p++;
    }

    /* null-terminate last field at * (checksum) or end */
    p = fields[count - 1];
    while (*p && *p != '*') p++;
    *p = '\0';

    return count;
}

/**
 * @brief Parse $GPGGA sentence
 */
static void _parse_gpgga(GPS_Instance* gps, char* sentence) {
    /* $GPGGA,time,lat,N,lon,E,quality,sats,hdop,alt,M,geoid,M,,*cs */
    char* fields[15];
    uint8_t n = _split_nmea(sentence, fields, 15);

    if (n < 10) return;

    /* fix quality */
    gps->fix_quality = (GPS_FixQuality)_parse_int(fields[6]);

    /* satellites */
    gps->satellites = (uint8_t)_parse_int(fields[7]);

    /* latitude */
    gps->latitude = _parse_nmea_coord(fields[2], fields[3][0]);

    /* longitude */
    gps->longitude = _parse_nmea_coord(fields[4], fields[5][0]);

    /* altitude */
    gps->altitude = _parse_float(fields[9]);

    /* fix valid */
    gps->fix_valid = (gps->fix_quality > GPS_FIX_NONE && gps->satellites >= 3) ? 1 : 0;

    /* time from GPGGA */
    if (n > 1 && fields[1][0] != '\0') {
        char time_str[7];
        uint32_t time_val;
        strncpy(time_str, fields[1], 6);
        time_str[6] = '\0';
        time_val = (uint32_t)_parse_int(time_str);
        gps->datetime.hour   = (uint8_t)(time_val / 10000);
        gps->datetime.minute = (uint8_t)((time_val / 100) % 100);
        gps->datetime.second = (uint8_t)(time_val % 100);
    }
}

/**
 * @brief Parse $GPRMC sentence
 */
static void _parse_gprmc(GPS_Instance* gps, char* sentence) {
    /* $GPRMC,time,status,lat,N,lon,E,speed,angle,date,,,*cs */
    char* fields[13];
    uint8_t n = _split_nmea(sentence, fields, 13);

    if (n < 10) return;

    /* status: A=active, V=void */
    if (fields[2][0] == 'A') {
        gps->fix_valid = 1;
    }

    /* time */
    if (fields[1][0] != '\0') {
        char time_str[7];
        uint32_t time_val;
        strncpy(time_str, fields[1], 6);
        time_str[6] = '\0';
        time_val = (uint32_t)_parse_int(time_str);
        gps->datetime.hour   = (uint8_t)(time_val / 10000);
        gps->datetime.minute = (uint8_t)((time_val / 100) % 100);
        gps->datetime.second = (uint8_t)(time_val % 100);
    }

    /* latitude */
    gps->latitude = _parse_nmea_coord(fields[3], fields[4][0]);

    /* longitude */
    gps->longitude = _parse_nmea_coord(fields[5], fields[6][0]);

    /* speed over ground (knots → km/h) */
    gps->speed_kmh = _parse_float(fields[7]) * 1.852f;

    /* date: ddmmyy */
    if (fields[9][0] != '\0') {
        char date_str[7];
        uint32_t date_val;
        strncpy(date_str, fields[9], 6);
        date_str[6] = '\0';
        date_val = (uint32_t)_parse_int(date_str);
        gps->datetime.day   = (uint8_t)(date_val / 10000);
        gps->datetime.month = (uint8_t)((date_val / 100) % 100);
        gps->datetime.year  = (uint8_t)(date_val % 100);
    }
}

/**
 * @brief Process one complete NMEA sentence
 */
static void _process_sentence(GPS_Instance* gps, char* sentence) {
    if (strncmp(sentence, NMEA_GPGGA, 6) == 0) {
        _parse_gpgga(gps, sentence);
        gps->data_ready = 1;
    } else if (strncmp(sentence, NMEA_GPRMC, 6) == 0) {
        _parse_gprmc(gps, sentence);
        gps->data_ready = 1;
    }
    /* ignore other NMEA types ($GPVTG, $GPGSA, $GPGSV, etc.) */
}

/* ========== Public Functions ========== */

GPS_Status GPS_Init(GPS_Instance* gps, USART_BaudRate baud_rate, USART_PinConfig pin_config) {
    if (gps == NULL) return GPS_ERROR_PARAM;

    gps->baud_rate   = baud_rate;
    gps->pin_config  = pin_config;
    gps->rx_idx      = 0;
    gps->data_ready  = 0;
    gps->fix_valid   = 0;
    gps->latitude    = 0.0f;
    gps->longitude   = 0.0f;
    gps->altitude    = 0.0f;
    gps->speed_kmh   = 0.0f;
    gps->satellites  = 0;
    gps->fix_quality = GPS_FIX_NONE;

    memset(&gps->datetime, 0, sizeof(GPS_DateTime));

    /* Initialize USART1 for GPS reception */
    USART_SimpleInit(baud_rate, pin_config);

    gps->initialized = 1;
    return GPS_OK;
}

GPS_Status GPS_Update(GPS_Instance* gps) {
    char c;

    if (gps == NULL || !gps->initialized) return GPS_ERROR_NOT_INIT;

    /* read all available characters from UART */
    while (USART_Available() > 0) {
        c = (char)USART_Read();

        /* buffer overflow protection */
        if (gps->rx_idx >= GPS_BUFFER_SIZE - 1) {
            gps->rx_idx = 0;  /* reset buffer */
        }

        /* NMEA sentence ends with \n (or \r\n) */
        if (c == '\n') {
            gps->rx_buf[gps->rx_idx] = '\0';

            /* process if starts with $ and has content */
            if (gps->rx_idx > 0 && gps->rx_buf[0] == '$') {
                _process_sentence(gps, gps->rx_buf);
            }

            gps->rx_idx = 0;  /* reset for next sentence */
        } else if (c == '\r') {
            /* skip carriage return */
        } else {
            /* add to buffer */
            if (gps->rx_idx == 0 && c != '$') {
                /* ignore characters before $ (partial/junk) */
                continue;
            }
            gps->rx_buf[gps->rx_idx++] = c;
        }
    }

    return GPS_OK;
}

uint8_t GPS_IsFixValid(GPS_Instance* gps) {
    if (gps == NULL || !gps->initialized) return 0;
    return gps->fix_valid;
}

float GPS_GetLatitude(GPS_Instance* gps) {
    if (gps == NULL || !gps->initialized) return 0.0f;
    return gps->latitude;
}

float GPS_GetLongitude(GPS_Instance* gps) {
    if (gps == NULL || !gps->initialized) return 0.0f;
    return gps->longitude;
}

float GPS_GetAltitude(GPS_Instance* gps) {
    if (gps == NULL || !gps->initialized) return 0.0f;
    return gps->altitude;
}

float GPS_GetSpeed(GPS_Instance* gps) {
    if (gps == NULL || !gps->initialized) return 0.0f;
    return gps->speed_kmh;
}

uint8_t GPS_GetSatellites(GPS_Instance* gps) {
    if (gps == NULL || !gps->initialized) return 0;
    return gps->satellites;
}

void GPS_GetDateTime(GPS_Instance* gps, GPS_DateTime* dt) {
    if (gps == NULL || !gps->initialized || dt == NULL) return;
    *dt = gps->datetime;
}
