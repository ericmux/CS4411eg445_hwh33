#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>

/* if debuging is desired set value to 1 */
#define DEBUG 0

/* for now kernel printfs are just regular printfs */
#define kprintf printf

/* Macro to clean up the code for waiting on mutexes */
#define WaitOnObject(mutex)                                      \
    if (WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0) { \
        printf("Error: code %ld.\n", GetLastError());            \
        exit(1);                                                 \
    }

#define AbortOnCondition(cond,message)                       \
    if (cond) {                                              \
        printf("Abort: %s:%d %d, MSG:%s\n",                  \
               __FILE__, __LINE__, GetLastError(), message); \
        exit(1);                                             \
    }

#define AbortOnError(fctcall)                         \
    if (fctcall == 0) {                               \
        printf("Error: file %s line %d: code %ld.\n", \
               __FILE__, __LINE__, GetLastError());   \
        exit(1);                                      \
    }


#endif /* _CONFIG_H_ */
