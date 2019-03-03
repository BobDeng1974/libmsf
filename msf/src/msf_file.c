
#include <msf_file.h>


s32 msf_open_tempfile(u8 *name, u32 persistent, u32 access)
{
    s32  fd;

    fd = open((const s8 *) name, O_CREAT|O_EXCL|O_RDWR,
              access ? access : 0600);

    if (fd != -1 && !persistent) {
		msf_delete_file(name);
    }

    return fd;
}


ssize_t msf_read_file(s32 fd, u8 *buf, size_t size, off_t offset)
{
    ssize_t  n;

#if (MSF_HAVE_PREAD)

    n = pread(fd, buf, size, offset);

    if (n == -1) {
        return -1;
    }

#else

    n = read(fd, buf, size);

    if (n == -1) {
        return -1;
    }


#endif

    return n;
}


ssize_t msf_write_file(s32 fd, u8 *buf, size_t size, off_t offset)
{
    ssize_t  n, written;
	
    written = 0;

#if (MSF_HAVE_PWRITE)

    for ( ;; ) {
        /* pwrite() �ѻ����� buf ��ͷ�� count ���ֽ�д���ļ������� 
         * fd offset ƫ��λ����,�ļ�ƫ��û�иı�*/
        n = pwrite(fd, buf + written, size, offset);

        if (n == -1) {
            return -1;
        }

        if ((size_t) n == size) {
            return written;
        }

        offset += n;
        size -= n;
    }

#else


    for ( ;; ) {
        n = write(fd, buf + written, size);

        if (n == -1) {

            return -1;
        }
    }
#endif
}



s32 msf_set_file_time(u8 *name, s32 fd, time_t s) {

	struct timeval  tv[2];

    //tv[0].tv_sec = ngx_time();
    tv[0].tv_usec = 0;
    tv[1].tv_sec = s;
    tv[1].tv_usec = 0;

    if (utimes((s8 *) name, tv) != -1) {
        return -1;
    }

    return -1;

}


s32 msf_create_file_mapping(struct msf_file_mapping *fm) {

	fm->fd = msf_open_file(fm->name, MSF_FILE_RDWR, MSF_FILE_TRUNCATE,
						  MSF_FILE_DEFAULT_ACCESS);

	if (ftruncate(fm->fd, fm->size) == -1) {

 	}

    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
        return 0;
    }

error:
	msf_close_file(fm->fd);
	return -1;
}


void msf_close_file_mapping(struct msf_file_mapping *fm) {

	if (munmap(fm->addr, fm->size) == -1) {

	}

	if (msf_close_file(fm->fd) == -1) {
	}

}



s32 msf_open_glob(struct msf_glob *gl) {
    int  n;

    n = glob((char *) gl->pattern, 0, NULL, &gl->pglob);

    if (n == 0) {
        return 0;
    }

#ifdef GLOB_NOMATCH

    if (n == GLOB_NOMATCH && gl->test) {
        return 0;
    }

#endif

    return -1;
}

s32 msf_read_glob(struct msf_glob *gl, s8 *name) {
    size_t  count;

#ifdef GLOB_NOMATCH
    count = (size_t) gl->pglob.gl_pathc;
#else
    count = (size_t) gl->pglob.gl_matchc;
#endif

    if (gl->n < count) {

        s32 len = (size_t) strlen(gl->pglob.gl_pathv[gl->n]);
        u8 *data = (u8 *) gl->pglob.gl_pathv[gl->n];
        gl->n++;

        return 0;
    }

    return -1;
}


void msf_close_glob(struct msf_glob *gl) {
    globfree(&gl->pglob);
}

/* ����һ�Ѷ���,���ȴ� */
s32 msf_trylock_rfd(s32 fd, short start, short whence, short len) {

	struct flock lock;

	lock.l_type = F_RDLCK;
	lock.l_start = start;
	lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
	lock.l_len = len;
	lock.l_pid = getpid();
	/* ������ʽ���� */
	if(fcntl(fd, F_SETLK, &lock) == 0)
		return 1;

	return 0;
}

s32 msf_lock_rfd(s32 fd, short start, short whence, short len) {

	struct flock lock;

	lock.l_type = F_RDLCK;
	lock.l_start = start;
	lock.l_whence = whence;//SEEK_CUR,SEEK_SET,SEEK_END
	lock.l_len = len;
	lock.l_pid = getpid();
	/* ������ʽ���� */
	if(fcntl(fd, F_SETLKW, &lock) == 0)
		return 1;

	return 0;
}


