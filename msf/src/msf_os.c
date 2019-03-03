
#include <msf_os.h>

s32  msf_ncpu; //cpu����
s32  msf_max_sockets;//ÿ�������ܴ򿪵�����ļ���
u32  msf_inherited_nonblocking;
u32  msf_tcp_nodelay_and_tcp_nopush;


/* ÿ�������ܴ򿪵�����ļ��� */
struct rlimit  rlmt;

/* ����һ����ҳ�Ĵ�С����λΪ�ֽ�(Byte).
 * ��ֵΪϵͳ�ķ�ҳ��С,��һ�����Ӳ����ҳ��С��ͬ*/
u32  msf_pagesize;

/* pagesizeΪ4M, pagesize_shiftӦ��Ϊ12 
 * pagesize������λ�Ĵ���, ��for (n = ngx_pagesize; 
 * n >>= 1; ngx_pagesize_shift++) {  }
 */
u32  msf_pagesize_shift;

/*
 * �����֪��CPU cache�еĴ�С,��ô�Ϳ���������Ե������ڴ�Ķ���ֵ,
 * ����������߳����Ч��.�з����ڴ�صĽӿ�, Nginx�Ὣ�ڴ�ر߽�
 * ���뵽 CPU cache�д�С32λƽ̨, cacheline_size=32 */
u32 msf_cacheline_size;


u8  msf_linux_kern_ostype[50];
u8  msf_linux_kern_osrelease[50];


int msf_osinfo() {

    struct utsname	u;   
    if (uname(&u) == -1) {
       return -1;
    }

    printf("u.sysname:%s\n", u.sysname); //��ǰ����ϵͳ��
    printf("u.nodename:%s\n", u.nodename); //�����ϵ�����
    printf("u.release:%s\n", u.release); //��ǰ��������
    printf("u.version:%s\n", u.version); //��ǰ�����汾
    printf("u.machine:%s\n", u.machine); //��ǰӲ����ϵ����
    //printf("u.__domainname:%s\n", u.__domainname); //��ǰӲ����ϵ����

#if _UTSNAME_DOMAIN_LENGTH - 0
#ifdef __USE_GNU
    //printf("u.domainname::%s\n ", u.domainname);
    //char domainname[_UTSNAME_DOMAIN_LENGTH]; //��ǰ����
#else
    printf("u.__domainname::%s\n", u.__domainname);
    //char __domainname[_UTSNAME_DOMAIN_LENGTH];
#endif
#endif
    return 0;
}

s32 msf_set_user(struct process *proc) {

    s8               *group = NULL;
    struct passwd    *pwd = NULL;
    struct group     *grp = NULL;

     if (proc->user != (uid_t) MSF_CONF_UNSET_UINT) {
         printf("is duplicate\n");
         return 0;
    }

    if (geteuid() != 0) {
        printf(
           "the \"user\" directive makes sense only "
           "if the master process runs "
           "with super-user privileges, ignored\n");
        return 0;
    }

    pwd = getpwnam((const s8 *) proc->username);
    if (pwd == NULL) {
        return -1;
    }

   	proc->user = pwd->pw_uid;

    grp = getgrnam(group);
    if (grp == NULL) {      
        return -1;
    }

    proc->group = grp->gr_gid;

    return 0;

}

#if (MSF_HAVE_CPUSET_SETAFFINITY)

#include <sys/cpuset.h>

void msf_setaffinity(u64 cpu_affinity) {
    cpuset_t    mask;
    u32  i;

    printf("cpuset_setaffinity(0x%08Xl)\n", cpu_affinity);

    CPU_ZERO(&mask);
    i = 0;
    do {
        if (cpu_affinity & 1) {
            CPU_SET(i, &mask);
        }
        i++;
        cpu_affinity >>= 1;
    } while (cpu_affinity);

    if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
                           sizeof(cpuset_t), &mask) == -1)
    {

    }
}

#elif (MSF_HAVE_SCHED_SETAFFINITY)

void msf_setaffinity(u64 cpu_affinity)
{
    cpu_set_t   mask;
    u32  i;

    printf("sched_setaffinity(%ld)\n", cpu_affinity);

    CPU_ZERO(&mask);
    i = 0;
    do {
        if (cpu_affinity & 1) {
            CPU_SET(i, &mask);
        }
        i++;
        cpu_affinity >>= 1;
    } while (cpu_affinity);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
    }
}

