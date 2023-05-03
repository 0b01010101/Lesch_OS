#ifndef SYNC_H
#define SYNC_H

#include "common.h"

typedef bool mutex_s;

bool get_mtx(mutex_s *mtx, bool wait);
void clean_mtx(mutex_s *mtx);

#endif
