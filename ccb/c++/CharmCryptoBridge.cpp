/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

#include "CharmCryptoBridge.h"
#include <stdarg.h>
#include <stdlib.h>
static PyObject *ccbModule = NULL;
static PyObject *compiledBuffer = NULL;
static char *ccb_python_module_buffer = NULL;
static bool pyFinalizePuntCalled = false;

#ifdef CCBTRACE
#include "structmember.h"
#include "frameobject.h"
#endif

#if PY_MAJOR_VERSION >= 3
#define PyText_To_CString(o) PyBytes_AS_STRING(o)
#define PyText_From_CString(o) PyBytes_FromString(o)
#define SETVALUE_ARGS_FORMAT "syyy"
#define INIT_ARGS_FORMAT "(y)"
#define SHUTDOWN_ARGS_FORMAT "()"
#define KEYGEN_ARGS_FORMAT "yyy"
#define ENCRYPT_ARGS_FORMAT "yy"
#define DECRYPT_ARGS_FORMAT "(y)"
#else
#define PyText_To_CString(o) PyString_AS_STRING(o)
#define PyText_From_CString(o) PyString_FromString(o)
#define SETVALUE_ARGS_FORMAT "ssss"
#define INIT_ARGS_FORMAT "(s)"
#define SHUTDOWN_ARGS_FORMAT "()"
#define KEYGEN_ARGS_FORMAT "sss"
#define ENCRYPT_ARGS_FORMAT "ss"
#define DECRYPT_ARGS_FORMAT "(s)"
#endif

void pyFinalizePunt(void) {
    if (!pyFinalizePuntCalled) {
        Py_Finalize();
    }
}

/* Begin exported functions */
static PyObject* emb_log(PyObject *self, PyObject *args)
{
    const char *str = NULL;

    if(!PyArg_ParseTuple(args, "s:log", &str))
        return NULL;

    if (str)
    {
        LOG("%s", str);
    }
    else
    {
        ERROR("emb_log: NULL str\n");
    }

    Py_RETURN_NONE;
}

