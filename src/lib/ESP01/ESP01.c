/**
 * @file ESP01.c
 * @brief ESP-01 / ESP-01S Wi-Fi AT Library Implementation
 */

#include "ESP01.h"
#include <stdio.h>
#include <string.h>

/* ========== Private ========== */

static void _uart_send_str(const char* s) {
    while (*s) {
        USART_WriteByte((uint8_t)*s++);
    }
}

static void _uart_flush_rx(void) {
    USART_Flush();
}

static ESP01_Status _read_line(char* line, uint16_t max_len, uint32_t timeout_ms) {
    uint16_t idx = 0;
    uint32_t start = Get_CurrentMs();

    if (line == NULL || max_len < 2) {
        return ESP01_ERROR_PARAM;
    }

    while ((Get_CurrentMs() - start) <= timeout_ms) {
        if (!USART_Available()) {
            continue;
        }

        uint8_t c = USART_Read();
        if (c == '\r') {
            continue;
        }
        if (c == '\n') {
            if (idx == 0) {
                continue;
            }
            line[idx] = '\0';
            return ESP01_OK;
        }

        if (idx < (uint16_t)(max_len - 1)) {
            line[idx++] = (char)c;
        }
    }

    line[idx] = '\0';
    return ESP01_ERROR_TIMEOUT;
}

static ESP01_Status _wait_for_ok_or_error(char* capture, uint16_t capture_len, uint32_t timeout_ms) {
    /* static: ประหยัด stack 192B สำคัญสำหรับ RAM จำกัด */
    static char line[ESP01_RX_LINE_MAX];
    uint32_t start = Get_CurrentMs();

    if (capture != NULL && capture_len > 0) {
        capture[0] = '\0';
    }

    while ((Get_CurrentMs() - start) <= timeout_ms) {
        ESP01_Status st = _read_line(line, sizeof(line), 150);
        if (st == ESP01_ERROR_TIMEOUT) {
            continue;
        }
        if (st != ESP01_OK) {
            return st;
        }

        if (capture != NULL && capture_len > 0) {
            size_t used = strlen(capture);
            size_t left = (used < capture_len) ? (capture_len - used - 1U) : 0U;
            if (left > 0) {
                strncat(capture, line, left);
                used = strlen(capture);
                left = (used < capture_len) ? (capture_len - used - 1U) : 0U;
                if (left > 0) {
                    strncat(capture, "\n", left);
                }
            }
        }

        if (strcmp(line, "OK") == 0 || strstr(line, "WIFI GOT IP") != NULL) {
            return ESP01_OK;
        }
        if (strcmp(line, "ERROR") == 0 || strstr(line, "FAIL") != NULL || strstr(line, "busy") != NULL) {
            return ESP01_ERROR_RESPONSE;
        }
    }

    return ESP01_ERROR_TIMEOUT;
}

static ESP01_Status _exec_cmd(ESP01_Instance* esp,
                              const char* cmd,
                              char* capture,
                              uint16_t capture_len,
                              uint32_t timeout_ms) {
    uint8_t attempt;

    if (esp == NULL || !esp->initialized || cmd == NULL) {
        return ESP01_ERROR_PARAM;
    }

    for (attempt = 0; attempt <= esp->retry; ++attempt) {
        _uart_flush_rx();
        _uart_send_str(cmd);
        _uart_send_str("\r\n");

        ESP01_Status st = _wait_for_ok_or_error(capture, capture_len, timeout_ms);
        if (st == ESP01_OK) {
            return ESP01_OK;
        }
        if (st == ESP01_ERROR_RESPONSE && attempt < esp->retry) {
            Delay_Ms(80);
            continue;
        }
        if (st == ESP01_ERROR_TIMEOUT && attempt < esp->retry) {
            Delay_Ms(120);
            continue;
        }
        return st;
    }

    return ESP01_ERROR_TIMEOUT;
}

/* ========== Public ========== */