s32 msf_trylock_wfd(s32 fd) {

	struct flock  fl;

	/* ����ļ��������������ļ��е�����,���Ϊ0 */
	msf_memzero(&fl, sizeof(struct flock));
	fl.l_type = F_WRLCK; /*F_WRLCK��ζ�Ų��ᵼ�½���˯��*/
	fl.l_whence = SEEK_SET;

	/* ��ȡ�������ɹ�ʱ�᷵��0,���򷵻ص���ʵ��errno������,
	 * �����������ΪEAGAIN����EACCESSʱ��ʾ��ǰû���õ�������,
	 * ���������Ϊfcntlִ�д���*/
	if (fcntl(fd, F_SETLK, &fl) == -1) {
	    return errno;
	}

	return 0;
}

/*
 * �ý����������̵�ִ��,ʹ��ʱ��Ҫ�ǳ�����,
 * �����ܻᵼ��worker��������˯��Ҳ����������������*/
s32 msf_lock_wfd(s32 fd) {

	struct flock  fl;

	msf_memzero(&fl, sizeof(struct flock));
	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;

	/* �������-1, ���ʾfcntlִ�д���
	 * һ������0, ��ʾ�ɹ����õ�����*/
	if (fcntl(fd, F_SETLKW, &fl) == -1) {
		return errno;
	}

	return 0;
}

s32 msf_unlock_fd(s32 fd) {

	struct flock  fl;

	msf_memzero(&fl, sizeof(struct flock));
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;

	if (fcntl(fd, F_SETLK, &fl) == -1) {
		return	errno;
	}

	return 0;
}


#if (MSF_HAVE_POSIX_FADVISE) && !(MSF_HAVE_F_READAHEAD)

s32 ngx_read_ahead(s32 fd, size_t n) {

	s32  err;

    err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    if (err == 0) {
        return 0;
    }

    errno = err;
    return -1;
}

#endif

#if (MSF_HAVE_O_DIRECT)

/* direct AIO���Բο�http://blog.csdn.net/bengda/article/details/21871413
 * ��ͨ����I/O�ŵ�:
 * ���� I/O ʹ���˲���ϵͳ�ں˻�����,��һ���̶��Ϸ�����Ӧ�ó���ռ��ʵ�ʵ������豸.
 * ���� I/O ���Լ��ٶ��̵Ĵ���, �Ӷ��������.
 * ��ͨ����I/O�ŵ�ȱ��:
 * �ڻ��� I/O ������, DMA ��ʽ���Խ�����ֱ�ӴӴ��̶���ҳ������,
 * ���߽����ݴ�ҳ����ֱ��д�ص�������,������ֱ����Ӧ�ó����ַ�ռ�ʹ���֮��������ݴ���,
 * �����Ļ�,�����ڴ����������Ҫ��Ӧ�ó����ַ�ռ��ҳ����֮����ж�����ݿ�������,
 * ��Щ���ݿ��������������� CPU �Լ��ڴ濪���Ƿǳ����.
 *
 * direct I/O�ŵ�:
 * ֱ�� I/O ����Ҫ���ŵ����ͨ�����ٲ���ϵͳ�ں˻�������Ӧ�ó����ַ�ռ�����ݿ�������,
 * �����˶��ļ���ȡ��д��ʱ�������� CPU ��ʹ���Լ��ڴ�����ռ��.
 * �����ĳЩ�����Ӧ�ó��򣬱����Ի���Ӧ�ó�����˵����ʧΪһ�ֺõ�ѡ��.
 * ���Ҫ������������ܴ�,ʹ��ֱ�� I/O �ķ�ʽ�������ݴ���,
 * ������Ҫ����ϵͳ�ں˵�ַ�ռ俽�����ݲ����Ĳ���,�⽫�����������.
 * direct I/Oȱ��:
 * ����ֱ�� I/O �Ŀ����ǳ���,��ֱ�� I/O �ֲ����ṩ���� I/O ������.
 * ���� I/O �Ķ��������ԴӸ��ٻ���洢���л�ȡ����,��ֱ��I/O �Ķ����ݲ�������ɴ��̵�ͬ����,
 * �����������ϵĲ���, ���ҵ��½�����Ҫ�ϳ���ʱ�����ִ����;
 */
s32 ngx_directio_on(s32 fd) {

	s32  flags;

	flags = fcntl(fd, F_GETFL);

	if (flags == -1) {
		return -1;
	}

	 return fcntl(fd, F_SETFL, flags | O_DIRECT);
}

s32 ngx_directio_off(s32 fd) {

	s32  flags;
	
	flags = fcntl(fd, F_GETFL);

	if (flags == -1) {
		return -1;
	}

	return fcntl(fd, F_SETFL, flags & ~O_DIRECT);
}

