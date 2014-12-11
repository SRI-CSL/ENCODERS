//
// util.cc
//

#include "defs.h"
#include "util.h"

// constructor
List::List(bool allowDuplicates)
{
    allowDuplicates_ = allowDuplicates;
    count_ = 0;
    head_ = NULL;
    curr_ = NULL;
}

// destructor
List::~List()
{
    clear();
}

// get the value of a key
void *List::get(char *key)
{
    if (count_ == 0) {
        //printf("List Empty!\n");
        return NULL;
    }

    ListElement *tmp = head_;
    while (tmp != NULL) {
        if (!strcmp(tmp->key, key)) {   // found
            return tmp->value;
        } else {
            tmp = tmp->next;
        }
    }

    // not found
    return NULL;
}

// get values and make conversions
int List::geti(char *key)
{
    char *value = (char *)get(key);
    assert(value != NULL);
    return atoi(value);
}

long List::getl(char *key)
{
    char *value = (char *)get(key);
    assert(value != NULL);
    return atol(value);
}

double List::getf(char *key)
{
    char *value = (char *)get(key);
    assert(value != NULL);
    return atof(value);
}

char *List::gets(char *key)
{
    char *value = (char *)get(key);
    assert(value != NULL);
    return value;
}

// insert a new element,
// returns 1 on success, 0 otherwise
int List::set(char *key, void *value)
{
    //printf ("Setting key = %s\n", key);

    if (key == NULL) return 0;
    int len = strlen(key);
    if (len == 0) return 0;

    ListElement *e = new ListElement();
    assert(e != NULL);

    e->key = Util::cloneStr(key);
    e->value = value;
    e->next = NULL;

    if (head_ == NULL) {    // the list was empty
        // insert the emlement at the begining
        head_ = e;
    } else { 
        ListElement *p1 = head_;
        ListElement *p2 = head_->next;

        int compare = 0;
        int inserted = 0;

        while (p2 != NULL) {
            compare = strcmp(key, p2->key);
            if (compare == 0 && !allowDuplicates_) { 
                // dumplicated key not allowed
                // replace the value
                delete p2->value;
                p2->value = value;
                delete e->key;
                delete e;
                inserted = 1;
                break;
            } else if (compare < 0) {   
                // correct place found
                // Note: if duplicated keys allowed, new elements 
                //       will always be inserted AFTER the previous 
                //       ones that have the same key.
                //       if we have ... compare <= 0 ... it is the reverse
                p1->next = e;
                e->next = p2;
                inserted = 1;
                break;
            }
            p1 = p1->next;
            p2 = p2->next;
        }

        if (inserted == 0) {        // come to the end of the list
            if (count_ == 1) {          // if only one element
                compare = strcmp (key, head_->key); 
                if (compare == 0) {         // dumplicated key
                    if (!allowDuplicates_) {
                        delete head_->key;
                        delete head_->value;
                        delete head_;
                        head_ = e;
                    } else {    
                        // duplicate keys allowed
                        // insert after head to keep consistency
                        head_->next = e;
                    }
                } else if (compare < 0) {   // insert before
                    e->next = head_;
                    head_ = e;
                } else {                    // insert after
                    head_->next = e;
                }
            } else {                    // more than 1 element
                p1->next = e;               // insert at the end
            }
        }
    }

    count_++;

    if (curr_ == NULL) {  // iterator should be initiated
        curr_ = head_;
    }

    return 1;
}

// get the next key in the list
char *List::nextKey()
{
    if (curr_ == NULL) {
        return NULL;
    }

    char *key = curr_->key;
    curr_ = curr_->next;
    return key;
}

// get the next value in the list
void *List::nextValue()
{
    if (curr_ == NULL) {
        return NULL;
    }

    void *value = curr_->value;
    curr_ = curr_->next;
    return value;
}

// resets the current point to the begining of the list
void List::reset()
{
    curr_ = head_;
}

// removes all elements in the list
void List::clear()
{
    ListElement *tmp;
    while (head_ != NULL) {
        tmp = head_->next;
        delete head_->key;
        delete head_->value;
        delete head_;
        head_ = tmp;
    }
    count_ = 0;
    curr_ = NULL;
}

// get the total number of elements in the list
int List::getCount()
{
    return count_;
}

char *Util::cloneStr(char *s)
{
    int len = strlen(s);
    assert(len <= MAX_STR_LEN);

    char *str = new char[len+1];
    assert(str != NULL);

    bzero(str, len+1);
    strcpy(str, s);

    return str;
}
