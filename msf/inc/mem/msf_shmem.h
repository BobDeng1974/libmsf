
#ifndef __MSF_SHMEM_H__
#define __MSF_SHMEM_H__

#include <msf_utils.h>

typedef struct {
/*
�����ڴ����ʵ��ַ��ʼ������:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(����ʵ�ʵ����ݲ��֣�
ÿ��ngx_pagesize����ǰ���һ��ngx_slab_page_t���й�������ÿ��ngx_pagesize��ǰ�˵�һ��obj��ŵ���һ�����߶��int����bitmap�����ڹ���ÿ������ȥ���ڴ�)
*/ //��ngx_init_zone_pool�������ڴ����ʼ��ַ��ʼ��sizeof(ngx_slab_pool_t)�ֽ��������洢�������ڴ��slab poll��
    u8      *addr; //�����ڴ���ʼ��ַ  
    size_t  size; //�����ڴ�ռ��С
    s8      *name; //��鹲���ڴ������
    u32     exists;   /* unsigned  exists:1;  */ //��ʾ�����ڴ��Ƿ��Ѿ�������ı�־λ��Ϊ1ʱ��ʾ�Ѿ�����
} ngx_shm_t;


s32 ngx_shm_alloc(ngx_shm_t *shm);
void ngx_shm_free(ngx_shm_t *shm);

