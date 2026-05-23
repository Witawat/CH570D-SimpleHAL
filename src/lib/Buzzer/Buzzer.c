/**
 * @file Buzzer.c
 * @brief Buzzer Control Library Implementation
 * @version 1.0.0
 * @date 2025-12-22
 */

#include "Buzzer.h"

/* ========== Private Variables ========== */

static struct {
    uint8_t pin;              // GPIO pin number
    uint8_t pwm_channel;      // PWM channel
    BuzzerType type;          // Active High/Low
    uint8_t volume;           // Volume 0-100%
    uint16_t current_freq;    // Current frequency
    BuzzerState state;        // Current state
    
    // Non-blocking melody playback
    const Note* melody;       // Current melody
    uint8_t melody_length;    // Total notes
    uint8_t melody_index;     // Current note index
    uint8_t melody_repeat;    // Repeat count (0 = infinite)
    uint8_t melody_count;     // Current repeat count
    uint32_t note_start_time; // Note start time
    uint8_t initialized;      // Init flag

    // Non-blocking beep pattern playback
    const BeepStep* pattern;        // Current pattern
    uint8_t pattern_length;         // Total steps
    uint8_t pattern_index;          // Current step index
    uint8_t pattern_repeat;         // Repeat count (0 = infinite)
    uint8_t pattern_count;          // Current repeat count
    uint8_t pattern_phase;          // 0 = ON phase, 1 = OFF phase
    uint32_t pattern_phase_start;   // Timestamp of current phase start
} buzzer = {0};

/* ========== Private Function Prototypes ========== */

static uint8_t pin_to_pwm_channel(uint8_t pin);
static void set_pwm_frequency(uint16_t frequency);
static void set_pwm_duty(uint8_t duty);
static void start_tone(uint16_t frequency);
static void stop_tone(void);

/* ========== Private Functions ========== */

/**
 * @brief แปลง GPIO pin เป็น PWM channel (CH570D: PA0-PA4 map to CH1-CH5)
 *
 * @param pin - GPIO pin number
 * @return uint8_t - PWM channel (0-4) หรือ 0xFF ถ้า pin ไม่รองรับ PWM
 *
 * @note CH570D: PA0=CH1, PA1=CH2, PA2=CH3, PA3=CH4, PA4=CH5
 * @note pin > 4 → return 0xFF (fallback to GPIO mode)
 */
static uint8_t pin_to_pwm_channel(uint8_t pin) {
    if (pin <= 4) return pin;
    return 0xFF;
}

/**
 * @brief ตั้งค่าความถี่ PWM
 *
 * @param frequency - ความถี่เป้าหมาย (Hz), 0 → ไม่ทำอะไร
 *
 * @note ใช้ PWM_Init() เพื่อเปลี่ยน frequency ของ PWM channel ปัจจุบัน
 * @note อัปเดต buzzer.current_freq หลังจากตั้งค่า
 * @note ถ้า pwm_channel == 0xFF (invalid) → return ทันที
 */
static void set_pwm_frequency(uint16_t frequency) {
    if(frequency == 0 || buzzer.pwm_channel == 0xFF) return;
    
    // Re-init PWM with new frequency
    PWM_Init(buzzer.pwm_channel, frequency);
    buzzer.current_freq = frequency;
}

/**
 * @brief ตั้งค่า duty cycle PWM (volume control)
 *
 * @param duty - duty cycle 0-100%
 *
 * @note ถ้า buzzer type เป็น BUZZER_ACTIVE_LOW → invert duty (100 - duty)
 * @note ถ้า pwm_channel == 0xFF → return ทันที (ไม่สามารถตั้ง duty ได้)
 * @note volume = 0 → duty = 0% (silent), volume = 100 → duty = 50% (max)
 */
static void set_pwm_duty(uint8_t duty) {
    if(buzzer.pwm_channel == 0xFF) return;
    
    // Invert for Active Low
    if(buzzer.type == BUZZER_ACTIVE_LOW) {
        duty = 100 - duty;
    }
    
    PWM_SetDutyCycle(buzzer.pwm_channel, duty);
}