#endif


#if (MSF_HAVE_STATFS)

size_t msf_fs_bsize(u8 *name) {

	struct statfs  fs;

	if (statfs((char *) name, &fs) == -1) {
		return 512;
	}

	if ((fs.f_bsize % 512) != 0) {
		return 512;
	}

	return (size_t) fs.f_bsize; /*ÿ��block����������ֽ���*/
}

#elif (MSF_HAVE_STATVFS)

size_t msf_fs_bsize(u8 *name) {

	struct statvfs  fs;

    if (statvfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_frsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_frsize;
}

#else

size_t msf_fs_bsize(u8 *name) {
    return 512;
}

#endif


s32 msf_create_full_path(u8 *dir, u32 access)
{
    u8    *p, ch;
    s32   err;

    err = 0;

#if (MSF_WIN32)
    p = dir + 3;
#else
    p = dir + 1;
#endif

    for ( /* void */ ; *p; p++) {
        ch = *p;

        if (ch != '/') {
            continue;
        }

        *p = '\0';

        if (msf_create_dir(dir, access) == -1) {
            err = errno;

            switch (err) {
            case EEXIST:
                err = 0;
            case EACCES:
                break;

            default:
                return err;
            }
        }

        *p = '/';
    }

    return err;
}

s32 msf_create_paths(s8 *path, uid_t user) {

	msf_create_dir(path, 0700);

	struct stat  fi;

    if (msf_file_info((const s8 *) path, &fi) == -1) {
        return -1;
    }

	if (fi.st_uid != user) {
        if (chown((const char *) path, user, -1) == -1) {
            return -1;
        }
    }

    if ((fi.st_mode & (S_IRUSR|S_IWUSR|S_IXUSR)) != (S_IRUSR|S_IWUSR|S_IXUSR)) {
        fi.st_mode |= (S_IRUSR|S_IWUSR|S_IXUSR);

        if (chmod((const s8 *) path, fi.st_mode) == -1) {
            return -1;
        }
    }
	return 0;
}


s32 msf_create_pidfile(const s8 *name) {

	u8 pid[MSF_INT64_LEN + 2];
	s32 fd = msf_open_file(name, MSF_FILE_RDWR,
                            MSF_FILE_TRUNCATE, MSF_FILE_DEFAULT_ACCESS);

	return 0;
}

void msf_delete_pidfile(const s8 *name) {
	msf_delete_file(name);
}

pid_t msf_read_pidfile(void) {

	FILE *pid_fp = NULL;
    const s8 *f = "";
    pid_t pid = -1;
    s32 i;

    if (f == NULL) {
        fprintf(stderr, "error: no pid file name defined\n");
        exit(1);
    }

    pid_fp = fopen(f, "r");
    if (pid_fp != NULL) {
        pid = 0;
        if (fscanf(pid_fp, "%d", &i) == 1)
            pid = (pid_t) i;
        fclose(pid_fp);
    } else {
        if (errno != ENOENT) {
            fprintf(stderr, "error: could not read pid file\n");
            fprintf(stderr, "\t%s: %s\n", f, strerror(errno));
            exit(1);
        }
    }
    return pid;
}


s32 msf_check_runningpid(void) {
    pid_t pid;
    pid = msf_read_pidfile();
    if (pid < 2)
        return 0;
    if (kill(pid, 0) < 0)
        return 0;
    fprintf(stderr, "nginx_master is already running!  process id %ld\n", (long int) pid);
    return 1;
}

void msf_write_pidfile(void)
{
    FILE *fp;
    const char *f = NULL;
    fp = fopen(f, "w+");
    if (!fp) {
        fprintf(stderr, "could not write pid file '%s': %s\n", f, strerror(errno));
        return;
    }
    fprintf(fp, "%d\n", (int) getpid());
    fclose(fp);
}


void msf_enable_coredump(void) {

	#if HAVE_PRCTL && defined(PR_SET_DUMPABLE)
	
    /* Set Linux DUMPABLE flag */
    if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) != 0) {
        fprintf(stderr, "prctl: %s\n", strerror(errno));
	}

	#endif

    /* Make sure coredumps are not limited */
    struct rlimit rlim;

    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = rlim.rlim_max;
        if (setrlimit(RLIMIT_CORE, &rlim) == 0) {
            fprintf(stderr, "Enable Core Dumps OK!\n");
            return;
        }
    }
    fprintf(stderr, "Enable Core Dump failed: %s\n",strerror(errno));
}


