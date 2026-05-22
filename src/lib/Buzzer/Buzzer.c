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
 */
static uint8_t pin_to_pwm_channel(uint8_t pin) {
    if (pin <= 4) return pin;
    return 0xFF;
}

/**
 * @brief ตั้งค่าความถี่ PWM
 */
static void set_pwm_frequency(uint16_t frequency) {
    if(frequency == 0 || buzzer.pwm_channel == 0xFF) return;
    
    // Re-init PWM with new frequency
    PWM_Init(buzzer.pwm_channel, frequency);
    buzzer.current_freq = frequency;
}

/**
 * @brief ตั้งค่า duty cycle
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
 * @brief เริ่มเล่นโทนเสียง
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
 * @brief หยุดเล่นโทนเสียง
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

void Buzzer_SetVolume(uint8_t volume) {
    if(volume > 100) volume = 100;
    buzzer.volume = volume;
    
    // Update duty if currently playing
    if(buzzer.state == BUZZER_PLAYING) {
        set_pwm_duty(volume / 2);
    }
}

/* ----- Basic Control ----- */

void Buzzer_On(void) {
    if(!buzzer.initialized) return;
    
    start_tone(buzzer.current_freq);
    buzzer.state = BUZZER_PLAYING;
}

void Buzzer_Off(void) {
    if(!buzzer.initialized) return;
    
    stop_tone();
    buzzer.state = BUZZER_IDLE;
}

void Buzzer_Toggle(void) {
    if(buzzer.state == BUZZER_PLAYING) {
        Buzzer_Off();
    } else {
        Buzzer_On();
    }
}

void Buzzer_Beep(uint16_t duration_ms) {
    if(!buzzer.initialized) return;
    
    Buzzer_On();
    Delay_Ms(duration_ms);
    Buzzer_Off();
}

void Buzzer_BeepMultiple(uint8_t count, uint16_t on_time, uint16_t off_time) {
    for(uint8_t i = 0; i < count; i++) {
        Buzzer_Beep(on_time);
        if(i < count - 1) {  // Don't delay after last beep
            Delay_Ms(off_time);
        }
    }
}

/* ----- Tone Control ----- */

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

void Buzzer_NoTone(void) {
    Buzzer_Off();
}

/* ----- Melody & Pattern ----- */

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

void Buzzer_Stop(void) {
    Buzzer_Off();
    buzzer.melody = NULL;
    buzzer.melody_length = 0;
    buzzer.pattern = NULL;
    buzzer.pattern_length = 0;
    buzzer.pattern_index = 0;
    buzzer.pattern_count = 0;
}

void Buzzer_Pause(void) {
    if(buzzer.state == BUZZER_PLAYING) {
        stop_tone();
        buzzer.state = BUZZER_PAUSED;
    }
}

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

uint8_t Buzzer_IsBusy(void) {
    return (buzzer.state == BUZZER_PLAYING) ? 1 : 0;
}

BuzzerState Buzzer_GetState(void) {
    return buzzer.state;
}

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

void Buzzer_AlertAsync(BuzzerAlertType type) {
    if(!buzzer.initialized || (uint8_t)type >= 6) return;
    Buzzer_PlayPatternAsync(_alert_steps[type], _alert_len[type], 0);
}

/* ========== Pre-defined Patterns ========== */

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

void Buzzer_PlayAlarm(uint16_t duration_ms) {
    if(!buzzer.initialized) return;
    
    uint32_t start = Get_CurrentMs();
    
    while(duration_ms == 0 || (Get_CurrentMs() - start) < duration_ms) {
        Buzzer_FrequencySweep(800, 1200, 500, 10);
        Buzzer_FrequencySweep(1200, 800, 500, 10);
        
        if(duration_ms > 0) break;  // One cycle for timed alarm
    }
}

void Buzzer_PlaySuccess(void) {
    static const Note success[] = {
        {NOTE_C4, 100},
        {NOTE_E4, 100},
        {NOTE_G4, 100},
        {NOTE_C5, 200}
    };
    Buzzer_PlayMelody(success, 4);
}

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
