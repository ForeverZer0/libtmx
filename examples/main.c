#include "TMX/tmx.h"
#include <stdio.h>

int main(int argc, const char *argv[])
{

    TMXmap map;
    map.name = "Hello world!";

    printf("%s\n", map.name);
    return 0;
}