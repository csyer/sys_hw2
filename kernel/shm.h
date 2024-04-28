#ifndef _SHM_H_
#define _SHM_H_

// 定义常量，例如最大共享内存段数
#define MAX_SHM_SEGMENTS 16

#define SHM_GET_PERM 1  // 命令码：获取共享内存权限
#define SHM_SET_PERM 2  // 命令码：设置共享内存权限

// 共享内存段的结构定义
struct shm_segment {
    int id;
    int refcount;
    uint64 addr;
    int permission;
    int used;
};

#endif // _SHM_H_