/**
 * @brief เริ่มเล่นโทนเสียงที่ความถี่ที่กำหนด
 *
 * @param frequency - ความถี่ (Hz), 0 → หยุดเล่น (เรียก stop_tone แทน)
 *
 * @note ตั้ง frequency ก่อนแล้วค่อย set duty cycle
 * @note duty = volume/2 (50% duty ที่ volume สูงสุด)
 * @note เรียก PWM_Start() เพื่อเริ่มส่งสัญญาณ PWM ออก pin
 */
static void start_tone(uint16_t frequency) {
    if(frequency == 0) {
        stop_tone();
        return;
    }
    
    set_pwm_frequency(frequency);
    set_pwm_duty(buzzer.volume / 2);  // 50% duty at max volume
    PWM_Start(buzzer.pwm_channel);
}

/**
 * @brief หยุดเล่นโทนเสียงและตั้ง pin ให้อยู่ใน safe state
 *
 * @note เรียก PWM_Stop() เพื่อหยุด PWM signal
 * @note ตั้ง pin ให้อยู่ในสถานะที่ไม่ activate buzzer:
 *       Active High → set LOW, Active Low → set HIGH
 */
static void stop_tone(void) {
    PWM_Stop(buzzer.pwm_channel);
    
    // Set pin to safe state
    if(buzzer.type == BUZZER_ACTIVE_HIGH) {
        digitalWrite(buzzer.pin, LOW);
    } else {
        digitalWrite(buzzer.pin, HIGH);
    }
}

/* ========== Public Functions ========== */

/* ----- Initialization ----- */

/**
 * @brief เริ่มต้นการใช้งาน Buzzer
 *
 * @param pin - หมายเลข GPIO pin (ต้องเป็น PWM pin: PA1, PC0, PC3-4, PD2-4, PD7)
 * @param type - ชนิดของ buzzer (BUZZER_ACTIVE_HIGH หรือ BUZZER_ACTIVE_LOW)
 *
 * @note ฟังก์ชันนี้จะ init PWM อัตโนมัติ (fallback เป็น GPIO ถ้า pin ไม่รองรับ PWM)
 * @note ตั้ง pin ให้ safe state ทันที (high สำหรับ Active Low, low สำหรับ Active High)
 * @note ต้องเรียกฟังก์ชันนี้ก่อนใช้งาน buzzer
 */
void Buzzer_Init(uint8_t pin, BuzzerType type) {
    buzzer.pin = pin;
    buzzer.type = type;
    buzzer.volume = BUZZER_DEFAULT_VOLUME;
    buzzer.current_freq = BUZZER_DEFAULT_FREQ;
    buzzer.state = BUZZER_IDLE;
    buzzer.initialized = 1;
    
    // Get PWM channel
    buzzer.pwm_channel = pin_to_pwm_channel(pin);
    
    if(buzzer.pwm_channel == 0xFF) {
        // Invalid pin - fallback to GPIO
        pinMode(pin, PIN_MODE_OUTPUT);
        digitalWrite(pin, (type == BUZZER_ACTIVE_HIGH) ? LOW : HIGH);
        return;
    }
    
    // Init PWM
    PWM_Init(buzzer.pwm_channel, BUZZER_DEFAULT_FREQ);
    PWM_Stop(buzzer.pwm_channel);
    
    // Set pin to safe state
    pinMode(pin, PIN_MODE_OUTPUT);
    digitalWrite(pin, (type == BUZZER_ACTIVE_HIGH) ? LOW : HIGH);
}

/**
 * @brief ตั้งค่าระดับเสียง (volume) 0-100%
 *
 * @param volume - ระดับเสียง 0-100% (clamp ที่ 100 ถ้าเกิน)
 *
 * @note ควบคุมผ่าน PWM duty cycle
 * @note 0 = ปิดเสียง (duty 0%), 100 = เสียงดังสุด (duty 50%)
 * @note ถ้ากำลังเล่นอยู่ จะอัปเดต duty ทันที
 */
void Buzzer_SetVolume(uint8_t volume) {
    if(volume > 100) volume = 100;
    buzzer.volume = volume;
    
    // Update duty if currently playing
    if(buzzer.state == BUZZER_PLAYING) {
        set_pwm_duty(volume / 2);
    }
}

/* ----- Basic Control ----- */

/**
 * @brief เปิด buzzer (continuous tone) ด้วยความถี่ที่ตั้งไว้ล่าสุด
 *
 * @note ใช้ความถี่จาก buzzer.current_freq (default: BUZZER_DEFAULT_FREQ = 2000Hz)
 * @note ต้องเรียก Buzzer_Off() หรือ Buzzer_Stop() เพื่อปิด
 * @note ตั้ง state เป็น BUZZER_PLAYING
 */
