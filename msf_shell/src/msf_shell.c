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
#include <cJSON.h>
#include <msf_log.h>
#include <msf_os.h>
#include <msf_svc.h>

#define MSF_MOD_SHELL "PROCESS"
#define MSF_SHELL_LOG(level, ...) \
    log_write(level, MSF_MOD_SHELL, MSF_FUNC_FILE_LINE, __VA_ARGS__)


#define DEF_PROC_KEY_NAME       "proc_name"
#define DEF_PROC_KEY_AUTHOR     "proc_author"
#define DEF_PROC_KEY_VERSION    "proc_version"
#define DEF_PROC_KEY_DESC       "proc_desc"
#define DEF_PROC_KEY_SVCS       "proc_svcs"

#define DEF_SVC_KEY_NAME        "svc_name"
#define DEF_SVC_KEY_LIBS        "svc_libs"

static struct process gproc;
struct process *g_proc = &gproc;


/**
 * 1����������ѡ��Ĳ�����
 * 2������¼��һ������λ��;
 * 3��ʾ�����Ƿ񽫴�����Ϣ�����stderr,Ϊ0ʱ��ʾ�����
 * 4��ʾ����ѡ���ַ���optstring�е�ѡ��
 * hvdc:k:? ��ʾʲô��?
 * �����һ��ѡ���ַ���,��Ӧ�������о���-h,-v,-d,-c,-k .
 * ð������ʲô��?
 * ð�ű�ʾ����,һ��ð�žͱ�ʾ���ѡ����������в���(�������Ż��߿ո����)
 * ����ð�ŵľͱ�ʾ���ѡ��Ĳ����ǿ�ѡ��,�������в���,Ҳ����û�в���
 * ��Ҫע���в���ʱ,������ѡ��֮�䲻���пո�,��һ���һ��ð��ʱ���������*/
extern s8 *optarg;
extern s32 optind;
extern s32 opterr;
extern s32 optopt;

extern s32 msf_init_setproctitle(s32 argc, s8 *argv[]);
extern void msf_set_proctitle(s32 argc, s8 *argv[], s8 *title);

