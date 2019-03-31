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
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <msf_process.h>
#include <msf_network.h>

s32 msf_lockfile(s32 fd) {
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return fcntl(fd, F_SETLK, &fl);
}

/* flock�����ͷŷǳ�������ɫ:
 * ���ɵ���LOCK_UN�������ͷ��ļ���
 * Ҳ����ͨ���ر�fd�ķ�ʽ���ͷ��ļ���
 * ��ζ��flock�����Ž��̵Ĺرն����Զ��ͷŵ� */
s32 process_try_lock(const s8 *proc_name, u32 mode) {

    s32 rc;
    s32 fd = -1;
    s8 proc_file[64] = { 0 };
    snprintf(proc_file, sizeof(proc_file)-1, "/var/run/%s", proc_name);

    /*Linux/Unix�ļ�ϵͳ�У���һ����������WRLCK��
    ֻ����һ�μ����ɹ������ҵ��������������˳����Ǳ����˳�,
    �����ɲ���ϵͳ�ͷš�
    ������������pid�ļ��ϣ��ر��ʺ��ڷ�ֹ�������̵Ķ���һ������
    ��ֹ�����������������ֻ�л��pid�ļ�(�̶�·���̶��ļ���)
    д��Ȩ��(F_WRLCK)�Ľ��̲��������������������PIDд����ļ��С�
    ����ͬһ������Ķ���������Զ��˳�

    flock���������������߱�ǿ����.
    һ������ʹ��flock���ļ���ס����һ�����̿���ֱ�Ӳ������ڱ������ļ���
    �޸��ļ��е����ݣ�ԭ������flockֻ�����ڼ���ļ��Ƿ񱻼�����
    ����ļ��Ѿ�����������һ������д�����ݵ������
    �ں˲�����ֹ������̵�д�������Ҳ���ǽ����������ں˴������

    flock��Ҫ���ֲ������ͣ�
    LOCK_SH����������������̿���ʹ��ͬһ������������������������
    LOCK_EX����������ͬʱֻ����һ������ʹ�ã���������д����
    LOCK_UN���ͷ���

    ����ʹ��flock�������ļ�ʱ������ļ��Ѿ�������������ס��
    ���̻ᱻ����ֱ�������ͷŵ��������ڵ���flock��ʱ��
    ����LOCK_NB�������ڳ�����ס���ļ���ʱ�򣬷����Ѿ�������������ס��
    �᷵�ش���errno������ΪEWOULDBLOCK��
    ���ṩ���ֹ���ģʽ�����������������

    flock�����ͷŷǳ�������ɫ�����ɵ���LOCK_UN�������ͷ��ļ�����
    Ҳ����ͨ���ر�fd�ķ�ʽ���ͷ��ļ�����flock�ĵ�һ��������fd��,
    ��ζ��flock�����Ž��̵Ĺرն����Զ��ͷŵ�

    flock���е�һ��ʹ�ó���Ϊ���������Ƿ��Ѿ�����

    �ڶ��߳̿����У��������������ڶ��ٽ���Դ�ı�������ֹ���ݵĲ�һ�£�
    ������Ϊ�ձ��ʹ�÷��������ڶ��������δ����ļ�֮���ͬ����


    flock()������

    flock()���õ�������������

    ֻ�ܶ������ļ����м��������ִ����ȵļ���������Э�����̼�Ĳ�����
    ������ڶ�����̣����и������̶���ͬʱ����ͬһ���ļ��Ĳ�ͬ���֡�
    ͨ��flock()ֻ�ܷ���Ȱ��ʽ����
    �ܶ�NFSʵ�ֲ�ʶ��flock()���õ�����
    ע�ͣ���Ĭ������£��ļ�����Ȱ��ʽ�ģ����ʾһ�����̿��Լ򵥵غ���
    ��һ���������ļ��Ϸ��õ�����Ҫʹ��Ȱ��ʽ����ģ���ܹ�����������
    ���з����ļ��Ľ��̶�����Ҫ��ϣ�����ִ���ļ�IO֮ǰ�ȷ���һ������
    */
    fd = open(proc_file, O_RDWR | O_CREAT | O_CLOEXEC, 
    				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        printf("Open prcoess lock file(%s) failed, errno(%d)\n", proc_name, errno);
        return true;
    }

    msf_socket_closeonexec(fd);

    if (0 == flock(fd, LOCK_EX | LOCK_NB)) {
        printf("(%s) has not been locked, lock it.\n", proc_file);
        rc = true;
    } else {
        printf("(%s) has locked.\n", proc_file);
        if (EACCES == errno || EAGAIN == errno || EWOULDBLOCK == errno) {

        }
        rc = false;
    }

    return rc;
}