void Buzzer_On(void) {
    if(!buzzer.initialized) return;
    
    start_tone(buzzer.current_freq);
    buzzer.state = BUZZER_PLAYING;
}

/**
 * @brief ปิด buzzer
 *
 * @note เรียก stop_tone() เพื่อหยุด PWM และ set pin safe state
 * @note ตั้ง state เป็น BUZZER_IDLE
 * @note ไม่ล้าง melody/pattern pointers — ใช้ Buzzer_Stop() เพื่อล้างทั้งหมด
 */
void Buzzer_Off(void) {
    if(!buzzer.initialized) return;
    
    stop_tone();
    buzzer.state = BUZZER_IDLE;
}

/**
 * @brief สลับสถานะ buzzer (open → ปิด, ปิด → เปิด)
 *
 * @note ตรวจสอบ state ปัจจุบัน: PLAYING → Off, IDLE/PAUSED → On
 * @note PAUSED → On (resume เล่นต่อ)
 */
void Buzzer_Toggle(void) {
    if(buzzer.state == BUZZER_PLAYING) {
        Buzzer_Off();
    } else {
        Buzzer_On();
    }
}

/**
 * @brief Beep ครั้งเดียว (blocking)
 *
 * @param duration_ms - ระยะเวลา beep (ms)
 *
 * @note ฟังก์ชันนี้จะบล็อกการทำงานจนกว่า beep จะเสร็จ (ใช้ Delay_Ms)
 * @note ใช้ความถี่ที่ตั้งไว้ล่าสุด (current_freq)
 * @note ถ้าไม่ initialized → return ทันที
 */
void Buzzer_Beep(uint16_t duration_ms) {
    if(!buzzer.initialized) return;
    
    Buzzer_On();
    Delay_Ms(duration_ms);
    Buzzer_Off();
}

/**
 * @brief Beep หลายครั้ง (blocking)
 *
 * @param count - จำนวนครั้งที่ต้องการให้ beep
 * @param on_time - ระยะเวลาเปิดเสียงต่อครั้ง (ms)
 * @param off_time - ระยะเวลาปิดระหว่างครั้ง (ms)
 *
 * @note ฟังก์ชันนี้จะบล็อกการทำงานจนกว่าจะ beep ครบทุกครั้ง
 * @note ครั้งสุดท้ายจะไม่ Delay_Ms(off_time) เพื่อจบแบบทันที
 * @note ใช้ Buzzer_Beep() ภายใน loop
 */
void Buzzer_BeepMultiple(uint8_t count, uint16_t on_time, uint16_t off_time) {
    for(uint8_t i = 0; i < count; i++) {
        Buzzer_Beep(on_time);
        if(i < count - 1) {  // Don't delay after last beep
            Delay_Ms(off_time);
        }
    }
}

/* ----- Tone Control ----- */

/**
 * @brief เล่นโทนเสียงที่ความถี่กำหนด (blocking)
 *
 * @param frequency - ความถี่ในหน่วย Hz (0 = ปิด buzzer)
 * @param duration_ms - ระยะเวลา (ms), 0 = เล่นต่อเนื่อง (ต้องเรียก Buzzer_NoTone() เพื่อหยุด)
 *
 * @note ฟังก์ชันนี้จะบล็อกการทำงานจนกว่าเสียงจะเสร็จ (ใช้ Delay_Ms)
 * @note ถ้า duration = 0 จะเล่นต่อเนื่องและ return ทันที
 * @note frequency = 0 → ปิด buzzer ทันที
 */
void Buzzer_Tone(uint16_t frequency, uint16_t duration_ms) {
    if(!buzzer.initialized) return;
    
    if(frequency == 0) {
        Buzzer_Off();
        return;
    }
    
    start_tone(frequency);
    buzzer.state = BUZZER_PLAYING;
    
    if(duration_ms > 0) {
        Delay_Ms(duration_ms);
        Buzzer_Off();
    }
}