ESP01_Status ESP01_Init(ESP01_Instance* esp, uint32_t baudrate, USART_PinConfig pin_config) {
    if (esp == NULL) {
        return ESP01_ERROR_PARAM;
    }

    esp->baudrate = baudrate;
    esp->pin_config = pin_config;
    esp->retry = ESP01_DEFAULT_RETRY;
    esp->initialized = 0;

    USART_SimpleInit((USART_BaudRate)baudrate, pin_config);
    Delay_Ms(100);

    esp->initialized = 1;

    if (ESP01_TestAT(esp) != ESP01_OK) {
        return ESP01_ERROR_NOT_READY;
    }

    (void)ESP01_SetEcho(esp, 0);
    return ESP01_OK;
}

ESP01_Status ESP01_TestAT(ESP01_Instance* esp) {
    return _exec_cmd(esp, "AT", NULL, 0, 800);
}

ESP01_Status ESP01_Reset(ESP01_Instance* esp, uint32_t timeout_ms) {
    ESP01_Status st = _exec_cmd(esp, "AT+RST", NULL, 0, timeout_ms);
    if (st != ESP01_OK) {
        return st;
    }
    /* ESP-01 ต้องการเวลา boot ~1.5-2 วินาที */
    Delay_Ms(2000);
    /* ลอง TestAT ซ้ำหลายครั้งจนกว่า เมื่อ boot เสร็จ */
    for (uint8_t i = 0; i < 5; ++i) {
        if (ESP01_TestAT(esp) == ESP01_OK) return ESP01_OK;
        Delay_Ms(200);
    }
    return ESP01_ERROR_NOT_READY;
}

ESP01_Status ESP01_SetEcho(ESP01_Instance* esp, uint8_t enable) {
    return _exec_cmd(esp, enable ? "ATE1" : "ATE0", NULL, 0, 1000);
}

ESP01_Status ESP01_GetVersion(ESP01_Instance* esp, char* out, uint16_t out_len, uint32_t timeout_ms) {
    if (out == NULL || out_len == 0) {
        return ESP01_ERROR_PARAM;
    }
    return _exec_cmd(esp, "AT+GMR", out, out_len, timeout_ms);
}

ESP01_Status ESP01_SetMode(ESP01_Instance* esp, ESP01_WiFiMode mode) {
    char cmd[16];

    if (mode < ESP01_MODE_STA || mode > ESP01_MODE_STA_AP) {
        return ESP01_ERROR_PARAM;
    }

    snprintf(cmd, sizeof(cmd), "AT+CWMODE=%u", (unsigned)mode);
    return _exec_cmd(esp, cmd, NULL, 0, ESP01_DEFAULT_TIMEOUT_MS);
}

ESP01_Status ESP01_JoinAP(ESP01_Instance* esp, const char* ssid, const char* pass, uint32_t timeout_ms) {
    char cmd[160];

    if (ssid == NULL || pass == NULL) {
        return ESP01_ERROR_PARAM;
    }

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
    return _exec_cmd(esp, cmd, NULL, 0, timeout_ms);
}

ESP01_Status ESP01_IsConnected(ESP01_Instance* esp, uint8_t* connected) {
    char cap[96];
    ESP01_Status st;

    if (connected == NULL) {
        return ESP01_ERROR_PARAM;
    }

    st = _exec_cmd(esp, "AT+CWJAP?", cap, sizeof(cap), 1500);
    if (st != ESP01_OK) {
        *connected = 0;
        return st;
    }

    *connected = (strstr(cap, "+CWJAP:") != NULL) ? 1U : 0U;
    return ESP01_OK;
}

ESP01_Status ESP01_GetIP(ESP01_Instance* esp, char* ip_buf, uint16_t ip_buf_len, uint32_t timeout_ms) {
    char cap[ESP01_RX_LINE_MAX];
    char* first_quote;
    char* second_quote;

    if (ip_buf == NULL || ip_buf_len == 0) {
        return ESP01_ERROR_PARAM;
    }

    ESP01_Status st = _exec_cmd(esp, "AT+CIFSR", cap, sizeof(cap), timeout_ms);
    if (st != ESP01_OK) {
        return st;
    }

    first_quote = strchr(cap, '\"');
    if (first_quote == NULL) {
        return ESP01_ERROR_RESPONSE;
    }
    second_quote = strchr(first_quote + 1, '\"');
    if (second_quote == NULL || second_quote <= first_quote + 1) {
        return ESP01_ERROR_RESPONSE;
    }

    size_t n = (size_t)(second_quote - first_quote - 1);
    if (n >= ip_buf_len) {
        n = ip_buf_len - 1U;
    }

    memcpy(ip_buf, first_quote + 1, n);
    ip_buf[n] = '\0';
    return ESP01_OK;
}

