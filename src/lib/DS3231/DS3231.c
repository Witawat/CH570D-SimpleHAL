/**
 * @file DS3231.c
 * @brief DS3231 Real-Time Clock Library Implementation
 * @version 1.0
 * @date 2026-04-29
 */

#include "DS3231.h"

/* ========== Private Functions: BCD Conversion ========== */

/** @brief แปลง Binary → BCD */
static uint8_t _to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}

/** @brief แปลง BCD → Binary */
static uint8_t _from_bcd(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* ========== Private Functions: Register Read/Write ========== */

static DS3231_Status _write_reg(uint8_t reg, uint8_t value) {
    I2C_Status st = I2C_WriteReg(DS3231_I2C_ADDR, reg, value);
    return (st == I2C_OK) ? DS3231_OK : DS3231_ERROR_I2C;
}

static DS3231_Status _read_reg(uint8_t reg, uint8_t* value) {
    I2C_Status st = I2C_ReadRegMulti(DS3231_I2C_ADDR, reg, value, 1);
    return (st == I2C_OK) ? DS3231_OK : DS3231_ERROR_I2C;
}

static DS3231_Status _read_regs(uint8_t reg, uint8_t* buf, uint8_t len) {
    uint8_t addr[1] = { reg };
    I2C_Status st = I2C_Write(DS3231_I2C_ADDR, addr, 1);
    if (st != I2C_OK) return DS3231_ERROR_I2C;
    st = I2C_Read(DS3231_I2C_ADDR, buf, len);
    return (st == I2C_OK) ? DS3231_OK : DS3231_ERROR_I2C;
}

/* ========== Public Functions ========== */

DS3231_Status DS3231_Init(DS3231_Instance* rtc) {
    if (rtc == NULL) return DS3231_ERROR_PARAM;

    rtc->hour_mode  = DS3231_24H;
    rtc->initialized = 0;

    /* ตรวจสอบการเชื่อมต่อ */
    uint8_t dummy;
    DS3231_Status st = _read_reg(DS3231_REG_SECONDS, &dummy);
    if (st != DS3231_OK) return DS3231_ERROR_I2C;

    /* ล้าง OSF flag ใน status register (บิต 7) */
    uint8_t status;
    st = _read_reg(DS3231_REG_STATUS, &status);
    if (st != DS3231_OK) return st;
    status &= ~(1 << 7);  /* ล้าง OSF */
    st = _write_reg(DS3231_REG_STATUS, status);
    if (st != DS3231_OK) return st;

    rtc->initialized = 1;
    return DS3231_OK;
}

DS3231_Status DS3231_SetDateTime(DS3231_Instance* rtc,
                                  uint16_t year, uint8_t month, uint8_t day,
                                  uint8_t hour, uint8_t min, uint8_t sec,
                                  DS3231_DayOfWeek dow) {
    if (rtc == NULL || !rtc->initialized) return DS3231_ERROR_PARAM;

    /* Clamp values */
    if (year  < 2000) year  = 2000;
    if (year  > 2099) year  = 2099;
    if (month < 1)    month = 1;
    if (month > 12)   month = 12;
    if (day   < 1)    day   = 1;
    if (day   > 31)   day   = 31;
    if (hour  > 23)   hour  = 23;
    if (min   > 59)   min   = 59;
    if (sec   > 59)   sec   = 59;

    uint8_t year_bcd  = _to_bcd((uint8_t)(year - 2000));
    uint8_t month_bcd = _to_bcd(month);
    uint8_t day_bcd   = _to_bcd(day);
    uint8_t hour_bcd  = _to_bcd(hour); /* bit 6 = 0 สำหรับ 24H */
    uint8_t min_bcd   = _to_bcd(min);
    uint8_t sec_bcd   = _to_bcd(sec);

    /* Century bit: ตั้งบิต 7 ของ month ถ้า year >= 2100 (ไม่จำเป็นสำหรับ 2000-2099) */

    DS3231_Status st;
    st = _write_reg(DS3231_REG_SECONDS, sec_bcd);   if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_MINUTES, min_bcd);   if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_HOURS,   hour_bcd);  if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_DAY,     (uint8_t)dow); if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_DATE,    day_bcd);   if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_MONTH,   month_bcd); if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_YEAR,    year_bcd);  if (st != DS3231_OK) return st;

    return DS3231_OK;
}

