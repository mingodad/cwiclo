// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "../memblock.h"
using namespace cwiclo;

void PrintMemblock (const memblock& b)
{
    printf ("%s\n", b.begin());
}

int main (void)
{
    memblock b = {'a','b','c','d','e','f','g','h','i','h','\0'};
    PrintMemblock (b);
    return EXIT_SUCCESS;
}
