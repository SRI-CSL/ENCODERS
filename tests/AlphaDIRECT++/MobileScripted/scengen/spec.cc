//
// spec.cc
//

#include "defs.h"
#include <ctype.h>

#include "spec.h"

// Constructor
Spec::Spec()
{
    file_ = NULL;
}

// Destructor
Spec::~Spec()
{
    if (file_ != NULL) {
        fclose(file_);
    }
}

//*************************
// Member functions
//*************************

// Open a file
// returns 1 on success, 0 otherwise
int Spec::openFile(char *filename)
{
    file_ = fopen(filename, "r");

    if (file_ == NULL) {
        fprintf(stderr, "Cannot open spec file: %s\n", filename);
        return 0;
    } else {
        //fprintf(stderr, "%s opened\n", filename);
        return 1;
    }
}

// Close the file opened
// returns 1 on success, 0 otherwise
int Spec::closeFile()
{
    if (file_ == NULL) {
        fprintf(stderr, "Warning: closeFile() called with no file opened\n");
        return 0;
    }

    int result = fclose(file_);
    file_ = NULL;

    //return 1;
    
    if (result == 0) {
        return 1;
    } else {
        return 0;
    }
}

// Removes all unecessary white spaces in a line for further processing
// returns the resulting line
char *Spec::removeSpace(char *line)
{
    char *result = NULL;
    int len = strlen(line);

    if (len <= 0) {       // sth is wrong, just return a null string
        result = new char[1];
        assert(result != NULL);
        result[0] = '\0';
        return result;
    }

    result = new char[len+3];
    assert(result != NULL);
    bzero(result, len+3);

    int i=0, j=0;       // keep track of indices in both string

    for (i=0; i<len; i++) {
        if (isspace((int)line[i])) {    // a space?
            continue;
        } else if (line[i] == '[') {
            result[j] = '[';j++;
            result[j] = ' ';j++;
        } else if (line[i] == ']') {
            result[j] = ' ';j++;
            result[j] = ']';j++;
        } else if (line[i] == '=') {
            result[j] = ' ';j++;
            result[j] = '=';j++;
            result[j] = ' ';j++;
        } else {
            result[j] = line[i];j++;
        }
    }

    return result;
}

// Gets a line from input file, 
// removes all comments, empty lines, and spaces within the line
// returns the result line on success, NULL otherwise
char *Spec::getLine()
{
    if (file_ == NULL) {
        fprintf(stderr, "Warning: trying to read file that is not opened\n");
        return NULL;
    }

    char buff[MAX_SPEC_LINE];
    char *result = NULL;

    while (fgets(buff, MAX_SPEC_LINE, file_) != NULL) {
        result = removeSpace(buff);
        if (strlen(result) == 0 || \
            result[0] == COMMENT_CHAR) {
            delete result;
            continue;
        } else {
            //printf("got line: %s\n", result);
            return result;
        }
    }

    return NULL;
}