static PyMethodDef EmbMethods[] = {
    {"log", emb_log, METH_VARARGS,
     "Logs the given message"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3

static PyModuleDef EmbModule = {
    PyModuleDef_HEAD_INIT, "emb", NULL, -1, EmbMethods,
    NULL, NULL, NULL, NULL
};

static PyObject* PyInit_emb(void)
{
    return PyModule_Create(&EmbModule);
}

#else

PyMODINIT_FUNC initemb(void) {
    Py_InitModule3("emb", EmbMethods, "This module contains functions provided by the CharmCryptoBridge.");
}

#endif

/* End exported functions */

int setPythonPath(const char *path_to_mod)
{
    // set path
    PyObject *sys_path;
    PyObject *path;

    sys_path = PySys_GetObject("path");
    if (sys_path == NULL)
        return -1;
    path = PyUnicode_FromString(path_to_mod);
    if (path == NULL)
        return -1;
    if (PyList_Append(sys_path, path) < 0)
        return -1;

    return 0;
}

void checkError(char *error_msg, bool ignoreErrors = false)
{
    if (PyErr_Occurred()) {
        PyErr_Print();
        if (ignoreErrors)
            LOG("PyError: %s\n", error_msg);
        else
            ERROR("PyError: %s\n", error_msg);
    } else {
        ERROR("In checkError() but no error occurred!\n");
    }
}

#ifdef CCBTRACE

#if PY_MAJOR_VERSION >= 3
#define TO_CSTR(s) PyBytes_AS_STRING(PyUnicode_AsASCIIString((s)))
#else
#define TO_CSTR(s) PyText_To_CString((s))
#endif

char *FILENAMES_TO_IGNORE[] = {"pyparsing.py", "sre_compile.py", "sre_parse.py", "pkg_resources.py", "posixpath.py", "copy.py", "datetime.py", "abc.py", "_weakrefset.py", "sre_constants.py", "re.py", "pickle.py"};
String FUNCNAMES_TO_IGNORE[] = {String("serializeDict"), String("b64decode"), String("b64encode")};

#define SIZEOF_ARRAY( a ) (sizeof( a ) / sizeof( a[ 0 ] )) 
#define FILES_TO_IGNORE SIZEOF_ARRAY(FILENAMES_TO_IGNORE)
#define FUNCTIONS_TO_IGNORE SIZEOF_ARRAY(FUNCNAMES_TO_IGNORE)

bool shouldTrace(String fileName, String funcName) {

    for (size_t i = 0; i < FILES_TO_IGNORE; i++) {
        if (fileName.find(FILENAMES_TO_IGNORE[i]) != String::npos)
            return false;
    }
    for (size_t i = 0; i < FUNCTIONS_TO_IGNORE; i++) {
        if (funcName == FUNCNAMES_TO_IGNORE[i])
            return false;
    }
    return true;
}

int CCB_Tracer(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg) {
    char *fileName, *funcName;

    if (frame->f_code != NULL) {
        fileName = TO_CSTR(frame->f_code->co_filename);
        funcName = TO_CSTR(frame->f_code->co_name);
    }

    switch (what) {
    case PyTrace_CALL:    

        if (shouldTrace(fileName, funcName))
            LOG("Entering line %d in filename: %s, co_name: %s\n", frame->f_lineno, fileName, funcName);
        break;

    case PyTrace_RETURN:   

        if (shouldTrace(fileName, funcName))
            LOG("Returning from line %d in filename: %s, co_name: %s\n", frame->f_lineno, fileName, funcName);
        break;

    case PyTrace_LINE:      

        if (shouldTrace(fileName, funcName))
            LOG("At line %d in filename: %s, co_name: %s\n", frame->f_lineno, fileName, funcName);
        break;

    case PyTrace_EXCEPTION:

        break;

    case PyTrace_C_CALL:
        if (shouldTrace(fileName, funcName))
            LOG("Calling C code at line %d in filename: %s, co_name: %s\n", frame->f_lineno, fileName, funcName);
        break;

    case PyTrace_C_RETURN:
        if (shouldTrace(fileName, funcName))
            LOG("Returning from C code at line %d in filename: %s, co_name: %s\n", frame->f_lineno, fileName, funcName);
        break;

    default:
        break;
    }

    return 0;
}

#endif

PyObject* callFunction(const char *funcName, PyObject *pArgs) {
    PyObject *pFunc = NULL, *pValue = NULL;

    pFunc = PyObject_GetAttrString(ccbModule, funcName);
    if (pFunc && PyCallable_Check(pFunc)) {
        pValue = PyObject_CallObject(pFunc, pArgs);
        if (!pValue)
            checkError((char *)"Calling function failed");
    }
    else {
        ERROR("Cannot find function \"%s\"\n", funcName);
    }
    Py_XDECREF(pFunc);
    return pValue;
}

PyObject *callVariadicFunction(const char *funcName, bool ignoreErrors, const char *fmt, ... ) {
    PyObject *pFunc = NULL, *pValue = NULL, *pArgs = NULL;
    va_list argp;
    va_start(argp, fmt);

    pArgs = Py_VaBuildValue(fmt, argp);
    if (!pArgs) {
        checkError((char *)"Error building argument list.\n");
        ERROR("Could not build argument list for variadic function call\n");
        va_end(argp);
        return NULL;
    }

    pFunc = PyObject_GetAttrString(ccbModule, funcName);
    if (pFunc && PyCallable_Check(pFunc)) {
        pValue = PyObject_CallObject(pFunc, pArgs);
        if (!pValue)
            checkError((char *)"Calling function failed", ignoreErrors);
    }
    else {
        ERROR("Cannot find function \"%s\"\n", funcName);
    }
    Py_XDECREF(pArgs);
    Py_XDECREF(pFunc);

    va_end(argp);
    return pValue;
}

int setValue(const char *funcName, const char *dict, const char *key, const char *value, bool serialize) {
    string ser = "False";
    if (serialize)
        ser = "True";

    PyObject *ret = callVariadicFunction("set_value", false, SETVALUE_ARGS_FORMAT, dict, key, value, ser.c_str());

    if (!ret) {
        ERROR("Error calling %s function.\n", funcName);
        return -1;
    }

    Py_XDECREF(ret);
    return 0;
}

int ccb_api::CharmCryptoBridge::startPython()
{
    size_t blob_size = &CCB_PYTHON_MODULE_BLOB_END - &CCB_PYTHON_MODULE_BLOB_START;

    if (ccbModule)
        return 0;

#if PY_MAJOR_VERSION >= 3
    PyImport_AppendInittab("emb", &PyInit_emb);
#else
    PyImport_AppendInittab("emb", &initemb);
#endif
    Py_Initialize();

// Useful for debugging


//     if (PyRun_SimpleString(
// "import io, sys\n"
// "import emb\n"
// "class Catcher(io.TextIOBase):\n"
// "   def __init__(self):\n"
// "       pass\n"
// "   def write(self, stuff):\n"
// "       emb.log(stuff)\n"
// "sys.stdout = Catcher()\n"
// "sys.stderr = Catcher()\n"
// "sys.stderr.write('Hello world!\\n')")) {
//         ERROR("Couldn't redirect stderr and stdout to debug log.\n");
//     }

//     if (PyRun_SimpleString(
// "from charm.schemes import dabe_aw11")) {
//         checkError("Couldn't import dabe_aw11.\n");
//         ERROR("Couldn't import dabe_aw11.\n");
//         return -1;
//     }

//     if (PyRun_SimpleString(
// "from charm.adapters import dabenc_adapt_hybrid")) {
//         checkError("Couldn't import dabenc_adapt_hybrid.\n");
//         ERROR("Couldn't import dabenc_adapt_hybrid.\n");
//         return -1;
//     }

//     if (PyRun_SimpleString(
// "from charm.toolbox.pairinggroup import PairingGroup")) {
//         checkError("Couldn't import PairingGroup.\n");
//         ERROR("Couldn't import PairingGroup.\n");
//         return -1;
//     }

//     if (PyRun_SimpleString(
// "from charm.toolbox.pairinggroup import G1")) {
//         checkError("Couldn't import G1.\n");
//         ERROR("Couldn't import G1.\n");
//         return -1;
//     }

//     if (PyRun_SimpleString(
// "from charm.core.engine import util")) {
//         checkError("Couldn't import util.\n");
//         ERROR("Couldn't import util.\n");
//         return -1;
//     }


    ccb_python_module_buffer = (char *) malloc(blob_size+1);
    if (!ccb_python_module_buffer) {
        ERROR("Couldn't allocate memory for ccb python module buffer.\n");
        return -1;
    }
    memcpy(ccb_python_module_buffer, &CCB_PYTHON_MODULE_BLOB_START, blob_size);
    ccb_python_module_buffer[blob_size] = '\0';

    compiledBuffer = Py_CompileString(ccb_python_module_buffer, "ccb.py", Py_file_input);
    if (!compiledBuffer) {
        checkError("Error compiling ccbModule\n");
        ERROR("Error compiling ccbModule\n");
    }

#ifdef CCBTRACE
    PyEval_SetTrace((Py_tracefunc)CCB_Tracer, NULL);
#endif

    ccbModule = PyImport_ExecCodeModule("ccb", compiledBuffer);
    if (!ccbModule) {
        checkError("Error importing ccbModule\n");
        ERROR("Error importing ccbModule\n");
        return -1;
    }

    return 0;
}

int ccb_api::CharmCryptoBridge::stopPython()
{
    if (!ccbModule) {
        ERROR("stopPython called before startPython\n");
        return -1;
    }

    Py_XDECREF(ccbModule);
    ccbModule = NULL;

    Py_XDECREF(compiledBuffer);
    compiledBuffer = NULL;

    if (ccb_python_module_buffer) {
        free(ccb_python_module_buffer);
    }

#ifdef CCBTRACE
    PyEval_SetTrace(NULL, NULL);
#endif

    // Py_Finalize();
    if (!atexit(pyFinalizePunt)) {
        LOG("Punted Py_Finalize call!\n");
    } else {
        ERROR("Error punting Py_Finalize call!\n");
    }
    return 0;
}

int ccb_api::CharmCryptoBridge::init(string persistence) {
    PyObject *pValue = NULL;

    pValue = callVariadicFunction("init", false, INIT_ARGS_FORMAT, persistence.c_str());
    if (!pValue) {
        ERROR("Error calling init.\n");
        return -1;
    }
    Py_XDECREF(pValue);

    return 0;
}

string ccb_api::CharmCryptoBridge::shutdown() {
    PyObject *pValue = NULL;
    string persistence;

    pValue = callVariadicFunction("shutdown", false, SHUTDOWN_ARGS_FORMAT);
    if (pValue) {
        persistence = PyText_To_CString(pValue);
        Py_XDECREF(pValue);
    } else {
        ERROR("Error calling shutdown.\n");
    }

    return persistence;
}

int ccb_api::CharmCryptoBridge::authsetup(List<string>& attributes, Map<string, string>& sk, Map<string, string>& pk) {
    PyObject *list = NULL, *attribute = NULL, *args = NULL, *ret = NULL, *tup = NULL, *k = NULL, *v = NULL;
    Py_ssize_t length = 0;
    Map<string, string>* maps[] = {&sk, &pk};

    list = PyList_New(0);
    if (!list)
    {
        ERROR("Error creating list.\n");
        return -1;
    }

    for (List<string>::iterator it = attributes.begin(); it != attributes.end(); it++) {
        attribute = PyText_From_CString((*it).c_str());
        if (!attribute || PyList_Append(list, attribute)) {
            checkError((char *)"Error adding attribute to list.\n");
            Py_XDECREF(attribute);
            Py_XDECREF(list);
            return -1;
        }
        Py_XDECREF(attribute);
    }

    args = PyTuple_Pack(1, list);
    if (!args) {
        ERROR("Error creating tuple.\n");
        Py_XDECREF(list);
        return -1;
    }

    ret = callFunction("authsetup", args);
    if (!ret) {
        ERROR("Error calling authsetup function.\n");
        Py_XDECREF(args);
        Py_XDECREF(list);
        return -1;
    }
    Py_XDECREF(args);
    Py_XDECREF(list);

    for (size_t map = 0; map < 2; map++) {
        list = PyTuple_GetItem(ret, map);
        if (!list) {
            ERROR("Error getting list %lu\n", map);
            return -1;
        }

        length = PyList_Size(list);
        for (Py_ssize_t i = 0; i < length; i++) {
            tup = PyList_GetItem(list, i);

            if (!tup) {
                ERROR("Error getting element from list.\n");
                Py_XDECREF(list);
                return -1;
            }

            k = PyTuple_GetItem(tup, 0);
            v = PyTuple_GetItem(tup, 1);

            if (!k || !v) {
                ERROR("Error getting pairs from tuple.\n");
                Py_XDECREF(list);
                return -1;
            }

            maps[map]->insert(make_pair(string(PyText_To_CString(k)), string(PyText_To_CString(v))));
        }
        Py_XDECREF(list);
    }

    Py_XDECREF(ret);
    return 0;
}

int ccb_api::CharmCryptoBridge::keygen(string attribute, string gid, bool self, string& key) {
    PyObject *value = NULL;
    string save = "False";

    if (self)
        save = "True";

    value = callVariadicFunction("keygen", false, KEYGEN_ARGS_FORMAT, attribute.c_str(), gid.c_str(), save.c_str());

    if (!value) {
        ERROR("Error calling keygen function.\n");
        return -1;
    }

    key = PyText_To_CString(value);
    Py_XDECREF(value);
    return 0;
}

int ccb_api::CharmCryptoBridge::encrypt(string pt, string policy, string& ct) {
    PyObject *value = callVariadicFunction("encrypt", false, ENCRYPT_ARGS_FORMAT, pt.c_str(), policy.c_str());

    if (!value) {
        ERROR("Error calling encrypt function.\n");
        return -1;
    }

    ct = PyText_To_CString(value);
    Py_XDECREF(value);
    return 0;
}

int ccb_api::CharmCryptoBridge::decrypt(string ct, string& pt) {
    PyObject *value = callVariadicFunction("decrypt", true, DECRYPT_ARGS_FORMAT, ct.c_str());

    if (!value) {
        LOG("Error calling decrypt function.\n");
        return -1;
    }

    pt = PyText_To_CString(value);
    Py_XDECREF(value);
    return 0;
}

int ccb_api::CharmCryptoBridge::set_gid(string gid) {
    return setValue("set_gid", "user_sk", "gid", gid.c_str(), false);
}

int ccb_api::CharmCryptoBridge::addAuthorityPK(string attribute, string key) {
    return setValue("addAuthorityPK", "authority_pk", attribute.c_str(), key.c_str(), true);
}

int ccb_api::CharmCryptoBridge::addUserSK(string attribute, string key) {
    return setValue("addUserSK", "user_sk", attribute.c_str(), key.c_str(), true);
}
