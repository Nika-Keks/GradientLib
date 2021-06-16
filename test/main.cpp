#include "../include/ILogger.h"
#include <iostream>
#include <math.h>
#include <functional>

void testIVector();

void testISet();

namespace comp{
    void testICompact();
}


int main()
{
    //testIVector();

    //testISet();

    comp::testICompact();

    return 0;
}
