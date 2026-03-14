/*
 * mem_profiler.h
 *
 * 内存与性能诊断工具 - 用于追踪1600x600迁移后zkgui内存异常增长问题
 *
 * 设计目标:
 *   1. 零侵入: 所有日志通过宏控制, 发布版本 #define MEM_PROFILER_ENABLE 0 即可完全消除
 *   2. 低开销: 仅在关键路径采样, 不引入额外线程、不分配堆内存
 *   3. 精准定位: 按 Activity 生命周期 (init/show/hide/quit) + 定时器 + 图片加载 三个维度采集
 *
 * 使用方式:
 *   在需要诊断的 Logic.cc 文件顶部:
 *     #include "utils/mem_profiler.h"
 *   然后在关键位置调用 MEM_SNAP / MEM_IMG_LOAD / MEM_TIMER_SNAP 等宏
 *
 * 日志格式 (便于 grep 过滤):
 *   [MEM] tag | VSZ:xxxKB RSS:xxxKB FreeMem:xxxKB | context_info
 *   [MEM_IMG] tag | file=xxx size=xxxKB decode_ms=xxx | FreeMem:xxxKB
 *   [MEM_TMR] tag | timer_id=x | FreeMem:xxxKB
 */

#ifndef _MEM_PROFILER_H_
#define _MEM_PROFILER_H_

/* ============================================================================
 * 总开关 - 发布时设为 0
 * ============================================================================ */
#define MEM_PROFILER_ENABLE  1

#if MEM_PROFILER_ENABLE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include "utils/Log.h"

/* --------------------------------------------------------------------------
 * 内部实现 - 不要直接调用, 使用下面的宏
 * -------------------------------------------------------------------------- */

// 获取当前进程的 VSZ 和 RSS (单位: KB)
static inline void _mem_prof_get_proc_mem(long *vsz_kb, long *rss_kb) {
    *vsz_kb = 0;
    *rss_kb = 0;
    FILE *f = fopen("/proc/self/statm", "r");
    if (f) {
        long pages_vsz = 0, pages_rss = 0;
        if (fscanf(f, "%ld %ld", &pages_vsz, &pages_rss) == 2) {
            long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
            *vsz_kb = pages_vsz * page_size_kb;
            *rss_kb = pages_rss * page_size_kb;
        }
        fclose(f);
    }
}

// 获取系统可用内存 (单位: KB)
static inline long _mem_prof_get_free_mem_kb() {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return (long)(si.freeram / 1024);
    }
    return -1;
}

// 获取单调时间戳 (毫秒)
static inline long long _mem_prof_get_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// 获取文件大小 (KB)
static inline long _mem_prof_file_size_kb(const char *path) {
    if (!path) return 0;
    struct stat st;
    if (stat(path, &st) == 0) {
        return (long)(st.st_size / 1024);
    }
    return 0;
}

// 获取 /proc/self/status 中的详细内存信息 (VmPeak, VmSize, VmRSS, VmData, VmStk, VmLib)
static inline void _mem_prof_dump_vm_detail(const char *tag) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Vm", 2) == 0) {
            // 去掉行末换行
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            LOGD("[MEM_DETAIL] %s | %s", tag, line);
        }
    }
    fclose(f);
}

/* ============================================================================
 * 公开宏 - 在 Logic 文件中使用
 * ============================================================================ */

/*
 * MEM_SNAP(tag, fmt, ...)
 * 通用内存快照 - 记录当前进程 VSZ/RSS 和系统可用内存
 * 用于 onUI_init / onUI_show / onUI_hide / onUI_quit 等关键生命周期节点
 *
 * 示例: MEM_SNAP("main_onUI_show", "page=%d", _current_page_index);
 */