ESP01_Status ESP01_TCPConnect(ESP01_Instance* esp, const char* host, uint16_t port, uint32_t timeout_ms) {
    char cmd[96];

    if (host == NULL) {
        return ESP01_ERROR_PARAM;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u", host, (unsigned)port);
    return _exec_cmd(esp, cmd, NULL, 0, timeout_ms);
}

ESP01_Status ESP01_TCPSend(ESP01_Instance* esp, const uint8_t* data, uint16_t len, uint32_t timeout_ms) {
    char cmd[24];
    char line[ESP01_RX_LINE_MAX];
    uint32_t start;
    uint16_t i;

    if (esp == NULL || !esp->initialized || data == NULL || len == 0) {
        return ESP01_ERROR_PARAM;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", (unsigned)len);

    _uart_flush_rx();
    _uart_send_str(cmd);
    _uart_send_str("\r\n");

    /* ต้องได้รับ '>' prompt ก่อนจึงส่งข้อมูล ถ้าไม่ได้ ห้ามส่ง */
    uint8_t prompt_seen = 0;
    start = Get_CurrentMs();
    while ((Get_CurrentMs() - start) <= timeout_ms) {
        ESP01_Status st = _read_line(line, sizeof(line), 120);
        if (st == ESP01_ERROR_TIMEOUT) {
            continue;
        }
        if (strstr(line, ">") != NULL || strstr(line, "OK") != NULL) {
            prompt_seen = 1;
            break;
        }
        if (strstr(line, "ERROR") != NULL || strstr(line, "FAIL") != NULL) {
            return ESP01_ERROR_RESPONSE;
        }
    }
    if (!prompt_seen) {
        return ESP01_ERROR_TIMEOUT;
    }

    for (i = 0; i < len; ++i) {
        USART_WriteByte(data[i]);
    }

    return _wait_for_ok_or_error(NULL, 0, timeout_ms);
}

ESP01_Status ESP01_TCPClose(ESP01_Instance* esp, uint32_t timeout_ms) {
    return _exec_cmd(esp, "AT+CIPCLOSE", NULL, 0, timeout_ms);
}

ESP01_Status ESP01_HTTPGet(ESP01_Instance* esp,
                           const char* host,
                           const char* path,
                           char* response,
                           uint16_t response_len,
                           uint32_t timeout_ms) {
    char req[256];
    ESP01_Status st;

    if (host == NULL || path == NULL || response == NULL || response_len == 0) {
        return ESP01_ERROR_PARAM;
    }

    st = ESP01_TCPConnect(esp, host, 80, timeout_ms);
    if (st != ESP01_OK) {
        return st;
    }

    snprintf(req, sizeof(req),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n\r\n",
             path, host);

    st = ESP01_TCPSend(esp, (const uint8_t*)req, (uint16_t)strlen(req), timeout_ms);
    if (st != ESP01_OK) {
        (void)ESP01_TCPClose(esp, 1000);
        return st;
    }

    st = _wait_for_ok_or_error(response, response_len, timeout_ms);
    (void)ESP01_TCPClose(esp, 1000);
    return st;
}

ESP01_Status ESP01_HTTPPost(ESP01_Instance* esp,
                            const char* host,
                            const char* path,
                            const char* content_type,
                            const char* body,
                            char* response,
                            uint16_t response_len,
                            uint32_t timeout_ms) {
    char req[384];
    ESP01_Status st;
    uint16_t body_len;

    if (host == NULL || path == NULL || body == NULL ||
        response == NULL || response_len == 0) {
        return ESP01_ERROR_PARAM;
    }

    if (content_type == NULL) {
        content_type = "application/json";
    }

    body_len = (uint16_t)strlen(body);

    /* สร้าง HTTP header (body ไม่ถูก embed ใน req เพื่อประหยัด RAM) */
    size_t hdr_len = (size_t)snprintf(req, sizeof(req),
                         "POST %s HTTP/1.1\r\n"
                         "Host: %s\r\n"
                         "Content-Type: %s\r\n"
                         "Content-Length: %u\r\n"
                         "Connection: close\r\n\r\n",
                         path, host, content_type, (unsigned)body_len);
    if (hdr_len >= sizeof(req)) {
        return ESP01_ERROR_OVERFLOW;
    }

    uint16_t total_len = (uint16_t)(hdr_len + body_len);

    st = ESP01_TCPConnect(esp, host, 80, timeout_ms);
    if (st != ESP01_OK) {
        return st;
    }

    /* ===== ส่ง AT+CIPSEND ครั้งเดียวด้วย total_len (header + body) ===== */
    {
        static char send_cmd[24];
        static char prompt_line[ESP01_RX_LINE_MAX];
        uint32_t t_start;

        snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%u", (unsigned)total_len);
        _uart_flush_rx();
        _uart_send_str(send_cmd);
        _uart_send_str("\r\n");

        uint8_t prompt_seen = 0;
        t_start = Get_CurrentMs();
        while ((Get_CurrentMs() - t_start) <= timeout_ms) {
            if (_read_line(prompt_line, sizeof(prompt_line), 120) == ESP01_OK) {
                if (strstr(prompt_line, ">") != NULL) { prompt_seen = 1; break; }
                if (strstr(prompt_line, "ERROR") != NULL) {
                    (void)ESP01_TCPClose(esp, 1000);
                    return ESP01_ERROR_RESPONSE;
                }
            }
        }
        if (!prompt_seen) {
            (void)ESP01_TCPClose(esp, 1000);
            return ESP01_ERROR_TIMEOUT;
        }

        /* ส่ง header ก่อน */
        for (size_t j = 0; j < hdr_len; ++j) {
            USART_WriteByte((uint8_t)req[j]);
        }
        /* ส่ง body ตาม */
        for (uint16_t j = 0; j < body_len; ++j) {
            USART_WriteByte((uint8_t)body[j]);
        }
    }

    st = _wait_for_ok_or_error(response, response_len, timeout_ms);
    (void)ESP01_TCPClose(esp, 1000);
    return st;
}

ESP01_Status ESP01_SetSleepMode(ESP01_Instance* esp, ESP01_SleepMode mode) {
    char cmd[20];

    if (mode > ESP01_SLEEP_MODEM) {
        return ESP01_ERROR_PARAM;
    }

    snprintf(cmd, sizeof(cmd), "AT+SLEEP=%u", (unsigned)mode);
    return _exec_cmd(esp, cmd, NULL, 0, 1200);
}

ESP01_Status ESP01_SetTxPowerQuarterDbm(ESP01_Instance* esp, uint8_t quarter_dbm) {
    char cmd[24];

    snprintf(cmd, sizeof(cmd), "AT+RFPOWER=%u", (unsigned)quarter_dbm);
    return _exec_cmd(esp, cmd, NULL, 0, 1200);
}

ESP01_Status ESP01_EnterDeepSleepUs(ESP01_Instance* esp, uint32_t sleep_us) {
    char cmd[32];

    if (sleep_us == 0) {
        return ESP01_ERROR_PARAM;
    }

    snprintf(cmd, sizeof(cmd), "AT+GSLP=%lu", (unsigned long)sleep_us);
    return _exec_cmd(esp, cmd, NULL, 0, 1000);
}

/* ========== TCP Receive ========== */

uint16_t ESP01_TCPReadAvailable(ESP01_Instance* esp) {
    if (esp == NULL || !esp->initialized) return 0;
    return USART_Available() ? 1U : 0U;
}

uint16_t ESP01_TCPRead(ESP01_Instance* esp,
                       uint8_t* buf,
                       uint16_t buf_len,
                       uint32_t timeout_ms) {
    /* รอรับ "+IPD,<len>:<data>" จาก ESP-01 */
    static char hdr[24];  /* "+IPD,XXXX:" */
    uint16_t i;
    uint32_t start;
    uint16_t ipd_len = 0;

    if (esp == NULL || !esp->initialized || buf == NULL || buf_len == 0) return 0;

    /* รอรับ header line "+IPD,..." */
    start = Get_CurrentMs();
    i = 0;
    while ((Get_CurrentMs() - start) <= timeout_ms && i < sizeof(hdr) - 1U) {
        if (!USART_Available()) continue;
        char c = (char)USART_Read();
        hdr[i++] = c;
        if (c == ':') break;
    }
    hdr[i] = '\0';

    /* parse "+IPD,<len>:" */
    char* p = strstr(hdr, "+IPD,");
    if (p == NULL) return 0;
    p += 5;
    while (*p >= '0' && *p <= '9') {
        ipd_len = (uint16_t)(ipd_len * 10U + (uint16_t)(*p - '0'));
        p++;
    }
    if (ipd_len == 0) return 0;

    /* จำกัดไม่ให้เกิน buf_len */
    if (ipd_len > buf_len) ipd_len = buf_len;

    /* อ่าน payload bytes */
    uint16_t received = 0;
    start = Get_CurrentMs();
    while (received < ipd_len && (Get_CurrentMs() - start) <= timeout_ms) {
        if (!USART_Available()) continue;
        buf[received++] = USART_Read();
    }
    return received;
}

/* ========== Wi-Fi Extras ========== */

ESP01_Status ESP01_DisconnectAP(ESP01_Instance* esp) {
    return _exec_cmd(esp, "AT+CWQAP", NULL, 0, 3000);
}

ESP01_Status ESP01_SetBaud(ESP01_Instance* esp, uint32_t new_baud) {
    char cmd[48];

    if (new_baud == 0) return ESP01_ERROR_PARAM;

    /* AT+UART_DEF=<baudrate>,8,1,0,0 — บันทึกลง flash ทุก power cycle */
    snprintf(cmd, sizeof(cmd), "AT+UART_DEF=%lu,8,1,0,0", (unsigned long)new_baud);
    ESP01_Status st = _exec_cmd(esp, cmd, NULL, 0, 2000);
    if (st != ESP01_OK) return st;

    esp->baudrate = new_baud;
    Delay_Ms(50);
    USART_SimpleInit((USART_BaudRate)new_baud, esp->pin_config);
    Delay_Ms(100);

    /* ยืนยัน */
    return ESP01_TestAT(esp);
}

/* ========== Power ========== */

ESP01_Status ESP01_GetCurrentProfile(ESP01_PowerState state, ESP01_CurrentProfile* profile) {
    if (profile == NULL) {
        return ESP01_ERROR_PARAM;
    }

    switch (state) {
        case ESP01_PWR_DEEP_SLEEP:
            profile->current_typ_ma = 1;
            profile->current_peak_ma = 1;
            profile->note = "Typical 0.02-0.20mA depending on module board.";
            break;
        case ESP01_PWR_SLEEP:
            profile->current_typ_ma = 10;
            profile->current_peak_ma = 20;
            profile->note = "Light/Modem sleep while associated.";
            break;
        case ESP01_PWR_IDLE_CONNECTED:
            profile->current_typ_ma = 65;
            profile->current_peak_ma = 90;
            profile->note = "Connected idle with beacons.";
            break;
        case ESP01_PWR_RX_ACTIVE:
            profile->current_typ_ma = 60;
            profile->current_peak_ma = 80;
            profile->note = "RX activity.";
            break;
        case ESP01_PWR_TX_PEAK:
            profile->current_typ_ma = 170;
            profile->current_peak_ma = 300;
            profile->note = "TX burst peaks, size regulator for >=500mA.";
            break;
        default:
            return ESP01_ERROR_PARAM;
    }

    return ESP01_OK;
}