/**
 * @brief เล่นโทนเสียงแบบ non-blocking
 *
 * @param frequency - ความถี่ในหน่วย Hz (0 = ปิด buzzer)
 * @param duration_ms - ระยะเวลา (ms), 0 = เล่นต่อเนื่อง
 *
 * @note ฟังก์ชันนี้จะ return ทันที ไม่บล็อกการทำงาน
 * @note ใช้ Buzzer_Update() ใน main loop เพื่อจัดการ timeout
 * @note เก็บ duration ไว้ใน melody_index และ melody_length = 1 เป็น flag สำหรับ async tone
 */
void Buzzer_ToneAsync(uint16_t frequency, uint16_t duration_ms) {
    if(!buzzer.initialized) return;
    
    if(frequency == 0) {
        Buzzer_Off();
        return;
    }
    
    start_tone(frequency);
    buzzer.state = BUZZER_PLAYING;
    buzzer.note_start_time = Get_CurrentMs();
    
    // Store duration in melody_length as a flag
    buzzer.melody_length = (duration_ms > 0) ? 1 : 0;
    buzzer.melody_index = duration_ms;  // Store duration
}

/**
 * @brief หยุดเล่นโทนเสียง (เรียก Buzzer_Off() ภายใน)
 *
 * @note ใช้กับ Buzzer_Tone() ที่ duration = 0 (continuous mode)
 * @note เทียบเท่ากับ Buzzer_Off() แต่ semantic ชัดเจนกว่าสำหรับ tone control
 */
void Buzzer_NoTone(void) {
    Buzzer_Off();
}

/* ----- Melody & Pattern ----- */

/**
 * @brief เล่นทำนอง (blocking)
 *
 * @param melody - array ของ Note structures (frequency, duration)
 * @param length - จำนวนโน้ตใน array
 *
 * @note ฟังก์ชันนี้จะบล็อกการทำงานจนกว่าทำนองจะเล่นเสร็จ (ใช้ Delay_Ms ระหว่างโน้ต)
 * @note frequency == 0 → rest (เงียบ) ในโน้ตนั้น
 * @note เรียก Buzzer_Off() หลังเล่นจบเสมอ
 */
void Buzzer_PlayMelody(const Note* melody, uint8_t length) {
    if(!buzzer.initialized || melody == NULL || length == 0) return;
    
    for(uint8_t i = 0; i < length; i++) {
        if(melody[i].frequency == 0) {
            // Rest
            Buzzer_Off();
        } else {
            start_tone(melody[i].frequency);
            buzzer.state = BUZZER_PLAYING;
        }
        
        Delay_Ms(melody[i].duration);
    }
    
    Buzzer_Off();
}

/**
 * @brief เล่นทำนองแบบ non-blocking
 *
 * @param melody - array ของ Note structures
 * @param length - จำนวนโน้ต
 * @param repeat - จำนวนครั้งที่เล่นซ้ำ (0 = infinite)
 *
 * @note ฟังก์ชันนี้จะ return ทันที
 * @note ต้องเรียก Buzzer_Update() ใน main loop เพื่อเลื่อนโน้ตตามเวลา
 * @note เริ่มเล่นโน้ตตัวแรกทันทีที่ถูกเรียก
 */
void Buzzer_PlayMelodyAsync(const Note* melody, uint8_t length, uint8_t repeat) {
    if(!buzzer.initialized || melody == NULL || length == 0) return;
    
    buzzer.melody = melody;
    buzzer.melody_length = length;
    buzzer.melody_index = 0;
    buzzer.melody_repeat = repeat;
    buzzer.melody_count = 0;
    buzzer.state = BUZZER_PLAYING;
    buzzer.note_start_time = Get_CurrentMs();
    
    // Start first note
    if(buzzer.melody[0].frequency == 0) {
        stop_tone();
    } else {
        start_tone(buzzer.melody[0].frequency);
    }
}

/**
 * @brief Update melody/pattern playback (สำหรับ non-blocking mode)
 *
 * @note ต้องเรียกฟังก์ชันนี้ใน main loop เมื่อใช้ async functions
 * @note จัดการ 3 โหมด: async tone, melody playback, beep pattern playback
 * @note สำหรับ async tone: เช็คว่า elapsed >= duration → ปิด buzzer
 * @note สำหรับ melody: เช็คว่า elapsed >= note.duration → เลื่อนไปโน้ตถัดไป
 * @note สำหรับ pattern: สลับ ON/OFF phase ตาม BeepStep
 */
