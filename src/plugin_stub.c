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

/*
	������һ������������ foo.������ͬʱʹ��test1.o��test2.o,
	������÷���foo,����ʹ�õ����� func.c�ж���ĺ���,
	������ __foo, ��Ȼ����һ������ foo.Ҳ����˵, 
	��������ʹ�õ��ĺ�������"ʵ������"���Ǹ�����
	��Ȼ, ����ʹ�� dummy.o ���ӵĻ�ʹ�õ����Ǹ��������¡��ĺ���.
	���test2.o �е� foo ���� weak symbol �Ļ�,������ʱ�������ͻ,���������Ҫʹ�� weak ��ԭ��
	
	glibc ��ʵ�����澭���� weak alias,�������� socket ������
	�� C �ļ�������ῴ��һ�� __socket ������������ʲô��û����,ֻ��������һЩ�������,����Щ��������.
	��ͬһ�� C �ļ������������һ�� __socket �� weak alias ���� socket.
	ʵ����ɹ����Ĵ���ͨ�������ʵ�֣�������Ļ���ļ������������ϵͳ���ú�,
	ִ�� sysenter ���� int �ȶ���ȥ����ϵͳ����.
	��ǰ�� glibc ����ϵͳ���õ�ʵ�ֵ�ʱ�����ƹ��ܾ�,�����Ǹ�ʱ���֪���� weak alias ���������
	
	����weak���η���ԭ����������weak�����͵ĺ������Ա�����ͬ���������ǣ������ᷢ����ͻ����
	���û������ͬ����������ʹ�ø�weak��������������Ĭ�Ϻ�����
	���ǣ��ܵ����⡱��һ�ر���ڵ�Ӱ�죬weak�����ͶԿ��á���̫�ɿ����ˣ�
	���Ҹ���Weak definitions aren��t so weak�����������°汾��glibc��weak���η�����Ծ�̬�����Ӱ�졣

	weakref������˵��weak��alias�Ľ�ϰ汾��������Ӣ�����ã������ڵ�ǰ����weakref���εķ��ű�����static�ġ�
	http://gcc.gnu.org/onlinedocs/gcc-4.4.0/gcc/Function-Attributes.html 

	Weak Alias �� gcc ��չ��Ķ�����ʵ�����Ǻ���������
	��������ڿ��ʵ��������ܻᾭ���õ������� glibc ��������˲��١�
	��¼һ�� gcc �ֲ�����Ļ������º��������Ǹ�ɶ��
	In GNU C, you declare certain things about functions called in your program 
	which help the compiler optimize function calls and check your code more carefully.
	*/
	
#include "plugin_stub.h"

int __foo() {
	printf("I do no thing1.\n");
	return 0;
}
int foo() __attribute__ ((weak, alias("__foo")));
//��Ч  asm(".weak foo\n\t .set symbol1, __foo\n\t");

/*
	weak �� alias �ֱ����������ԡ�
	weak ʹ�� foo ���������Ŀ���ļ�����Ϊ weak symbol ������ global symbol��
	�� nm ����鿴���� dummy.c ���ɵ�Ŀ���ļ����ÿ��� foo ��һ�� weak symbol��
	��ǰ��ı���� W������������weak����ʱ,��ʹ����û���壬����������Ҳ���Ա���ɹ���
	00000000 T __foo 00000000 W foo U puts
	
	�� alias ��ʹ foo �� __foo ��һ������,__foo �� foo ������ͬһ�����뵥Ԫ�ж���,�����������
	
	/�ܽ�weak����
	//��1��asm(".weak symbol1\n\t .set symbol1, symbol222\n\t");��
	//     void symbol1() __attribute__ ((weak,alias("symbol222")));��Ч��
	//��2������������weak����ʱ����ʹ����û���壬����������Ҳ���Ա���ɹ���
	//��3��������������ͬ��ʱ����ʹ��ǿ���ţ�Ҳ��ȫ�ַ���,��û�м�weak�ĺ����������������ţ���weak�ĺ�������
	//��4��������û�ж��壬������ǡ�ĳ���������ı���ʱ������ú��������ã��ͼ�ӵ��á�ĳ����������
	
	*/



int plugin_func_stub(const char* func, ...)
{
	printf("plugin_func_stub func[%s] 401 \n", func);
	return 401;	
}


