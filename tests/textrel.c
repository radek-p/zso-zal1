#include <stdio.h>
#include <string.h>
#include "hello.h"

static int var = 3;

int get_var() {
    return var;
}

void call_and_set() {
    var = strlen(gethellostr());
}