void Buzzer_Update(void) {
    if(!buzzer.initialized) return;
    
    // Handle async tone
    if(buzzer.melody == NULL && buzzer.melody_length == 1) {
        uint32_t elapsed = Get_CurrentMs() - buzzer.note_start_time;
        if(elapsed >= buzzer.melody_index) {
            Buzzer_Off();
            buzzer.melody_length = 0;
        }
        return;
    }
    
    // Handle melody playback
    if(buzzer.melody != NULL && buzzer.state == BUZZER_PLAYING) {
        uint32_t elapsed = Get_CurrentMs() - buzzer.note_start_time;
        
        if(elapsed >= buzzer.melody[buzzer.melody_index].duration) {
            // Move to next note
            buzzer.melody_index++;
            
            if(buzzer.melody_index >= buzzer.melody_length) {
                // Melody finished
                buzzer.melody_count++;
                
                if(buzzer.melody_repeat == 0 || buzzer.melody_count < buzzer.melody_repeat) {
                    // Repeat
                    buzzer.melody_index = 0;
                } else {
                    // Done
                    Buzzer_Off();
                    buzzer.melody = NULL;
                    return;
                }
            }
            
            // Play next note
            buzzer.note_start_time = Get_CurrentMs();
            
            if(buzzer.melody[buzzer.melody_index].frequency == 0) {
                stop_tone();
            } else {
                start_tone(buzzer.melody[buzzer.melody_index].frequency);
            }
        }
        return;
    }

    // Handle beep pattern playback
    if(buzzer.pattern != NULL && buzzer.state == BUZZER_PLAYING) {
        uint32_t elapsed = Get_CurrentMs() - buzzer.pattern_phase_start;
        const BeepStep* step = &buzzer.pattern[buzzer.pattern_index];

        if(buzzer.pattern_phase == 0) {
            // ON phase
            if(elapsed >= step->on_ms) {
                stop_tone();
                if(step->off_ms == 0) {
                    // No OFF phase — advance to next step immediately
                    buzzer.pattern_index++;
                    if(buzzer.pattern_index >= buzzer.pattern_length) {
                        buzzer.pattern_index = 0;
                        buzzer.pattern_count++;
                        if(buzzer.pattern_repeat != 0 && buzzer.pattern_count >= buzzer.pattern_repeat) {
                            Buzzer_Off();
                            buzzer.pattern = NULL;
                            return;
                        }
                    }
                    // Start next step ON phase
                    buzzer.pattern_phase = 0;
                    buzzer.pattern_phase_start = Get_CurrentMs();
                    start_tone(buzzer.current_freq);
                } else {
                    // Move to OFF phase
                    buzzer.pattern_phase = 1;
                    buzzer.pattern_phase_start = Get_CurrentMs();
                }
            }
        } else {
            // OFF phase
            if(elapsed >= step->off_ms) {
                // Advance to next step
                buzzer.pattern_index++;
                if(buzzer.pattern_index >= buzzer.pattern_length) {
                    buzzer.pattern_index = 0;
                    buzzer.pattern_count++;
                    if(buzzer.pattern_repeat != 0 && buzzer.pattern_count >= buzzer.pattern_repeat) {
                        Buzzer_Off();
                        buzzer.pattern = NULL;
                        return;
                    }
                }
                // Start next step ON phase
                buzzer.pattern_phase = 0;
                buzzer.pattern_phase_start = Get_CurrentMs();
                start_tone(buzzer.current_freq);
            }
        }
    }
}

/* ----- Advanced Functions ----- */

/**
 * @brief หยุดการเล่นทั้งหมด
 *
 * @note หยุดทั้ง blocking และ non-blocking operations
 * @note เรียก Buzzer_Off() → ล้าง melody และ pattern pointers
 * @note state → BUZZER_IDLE
 */
void Buzzer_Stop(void) {
    Buzzer_Off();
    buzzer.melody = NULL;
    buzzer.melody_length = 0;
    buzzer.pattern = NULL;
    buzzer.pattern_length = 0;
    buzzer.pattern_index = 0;
    buzzer.pattern_count = 0;
}

/**
 * @brief พักการเล่น (pause) — ใช้กับ non-blocking mode
 *
 * @note เรียก stop_tone() เพื่อหยุดเสียง แต่คง state เป็น PAUSED
 * @note เรียก Buzzer_Resume() เพื่อเล่นต่อจากตำแหน่งที่ค้างไว้
 * @note ถ้า state ไม่ใช่ PLAYING → ไม่ทำอะไร
 */
