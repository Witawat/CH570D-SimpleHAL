#ifndef __DEBUG_STUB_H__
#define __DEBUG_STUB_H__
#include <stdio.h>
#ifndef DEBUG
#define DEBUG(...)
#endif
#ifdef PRINT
#undef PRINT
#endif
#define PRINT(...)
#ifndef PRINTF
#define PRINTF(...)
#endif
#endif
