#ifndef __HAL_UTILS_H__
#define __HAL_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ค่าที่น้อยกว่าระหว่าง a และ b
 * @param   a   ค่าที่ 1
 * @param   b   ค่าที่ 2
 * @return  ค่าที่น้อยกว่า
 */
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

/**
 * @brief   ค่าที่มากกว่าระหว่าง a และ b
 * @param   a   ค่าที่ 1
 * @param   b   ค่าที่ 2
 * @return  ค่าที่มากกว่า
 */
#ifndef MAX
#define MAX(a, b)  (((a) < (b)) ? (b) : (a))
#endif

/**
 * @brief   ค่าสัมบูรณ์ของ x
 * @param   x   ค่าที่ต้องการหาค่าสัมบูรณ์
 * @return  ค่าสัมบูรณ์ของ x
 */
#ifndef ABS
#define ABS(x)     (((x) < 0) ? -(x) : (x))
#endif

/**
 * @brief   สร้าง bitmask โดยเลื่อน 1 ไปทางซ้าย n ตำแหน่ง
 * @param   n   ตำแหน่งบิต (0 = LSB)
 * @return  bitmask ค่า (1 << n)
 */
#ifndef BV
#define BV(n)      (1 << (n))
#endif

/**
 * @brief   หาจำนวนสมาชิกใน array
 * @param   a   ชื่อ array
 * @return  จำนวนสมาชิก
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifdef __cplusplus
}
#endif

#endif
