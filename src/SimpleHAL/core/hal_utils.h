#ifndef __HAL_UTILS_H__
#define __HAL_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)  (((a) < (b)) ? (b) : (a))
#endif

#ifndef ABS
#define ABS(x)     (((x) < 0) ? -(x) : (x))
#endif

#ifndef BV
#define BV(n)      (1 << (n))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifdef __cplusplus
}
#endif

#endif
