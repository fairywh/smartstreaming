/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */
#include "tmss_dll.hpp"
#include "defs/err.hpp"


#define PROCTECT_AREA_SIZE (512*1024)

tmss_dll_func_t tmss_dll;

int load_dll(const char * file) {
    memset(&tmss_dll, 0x0, sizeof(tmss_dll_func_t));

    bool isGlobal = false;
    int flag = isGlobal ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW;
    mmap(NULL, PROCTECT_AREA_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    void* handle = dlopen(file, flag);
    mmap(NULL, PROCTECT_AREA_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (!handle) {
        printf("[ERROR]dlopen %s failed, errmsg:%s\n", file, dlerror());
        exit(-1);
    }
    void* func = dlsym(handle, "init_user_control");
    if (func != NULL) {
        tmss_dll.init_user_control = (init_user_control_func)dlsym(handle,
            "init_user_control");
        if (tmss_dll.handle_user_control!= NULL) {
            dlclose(tmss_dll.handle_user_control);
        }
        tmss_dll.handle_user_control = handle;
    } else {
    }
    return 0;
}
