/* Copyright 2008 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *     
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */ 
#include "metadata.h"
#include <libhaggle/list.h>
#include <libxml/tree.h>
#include <libhaggle/debug.h>
#include <string.h>

static int metadata_xml_parse(metadata_t *m, xmlNodePtr xn)
{
        xmlAttrPtr xa;
        xmlNodePtr xnc;
        
        for (xa = xn->properties; xa; xa = xa->next) {
                metadata_set_parameter(m, (char *)xa->name, (char *)xa->children->content);
        }
        for (xnc = xn->children; xnc; xnc = xnc->next) {
                if (xnc->type == XML_ELEMENT_NODE) {
                        metadata_t *mc = metadata_new((char *)xnc->name, NULL, m);
			
                        if (!mc)
                                return -1;
                        
                        if (metadata_xml_parse(mc, xnc) < 0)
                                return -1;
                        
                } else if (xmlNodeIsText(xnc)) {
                        if (!xmlIsBlankNode(xnc)) {
                                xmlChar *content = xmlNodeGetContent(xnc);
                                if (content) {
                                        metadata_set_content(m, (char *)content);
                                        xmlFree(content);
                                }
                        }
                }
        }

        return 1;
}

static int metadata_xml_create_xml(metadata_t *m, xmlNodePtr xn)
{
        list_t *pos;
        metadata_t *mc;

        if (!xn)
                return -1;
	
	//printf("Adding node %s\n", m->name);
        
        // Add parameters
        list_for_each(pos, &m->parameters->attributes) {
                struct attribute *a = (struct attribute *)pos;
                if (!xmlNewProp(xn, (xmlChar *)haggle_attribute_get_name(a), (xmlChar *)haggle_attribute_get_value(a)))
                        return -1;
        }

        mc = metadata_get(m, NULL);

        // Add recursively
        while (mc) {
                if (metadata_xml_create_xml(mc, xmlNewChild(xn, NULL, (xmlChar *) metadata_get_name(mc), (xmlChar *)metadata_get_content(mc))) < 0)
                        return -1;
                
                mc = metadata_get_next(m);
        }
        return 1;
}

static xmlDocPtr metadata_xml_create_doc(metadata_t *m)
{
        xmlDocPtr doc;
        xmlNodePtr xn;

        doc = xmlNewDoc((xmlChar *)"1.0");

        if (!doc)
                return NULL;

        xn = xmlNewNode(NULL, (xmlChar *)metadata_get_name(m));
        
	if (!xn)
		goto out_err;
	
        xmlDocSetRootElement(doc, xn);
	
        if (!metadata_xml_create_xml(m, xn))
                goto out_err;

        return doc;
out_err:
        xmlFreeDoc(doc);
        return NULL;
}

metadata_t *metadata_xml_new_from_xml(const char *raw, size_t len)
{
        xmlDocPtr doc;
        xmlNodePtr xn;
        xmlChar *content;
        metadata_t *m;
        
        doc = xmlParseMemory(raw, len);
        
        if (!doc) {
                LIBHAGGLE_ERR("initDoc failed for:\n%s\n", raw);
                return NULL;
        }

        xn = xmlDocGetRootElement(doc);
        
        m = metadata_new((char *)xn->name, NULL, NULL);
        
        if (!m) {
                xmlFreeDoc(doc);
                return NULL;
        }

        if (!metadata_xml_parse(m, xmlDocGetRootElement(doc))) {
                LIBHAGGLE_ERR("Parse XML failed\n");
                xmlFreeDoc(doc);
                metadata_free(m);
                return NULL;
        }
        
// CBMEN, HL - libxml aggregates content, which is not what we want.
// We only want the content for the current node, if there is any.
        if (xmlNodeIsText(xmlDocGetRootElement(doc))) {
                content = xmlNodeGetContent(xmlDocGetRootElement(doc));
                
                if (content) {
                        metadata_set_content(m, (char *)content);
                        xmlFree(content);
                }
        }
// CBMEN, HL, End
        
        xmlFreeDoc(doc);
        
        return m;
}


ssize_t metadata_xml_get_raw(metadata_t *m, unsigned char *buf, size_t len)
{
	int xmlLen;
	xmlChar *xml;
        xmlDocPtr doc;

	if (!buf)
		return -1;

	memset(buf, 0, len);

        doc = metadata_xml_create_doc(m);

        if (!doc)
                return -1;

	xmlDocDumpFormatMemory(doc, &xml, &xmlLen, 1);

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

int metadata_xml_get_raw_alloc(metadata_t *m, unsigned char **buf, size_t *len)
{
        int count;
        xmlDocPtr doc;

	if (!buf || !len)
		return -1;

	*len = 0;

        doc = metadata_xml_create_doc(m);

        if (!doc)
                return -1;

	xmlDocDumpFormatMemory(doc, (xmlChar **)buf, &count, 1);
	
        xmlFreeDoc(doc);

	if (count <= 0)
                return -1;
        
        *len = count;

        return count;
}
