//
// spec.h
//

#ifndef _spec_
#define _spec_

#include <stdio.h>

#include "util.h"

#define MAX_SPEC_LINE MAX_LINE_LEN
#define COMMENT_CHAR '#'

class Spec
{
public:
    Spec();
    ~Spec();

    //virtual int load(char *filename);

protected:
    int openFile(char *filename);
    int closeFile();
    char *removeSpace(char *line);      // removes white spaces in a line
    char *getLine();                    // get a line from input file
                                        // with spaces removed

    FILE *file_;
};

#endif
