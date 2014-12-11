/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

/*
 * testcallback.c
 *
 *  Created on: Aug 1, 2012
 *      Author: jjoy
 */




#include <stdio.h>

void myprint(void);
int runcallback(int(*foo)(int));


void myprint()
{
    printf("hello world\n");
}

int runcallback(int(*foo)(int)) {
    printf("running callback\n");
    foo(1);
    printf("done running callback\n");
    return 0;
}
