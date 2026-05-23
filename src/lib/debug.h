/**
 * @file debug.h
 * @brief Debug macros — all stubs that expand to nothing when DEBUG is not defined
 */

#ifndef __DEBUG_STUB_H__
#define __DEBUG_STUB_H__
#include <stdio.h>

/**
 * @brief Conditional debug print macro
 * @note  Expands to nothing unless a DEBUG macro is defined before inclusion.
 *        Define `DEBUG` to enable: `#define DEBUG` before `#include "debug.h"`.
 */
#ifndef DEBUG
#define DEBUG(...)
#endif

/**
 * @brief Unconditional print macro (always disabled stub)
 * @note  Always a no-op. Provided as placeholder; replace with actual print
 *        implementation when porting to a platform with serial output.
 */
#ifdef PRINT
#undef PRINT
#endif
#define PRINT(...)

/**
 * @brief Formatted print macro (always disabled stub)
 * @note  Always a no-op. Intended for printf-style debugging.
 *        Define or redirect to platform-specific UART/printf on target hardware.
 */
#ifndef PRINTF
#define PRINTF(...)
#endif

#endif