void Buzzer_Pause(void) {
    if(buzzer.state == BUZZER_PLAYING) {
        stop_tone();
        buzzer.state = BUZZER_PAUSED;
    }
}

/**
 * @brief เล่นต่อจากที่พัก (resume) — ใช้กับ non-blocking mode
 *
 * @note ต้อง state เป็น PAUSED เท่านั้นถึงจะ resume ได้
 * @note ถ้า melody ค้างอยู่ จะเล่นโน้ตปัจจุบันต่อ (frequency > 0)
 * @note ตั้ง state กลับเป็น PLAYING
 */
void Buzzer_Resume(void) {
    if(buzzer.state == BUZZER_PAUSED) {
        if(buzzer.melody != NULL && buzzer.melody_index < buzzer.melody_length) {
            if(buzzer.melody[buzzer.melody_index].frequency > 0) {
                start_tone(buzzer.melody[buzzer.melody_index].frequency);
            }
        }
        buzzer.state = BUZZER_PLAYING;
    }
}

/**
 * @brief ตรวจสอบว่า buzzer กำลังทำงานอยู่หรือไม่
 *
 * @return uint8_t - 1 = กำลังทำงาน (BUZZER_PLAYING), 0 = ว่าง (IDLE/PAUSED)
 *
 * @note PAUSED ถือว่าไม่ busy (ไม่มีการส่งเสียง)
 */
uint8_t Buzzer_IsBusy(void) {
    return (buzzer.state == BUZZER_PLAYING) ? 1 : 0;
}

/**
 * @brief อ่านสถานะปัจจุบันของ buzzer
 *
 * @return BuzzerState - BUZZER_IDLE (0), BUZZER_PLAYING (1), BUZZER_PAUSED (2)
 *
 * @note ใช้ตรวจสอบสถานะก่อนเรียกฟังก์ชันอื่น
 */
BuzzerState Buzzer_GetState(void) {
    return buzzer.state;
}

/**
 * @brief Sweep ความถี่ (frequency sweep) — blocking
 *
 * @param start_freq - ความถี่เริ่มต้น (Hz)
 * @param end_freq - ความถี่สุดท้าย (Hz)
 * @param duration_ms - ระยะเวลาทั้งหมด (ms)
 * @param step_ms - ระยะเวลาแต่ละ step (ms)
 *
 * @note ฟังก์ชันนี้จะบล็อกการทำงาน
 * @note ใช้สร้างเสียงพิเศษ เช่น siren
 * @note ถ้า start_freq < end_freq → sweep ขึ้น, ถ้ามากกว่า → sweep ลง
 * @note clamp ค่าที่ end_freq เมื่อถึง
 */
void Buzzer_FrequencySweep(uint16_t start_freq, uint16_t end_freq, 
                           uint16_t duration_ms, uint16_t step_ms) {
    if(!buzzer.initialized || step_ms == 0) return;
    
    int32_t freq_step = (int32_t)(end_freq - start_freq) * step_ms / duration_ms;
    int32_t current = start_freq;
    uint32_t elapsed = 0;
    
    while(elapsed < duration_ms) {
        start_tone((uint16_t)current);
        buzzer.state = BUZZER_PLAYING;
        Delay_Ms(step_ms);
        
        current += freq_step;
        elapsed += step_ms;
        
        // Clamp
        if(freq_step > 0 && current > end_freq) current = end_freq;
        if(freq_step < 0 && current < end_freq) current = end_freq;
    }
    
    Buzzer_Off();
}

/* ========== Beep Pattern & Alert ========== */

/* Pre-defined alert step arrays */
static const BeepStep _alert_long[]    = {{2000, 0}};
static const BeepStep _alert_pulse[]   = {{500, 500}};
static const BeepStep _alert_tremolo[] = {{50, 50}};
static const BeepStep _alert_double[]  = {{100, 100}, {100, 500}};
static const BeepStep _alert_triple[]  = {{100, 100}, {100, 100}, {100, 500}};
static const BeepStep _alert_urgent[]  = {{80, 40}, {80, 40}, {80, 300}};

static const uint8_t _alert_len[] = {1, 1, 1, 2, 3, 3};

static const BeepStep* const _alert_steps[] = {
    _alert_long,
    _alert_pulse,
    _alert_tremolo,
    _alert_double,
    _alert_triple,
    _alert_urgent
};