void process_wait_child_termination(pid_t v_pid) {
    pid_t pid;
    s32 status;

    /*wait����ʹ����while��,��Ϊwait��������,���統ĳ�ӽ���һֱ��ֹͣ��
    ����һֱ����,�Ӷ�Ӱ���˸�����.����ʹ��waitpid��options����ΪWNOHANG,
    �����ڷ���������´������˳��ӽ���*/
    while ((pid = waitpid(v_pid, &status, WNOHANG))) {
        if (pid == -1)  {
            if (errno == EINTR)
                continue;
            else
                break;
        }
    }

    if (WIFEXITED(status)) {
        printf("Child(%d) normal exit(%d)\n", v_pid, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child abnormally exit, signal(%d)\n", WTERMSIG(status));
    }
}



void process_reap_children(void)
{

}
#include <sys/socket.h>
#include <sys/prctl.h>

//http://www.cnblogs.com/mickole/p/3188321.html
s32 process_spwan_grandson(s32 wfd, struct process_desc *proc_desc) {

    pid_t grandson;

    grandson = fork();
    if (grandson > 0) {
        printf("Now child process exit, pid(%d).\n", getpid());
        /*�ӽ����˳�,���ӽ�����init�����й�,���⽩ʬ����*/
        exit(0);
    } else if (0 == grandson) {

        printf("Now grandson process(%s) pid(%d) enter.\n", 
        proc_desc->proc_name, getpid());

        pid_t pid = getpid();
        while (send(wfd, (s8*)&pid, sizeof(pid), MSG_NOSIGNAL) != sizeof(pid_t));

        s8 *argv[] = { proc_desc->proc_path, "-c", proc_desc->proc_conf, NULL };// �������һλ��ҪΪ0
        execvp(proc_desc->proc_path, argv);

        //execvp
        //execϵͳ���û�ӵ�ǰ�����аѵ�ǰ����Ļ���ָ�������
        //Ȼ���ڿյĽ������������ʱָ���ĳ������
        execv(proc_desc->proc_path, argv);
        printf("Now grandson process pid(%d) exit.\n", getpid());
        exit(-1);
    }

    return getpid();
}

s32 process_spwan(struct process_desc *proc_desc) {

        s32 rc;
        s32 fd[2];
        pid_t child;

        pid_t exit_child;
        s32 status;

#if 0
        if (proc_desc->restart_times++ > 0) {
            rc = process_try_lock(proc_desc->proc_name, 0);
            if (false == rc) {
                if (proc_desc->proc_pid > 0) {
                    printf("Now kill old process, pid(%d).\n", proc_desc->proc_pid);
                    kill(proc_desc->proc_pid, SIGKILL);
                }
            }
        }
#endif

    /*
      Making the second socket nonblocking is a bit subtle, given that we
      ignore any EAGAIN returns when writing to it, and you don't usally
      do that for a nonblocking socket. But if the kernel gives us EAGAIN,
      then there's no need to add any more data to the buffer, since
      the main thread is already either about to wake up and drain it,
      or woken up and in the process of draining it.
    */

    //socketpair(AF_UNIX, SOCK_STREAM, 0, ngx_processes[s].channel)

    //rc = pipe(fd);
    rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
    if (rc < 0) {  
        perror ("pipe error\n");  
        return -1;  
    }

    //nolocking fd 0,1
#if 0
	 /* 
            �����첽ģʽ�� ������Կ��¡������̾�һ����ioctl������fcntl���� or ���ϲ�ѯ 
          */  
        on = 1; // ���λ��ioctl���������0�������ã���0������  

        /* 
          ����channel[0]���ź������첽I/O��־ 
          FIOASYNC����״̬��־�����Ƿ���ȡ���socket���첽I/O�źţ�SIGIO�� 
          ����O_ASYNC�ļ�״̬��־��Ч����ͨ��fcntl��F_SETFL��������or��� 
         */ 
        if (ioctl(ngx_processes[s].channel[0], FIOASYNC, &on) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "ioctl(FIOASYNC) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        /* F_SETOWN������ָ������SIGIO��SIGURG�źŵ�socket����������ID�������ID�� 
          * ������˼��ָ��Master���̽���SIGIO��SIGURG�ź� 
          * SIGIO�źű�������socket����Ϊ�ź������첽I/O���ܲ���������һ������ 
          * SIGURG�ź������µĴ������ݵ���socketʱ������ 
         */ 
        if (fcntl(ngx_processes[s].channel[0], F_SETOWN, ngx_pid) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(F_SETOWN) failed while spawning \"%s\"", name);
            ngx_close_channel(ngx_processes[s].channel, cycle->lo;
            return NGX_INVALID_PID;
        }
            
        
        /* FD_CLOEXEC�����������ļ���close-on-exec״̬��׼ 
          *             ��exec()���ú�close-on-exec��־Ϊ0������£����ļ������رգ���������exec()�󱻹ر� 
          *             Ĭ��close-on-exec״̬Ϊ0����Ҫͨ��FD_CLOEXEC���� 
          *     ������˼�ǵ�Master������ִ����exec()���ú󣬹ر�socket        
          */
        if (fcntl(ngx_processes[s].channel[0], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }

        
        /* ͬ�ϣ�������˼�ǵ�Worker�ӽ���ִ����exec()���ú󣬹ر�socket */  
        if (fcntl(ngx_processes[s].channel[1], F_SETFD, FD_CLOEXEC) == -1) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            ngx_close_channel(ngx_processes[s].channel, cycle->log);
            return NGX_INVALID_PID;
        }
#endif

    child = fork();
    if (child > 0) {
        sclose(fd[1]);
        proc_desc->proc_fd = fd[0];

        while (recv(fd[0], &proc_desc->proc_pid, 
            sizeof(pid_t), 0) != sizeof(pid_t));

        process_wait_child_termination(child);

        printf("Parent recv real %s pid(%d).\n",  proc_desc->proc_name, proc_desc->proc_pid);

        return proc_desc->proc_pid;	

    } else if (0 == child) {

            printf("Child name %s pid(%d).\n",  proc_desc->proc_name, getpid());

            sclose(fd[0]);

            /* set files mask */
            umask(0);
            if (setsid() == -1) exit(0);

            signal(SIGHUP, SIG_IGN);
            signal(SIGTTOU, SIG_IGN); //���Ժ�̨����д�����ն��ź�
            signal(SIGTTIN, SIG_IGN); //���Ժ�̨���̶������ն��ź�
            signal(SIGTSTP, SIG_IGN); //�����ն˹���

            process_spwan_grandson(fd[1], proc_desc);

        } else {
            perror("fork error\n");  
        }

        return 0;
}