#define MEM_SNAP(tag, fmt, ...) \
    do { \
        long _vsz, _rss; \
        _mem_prof_get_proc_mem(&_vsz, &_rss); \
        long _free = _mem_prof_get_free_mem_kb(); \
        LOGD("[MEM] %s | VSZ:%ldKB RSS:%ldKB FreeMem:%ldKB | " fmt, \
             tag, _vsz, _rss, _free, ##__VA_ARGS__); \
    } while(0)

/*
 * MEM_SNAP_SIMPLE(tag)
 * 简化版 - 不需要额外格式参数
 */
#define MEM_SNAP_SIMPLE(tag) \
    do { \
        long _vsz, _rss; \
        _mem_prof_get_proc_mem(&_vsz, &_rss); \
        long _free = _mem_prof_get_free_mem_kb(); \
        LOGD("[MEM] %s | VSZ:%ldKB RSS:%ldKB FreeMem:%ldKB", \
             tag, _vsz, _rss, _free); \
    } while(0)

/*
 * MEM_IMG_LOAD_BEGIN(var)
 * MEM_IMG_LOAD_END(tag, var, filepath)
 * 图片加载前后对比 - 追踪 setBackgroundPic / setBackgroundBmp / loadBitmapFromFile 的内存开销
 *
 * 示例:
 *   MEM_IMG_LOAD_BEGIN(t1);
 *   mCarPlayButtonPtr->setBackgroundPic(path);
 *   MEM_IMG_LOAD_END("dock1_carplay", t1, path);
 */
#define MEM_IMG_LOAD_BEGIN(var) \
    long long var##_ms = _mem_prof_get_ms(); \
    long var##_vsz_before, var##_rss_before; \
    _mem_prof_get_proc_mem(&var##_vsz_before, &var##_rss_before)

#define MEM_IMG_LOAD_END(tag, var, filepath) \
    do { \
        long _vsz_after, _rss_after; \
        _mem_prof_get_proc_mem(&_vsz_after, &_rss_after); \
        long long _elapsed = _mem_prof_get_ms() - var##_ms; \
        long _free = _mem_prof_get_free_mem_kb(); \
        long _file_kb = _mem_prof_file_size_kb(filepath); \
        LOGD("[MEM_IMG] %s | file=%s disk:%ldKB decode_ms:%lld | " \
             "VSZ_delta:%+ldKB RSS_delta:%+ldKB FreeMem:%ldKB", \
             tag, filepath ? filepath : "NULL", _file_kb, _elapsed, \
             _vsz_after - var##_vsz_before, \
             _rss_after - var##_rss_before, _free); \
    } while(0)

/*
 * MEM_TIMER_SNAP(tag, timer_id)
 * 定时器回调中的快照 - 轻量, 仅记录 FreeMem 防止定时器中高频打印影响性能
 * 建议: 每 10 次调用才采样一次, 用 MEM_TIMER_SNAP_SAMPLED
 */
#define MEM_TIMER_SNAP(tag, timer_id) \
    do { \
        long _free = _mem_prof_get_free_mem_kb(); \
        LOGD("[MEM_TMR] %s | timer_id=%d | FreeMem:%ldKB", tag, timer_id, _free); \
    } while(0)

/*
 * MEM_TIMER_SNAP_SAMPLED(tag, timer_id, interval)
 * 采样版定时器快照 - 每 interval 次才真正打印一次
 */
#define MEM_TIMER_SNAP_SAMPLED(tag, timer_id, interval) \
    do { \
        static int _cnt_##timer_id = 0; \
        if ((++_cnt_##timer_id) % (interval) == 0) { \
            MEM_TIMER_SNAP(tag, timer_id); \
        } \
    } while(0)

/*
 * MEM_VM_DETAIL(tag)
 * 详细转储 /proc/self/status 中的所有 Vm 开头的行
 * 开销较大, 仅在关键节点使用 (如 onUI_init, 倒车进入/退出)
 */
#define MEM_VM_DETAIL(tag) _mem_prof_dump_vm_detail(tag)

/*
 * MEM_LIFECYCLE(activity, event)
 * Activity 生命周期的标准化记录
 * event: "init" / "show" / "hide" / "quit"
 */
#define MEM_LIFECYCLE(activity, event) \
    MEM_SNAP(activity "_" event, "lifecycle=%s", event)

/*
 * MEM_WARN_IF_LOW(tag, threshold_kb)
 * 低内存预警 - 当可用内存低于阈值时打印警告
 */
#define MEM_WARN_IF_LOW(tag, threshold_kb) \
    do { \
        long _free = _mem_prof_get_free_mem_kb(); \
        if (_free < (threshold_kb)) { \
            long _vsz, _rss; \
            _mem_prof_get_proc_mem(&_vsz, &_rss); \
            LOGE("[MEM_LOW] %s | FreeMem:%ldKB < threshold:%dKB | VSZ:%ldKB RSS:%ldKB", \
                 tag, _free, (int)(threshold_kb), _vsz, _rss); \
            _mem_prof_dump_vm_detail(tag "_LOW"); \
        } \
    } while(0)

/*
 * MEM_TRANSITION(from, to)
 * 页面切换时的内存记录
 */
#define MEM_TRANSITION(from, to) \
    MEM_SNAP("transition", "from=%s to=%s", from, to)

#else /* MEM_PROFILER_ENABLE == 0 */

/* 发布版本 - 所有宏展开为空, 零开销 */
#define MEM_SNAP(tag, fmt, ...)              ((void)0)
#define MEM_SNAP_SIMPLE(tag)                 ((void)0)
#define MEM_IMG_LOAD_BEGIN(var)              ((void)0)
#define MEM_IMG_LOAD_END(tag, var, filepath) ((void)0)
#define MEM_TIMER_SNAP(tag, timer_id)        ((void)0)
#define MEM_TIMER_SNAP_SAMPLED(tag, timer_id, interval) ((void)0)
#define MEM_VM_DETAIL(tag)                   ((void)0)
#define MEM_LIFECYCLE(activity, event)        ((void)0)
#define MEM_WARN_IF_LOW(tag, threshold_kb)   ((void)0)
#define MEM_TRANSITION(from, to)             ((void)0)

#endif /* MEM_PROFILER_ENABLE */

#endif /* _MEM_PROFILER_H_ */
