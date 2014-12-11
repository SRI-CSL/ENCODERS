/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

#ifndef _METADATA_H
#define _METADATA_H

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/String.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>

using namespace haggle;

/**
   Metadata implements an abstract representation for hierarchical
   metadata in data objects, along with an interface.

   Each metadata object has a name-key and content, and may have any
   number of optional children which are also metadata objects. There
   is hence one root metadata, from which a tree of other metadata
   expands. There may be multiple children with the same name-key.

   Each metadata may also have associated parameters in the form of
   name-value pairs. There may only be one parameter with a specific
   name.

   Metadata is essentially a simple wrapper for XML, which is the
   typical backend for the raw wire representation of the metadata. In
   the XML representation, a metadata object corresponds to an XML
   node, and the parameters to XML node attributes.

   One reason for this class is to implement a backend independent
   interface for metadata, so that it is simple to replace the wire
   format with something else than XML, if one so wishes.

   Another reason is that the interface provided by the Metadata class
   is much simpler than the one provided by XML. The interface is
   hence adapted for the needs of Haggle, and thus hides the
   complexity of working with XML directly.
 */
class Metadata
{
    public:
        typedef Pair<string, string> parameter_t;
    protected:
        Metadata *parent;
        string name;
        string content;
        typedef Map<string, string> parameter_registry_t;
        typedef HashMap<string, Metadata *> registry_t;
        parameter_registry_t param_registry;
        registry_t registry;
        Metadata(const string _name, const string _content = "", Metadata *_parent = NULL);
        Metadata(const Metadata& m);
        // This function should be called by addMetadata() in the
        // derived class
        bool _addMetadata(Metadata *m);
    private:
        // Used for iteration
        Pair<registry_t::iterator, registry_t::iterator> r;
        Pair<registry_t::const_iterator, registry_t::const_iterator> r_const;
    public:
        virtual ~Metadata() = 0;
        virtual Metadata *copy() const = 0;
        virtual ssize_t getRaw(unsigned char *buf, size_t len) = 0;
        virtual bool getRawAlloc(unsigned char **buf, size_t *len) = 0;
        // This function should be called by addMetadata() in the
        // derived class
        virtual bool addMetadata(Metadata *m) = 0;
        virtual Metadata *addMetadata(const string name, const string content = "") = 0;
	virtual bool initFromRaw(const unsigned char *raw, size_t len) { return false; }
	bool isName(const string _name) const;
        string getName() const { return name; }
        const string& getName() { return name; }
        bool removeMetadata(const string name);
        Metadata *getMetadata(const string name, unsigned int n = 0);    
        const Metadata *getMetadata(const string name, unsigned int n = 0) const;    
	Metadata *getNextMetadata();	
	const Metadata *getNextMetadata() const;
	
        string& setParameter(const string name, const string value);
        string& setParameter(const string name, const unsigned int n);
        string& setParameter(const string name, const long unsigned int n);
        string& setParameter(const string name, const int n);
        string& setParameter(const string name, const long int n);
        string& setParameter(const string name, const long long int n);
        string& setParameter(const string name, const double n);
        bool removeParameter(const string name);
        const char *getParameter(const string name) const;
        string& setContent(const string content);
        const string& getContent() const;
};

#endif /* _METADATA_H */