/**
 * @brief เล่น pattern ที่กำหนดเอง (blocking)
 *
 * @param steps - array ของ BeepStep (on_ms / off_ms ต่อ step)
 * @param length - จำนวน steps
 * @param repeat - จำนวนรอบ (0 = infinite — ใช้ Buzzer_Stop() เพื่อหยุด)
 *
 * @note ฟังก์ชันนี้จะบล็อกการทำงานจนกว่าจะเล่นครบรอบ (หรือตลอดถ้า repeat=0)
 * @note off_ms = 0 → ข้ามช่วงเงียบของ step นั้นทันที
 * @note แต่ละ step: ON phase (Buzzer_On + Delay) → OFF phase (Delay) → next step
 */
void Buzzer_PlayPattern(const BeepStep* steps, uint8_t length, uint8_t repeat) {
    if(!buzzer.initialized || steps == NULL || length == 0) return;

    uint8_t count = 0;
    while(repeat == 0 || count < repeat) {
        for(uint8_t i = 0; i < length; i++) {
            // ON phase
            Buzzer_On();
            Delay_Ms(steps[i].on_ms);
            Buzzer_Off();
            // OFF phase
            if(steps[i].off_ms > 0) {
                Delay_Ms(steps[i].off_ms);
            }
        }
        if(repeat != 0) {
            count++;
        }
    }
}

/**
 * @brief เล่น pattern ที่กำหนดเองแบบ non-blocking
 *
 * @param steps - array ของ BeepStep
 * @param length - จำนวน steps
 * @param repeat - จำนวนรอบ (0 = infinite)
 *
 * @note ต้องเรียก Buzzer_Update() ใน main loop
 * @note เรียก Buzzer_Stop() เพื่อหยุดกลางคัน
 * @note หยุด melody (buzzer.melody = NULL) ก่อนเริ่ม pattern
 * @note เริ่ม ON phase ทันทีที่ถูกเรียก
 */
void Buzzer_PlayPatternAsync(const BeepStep* steps, uint8_t length, uint8_t repeat) {
    if(!buzzer.initialized || steps == NULL || length == 0) return;

    // Stop any melody
    buzzer.melody = NULL;
    buzzer.melody_length = 0;

    buzzer.pattern = steps;
    buzzer.pattern_length = length;
    buzzer.pattern_index = 0;
    buzzer.pattern_repeat = repeat;
    buzzer.pattern_count = 0;
    buzzer.pattern_phase = 0;  // ON phase first
    buzzer.pattern_phase_start = Get_CurrentMs();
    buzzer.state = BUZZER_PLAYING;

    // Start first ON phase
    start_tone(buzzer.current_freq);
}

/**
 * @brief เล่นเสียงแจ้งเตือนสำเร็จรูป (blocking)
 *
 * @param type - ชนิดเสียงแจ้งเตือน (BuzzerAlertType)
 * @param duration_ms - ระยะเวลาทั้งหมดที่เล่น (ms), 0 = เล่นครบ 1 รอบ pattern
 *
 * @note ใช้ predefined alert steps (ดู _alert_stacks[])
 * @note ถ้า type >= 6 → return ทันที (invalid)
 * @note duration = 0 → เล่นแค่รอบเดียวของ alert pattern
 */
void Buzzer_Alert(BuzzerAlertType type, uint16_t duration_ms) {
    if(!buzzer.initialized || (uint8_t)type >= 6) return;

    const BeepStep* steps = _alert_steps[type];
    uint8_t length = _alert_len[type];

    if(duration_ms == 0) {
        Buzzer_PlayPattern(steps, length, 1);
        return;
    }

    uint32_t start = Get_CurrentMs();
    while((Get_CurrentMs() - start) < duration_ms) {
        for(uint8_t i = 0; i < length; i++) {
            if((Get_CurrentMs() - start) >= duration_ms) break;
            Buzzer_On();
            Delay_Ms(steps[i].on_ms);
            Buzzer_Off();
            if(steps[i].off_ms > 0 && (Get_CurrentMs() - start) < duration_ms) {
                Delay_Ms(steps[i].off_ms);
            }
        }
    }
    Buzzer_Off();
}

