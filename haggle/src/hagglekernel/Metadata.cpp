/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

#include "Metadata.h"
#include <stdio.h>

Metadata::Metadata(const string _name, const string _content, Metadata *_parent) :
                parent(_parent), name(_name), content(_content)
{
}

Metadata::Metadata(const Metadata& m) : 
                parent(m.parent), name(m.name), content(m.content), 
                param_registry(m.param_registry), 
                registry()
{
        for (registry_t::const_iterator it = m.registry.begin(); it != m.registry.end(); it++) {
                const Metadata *m = (*it).second;
                Metadata *mcopy = m->copy();
                mcopy->parent = this;
                registry.insert(make_pair(m->name, mcopy));
        }
}

Metadata::~Metadata()
{
        while (!registry.empty()) {
                registry_t::iterator it = registry.begin();
                Metadata *m = (*it).second;
                registry.erase(it);
                delete m;                
        }
}

bool Metadata::isName(const string _name) const
{
	return name == _name;
}

bool Metadata::_addMetadata(Metadata *m)
{ 
        if (!m)
                return false;

        registry.insert(make_pair(m->name, m));

        return true;
}


bool Metadata::removeMetadata(const string name)
{
        bool ret = false;
        
        r = registry.equal_range(name);

         while (r.first != r.second) {
                 registry_t::iterator it = r.first;
                 Metadata *m = (*it).second;
                 r.first++;
                 registry.erase(it);
                 delete m;
                 ret = true;
         }
         return ret;
}

Metadata *Metadata::getMetadata(const string name, unsigned int n)
{
        r = registry.equal_range(name);

        if (r.first == r.second)
                return NULL;
        
        while (n) { 
                r.first++;
                n--;
                
                if (r.first == r.second)
                        return NULL;
        }
        return (*r.first).second;
}

const Metadata *Metadata::getMetadata(const string name, unsigned int n) const
{
        const_cast<Metadata*>(this)->r_const = registry.equal_range(name);

        if (r_const.first == r_const.second)
                return NULL;
        
        while (n) { 
                const_cast<Metadata*>(this)->r_const.first++;
                n--;
                
                if (r_const.first == r_const.second)
                        return NULL;
        }
        return (*r_const.first).second;
}

Metadata *Metadata::getNextMetadata()
{
        if (r.first == registry.end() || r.first == r.second)
                return NULL;
        
        r.first++;

        if (r.first == r.second)
                return NULL;

        return (*r.first).second;
}

const Metadata *Metadata::getNextMetadata() const
{
	if (r_const.first == registry.end() || r_const.first == r_const.second)
                return NULL;
        
        const_cast<Metadata*>(this)->r_const.first++;

        if (r_const.first == r_const.second)
                return NULL;

        return (*r_const.first).second;
}

string& Metadata::setParameter(const string name, const string value)
{
        Pair<parameter_registry_t::iterator, bool> p;

        p = param_registry.insert(make_pair(name, value));

        if (!p.second) {
                // Update value
                (*p.first).second = value;
        }
        return (*p.first).second;
}

string& Metadata::setParameter(const string name, const unsigned int value)
{
	char tmp[32];
	sprintf(tmp, "%u", value);
	return setParameter(name, tmp);
}

string& Metadata::setParameter(const string name, const long unsigned int value)
{
    char tmp[64];
    sprintf(tmp, "%lu", value);
    return setParameter(name, tmp);
}

string& Metadata::setParameter(const string name, const int value)
{
    char tmp[32];
    sprintf(tmp, "%d", value);
    return setParameter(name, tmp);
}

string& Metadata::setParameter(const string name, const long int value)
{
    char tmp[64];
    sprintf(tmp, "%ld", value);
    return setParameter(name, tmp);
}

string& Metadata::setParameter(const string name, const long long int value)
{
    char tmp[64];
    sprintf(tmp, "%lld", value);
    return setParameter(name, tmp);
}

string& Metadata::setParameter(const string name, const double value)
{
    char tmp[32];
    sprintf(tmp, "%lf", value);
    return setParameter(name, tmp);
}

bool Metadata::removeParameter(const string name)
{
        return param_registry.erase(name) == 1;
}

const char *Metadata::getParameter(const string name) const
{
        parameter_registry_t::const_iterator it;

        it = param_registry.find(name);

        if (it == param_registry.end())
                return NULL;
        		
        return (*it).second.c_str();
}


string& Metadata::setContent(const string _content)
{
        content = _content;
        return content;
}

const string& Metadata::getContent() const
{
        return content;
}