static s32 config_parse_svc(cJSON *svc_array, s32 proc_idx) {

    cJSON *tmp = NULL;
    cJSON *svc_node = NULL;
    struct svcinst *svc = NULL;

    svc = &(g_proc->proc_svcs[proc_idx]);
    svc_node = cJSON_GetArrayItem(svc_array, proc_idx);
    if (!svc_node) {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(svc_node, DEF_SVC_KEY_NAME);
    if (tmp) {
        memcpy(svc->svc_name, tmp->valuestring, 
            min(sizeof(svc->svc_name)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(svc_node, DEF_SVC_KEY_LIBS);
    if (tmp) {
        memcpy(svc->svc_lib, tmp->valuestring, 
            min(sizeof(svc->svc_lib)-1, strlen(tmp->valuestring)));
    } else {
        goto cleanup;
    }

    return 0;

cleanup:
    return -1;
}

static s32 config_parse_proc(const s8 *confbuff) {

    s32 rc = -1;
    u32 proc_idx;	
    cJSON *tmp = NULL;
    cJSON *root = NULL;
    cJSON *svc_array = NULL;

    root = cJSON_Parse(confbuff);
    if (!root) {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse buff to json object.");
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_PROC_KEY_NAME);
    if (tmp) {
        memcpy(g_proc->proc_name, tmp->valuestring, 
            min(sizeof(g_proc->proc_author)-1, strlen(tmp->valuestring)));
    } else {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse author json object.");
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_PROC_KEY_AUTHOR);
    if (tmp) {
        memcpy(g_proc->proc_author, tmp->valuestring, 
            min(sizeof(g_proc->proc_author)-1, strlen(tmp->valuestring)));
    } else {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse author json object.");
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_PROC_KEY_VERSION);
    if (tmp) {
        memcpy(g_proc->proc_version, tmp->valuestring, 
            min(sizeof(g_proc->proc_version)-1, strlen(tmp->valuestring)));
    } else {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse version json object.");
        goto cleanup;
    }

    tmp = cJSON_GetObjectItem(root, DEF_PROC_KEY_DESC);	
    if (tmp) {
        memcpy(g_proc->proc_desc, tmp->valuestring, 
            min(sizeof(g_proc->proc_desc)-1, strlen(tmp->valuestring)));
    } else {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse desc json object.");
        goto cleanup;
    }

    svc_array = cJSON_GetObjectItem(root, DEF_PROC_KEY_SVCS);
    if (svc_array) {
        g_proc->proc_svc_num = cJSON_GetArraySize(svc_array);		
    } else {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse svcs json object.");
        goto cleanup;
    }

    if (0 == g_proc->proc_svc_num) {
        MSF_SHELL_LOG(DBG_ERROR, "Process svc num zreo.");
        goto cleanup;
    }

    g_proc->proc_svcs = MSF_NEW(struct svcinst, g_proc->proc_svc_num);
    if (!g_proc->proc_svcs) {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to new svcs object.");
        goto cleanup;
    } else {
        memset(g_proc->proc_svcs, 0, 
        sizeof(struct svcinst) * g_proc->proc_svc_num);
    }

    for (proc_idx = 0; proc_idx < g_proc->proc_svc_num; proc_idx++) {		
        rc = config_parse_svc(svc_array, proc_idx);
        if (rc < 0) {
            MSF_SHELL_LOG(DBG_ERROR, "Failed to parse svc %d.", proc_idx);
            sfree(g_proc->proc_svcs);
            goto cleanup;
        }
    }

    return 0;
cleanup:
    cJSON_Delete(root);
    return rc;
}

s32 config_init(s8 *conf_file) {

    if (unlikely(!conf_file)) {
        MSF_SHELL_LOG(DBG_ERROR, "Config file is null.");
        return -1;
    }

    s8 *confbuff = NULL;

    confbuff = config_read_file(conf_file);
    if (!confbuff) {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to read %s.", conf_file);
        return -1;
    }

    if (config_parse_proc(confbuff) < 0) {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to parse %s.", conf_file);
        sfree(confbuff);
        return -1;
    }

    sfree(confbuff);
    return 0;
}

s32 logger_init(void) {

    return 0;
}

s32 service_init(void) {

    u32 svc_idx;
    struct svc *svc_cb;

    for (svc_idx = 0; svc_idx < g_proc->proc_svc_num; svc_idx++) {
        MSF_SHELL_LOG(DBG_ERROR, "Start to load svc %d.", svc_idx);
        if (msf_svcinst_init(&(g_proc->proc_svcs[svc_idx])) < 0) {
            MSF_SHELL_LOG(DBG_ERROR, "Failed to load svc %d.", svc_idx);
            //return -1;
        };
    }

    for (svc_idx = 0; svc_idx < g_proc->proc_svc_num; svc_idx++) {
        svc_cb = g_proc->proc_svcs[svc_idx].svc_cb;
        if (svc_cb->init(NULL, 0) < 0) {
            MSF_SHELL_LOG(DBG_ERROR, "Failed to init svc %d.", svc_idx);
            return -1;
        }
    }
    return 0;
}

s32 process_init(s32 argc, s8 *argv[]) {

    s32 rc;

    //process_try_lock(g_proc->proc_name, 1);

    if (msf_init_setproctitle(argc, argv) < 0) {
        MSF_SHELL_LOG(DBG_ERROR, "Failed to init process title.");
        return -1;
    }

    msf_set_proctitle(argc, argv, g_proc->proc_name);

    if (service_init() < 0) {
        return -1;
    }

    return 0;
}

void show_version(void) {
    MSF_SHELL_LOG(DBG_INFO, 
        "process name:%s \n"
        "process author:%s \n"
        "process version: %s\n"
        "process desc:%s\n",
        g_proc->proc_name,
        g_proc->proc_author,
        g_proc->proc_version,
        g_proc->proc_desc);
}

void show_usage(void) {
    MSF_SHELL_LOG(DBG_INFO, 
            "Usage: %s [-?hvdc] [-d level] [-c config-file] [-k signal]\n"
            "       -h        Print help message.\n"
            "       -v        Show Version and exit.\n"
            "       -d        daemon mode.\n"
            "       -c file   Use given config-file instead of\n"
            "                 %s\n"
            "       -k reload|rotate|kill|parse\n"
            "                 kill is fast shutdown\n"
            "                 Parse configuration file, then send signal to \n"
            "                 running copy (except -k parse) and exit.\n",
            g_proc->proc_name, g_proc->confile);
    exit(1);
}

s32 options_parse(s32 argc, s8 *argv[]) {
    s32 c;

    MSF_SHELL_LOG(DBG_INFO, "optind:%d, opterr: %d\n", optind, opterr);

    //getopt_long
    while ((c = getopt(argc, argv, "hvdc:k:?")) != -1) {
        switch (c) {
            case 'h':
            case 'v':
                show_usage();
                break;
            case 'd':
                g_proc->proc_daemon = true;
                break;
            case 'c':
                g_proc->confile = strdup(optarg);
                break;
            case 'k':
                if ((s32) strlen(optarg) < 1)
                   show_usage();
                if (!msf_strncmp(optarg, "reload", strlen(optarg)))
                   g_proc->proc_signo = SIGHUP;
                else if (!msf_strncmp(optarg, "rotate", strlen(optarg)))
                   g_proc->proc_signo = SIGUSR1;
                else if (!msf_strncmp(optarg, "shutdown", strlen(optarg)))
                   g_proc->proc_signo = SIGTERM;
                else if (!msf_strncmp(optarg, "kill", strlen(optarg)))
                   g_proc->proc_signo = SIGKILL;
                else
                   show_usage();
                break;
            case 'm':
                if ((s32) strlen(optarg) < 1)
                   show_usage();
                if (!msf_strncmp(optarg, "reload", strlen(optarg)))
                   g_proc->proc_signo = SIGHUP;
                else
                   show_usage();
                break;
            case '?':
            default:
                MSF_SHELL_LOG(DBG_ERROR, "Unknown option: %c.",(s8)optopt);
                show_usage();
                break;
        }
    }

    return 0;
}


static  __attribute__((constructor(101))) void before1()
{
 
    MSF_SHELL_LOG(DBG_ERROR, "before1");
}
static  __attribute__((constructor(102))) void before2()
{
 
    MSF_SHELL_LOG(DBG_ERROR, "before2.");
}


/* �����ʾһ�������ķ���ֵֻ�ɲ�������, �����������Ļ�,
    �Ͳ��ٵ��ô˺�����ֱ�ӷ���ֵ.�����ҵĳ��Է��ֻ��ǵ�����,
    ���־������Ϸ���Ҫ��gcc��һ��-O�Ĳ����ſ���.
    �ǶԺ������õ�һ���Ż�*/
__attribute__((const)) s32 test2()
{
    return 5;
}

/* ��ʾ�����ķ���ֵ���뱻����ʹ��,����ᾯ��*/
__attribute__((unused)) s32 test3()
{
    return 5;
}



/* ��δ����ܹ���֤������������,��Ϊ�����ֻ���������Ļ�,
	����������һ�����������ķ�ʽ����,
	�������̫���������������Ҳ��һ����������������Ļ���ǿ������*/
static inline __attribute__((always_inline)) void test5()
{

}

__attribute__((destructor)) void after_main()  
{  
   MSF_SHELL_LOG(DBG_ERROR, "after main.");  
} 


/* ./msf_shell -c msf_daemon_config.json */
s32 main(s32 argc, s8 *argv[]) {

    if (unlikely((3 != argc && 2 != argc) || (3 == argc && !argv[2]))) {
        return -1;
    }

    s8 log_path[256] = { 0 };

    snprintf(log_path, sizeof(log_path)-1, "logger/%s.log", "AGENT");

    if (log_init(log_path) < 0) {
        return -1;
    }

    if (options_parse(argc, argv) < 0) {
        return -1;
    }

    memset(g_proc, 0, sizeof(struct process));
    g_proc->user  = MSF_CONF_UNSET_UINT;
    g_proc->group = MSF_CONF_UNSET_UINT;

    MSF_SHELL_LOG(DBG_DEBUG, "Msf shell init pid(%d) argv(%s).", getpid(), argv[2]);

    if (config_init(argv[2]) < 0) {
        return -1;
    }

    MSF_SHELL_LOG(DBG_DEBUG, "Msf config init sucessful.");

    if (likely(g_proc->logfd) > 0) {
        //msf_set_stderr(g_proc->logfd);
    }

    if (process_init(argc, argv) < 0) {
        return -1;
    }

    MSF_SHELL_LOG(DBG_DEBUG, "Msf process init sucessful.");

    for ( ;; ) {
        sleep(1);
    }

    return 0;
}