#endif


s32 msf_set_priority(struct process *proc) {

    setpriority(PRIO_PROCESS, 0, proc->priority);

    return 0;
}

s32 msf_set_rlimit(struct process *proc) {

    struct rlimit     rlmt;

    if (proc->rlimit_nofile != -1) {
        rlmt.rlim_cur = (rlim_t) proc->rlimit_nofile;
        rlmt.rlim_max = (rlim_t) proc->rlimit_nofile;

        //RLIMIT_NOFILEָ���˽��̿ɴ򿪵�����ļ������ʴ�һ��ֵ��������ֵ���������EMFILE����
        if (setrlimit(RLIMIT_NOFILE, &rlmt) == -1) {
        }
    }

    if (proc->rlimit_core != -1) {
        rlmt.rlim_cur = (rlim_t) proc->rlimit_core;
        rlmt.rlim_max = (rlim_t) proc->rlimit_core;
        //�޸Ĺ������̵�core�ļ��ߴ�����ֵ����(RLIMIT_CORE)�������ڲ����������̵��������������ơ�
        if (setrlimit(RLIMIT_CORE, &rlmt) == -1) {
        }

	}


    if (geteuid() == 0) {
        if (setgid(proc->group) == -1) {
            /* fatal */
            exit(2);
        }

        if (initgroups(proc->username, proc->group) == -1) {
        }

        if (setuid(proc->user) == -1) {
            /* fatal */
            exit(2);
        }
    }


    msf_setaffinity(proc->cpu_affinity);

    /* allow coredump after setuid() in Linux 2.4.x */
    msf_enable_coredump();

    return 0;
}


/**
 * RAM
 */
s32 msf_get_meminfo(struct msf_meminfo *mem) {

    FILE *fp = NULL;                      
    s8 buff[256];
                                                                                                         
    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        return -1;
    }

    msf_memzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff), fp); 
    sscanf(buff, "%s %le %s*", mem->name1, &mem->total, mem->name2); 

    msf_memzero(buff, sizeof(buff));
    fgets(buff, sizeof(buff), fp);
    sscanf(buff, "%s %le %s*", mem->name1, &mem->free, mem->name2); 

    mem->used_rate= (1 - mem->total/ mem->total) * 100;

    sfclose(fp);

    return 0;
}

s32 msf_get_hdinfo(struct msf_hdinfo *hd) {

    FILE *fp = NULL;
    s8 buffer[80],a[80],d[80],e[80],f[80], buf[256];
    double c,b;
    double dev_total = 0, dev_used = 0;

    fp = popen("df", "r");
    if (!fp) {
        return -1;
    }

    fgets(buf, sizeof(buf), fp);
    while (6 == fscanf(fp, "%s %lf %lf %s %s %s", a, &b, &c, d, e, f)) {
        dev_total += b;
        dev_used += c;
    }

    hd->total = dev_total / (1024*1024);
    hd->used_rate = dev_used/ dev_total * 100;

    pclose(fp);

    return 0;
}

s32 msf_os_init(void) {

    u32  n;

    struct utsname  u;
    
    if (uname(&u) == -1) {
        return -1;
    }

    (void) memcpy(msf_linux_kern_ostype, (u8 *) u.sysname,
                       sizeof(msf_linux_kern_ostype));

    (void) memcpy(msf_linux_kern_osrelease, (u8 *) u.release,
                       sizeof(msf_linux_kern_osrelease));

    msf_pagesize = getpagesize();

    for (n = msf_pagesize; n >>= 1; msf_pagesize_shift++) { /* void */ }

    if (msf_ncpu == 0) {
        msf_ncpu = sysconf(_SC_NPROCESSORS_ONLN); //��ȡϵͳ�п��õ� CPU ����, û�б������ CPU ��ͳ�� ����, ��������Ӻ�û�м����. 
    }


    if (msf_ncpu < 1) {
        msf_ncpu = 1;
    }

    msf_cpuinfo();

    if (getrlimit(RLIMIT_NOFILE, &rlmt) == -1) {  
          return -1;
    }

    msf_max_sockets = (s32) rlmt.rlim_cur;

    return 0;
}
