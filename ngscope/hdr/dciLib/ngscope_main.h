#ifndef NGSCOPE_MAIN_H
#define NGSCOPE_MAIN_H

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "ngscope_def.h"

#ifdef __cplusplus
extern "C" {
#endif

int ngscope_main(ngscope_config_t* config);

#ifdef __cplusplus
}
#endif

#endif
