/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 */

#include "ReplicationConcurrentSend.h"

bool
ReplicationConcurrentSend::tokenAvailable(string node_id)
{
    node_tokens_t::iterator it = node_tokens.find(node_id);
    int used_tokens = node_tokens.size(); 
    if (!tokens_per_node) {
         if (used_tokens < maxTokens) {
             return true;
         } else {
	     return false;
         } 
    }
    if (it == node_tokens.end()) {
       return true; 
    }
    used_tokens = 0;
    for (;it != node_tokens.end() ; it++) {
	if( (*it).first == node_id) {
        used_tokens++; }
    }
    if (used_tokens < maxTokens) {
        return true;
    }

    return false;
}

void 
ReplicationConcurrentSend::freeToken(string node_id, string dobj_id)
{
    node_tokens_t::iterator it = node_tokens.find(node_id);
  
    //corner case, where config file is processed as DO, but there is no
    //node associated with it 
    if (node_id.size() == 0) {
        return;
    }
    if (it == node_tokens.end()) {
        HAGGLE_ERR("Cannot free token: %s, %s\n", node_id.c_str(), dobj_id.c_str());
        return;
    }

    bool found = false;
    for (;it != node_tokens.end() && (*it).first == node_id; it++) {
        if ((*it).second == dobj_id) {
            found = true;
            node_tokens.erase(it);
            break;
        }
    }

    if (!found) {
        HAGGLE_ERR("Could not find token: %s, %s\n", node_id.c_str(), dobj_id.c_str());
        return;
    }
}

void 
ReplicationConcurrentSend::reserveToken(string node_id, string dobj_id)
{
    node_tokens_t::iterator it = node_tokens.find(node_id);
    int used_tokens = node_tokens.size(); 
    if (!tokens_per_node) {
         if (used_tokens >= maxTokens) {
	    return;
         } 
    }

    int token_count = 0;
    if (it != node_tokens.end()) {
        bool found = false;
        for (;it != node_tokens.end() && (*it).first == node_id; it++) {
            token_count++;
            if ((*it).second == dobj_id) {
                found = true;
                break;
            }
        }
        if (found) {
            HAGGLE_ERR("dobj already has token.\n");
            return;
        }
    }

    if (token_count >= maxTokens) {
        HAGGLE_ERR("token count exceeded.\n");
        return;
    }

    node_tokens.insert(make_pair(node_id, dobj_id));
}

bool
ReplicationConcurrentSend::tokenExists(string node_id, string dobj_id)
{
    node_tokens_t::iterator it = node_tokens.find(node_id);
    if (it == node_tokens.end()) {
            return false;
    }

    bool found = false;
    for (;it != node_tokens.end() && (*it).first == node_id; it++) {
        if ((*it).second == dobj_id) {
            found = true;
            break;
        }
    }
    return found;
}

ReplicationConcurrentSend::~ReplicationConcurrentSend()
{
    node_tokens_t::iterator it;
    while ((it = node_tokens.begin()) != node_tokens.end()) {
        node_tokens.erase(it);
    }
}

void
ReplicationConcurrentSend::getReservedDataObjectsForNode(string node_id, List<string> &dObjs_id)
{
    node_tokens_t::iterator it = node_tokens.find(node_id);
    if (it == node_tokens.end()) {
        return;
    }

    for (;it != node_tokens.end() && (*it).first == node_id; it++) {
        dObjs_id.push_front((*it).second);      
    }
}

bool
ReplicationConcurrentSend::runSelfTest()
{
    ReplicationConcurrentSend *concurrentSend = new ReplicationConcurrentSend();
    if (!concurrentSend->tokenAvailable("a")) {
        HAGGLE_ERR("self test 1 failed.\n");
        return false;
    }
    if (concurrentSend->tokenExists("a", "b")) {
        HAGGLE_ERR("self test 2 failed.\n");
        return false;
    }

    int i;

    for (i = 0; i < REPL_CONN_SEND_DEFAULT_CONN; i++) {
        concurrentSend->reserveToken("a", string('1'+i));
    }

    if (concurrentSend->tokenAvailable("a")) {
        HAGGLE_ERR("self test 3 failed.\n");
        return false;
    }

    if (!concurrentSend->tokenAvailable("b")) {
        HAGGLE_ERR("self test 4 failed.\n");
        return false;
    }

    for (i = 0; i < REPL_CONN_SEND_DEFAULT_CONN; i++) {
        if (!concurrentSend->tokenExists("a", string('1'+i))) {
            HAGGLE_ERR("self test %s token exists failed.\n", string(i + '5').c_str());
            return false;
        }
    }

    concurrentSend->freeToken("a", "1");
    if (!concurrentSend->tokenAvailable("a")) {
        HAGGLE_ERR("self test 10 failed.\n");
        return false;
    }

    if (concurrentSend->tokenExists("a", "1")) {
        HAGGLE_ERR("self test 11 failed.\n");
        return false;
    }

    delete concurrentSend;
    
    return true;
}
