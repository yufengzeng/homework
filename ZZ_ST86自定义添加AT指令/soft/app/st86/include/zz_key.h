#ifndef _ZZ_KEY_H_
#define _ZZ_KEY_H_

#include "sl_type.h"

/*
 * 能够检测的最大按键数
 */
#define  KEY_MAX_NUM      (1)

/*
 * 按键按下时的去抖时间 : KEY_DOWN_TIMES * 扫描周期
 */
#define  KEY_DOWN_TIMES  (2)

/*
 * 按键抬键时的去抖时间 : KEY_UP_TIMES * 扫描周期
 */
#define  KEY_UP_TIMES     (2)

/*
 * 按键长按时间: KEY_LONG_TIMES *扫描周期
 */
#define  KEY_LONG_TIMES   (200)

/*
 * 按键双击的检测时间
 */
#define  KEY_TWICE_TIMES  (12)

typedef void (*KeyPressProcCb)(void);
typedef bool (*KeyIsPressCb)(void);

/*
 * 按键注册函数
 * KeyIsPressCb : 判断按键是否按下回调函数，返回true则为按下
 * _key_short_press_proc_cb: 按键短按回调函数
 * _key_short_up_press_proc_cb : 按键短按抬起回调函数
 * _key_long_press_proc_cb : 按键长按回调函数
 * _key_long_up_press_proc_cb: 按键长按抬键回调函数
 * _key_press_twice_proc_cb : 按键双击回调函数
 */
bool keyRegisterSingle(
                        KeyIsPressCb   _key_is_press,
                        KeyPressProcCb _key_short_press_proc_cb,
                        KeyPressProcCb _key_short_up_press_proc_cb,
                        KeyPressProcCb _key_long_press_proc_cb,
                        KeyPressProcCb _key_long_up_press_proc_cb,
                        KeyPressProcCb _key_press_twice_proc_cb);

/*
 * 按键扫描，置于定时器的回调函数中
 */
void keyScan(void);


#endif