/**
 * @brief เล่นเสียงแจ้งเตือนสำเร็จรูปแบบ non-blocking (infinite loop)
 *
 * @param type - ชนิดเสียงแจ้งเตือน (BuzzerAlertType)
 *
 * @note ต้องเรียก Buzzer_Update() ใน main loop
 * @note เรียก Buzzer_Stop() เพื่อหยุด (play repeat = 0 = infinite)
 * @note ถ้า type >= 6 → return ทันที (invalid)
 */
void Buzzer_AlertAsync(BuzzerAlertType type) {
    if(!buzzer.initialized || (uint8_t)type >= 6) return;
    Buzzer_PlayPatternAsync(_alert_steps[type], _alert_len[type], 0);
}

/* ========== Pre-defined Patterns ========== */

/**
 * @brief เล่น SOS pattern (... --- ...)
 *
 * @note S = 3 short beeps (100ms on, 100ms off)
 * @note O = 3 long beeps (300ms on, 100ms off)
 * @note ใช้ Buzzer_BeepMultiple() และ Delay_Ms ภายใน
 * @note blocking function
 */
void Buzzer_PlaySOS(void) {
    // S: ... (3 short)
    Buzzer_BeepMultiple(3, 100, 100);
    Delay_Ms(200);
    
    // O: --- (3 long)
    Buzzer_BeepMultiple(3, 300, 100);
    Delay_Ms(200);
    
    // S: ... (3 short)
    Buzzer_BeepMultiple(3, 100, 100);
}

/**
 * @brief เล่นเสียงเตือนภัย (alarm) — frequency sweep ระหว่าง 800-1200Hz
 *
 * @param duration_ms - ระยะเวลาทั้งหมด (ms), 0 = เล่นต่อเนื่อง
 *
 * @note ใช้ Buzzer_FrequencySweep() สลับขึ้น-ลงระหว่าง 800Hz ↔ 1200Hz
 * @note ถ้า duration_ms > 0 → เล่นแค่ 1 รอบ sweep แล้วจบ
 * @note blocking function
 */
void Buzzer_PlayAlarm(uint16_t duration_ms) {
    if(!buzzer.initialized) return;
    
    uint32_t start = Get_CurrentMs();
    
    while(duration_ms == 0 || (Get_CurrentMs() - start) < duration_ms) {
        Buzzer_FrequencySweep(800, 1200, 500, 10);
        Buzzer_FrequencySweep(1200, 800, 500, 10);
        
        if(duration_ms > 0) break;  // One cycle for timed alarm
    }
}

/**
 * @brief เล่นเสียงสำเร็จ (success sound) — ascending arpeggio C4-E4-G4-C5
 *
 * @note Blocking: ใช้ Buzzer_PlayMelody() ภายใน
 * @note โน้ต: C4(100ms) → E4(100ms) → G4(100ms) → C5(200ms)
 */
void Buzzer_PlaySuccess(void) {
    static const Note success[] = {
        {NOTE_C4, 100},
        {NOTE_E4, 100},
        {NOTE_G4, 100},
        {NOTE_C5, 200}
    };
    Buzzer_PlayMelody(success, 4);
}

/**
 * @brief เล่นเสียงผิดพลาด (error sound) — G4 สามครั้งสลับ rest
 *
 * @note Blocking: ใช้ Buzzer_PlayMelody() ภายใน
 * @note Pattern: G4(100ms) → rest(50ms) → G4(100ms) → rest(50ms) → G4(200ms)
 */
void Buzzer_PlayError(void) {
    static const Note error[] = {
        {NOTE_G4, 100},
        {NOTE_REST, 50},
        {NOTE_G4, 100},
        {NOTE_REST, 50},
        {NOTE_G4, 200}
    };
    Buzzer_PlayMelody(error, 5);
}

/**
 * @brief เล่นเสียงเริ่มต้น (startup sound) — ascending scale C4-D4-E4-F4-G4
 *
 * @note Blocking: ใช้ Buzzer_PlayMelody() ภายใน
 * @note โน้ต: C4(100ms) → D4(100ms) → E4(100ms) → F4(100ms) → G4(200ms)
 */
void Buzzer_PlayStartup(void) {
    static const Note startup[] = {
        {NOTE_C4, 100},
        {NOTE_D4, 100},
        {NOTE_E4, 100},
        {NOTE_F4, 100},
        {NOTE_G4, 200}
    };
    Buzzer_PlayMelody(startup, 5);
}