DS3231_Status DS3231_GetDateTime(DS3231_Instance* rtc, DS3231_DateTime* dt) {
    if (rtc == NULL || !rtc->initialized || dt == NULL) return DS3231_ERROR_PARAM;

    uint8_t buf[7];
    DS3231_Status st = _read_regs(DS3231_REG_SECONDS, buf, 7);
    if (st != DS3231_OK) return st;

    dt->second      = _from_bcd(buf[0] & 0x7F);
    dt->minute      = _from_bcd(buf[1] & 0x7F);

    /* ตรวจสอบ 12H/24H mode (bit 6 ของ HOURS register) */
    if (buf[2] & (1 << 6)) {
        /* 12H mode */
        dt->is_pm = (buf[2] >> 5) & 0x01;
        dt->hour  = _from_bcd(buf[2] & 0x1F);
        rtc->hour_mode = DS3231_12H;
    } else {
        /* 24H mode */
        dt->is_pm = 0;
        dt->hour  = _from_bcd(buf[2] & 0x3F);
        rtc->hour_mode = DS3231_24H;
    }

    dt->day_of_week = (DS3231_DayOfWeek)(buf[3] & 0x07);
    dt->day         = _from_bcd(buf[4] & 0x3F);
    dt->month       = _from_bcd(buf[5] & 0x1F);
    dt->year        = 2000 + _from_bcd(buf[6]);

    return DS3231_OK;
}

DS3231_Status DS3231_GetTemperature(DS3231_Instance* rtc, float* temp) {
    if (rtc == NULL || !rtc->initialized || temp == NULL) return DS3231_ERROR_PARAM;

    uint8_t msb, lsb;
    DS3231_Status st;

    st = _read_reg(DS3231_REG_TEMP_MSB, &msb);
    if (st != DS3231_OK) return st;
    st = _read_reg(DS3231_REG_TEMP_LSB, &lsb);
    if (st != DS3231_OK) return st;

    /* MSB: integer part (signed), LSB: bits 7-6 = fractional (0.25°C resolution) */
    int8_t  int_part  = (int8_t)msb;
    uint8_t frac_part = (lsb >> 6) & 0x03; /* 0, 1, 2, 3 → 0, 0.25, 0.5, 0.75 */

    *temp = (float)int_part + (float)frac_part * 0.25f;
    return DS3231_OK;
}

DS3231_Status DS3231_SetAlarm1(DS3231_Instance* rtc, DS3231_Alarm1Mode mode,
                                uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (rtc == NULL || !rtc->initialized) return DS3231_ERROR_PARAM;

    /* A1Mx bits: MSB ของแต่ละ register */
    uint8_t sec_reg  = _to_bcd(sec)  | (((mode >> 0) & 1) << 7);
    uint8_t min_reg  = _to_bcd(min)  | (((mode >> 1) & 1) << 7);
    uint8_t hr_reg   = _to_bcd(hour) | (((mode >> 2) & 1) << 7);
    uint8_t day_reg  = _to_bcd(day)  | (((mode >> 3) & 1) << 7);

    DS3231_Status st;
    st = _write_reg(DS3231_REG_A1_SEC, sec_reg);  if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_A1_MIN, min_reg);  if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_A1_HR,  hr_reg);   if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_A1_DAY, day_reg);  if (st != DS3231_OK) return st;

    return DS3231_OK;
}

DS3231_Status DS3231_SetAlarm2(DS3231_Instance* rtc, DS3231_Alarm2Mode mode,
                                uint8_t day, uint8_t hour, uint8_t min) {
    if (rtc == NULL || !rtc->initialized) return DS3231_ERROR_PARAM;

    uint8_t min_reg = _to_bcd(min)  | (((mode >> 0) & 1) << 7);
    uint8_t hr_reg  = _to_bcd(hour) | (((mode >> 1) & 1) << 7);
    uint8_t day_reg = _to_bcd(day)  | (((mode >> 2) & 1) << 7);

    DS3231_Status st;
    st = _write_reg(DS3231_REG_A2_MIN, min_reg);  if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_A2_HR,  hr_reg);   if (st != DS3231_OK) return st;
    st = _write_reg(DS3231_REG_A2_DAY, day_reg);  if (st != DS3231_OK) return st;

    return DS3231_OK;
}

