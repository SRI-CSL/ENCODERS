/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#ifndef _SCRATCHPAD_H
#define _SCRATCHPAD_H

template <class T> class Scratchpad;
class ScratchpadString;
class ScratchpadDouble;

#include "Attribute.h"
#include "DataObject.h"

template <class T>
class Scratchpad
{
private:
    typedef Pair<string, T> scratchpad_attribute_t;
    typedef Reference<scratchpad_attribute_t> ScratchpadAttributeRef;
    typedef ReferenceList<scratchpad_attribute_t> ScratchpadAttributeRefList;
    typedef HashMap<string, ScratchpadAttributeRefList> scratchpad_t;
    scratchpad_t scratchpad;
    Mutex scratchpad_lock;
public:
    Scratchpad() {};
    virtual ~Scratchpad() {};

    void setScratchpadAttribute(string dataobject_id, string name, T value) {
        Mutex::AutoLocker l(scratchpad_lock);

        typename Scratchpad<T>::scratchpad_t::iterator it = scratchpad.find(dataobject_id);
        if (it == scratchpad.end()) {
            ScratchpadAttributeRef attr = new Pair<string, T>(name, value);
            ScratchpadAttributeRefList attrs;
            attrs.add(attr);
            scratchpad.insert(make_pair(dataobject_id, attrs));
            return;
        }

        typename Scratchpad<T>::ScratchpadAttributeRefList& attrs = (*it).second;

        for (typename Scratchpad<T>::ScratchpadAttributeRefList::iterator itt = attrs.begin(); 
             itt != attrs.end(); itt++) {
            typename Scratchpad<T>::ScratchpadAttributeRef& attr = (*itt);
            if (attr->first == name) {
                attr->second = value;
                return;
            }
        }

        typename Scratchpad<T>::ScratchpadAttributeRef attr = new Pair<string, T>(name, value);
        attrs.add(attr);
    }

    bool hasScratchpadAttribute(string dataobject_id, string name) {
        Mutex::AutoLocker l(scratchpad_lock);

        typename Scratchpad<T>::scratchpad_t::iterator it = scratchpad.find(dataobject_id);
        if (it == scratchpad.end()) {
            return false;
        }

        typename Scratchpad<T>::ScratchpadAttributeRefList& attrs = (*it).second;
        for (typename Scratchpad<T>::ScratchpadAttributeRefList::iterator itt = attrs.begin(); 
             itt != attrs.end(); itt++) {
            typename Scratchpad<T>::ScratchpadAttributeRef& attr = (*itt);
            if (attr->first == name) {
                return true;
            }
        }

        return false;
    }

    T getScratchpadAttribute(string dataobject_id, string name) {
        Mutex::AutoLocker l(scratchpad_lock);

        typename Scratchpad<T>::scratchpad_t::iterator it = scratchpad.find(dataobject_id);
        if (it == scratchpad.end()) {
            return getDefaultValue();
        }

        typename Scratchpad<T>::ScratchpadAttributeRefList& attrs = (*it).second;

        for (typename Scratchpad<T>::ScratchpadAttributeRefList::iterator itt = attrs.begin(); 
             itt != attrs.end(); itt++) {
            typename Scratchpad<T>::ScratchpadAttributeRef& attr = (*itt);
            if (attr->first == name) {
                return attr->second;
            }
        }

        return getDefaultValue();
    }

    void removeScratchpadAttribute(string dataobject_id, string name) {
        Mutex::AutoLocker l(scratchpad_lock);

        typename Scratchpad<T>::scratchpad_t::iterator it = scratchpad.find(dataobject_id);
        if (it == scratchpad.end()) {
            return;
        }

        typename Scratchpad<T>::ScratchpadAttributeRefList& attrs = (*it).second;

        for (typename Scratchpad<T>::ScratchpadAttributeRefList::iterator itt = attrs.begin(); 
             itt != attrs.end(); itt++) {
            typename Scratchpad<T>::ScratchpadAttributeRef& attr = (*itt);
            if (attr->first == name) {
                attrs.erase(itt);
                return;
            }
        }
    }

