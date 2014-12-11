/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

#include "CharmCryptoBridge.h"

#include <libcpphaggle/String.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/Map.h>
using namespace haggle;

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sys/time.h>

typedef ccb_api::CharmCryptoBridge bridge;

double getTimeAsDouble(struct timeval t) {
    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}

#ifdef CCBTEST

#include <sys/time.h>
#define TRACE_BUFLEN 4096
static struct timeval *startTime = NULL;
void TRACE(const char *_type, FILE *stream, const char *fmt, ...)
{
    if (!startTime) {
        startTime = (struct timeval *) malloc(sizeof(struct timeval));
        gettimeofday(startTime, NULL);
    }
    char buf[TRACE_BUFLEN] = { 0 };
    va_list args;
    int len;
    struct timeval now, t;

    gettimeofday(&now, NULL);
    timersub(&now, startTime, &t);

    memset(buf, '\0', TRACE_BUFLEN);
    memset(&args, 0, sizeof(va_list));

    va_start(args, fmt);
    len = vsnprintf(buf, TRACE_BUFLEN, fmt, args);
    va_end(args);

    if (strcmp(_type, "LOG") == 0) {
        fprintf(stream, "%.6lf: %s", getTimeAsDouble(t), buf);
    } else if (strcmp(_type, "ERROR") == 0) {
        fprintf(stream, "%.6lf: ERROR: %s", getTimeAsDouble(t), buf);
    } else {
        fprintf(stderr, "Invalid TRACE type %s!\n", _type);
    }
}

#endif

char *load_persistence(const char *filename) {
    std::streampos fsize = 0;
    std::ifstream fin(filename, std::ios::binary);
    char *persistence = NULL;

    fsize = fin.tellg();
    fin.seekg( 0, std::ios::end );
    fsize = fin.tellg() - fsize;
    fin.seekg(0);

    persistence = new char[((size_t)fsize)+1];
    fin.read(persistence, fsize);
    persistence[fsize] = '\0';

    fin.close();

    return persistence;
}

void save_persistence(const char *filename, string data) {
    std::ofstream fout(filename, std::ios::binary);

    if (fout.good()) {
        fout.write(data.c_str(), data.size());
    }

    fout.close();
}

