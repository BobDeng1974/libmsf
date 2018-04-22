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

/*  Open Shared Object
	RTLD_LAZY 	�ݻ�����,������Ҫʱ�ٽ������ 
����RTLD_NOW 	��������,����ǰ�������δ�����ķ��� 
����RTLD_LOCAL  ��RTLD_GLOBAL�����෴,
				��̬���ж���ķ��Ų��ܱ����򿪵��������ض�λ
				���û��ָ����RTLD_GLOBAL����RTLD_LOCAL,��ȱʡΪRTLD_LOCAL.
����RTLD_GLOBAL ����������  ��̬���ж���ķ��ſɱ����򿪵��������ض�λ
����RTLD_GROUP 
����RTLD_WORLD 
	RTLD_NODELETE:��dlclose()�ڼ䲻ж�ؿ�,
				�������Ժ�ʹ��dlopen()���¼��ؿ�ʱ����ʼ�����еľ�̬����
				���flag����POSIX-2001��׼�� 
	RTLD_NOLOAD:�����ؿ�
				�����ڲ��Կ��Ƿ��Ѽ���(dlopen()����NULL˵��δ����,
				����˵���Ѽ��أ�,Ҳ�����ڸı��Ѽ��ؿ��flag
				��:��ǰ���ؿ��flagΪRTLD��LOCAL
				��dlopen(RTLD_NOLOAD|RTLD_GLOBAL)��flag�����RTLD_GLOBAL
				���flag����POSIX-2001��׼
	RTLD_DEEPBIND:������ȫ�ַ���ǰ���������ڵķ���,����ͬ�����ŵĳ�ͻ
				���flag����POSIX-2001��׼��		
*/

#include <stdio.h>
#include <dlfcn.h>

/* Linux dl API*/
#ifndef WIN32  
#define DLHANDLE void*
#define DLOPEN_L(name) dlopen((name), RTLD_LAZY | RTLD_GLOBAL)
#define DLOPEN_N(name) dlopen((name), RTLD_NOW)


#if defined(WIN32)
#define DLSYM(handle, symbol) 	GetProcAddress((handle), (symbol))
#define DLCLOSE(handle)			FreeLibrary(handle)
#else
#define DLSYM(handle, symbol) 	dlsym((handle), (symbol))
#define DLCLOSE(handle) 		dlclose(handle)
#endif
#define DLERROR() dlerror()
#endif

void* plugin_load_dynamic(const char* path);

void* plugin_load_symbol(void* handler, const char* symbol);



