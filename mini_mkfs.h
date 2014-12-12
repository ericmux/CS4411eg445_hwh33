#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "minithread.h"
#include "synch.h"
#include "disk.h"

#define DISK_NAME "MINIFILESYSTEM"

/*
* Formats the filesystem, creating a disk with disk_size blocks named DISK_NAME.
*/
void mkfs(int dsk_siz);