int main() {

#ifdef OS_ANDROID
#if PY_MAJOR_VERSION >= 3
setenv("EXTERNAL_STORAGE", "/mnt/sdcard/com.googlecode.python3forandroid", true);
setenv("PY34A", "/data/data/com.googlecode.python3forandroid/files/python3", true);
setenv("PY4A_EXTRAS", "/mnt/sdcard/com.googlecode.python3forandroid/extras", true);
setenv("PYTHONPATH", "/mnt/sdcard/com.googlecode.python3forandroid/extras/python3:/data/data/com.googlecode.python3forandroid/files/python3/lib/python3.2/lib-dynload:/mnt/sdcard/com.googlecode.python3forandroid/extras/python3/site-packages", true);
setenv("TEMP", "/mnt/sdcard/com.googlecode.python3forandroid/extras/python3/tmp", true);
setenv("HOME", "/sdcard", true);
setenv("PYTHON_EGG_CACHE", "/mnt/sdcard/com.googlecode.python3forandroid/extras/python3/tmp", true);
setenv("PYTHONHOME", "/data/data/com.googlecode.python3forandroid/files/python3", true);
setenv("LD_LIBRARY_PATH", (getenv("LD_LIBRARY_PATH") + string(":/data/data/com.googlecode.python3forandroid/files/python3/lib")).c_str(), true);
setenv("LD_RUN_PATH", (getenv("LD_LIBRARY_PATH") + string(":/data/data/com.googlecode.python3forandroid/files/python3/lib")).c_str(), true);
#else
#define PY4A_BASE "/data/data/org.haggle.kernel/python"
setenv("PY34A", PY4A_BASE, true);
setenv("PYTHONPATH", PY4A_BASE "lib/python2.7:" PY4A_BASE "lib/python2.7/lib-dynload:" PY4A_BASE "lib/python2.7/site-packages", true);
setenv("TEMP", PY4A_BASE "lib/python2.7/tmp", true);
setenv("HOME", "/sdcard", true);
setenv("PYTHON_EGG_CACHE", PY4A_BASE "lib/python2.7/tmp", true);
setenv("PYTHONHOME", PY4A_BASE, true);
LOG("Set all paths\n");
#endif
#endif

    Map<string, string> sk, pk;
    List<string> attributes;
    string key, ct, pt, saved;
    int retval;
    struct timeval start, end, cmp;
    const char *persistence = load_persistence("charm_state.pickle");
    // const char *persistence = NULL;

    attributes.push_back("A1");
    attributes.push_back("A2");
    attributes.push_back("A3");
    attributes.push_back("A4");
    attributes.push_back("A5");
    attributes.push_back("A6");
    attributes.push_back("A7");
    attributes.push_back("A8");

    if (bridge::startPython() == -1) {
        ERROR("Couldn't start python, exiting\n");
        return -1;
    }

    if (bridge::init(persistence) == -1) {
        ERROR("Couldn't init ccb, exiting\n");
        return -1;
    }

    retval = bridge::set_gid("ALICE");
    if (retval) {
        ERROR("set_gid call failed.\n");
        return -1;
    }

    retval = bridge::authsetup(attributes, sk, pk);
    if (retval) {
        ERROR("Authsetup call failed.\n");
        return -1;
    }

    for (Map<string, string>::iterator it = sk.begin(); it != sk.end(); it++) {
        LOG("SK: %s => %s\n", (*it).first.c_str(), (*it).second.c_str());
    }
    for (Map<string, string>::iterator it = pk.begin(); it != pk.end(); it++) {
        LOG("PK: %s => %s\n", (*it).first.c_str(), (*it).second.c_str());
    }

    for (List<string>::iterator it = attributes.begin(); it != attributes.end(); it++) {
        retval = bridge::keygen((*it), "ALICE", true, key);
        if (retval) {
            ERROR("keygen call failed.\n");
            return -1;
        }
        LOG("Keygen => %s\n", key.c_str());
    }

    // retval = ccb::addUserSK("a1", "gAN9cQBYAQAAAGtxAUNaMTpQbE8vOGNycXFmOFFScXBtSGV0V3NROGVWMmVmRndGNFd0cDFXaW00RmgrUk4rVktnbVV6YlhFUkl4eDUxMkc1aUtTdFZqYWVCZG5Xd3BxMFhUckhjUUU9cQJzLg==");
    // if (retval) {
    //     LOG("addUserSK call failed.\n");
    //     return -1;
    // }

    if (gettimeofday(&start, NULL)) {
        ERROR("gettimeofday call failed.\n");
        return -1;
    }

    retval = bridge::encrypt("Hello this is ALICE", "A1 AND A2 AND A3 AND A4 AND A5 AND A6 AND A7 AND A8", ct);
    if (retval) {
        ERROR("encrypt call failed.\n");
        return -1;
    }

    if (gettimeofday(&end, NULL)) {
        ERROR("gettimeofday call failed.\n");
        return -1;
    }

    timersub(&end, &start, &cmp);
    LOG("Encrypt (%.6lf s) => %s\n", getTimeAsDouble(cmp), ct.c_str());

    if (gettimeofday(&start, NULL)) {
        ERROR("gettimeofday call failed.\n");
        return -1;
    }

    retval = bridge::decrypt(ct, pt);
    if (retval) {
        ERROR("decrypt call failed.\n");
        return -1;
    }

    if (gettimeofday(&end, NULL)) {
        ERROR("gettimeofday call failed.\n");
        return -1;
    }

    timersub(&end, &start, &cmp);
    LOG("Decrypt (%.6lf s) => %s\n", getTimeAsDouble(cmp), pt.c_str());

    saved = bridge::shutdown();
    
    if (bridge::stopPython() == -1) {
        ERROR("Error shutting down python!\n");
        return -1;
    }
    // save_persistence("charm_state.pickle", saved);

    return 0;
}
