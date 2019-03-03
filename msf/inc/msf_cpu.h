#include <msf_utils.h>


/* CPU��Ϣ����
 * (user, nice, system,idle, iowait, irq, softirq, stealstolen, guest)��9Ԫ��
 *
 * ϵͳ���� : top����鿴���������ʵʱϵͳ��Ϣ
 * 1. CPUʹ����: cat /proc/stat 
 *    �û�ģʽ(user)
 *    �����ȼ����û�ģʽ(nice)
 *    �ں�ģʽ(system)
 *    ���еĴ�����ʱ��(idle)
 *    CPU������ = 100*(user+nice+system)/(user+nice+system+idle)
 *    ��һ��: cpu 711 56 2092 7010 104 0 20 0 0 0
 *    ���湲��10��ֵ (��λ:jiffies)ǰ��8��ֵ�ֱ�Ϊ:
 *	                      
 *    User time, 711(�û�ʱ��)             Nice time, 56 (Niceʱ��)
 *    System time, 2092(ϵͳʱ��)          Idle time, 7010(����ʱ��)
 *    Waiting time, 104(�ȴ�ʱ��)          Hard Irq time, 0(Ӳ�жϴ���ʱ��)
 *    SoftIRQ time, 20(���жϴ���ʱ��)     Steal time, 0(��ʧʱ��)
 *
 *    CPUʱ��=user+system+nice+idle+iowait+irq+softirq+Stl
 *
 *
 * 2. �ڴ�ʹ�� : cat /proc/meminfo
 *    ��ǰ�ڴ��ʹ����(cmem)�Լ��ڴ�����(amem)
 *    �ڴ�ʹ�ðٷֱ� = 100 * (cmem / umem) 
 * 3. ���縺�� : cat /proc/net/dev
 *    �ӱ�����������ݰ���,���뱾�������ݰ���
 *    ƽ�����縺�� = (��������ݰ�+��������ݰ�)/2
 */

#define CPU_KEY_LEN     16
#define MOD_NAME_CPU 	"CPU"
 
struct cpu_key {
 	u32 length;
 	s8  name[CPU_KEY_LEN];
};

 struct cpu_snapshot {
    s8	cpuid[20];
	unsigned long user;
	unsigned long nice;
	unsigned long system;
	unsigned long idle;
	unsigned long iowait;

	/* percent values */
	unsigned long irq;	/* Overall CPU usage */
	unsigned long softirq;/* user space (user + nice) */
	unsigned long stealstolen; /* kernel space percent     */
	unsigned long guest;

	double avg_all;
	double avg_user;
	double avg_kernel;
	/* necessary... */
    struct cpu_key k_cpu;
    struct cpu_key k_user;
    struct cpu_key k_system;
}__attribute__((__packed__));


/* Default collection time: every 1 second (0 nanoseconds) */
#define DEFAULT_INTERVAL_SEC    1
#define DEFAULT_INTERVAL_NSEC   0
#define CPU_SNAP_ACTIVE_A    	0
#define CPU_SNAP_ACTIVE_B    	1

#if 0
struct cpu_snapshot {
    /* data snapshots */
    s8  v_cpuid[8];
    u32 v_user;
    u32 v_nice;
    u32 v_system;
    u32 v_idle;
    u32 v_iowait;

    /* percent values */
    double p_cpu;           /* Overall CPU usage        */
    double p_user;          /* user space (user + nice) */
    double p_system;        /* kernel space percent     */

    /* necessary... */
    struct cpu_key k_cpu;
    struct cpu_key k_user;
    struct cpu_key k_system;
};
#endif

struct cpu_stats {
    u32 snap_active;

    /* CPU snapshots, we always keep two snapshots */
    struct cpu_snapshot *snap_a;
    struct cpu_snapshot *snap_b;
};

/* CPU Input configuration & context */
struct cpu_config {
    /* setup */
	long processors_configured;
    long processors_avalible;   /* number of processors currently online (available) */
    long cpu_ticks;      		/* CPU ticks (Kernel setting) */
	long total_pages;
	long free_pages;
	long page_size;
	long long total_mem;
	long long free_mem ;
	long total_fds;				/* one process open file fd MAX %ld */

	long n_processors;
    s32 interval_sec;    		/* interval collection time (Second) */
    s32 interval_nsec;  	 	/* interval collection time (Nanosecond) */
    struct cpu_stats cstats;
};


