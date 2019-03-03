
#include <msf_utils.h>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/uio.h>

#include <limits.h>             /* IOV_MAX */
#include <sys/sendfile.h>
#include <sys/eventfd.h>
#include <linux/aio_abi.h>

#define MSF_LISTEN_BACKLOG  64


//��Ƶ����ʹ��Ƶ�����ַ�ʽͨ��ǰ���뷢�͵�����
#define MSF_CMD_OPEN_CHANNEL   1
//�ر��Ѿ��򿪵�Ƶ����ʵ����Ҳ���ǹر��׽���
#define MSF_CMD_CLOSE_CHANNEL  2
//Ҫ����շ��������˳�����
#define MSF_CMD_QUIT           3
//Ҫ����շ�ǿ�Ƶؽ�������
#define MSF_CMD_TERMINATE      4
//Ҫ����շ����´򿪽����Ѿ��򿪹����ļ�
#define MSF_CMD_REOPEN         5


/* ��װ�˸��ӽ���֮�䴫�ݵ���Ϣ
 * �����̴������ӽ��̺�,ͨ������Ѹ��ӽ��̵�
 * �����Ϣȫ�����ݸ��������н���,���������ӽ��̾�֪���ý��̵�channel��Ϣ��.
 * ��ΪNginx�������Ƶ��ͬ��master������worker���̼��״̬ */

struct msf_channel {	
	 u32  	cmd;
     pid_t  pid;
     s32    slot; 	/*��ȫ�ֽ��̱��е�λ��*/
     s32    fd; 	/* ���ݽ����ļ������� */
} __attribute__((__packed__));


s32 msf_write_channel(s32 fd, struct msf_channel *ch, size_t size);
s32 msf_read_channel(s32 fd, struct msf_channel *ch, size_t size);
s32 msf_add_channel_event(s32 fd, s32 event);
void msf_close_channel(s32 *fd);


