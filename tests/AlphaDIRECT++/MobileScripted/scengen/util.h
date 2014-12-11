//
// util.h
//

#ifndef _util_
#define _util_

#define MAX_STR_LEN 1024
#define MAX_LINE_LEN 80

struct ListElement
{
    char *key;
    void *value;
    struct ListElement *next;
};

class List
{
public:
    List(bool allowDuplicates = false);
    ~List();

    // access to the elements 
    void *get(char *key);               // get the value for the key
    int set(char* key, void* value);    // insert a new element

    // get the value and convert to the right format
    // Note: asserts are added
    int geti(char *key);
    long getl(char *key);
    double getf(char *key);
    char *gets(char *key);

    // iterator
    char* nextKey();
    void* nextValue();
    void reset();           // reset the current element to head_

    // others
    void clear();           // removes all elements
    int getCount();         // get the size of the list

protected:
    bool allowDuplicates_;  // allows elements with the same key?
    int count_;             // total # of elements
    ListElement *head_;     // head of the list
    ListElement *curr_;     // current element, used in iterator
};

class Util
{
public:
    static char *cloneStr(char *s);
};

#endif
