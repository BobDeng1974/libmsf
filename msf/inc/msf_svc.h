/**************************************************************************
*
* Copyright (c) 2017-2018, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/
/** ΢�ں˵Ĳ���ܹ� -- OSGI�� "΢�ں�+ϵͳ���+Ӧ�ò��" �ṹ

	��ܷ�Ϊ��������Ͳ������������, �������Զ������ڣ����Զ�̬���ز��.

	1. ����ļ���
	   ����Linux�����Ķ�̬���ؼ���
	2. �����ע��
	   ���ñ������ķ���. ���Ĳ���ά��һ������ע���, ���ط���ӿھ���ʵ��.
	3. ����ĵ��� 
	   ��������Ļص�����ṩ�ķ���ӿں���
	4. ����Ĺ���
	   �����������������������, �������Ĵ����¼�
	5. �����ͨ��
	   ���֮��������ͨ��/·�ɻ���, �ο�[luotang.me---libipc]
	6. �����¼��Ĺ㲥�Ͷ���(���Ŀǰ��û�п���Ҫ֧��)
	   ���ù۲���ģʽ,�������м��ز����,ʵ���¼�ע��,�������ͨ������

    Discovery
    Registration
    Application hooks to which plugins attach
    Exposing application capabilities back to plugins

    Compile: gcc -fPIC -shared xxx.c -o xxx.so 
*/
	
#include <msf_utils.h>

typedef s32 (*svc_func)(void *data, u32 len);

typedef struct svc plugin;

struct svc {
    svc_func init;
    svc_func deinit;
    svc_func start;
    svc_func stop;
    svc_func get_param;
    svc_func set_param;
    svc_func msg_handler;
} __attribute__((__packed__));

