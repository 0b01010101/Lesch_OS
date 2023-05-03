
#include "sync.h"

bool get_mtx(mutex_s *mtx, bool wait) {

    bool var = true;

    do {
        asm volatile ("xchg (,%1,), %0":"=a"(var):"b"(mtx), "a"(var));

    } while (var && wait);

    return !var;
}

void clean_mtx(mutex_s *mtx) {

    *mtx = false;
    return;
}
//---------------------------------------------------------------------------------
