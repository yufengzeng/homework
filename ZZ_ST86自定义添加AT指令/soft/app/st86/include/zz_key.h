#ifndef _ZZ_KEY_H_
#define _ZZ_KEY_H_

#include "sl_type.h"

/*
 * �ܹ�������󰴼���
 */
#define  KEY_MAX_NUM      (1)

/*
 * ��������ʱ��ȥ��ʱ�� : KEY_DOWN_TIMES * ɨ������
 */
#define  KEY_DOWN_TIMES  (2)

/*
 * ����̧��ʱ��ȥ��ʱ�� : KEY_UP_TIMES * ɨ������
 */
#define  KEY_UP_TIMES     (2)

/*
 * ��������ʱ��: KEY_LONG_TIMES *ɨ������
 */
#define  KEY_LONG_TIMES   (200)

/*
 * ����˫���ļ��ʱ��
 */
#define  KEY_TWICE_TIMES  (12)

typedef void (*KeyPressProcCb)(void);
typedef bool (*KeyIsPressCb)(void);

/*
 * ����ע�ắ��
 * KeyIsPressCb : �жϰ����Ƿ��»ص�����������true��Ϊ����
 * _key_short_press_proc_cb: �����̰��ص�����
 * _key_short_up_press_proc_cb : �����̰�̧��ص�����
 * _key_long_press_proc_cb : ���������ص�����
 * _key_long_up_press_proc_cb: ��������̧���ص�����
 * _key_press_twice_proc_cb : ����˫���ص�����
 */
bool keyRegisterSingle(
                        KeyIsPressCb   _key_is_press,
                        KeyPressProcCb _key_short_press_proc_cb,
                        KeyPressProcCb _key_short_up_press_proc_cb,
                        KeyPressProcCb _key_long_press_proc_cb,
                        KeyPressProcCb _key_long_up_press_proc_cb,
                        KeyPressProcCb _key_press_twice_proc_cb);

/*
 * ����ɨ�裬���ڶ�ʱ���Ļص�������
 */
void keyScan(void);


#endif
