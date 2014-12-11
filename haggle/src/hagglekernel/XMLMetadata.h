/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _XMLMETADATA_H
#define _XMLMETADATA_H

#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>
#include <libxml/tree.h>

#include "Metadata.h"

using namespace haggle;

class XMLMetadata : public Metadata {
        xmlDocPtr doc; // Temporary pointer to doc that we are parsing
        char *initDoc(const char *raw, const size_t len);
        bool createXML(xmlNodePtr xn);
        bool createXMLDoc();
        bool parseXML(xmlNodePtr xn);
    public:
        XMLMetadata(const string name, const string content = "", XMLMetadata *parent = NULL);
        XMLMetadata(const XMLMetadata& m);
        XMLMetadata();
        ~XMLMetadata();
        XMLMetadata *copy() const;
	bool initFromRaw(const unsigned char *raw, size_t len);
        ssize_t getRaw(unsigned char *buf, size_t len);
        bool getRawAlloc(unsigned char **buf, size_t *len);
        bool addMetadata(Metadata *m);
        Metadata *addMetadata(const string name, const string content = "");

        // SW: TODO: HACK: this needs to be called during shutdown so that the XML
        // library can properly free its datastructures and not display memory
        // leaks in valgrind.
        static void shutdown_cleanup();
};

#endif /* _XMLMETADATA_H */
