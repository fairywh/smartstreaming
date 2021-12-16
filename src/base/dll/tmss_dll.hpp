/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#pragma once
#include <string.h>
#include<dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>


typedef void (*init_user_control_func)(int num, char** param);

typedef struct {
    void *handle_user_control;
    init_user_control_func init_user_control;
} tmss_dll_func_t;

extern tmss_dll_func_t tmss_dll;

int load_dll(const char * file);