DS3231_Status DS3231_EnableAlarm1(DS3231_Instance* rtc, uint8_t enable) {
    if (rtc == NULL || !rtc->initialized) return DS3231_ERROR_PARAM;
    uint8_t ctrl;
    DS3231_Status st = _read_reg(DS3231_REG_CONTROL, &ctrl);
    if (st != DS3231_OK) return st;

    if (enable) {
        ctrl |=  (1 << 0);  /* A1IE bit */
        ctrl |=  (1 << 2);  /* INTCN bit — ใช้ alarm interrupt แทน square wave */
    } else {
        ctrl &= ~(1 << 0);
    }
    return _write_reg(DS3231_REG_CONTROL, ctrl);
}

DS3231_Status DS3231_EnableAlarm2(DS3231_Instance* rtc, uint8_t enable) {
    if (rtc == NULL || !rtc->initialized) return DS3231_ERROR_PARAM;
    uint8_t ctrl;
    DS3231_Status st = _read_reg(DS3231_REG_CONTROL, &ctrl);
    if (st != DS3231_OK) return st;

    if (enable) {
        ctrl |=  (1 << 1);  /* A2IE bit */
        ctrl |=  (1 << 2);  /* INTCN */
    } else {
        ctrl &= ~(1 << 1);
    }
    return _write_reg(DS3231_REG_CONTROL, ctrl);
}

uint8_t DS3231_IsAlarm1Fired(DS3231_Instance* rtc) {
    if (rtc == NULL || !rtc->initialized) return 0;
    uint8_t status;
    if (_read_reg(DS3231_REG_STATUS, &status) != DS3231_OK) return 0;
    return (status >> 0) & 0x01; /* A1F bit */
}

uint8_t DS3231_IsAlarm2Fired(DS3231_Instance* rtc) {
    if (rtc == NULL || !rtc->initialized) return 0;
    uint8_t status;
    if (_read_reg(DS3231_REG_STATUS, &status) != DS3231_OK) return 0;
    return (status >> 1) & 0x01; /* A2F bit */
}

DS3231_Status DS3231_ClearAlarmFlag(DS3231_Instance* rtc, uint8_t alarm_no) {
    if (rtc == NULL || !rtc->initialized) return DS3231_ERROR_PARAM;
    uint8_t status;
    DS3231_Status st = _read_reg(DS3231_REG_STATUS, &status);
    if (st != DS3231_OK) return st;

    if (alarm_no == 1) status &= ~(1 << 0); /* ล้าง A1F */
    if (alarm_no == 2) status &= ~(1 << 1); /* ล้าง A2F */

    return _write_reg(DS3231_REG_STATUS, status);
}

const char* DS3231_DayOfWeekStr(DS3231_DayOfWeek dow) {
    switch (dow) {
        case DS3231_SUNDAY:    return "Sunday";
        case DS3231_MONDAY:    return "Monday";
        case DS3231_TUESDAY:   return "Tuesday";
        case DS3231_WEDNESDAY: return "Wednesday";
        case DS3231_THURSDAY:  return "Thursday";
        case DS3231_FRIDAY:    return "Friday";
        case DS3231_SATURDAY:  return "Saturday";
        default:               return "Unknown";
    }
}

const char* DS3231_DayOfWeekStrTH(DS3231_DayOfWeek dow) {
    switch (dow) {
        case DS3231_SUNDAY:    return "อาทิตย์";
        case DS3231_MONDAY:    return "จันทร์";
        case DS3231_TUESDAY:   return "อังคาร";
        case DS3231_WEDNESDAY: return "พุธ";
        case DS3231_THURSDAY:  return "พฤหัสบดี";
        case DS3231_FRIDAY:    return "ศุกร์";
        case DS3231_SATURDAY:  return "เสาร์";
        default:               return "ไม่ทราบ";
    }
}
