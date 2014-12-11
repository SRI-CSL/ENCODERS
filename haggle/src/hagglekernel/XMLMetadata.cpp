/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 */

#include "XMLMetadata.h"
#include "MetadataParser.h"

#include <string.h>

char *XMLMetadata::initDoc(const char *raw, size_t len)
{
	if (doc)
		xmlFreeDoc(doc);

        doc = xmlParseMemory(raw, len);
        
        if (!doc) {
                fprintf(stderr, "initDoc failed\n");
                return NULL;
        }

	xmlNodePtr root = xmlDocGetRootElement(doc);

	if(!root) { // MOS
	  fprintf(stderr, "initDoc failed - no root element\n");
	  // xmlFreeDoc(doc); // MOS - risk minor leak here
	  return NULL;
	}

        return (char *)root->name;
}

XMLMetadata::XMLMetadata(const string name, const string content, XMLMetadata *parent) :
	Metadata(name, content, parent), doc(NULL)
{
}

XMLMetadata::XMLMetadata(const XMLMetadata& m) :
                Metadata(m), doc(m.doc)
{
}
#if defined(OS_WINDOWS)
// Make sure MSVSC does not complain about passing this pointer in 
// base member initialization list
#pragma warning(disable : 4355)
#endif
XMLMetadata::XMLMetadata() : Metadata("", "", this), doc(NULL)
{
	
}

XMLMetadata::~XMLMetadata()
{
	if (doc)
		xmlFreeDoc(doc);
}

bool XMLMetadata::initFromRaw(const unsigned char *raw, size_t len)
{
	char *_name = initDoc((const char *)raw, len);

	if (!_name) {
		fprintf(stderr, "Could not create document\n");
		if (doc)
			xmlFreeDoc(doc);
		doc = NULL;
		return false;
	}

        if (!parseXML(xmlDocGetRootElement(doc))) {
                fprintf(stderr, "Parse XML failed\n");
                xmlFreeDoc(doc);
		doc = NULL;
                return false;
        }

	name = _name;
        
        xmlChar *content = xmlNodeGetContent(xmlDocGetRootElement(doc));
        
	if (content) {
                setContent((char *)content);
		xmlFree(content);
	}

        xmlFreeDoc(doc);
	doc = NULL;

	return true;
}

XMLMetadata *XMLMetadata::copy() const
{
        return new XMLMetadata(*this);
}

bool XMLMetadata::parseXML(xmlNodePtr xn)
{
        for (xmlAttrPtr xmlAttr = xn->properties; xmlAttr; xmlAttr = xmlAttr->next) {
                setParameter((char *)xmlAttr->name, (char *)xmlAttr->children->content);
        }
        for (xmlNodePtr xnc = xn->children; xnc; xnc = xnc->next) {
                if (xnc->type == XML_ELEMENT_NODE) {
                        XMLMetadata *m;
                        
                        if (xnc->children && 
                            xnc->children->type == XML_TEXT_NODE && 
                            !xmlIsBlankNode(xnc->children)) {
                                xmlChar *content = xmlNodeGetContent(xnc);
                                m = static_cast<XMLMetadata *>(addMetadata((char *)xnc->name, (char  *)content));
                                xmlFree(content);
                        } else {
                                m = static_cast<XMLMetadata *>(addMetadata((char *)xnc->name));
                        }

                        // Parse any children
                        if (!m || !m->parseXML(xnc))
                                return false;
                        
                } 
        }
#if defined(ENABLE_METADATAPARSER)
        MetadataParser *mp = MetadataParser::getParser(name);

        if (mp)
                return mp->onParseMetadata(this);
#endif
        return true;
}
bool XMLMetadata::addMetadata(Metadata *m)
{
        return _addMetadata(m);
}

Metadata *XMLMetadata::addMetadata(const string name, const string content)
{
        XMLMetadata *m = new XMLMetadata(name, content, this);

        if (!m)
                return NULL;

        //printf("XMLMetadata::addMetadata() adding metadata %s=%s\n", name.c_str(), content.c_str());
        if (!addMetadata(m)) {
                delete m;
                return NULL;
        }

        return m;
}

ssize_t XMLMetadata::getRaw(unsigned char *buf, size_t len)
{
	int xmlLen;
	xmlChar *xml;

	if (!buf)
		return -1;

	memset(buf, 0, len);

        if (!createXMLDoc())
                return false;

	xmlDocDumpFormatMemory(doc, &xml, &xmlLen, 1);

        xmlFreeDoc(doc);
	doc = NULL;

        if (xmlLen < 0)
                return -2;
        
	if ((unsigned int) xmlLen > len) {
		xmlFree(xml);
		return -3;
	}
        
	memcpy(buf, xml, xmlLen);

	xmlFree(xml);

	return xmlLen;
}

bool XMLMetadata::getRawAlloc(unsigned char **buf, size_t *len)
{
        int count;

	if (!buf || !len)
		return false;

	*len = 0;

        if (!createXMLDoc())
                return false;

	xmlDocDumpFormatMemory(doc, (xmlChar **) buf, &count, 1);
	
        xmlFreeDoc(doc);
	doc = NULL;
        
	if (count <= 0)
                return false;
        
        *len = count;

        return true;
}

bool XMLMetadata::createXML(xmlNodePtr xn)
{
        if (!xn)
                return false;
        
        // Add parameters
        for (parameter_registry_t::iterator it = param_registry.begin(); it != param_registry.end(); it++) {
                if (!xmlNewProp(xn, (xmlChar *)(*it).first.c_str(), (xmlChar *)(*it).second.c_str()))
                        return false;
        }

        // Add recursively
        for (registry_t::iterator it = registry.begin(); it != registry.end(); it++) {
                XMLMetadata *m = static_cast<XMLMetadata *>((*it).second);
                
                if (!m->createXML(xmlNewChild(xn, NULL, (const xmlChar *) m->name.c_str(), m->content.length() != 0 ? (const xmlChar *)m->content.c_str() : NULL)))
                        return false;
        }
        return true;
}

bool XMLMetadata::createXMLDoc()
{
	if (doc)
		xmlFreeDoc(doc);

        doc = xmlNewDoc((xmlChar *)"1.0");

        if (!doc)
                return false;

        xmlNodePtr xn = xmlNewNode(NULL, (xmlChar *)name.c_str());
        
        if (!createXML(xn))
                goto out_err;

        xmlDocSetRootElement(doc, xn);

        return true;
out_err:
        xmlFreeDoc(doc);
        doc = NULL;
	return false;
}

// SW: allow clean shutdown of XML data structures, to avoid memleak.
void XMLMetadata::shutdown_cleanup() 
{
    xmlCleanupParser();
}
