/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

#ifndef CHARM_CRYPTO_BRIDGE_H
#define CHARM_CRYPTO_BRIDGE_H
#include <Python.h>

#include <libcpphaggle/String.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>
using namespace haggle;

#ifdef CCBTEST

#define LOG(format, ...) (TRACE("LOG", stdout, format, ## __VA_ARGS__))
#define ERROR(format, ...) (TRACE("ERROR", stderr, format, ## __VA_ARGS__))
void TRACE(const char *_type, FILE *stream, const char *fmt, ...);

extern "C" {
    extern char _binary____python_ccb_py_start;
    extern char _binary____python_ccb_py_end;
    extern char _binary____python_ccb_py_size;
}

#define CCB_PYTHON_MODULE_BLOB_START _binary____python_ccb_py_start
#define CCB_PYTHON_MODULE_BLOB_END _binary____python_ccb_py_end

#else

#include "Trace.h"
#define LOG(...) HAGGLE_DBG(__VA_ARGS__)
#define ERROR(...) HAGGLE_ERR(__VA_ARGS__)

extern "C" {
    extern char _binary__________ccb_python_ccb_py_start;
    extern char _binary__________ccb_python_ccb_py_end;
    extern char _binary__________ccb_python_ccb_py_size;
}

#define CCB_PYTHON_MODULE_BLOB_START _binary__________ccb_python_ccb_py_start
#define CCB_PYTHON_MODULE_BLOB_END _binary__________ccb_python_ccb_py_end

#endif

namespace ccb_api {
class CharmCryptoBridge {
public:
        static int startPython();
        static int stopPython();

        static int init(string persistence);
        static string shutdown();

        static int authsetup(List<string>& attributes, Map<string, string>& sk, Map<string, string>& pk);
        static int keygen(string attribute, string gid, bool self, string& key);

        static int encrypt(string pt, string policy, string& ct);
        static int decrypt(string ct, string& pt);

        static int set_gid(string gid);
        static int addAuthorityPK(string attribute, string key);
        static int addUserSK(string attribute, string key);
    };
}

#endif