    void removeScratchpadDataObject(string dataobject_id) {
        Mutex::AutoLocker l(scratchpad_lock);

        typename Scratchpad<T>::scratchpad_t::iterator it = scratchpad.find(dataobject_id);
        if (it == scratchpad.end()) {
            return;
        }

        typename Scratchpad<T>::ScratchpadAttributeRefList& attrs = (*it).second;
        while (!attrs.empty()) {
            typename Scratchpad<T>::ScratchpadAttributeRefList::iterator itt = attrs.begin(); 
            attrs.erase(itt);
        }
        scratchpad.erase(it);
    }

    void setScratchpadAttribute(DataObjectRef& dObj, string name, T value) {
        return this->setScratchpadAttribute(DataObject::idString(dObj), name, value);
    }

    bool hasScratchpadAttribute(DataObjectRef& dObj, string name) {
        return this->hasScratchpadAttribute(DataObject::idString(dObj), name);
    }

    T getScratchpadAttribute(DataObjectRef& dObj, string name) {
        return this->getScratchpadAttribute(DataObject::idString(dObj), name);
    }

    void removeScratchpadAttribute(DataObjectRef &dObj, string name) {
        return this->removeScratchpadAttribute(DataObject::idString(dObj), name);
    }

    void removeScratchpadDataObject(DataObjectRef &dObj) {
        this->removeScratchpadDataObject(DataObject::idString(dObj));
    }

    DataObjectRef getScratchpadDataObject() {
        Mutex::AutoLocker l(scratchpad_lock);

        DataObjectRef dObj = DataObject::create();

        for (typename Scratchpad<T>::scratchpad_t::iterator it = scratchpad.begin();
             it != scratchpad.end(); it++) {

            string dataobject_id = (*it).first;
            typename Scratchpad<T>::ScratchpadAttributeRefList& attrs = (*it).second;
            for (typename Scratchpad<T>::ScratchpadAttributeRefList::iterator itt = attrs.begin(); 
                 itt != attrs.end(); itt++) {
                typename Scratchpad<T>::ScratchpadAttributeRef& attr = (*itt);
                string key = attr->first;
                T value = attr->second;
                // TODO: check if they contain "|"
                // munge id and key
                dObj->addAttribute(dataobject_id + "|" + key, serializeValue(value));
            }
        }
        return dObj;
    }

    void loadScratchpadDataObject(DataObjectRef dObj) {
        Mutex::AutoLocker l(scratchpad_lock);

        const Attributes *attrs = dObj->getAttributes();

        for (Attributes::const_iterator it = attrs->begin(); 
             it != attrs->end(); it++) {
            const Attribute& a = (*it).second;
            string munge = a.getName();
            size_t found = munge.find_first_of("|");
            string dataobject_id = munge.substr(0, found);
            string key = munge.substr(found);
            T value = unserializeValue(a.getValue());
            setScratchpadAttribute(dataobject_id, key, value);
        }
    }

    virtual T getDefaultValue() = 0;
    virtual string serializeValue(T value) = 0;
    virtual T unserializeValue(string value) = 0;
};

class ScratchpadString : public Scratchpad<string>
{
public:
    ScratchpadString() : Scratchpad<string>() {};
    string getDefaultValue() { return string(""); }
    string serializeValue(string value) { return value; }
    string unserializeValue(string value) { return value; }

};

class ScratchpadDouble : public Scratchpad<double>
{
public:
    ScratchpadDouble() : Scratchpad<double>() {};
    double getDefaultValue() { return 0; }
    string serializeValue(double value) { return string(""); }
    double unserializeValue(string value) { return 0; }
};

#endif /* _SCRATCHPAD_H */
