/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 *   James Mathewson (JM, JLM)
 *   Yu-Ting Yu (yty)
 */

/* Copyright 2008-2009 Uppsala University
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>

#if defined(OS_ANDROID)
// extern "C" {
//   extern int mallopt(int  param_number, int  param_value);
// }
#endif

#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Thread.h>
#include <haggleutils.h>

#include <libxml/parser.h>

#include "DebugManager.h"
#include "HaggleKernel.h"
#include "SQLDataStore.h"
#include "MemoryDataStore.h"
#include "DataManager.h"
// SW: START: SendPriorityManager
#include "SendPriorityManager.h"
// SW: END: SendPriorityManager
#include "NodeManager.h"
#include "ProtocolManager.h"
#include "ProtocolSocket.h"
#include "Utility.h"
#if defined(OS_UNIX)
#include "ProtocolLOCAL.h"
#endif
#include "ProtocolUDP.h"
#include "ProtocolTCP.h"
#include "ConnectivityManager.h"
#include "LossEstimateManager.h"
#include "ForwardingManager.h"
#include "ApplicationManager.h"
#include "BenchmarkManager.h"
#include "ResourceManager.h"
// SW: TODO: HACK: we need to call memory cleanup method
// during shutdown so XML library can free its internal
// data structures and not show leaks in valgrind.
//#ifdef DEBUG_LEAKS
#include "XMLMetadata.h"
//#endif
#include "SecurityManager.h"

//JM, taking over SM work
#include "ReplicationManager.h"
//
#include "fragmentation/manager/FragmentationManager.h"
#include "networkcoding/manager/NetworkCodingManager.h"

// SW: START: interest manager
#include "InterestManager.h"
// SW: END: interest manager

#ifdef OS_WINDOWS_MOBILE
#include <TrayNotifier.h>
HINSTANCE g_hInstance;
#endif

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_WINDOWS_DESKTOP)
#define HAGGLE_LOCAL_SOCKET "haggle.sock"
#include <signal.h>
#endif

#if defined(OS_UNIX)
#include <sys/stat.h>
#endif

HaggleKernel *kernel;
static bool shouldCleanupPidFile = true;
static bool setCreateTimeOnBloomfilterUpdate = false;
static bool recreateDataStore = false;
static bool runAsInteractive = true;

static bool useMemoryDB = false;
// SW: START CONFIG PATH:
static string configFile = "";
// SW: END CONFIG PATH.
static SecurityLevel_t securityLevel = SECURITY_LEVEL_MEDIUM;
/* Command line options variables. */
// Benchmark specific variables
static bool isBenchmarking = false;
static unsigned int Benchmark_DataObjects_Attr = 10;
static unsigned int Benchmark_Nodes_Attr = 100;
static unsigned int Benchmark_Attr_Num = 1000;
static unsigned int Benchmark_DataObjects_Num = 10000;
static unsigned int Benchmark_Test_Num = 100;

#if defined(OS_UNIX) && !defined(OS_ANDROID)

#include <termios.h>

static struct termios org_opts, new_opts;

static int setrawtty()
{	
	// Set non-buffered operation on stdin so that we notice one character keypresses
	int res;

	res = tcgetattr(STDIN_FILENO, &org_opts);
	
	if (res != 0) {
		fprintf(stderr, "Could not get tc opts: %s\n", strerror(errno));
		return res;
	}
	
	memcpy(&new_opts, &org_opts, sizeof(struct termios));
	
	new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	
	res = tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
        
	return res;
}

static void resettty()
{
	// Open a stdin file descriptor in case someone closed 0
	int fd = open("/dev/stdin", O_RDONLY);
	
	if (!fd) {
		fprintf(stderr, "Could not open stdin: %s\n", strerror(errno));
                return;
	}
	int res = tcsetattr(fd, TCSAFLUSH, &org_opts);
	
	if (res != 0) {
		//fprintf(stderr, "Could not set tc opts: %s\n", strerror(errno));
		// We failed to reset the terminal to its original setting, so
		// we instead pass the terminal the command 'reset' to hard-reset
		// it. Its ugly, but better than a non-usable terminal.
                res = system("reset");

                if (res == -1) {
                        fprintf(stderr, "call to system() failed : %s\n", strerror(errno));
                }
	}
	close(fd);
}

static void daemonize()
{
        int i, sid;
	FILE *f;

        /* check if already a daemon */
	if (getppid() == 1) 
                return; 
	
	i = fork();

	if (i < 0) {
		fprintf(stderr, "Fork error...\n");
                exit(EXIT_FAILURE); /* fork error */
	}
	if (i > 0) {
		//printf("Parent done... pid=%u\n", getpid());
                exit(EXIT_SUCCESS); /* parent exits */
	}
	/* new child (daemon) continues here */

	Trace::disableStdout();
	
	/* Change the file mode mask */
	umask(0);
		
	/* Create a new SID for the child process */
	sid = setsid();
	
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}
	
	/* 
	 Change the current working directory. This prevents the current
	 directory from being locked; hence not being able to remove it. 
	 */
	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
	
	/* Redirect standard files to /dev/null */
	f = freopen("/dev/null", "r", stdin);

	if (!f) {
		perror("redirect stdin:");
		exit(EXIT_FAILURE);
	}

	f = freopen("/dev/null", "w", stdout);

	if (!f) {
		perror("redirect stdout:");
		exit(EXIT_FAILURE);
	}

	f = freopen("/dev/null", "w", stderr);

	if (!f) {
		perror("redirect stderr:");
		exit(EXIT_FAILURE);
	}
}

#endif /* OS_UNIX */

#if defined(OS_WINDOWS)
typedef DWORD pid_t;

static pid_t getpid()
{
        return GetCurrentProcessId();
}
static void daemonize()
{
        
}
#endif

/*
	NOTE: if this path changes for any reason whatsoever, the path used in
	libhaggle to find the pid file also needs to change. Remember to change it!
*/
#define PID_FILE string(DEFAULT_DATASTORE_PATH).append("/haggle.pid")

enum {
        HAGGLE_PROCESS_BAD_PID = -3,
        HAGGLE_PROCESS_CANNOT_WRITE_PID = -2,
        HAGGLE_PROCESS_NO_ERROR = 0,
        HAGGLE_PROCESS_ALREADY_RUNNING = 1
};

static int write_pid_file(const pid_t pid)
{
#define BUFLEN 50
        char buf[BUFLEN];
        string pidfile = PID_FILE;

	if (!create_path(DEFAULT_DATASTORE_PATH)) {
		HAGGLE_ERR("Could not create directory path \'%s\'\n", DEFAULT_DATASTORE_PATH);
		return false;
	}
        FILE *fp = fopen(pidfile.c_str(), "r");

	if (fp) {
		pid_t old_pid;
		bool old_instance_is_running = false;
		HAGGLE_DBG("Detected old PID file, checking if a daemon is already running...\n");
		// The Pid file already exists. Check if the PID in that file is a 
		// running process.
		int ret = fscanf(fp, "%u", &old_pid);
		fclose(fp);
		
                if (ret == 0 || old_pid == 0) {
                        return HAGGLE_PROCESS_BAD_PID;
                } 
               
#if defined(OS_LINUX)
                snprintf(buf, BUFLEN, "/proc/%d/cmdline", old_pid);
                
                fp = fopen(buf, "r");
                
                if (fp != NULL) {
                        size_t nitems = fread(buf, 1, BUFLEN, fp);
                        if (nitems && strstr(buf, "haggle") != NULL) 
                                old_instance_is_running = true;
                        fclose(fp);
                }
#elif defined(OS_UNIX)
		int res = kill(old_pid, 0);
		old_instance_is_running = (res != -1);
#elif defined(OS_WINDOWS)
                HANDLE p;
		
                p = OpenProcess(0, FALSE, old_pid);
                old_instance_is_running = (p != NULL);
                if (p != NULL)
                        CloseHandle(p);
#endif
		if (old_instance_is_running)
			return HAGGLE_PROCESS_ALREADY_RUNNING;
	}

	HAGGLE_DBG("A Haggle daemon is not running, creating new PID file\n");
	
        snprintf(buf, 20, "%u\n", pid);

        // Ok, open and create the file
        fp = fopen(pidfile.c_str(), "w+");
        
        if (!fp) 
                return HAGGLE_PROCESS_CANNOT_WRITE_PID;

        // Write the pid number to the file
        size_t ret = fwrite(buf, strlen(buf), 1, fp);
        
        fclose(fp);

        if (ret != 1)
                return HAGGLE_PROCESS_CANNOT_WRITE_PID;

        return HAGGLE_PROCESS_NO_ERROR;
}

static void cleanup_pid_file()
{
        string pidfile = PID_FILE;
#if defined(OS_WINDOWS_MOBILE) || defined(OS_WINDOWS_VISTA)
	wchar_t *wpidfile = strtowstr_alloc(pidfile.c_str());
	if (wpidfile) {
		DeleteFile(wpidfile);
		free(wpidfile);
	}
#elif defined(OS_WINDOWS)
	DeleteFileA(pidfile.c_str());
#else
	unlink(pidfile.c_str());
#endif
}

#ifdef OS_WINDOWS_MOBILE

TrayNotifier *trayNotifier;

static void tray_notification_add(HINSTANCE hInstance, HaggleKernel *kernel)
{
	trayNotifier = new TrayNotifier(hInstance, kernel);

	trayNotifier->start();
}

static void tray_notification_remove()
{
	if (trayNotifier) {
		delete trayNotifier;
		trayNotifier = NULL;
	}
}

#endif /* WINDOWS_MOBILE */

void cleanup()
{
	HAGGLE_DBG("Cleaning up\n");

	if (kernel)
		delete kernel;

#ifdef DEBUG_LEAKS
	/*
		There seems to be some artificial limit in printf statements
		on Windows mobile. If the leak report is printed here using
		the HAGGLE_DBG macro nothing will be printed.
		Using the cout stream is slow but seems to work ok.

                However, on some platforms we do not have STL, so we use printf
                here anyway.
	*/
	printf("===== Leak report ===\n");
	LeakMonitor::reportLeaks();
	printf("=====================\n");
#endif
#if defined(OS_UNIX) && !defined(OS_ANDROID)
	resettty();
#endif
#ifdef OS_WINDOWS_MOBILE
	tray_notification_remove();
#endif
        if (shouldCleanupPidFile)
                cleanup_pid_file();

    // SW: HACK: ideally this should go elsewhere, we put it here 
    // for the meanwhile to remove detected memory leaks in 
    // in libxml2 from valgrind (some library data structures).
    XMLMetadata::shutdown_cleanup();
    SecurityManager::shutdown_cleanup();

    // MOS
    HAGGLE_LOG("\n\n****************** SHUTDOWN COMPLETED *********************\n\n");
}

#if !defined(OS_WINDOWS_MOBILE)
static int shutdown_counter = 0;

static void signal_handler(int signal)
{
	switch (signal) {
#if defined(OS_UNIX)
	case SIGKILL:
	case SIGHUP:
#endif
	case SIGINT:		// Not supported by OS_WINDOWS?
	case SIGTERM:
		if (shutdown_counter == 0) {
		        // MOS - no loging in signal handlers please - danger of deadlock in Trace::write
		        // HAGGLE_DBG("Interrupt: shutting down. Please wait or interrupt twice more to quit hard.\n");
			kernel->shutdown();
		} else if (shutdown_counter == 1) {
		        // MOS - no loging in signal handlers please - danger of deadlock in Trace::write
			// HAGGLE_DBG("Already shutting down. Please wait or interrupt one more time to quit hard.\n");
		} else {
		        // MOS - no loging in signal handlers please - danger of deadlock in Trace::write
			// HAGGLE_DBG("Forced hard shutdown!\n");
			exit(1);
		}
		shutdown_counter++;
		break;
	default:
		break;
	}
}
#endif // !OS_WINDOWS_MOBILE

#if HAVE_EXCEPTION
#include <libcpphaggle/Exception.h>
static void unexpected_exception_handler()
{
	HAGGLE_ERR("Uncaught exception in main thread!\n");
	HAGGLE_ERR("Please do exception handling...\n");
	kernel->shutdown();
}
#endif

int run_haggle()
{
	srand(time(NULL));
#ifdef ENABLE_DEBUG_MANAGER
	DebugManager *db = NULL;
#endif
	ApplicationManager *am = NULL;
	DataManager *dm = NULL;
// SW: START: SendPriorityManager
	SendPriorityManager *spm = NULL;
// SW: END: SendPriorityManager
	NodeManager *nm = NULL;
	ProtocolManager *pm = NULL;
	ForwardingManager *fm = NULL;
	SecurityManager *sm = NULL;
	ConnectivityManager *cm = NULL;
	LossEstimateManager *lm = NULL;
	NetworkCodingManager* networkCodingManager = NULL;
	FragmentationManager* fragmentationManager = NULL;
	//JM
	ReplicationManager *replicationManager = NULL;
	BenchmarkManager *bm = NULL;
	ResourceManager *rm = NULL;
// SW: START: interest manager
    InterestManager *im = NULL;
// SW: END: interest manager

	ProtocolSocket *p = NULL;
#ifdef OS_WINDOWS_MOBILE

	// For testing we force the deletion of the data store
	//recreateDataStore = true;
#endif
	int retval = EXIT_FAILURE;

#if defined(OS_ANDROID)
	//mallopt(-1, -1); // MOS - avoid trimming
#elif defined(OS_LINUX)
        mallopt(M_TRIM_THRESHOLD, -1); // MOS - avoid trimming
#endif

	xmlInitParser(); // MOS - this need to be called here for thread-safe libxml use

#ifdef DEBUG
    Trace::trace.enableFileTrace();
#endif

	HAGGLE_LOG("\n\n****************** HAGGLE STARTUP *********************\n\n");

	if (!create_path(HAGGLE_DEFAULT_STORAGE_PATH)) {
                HAGGLE_ERR("Could not create Haggle storage path : %s\n", HAGGLE_DEFAULT_STORAGE_PATH);
                return -1;
        }
	
        retval = write_pid_file(getpid());

        if (retval != HAGGLE_PROCESS_NO_ERROR) {
                switch (retval) {
                        case HAGGLE_PROCESS_BAD_PID:
                                HAGGLE_ERR("Cannot read PID file %s.\n", PID_FILE.c_str());
                                break;
                        case HAGGLE_PROCESS_CANNOT_WRITE_PID:
                                HAGGLE_ERR("Cannot write PID file %s\n", PID_FILE.c_str());
                                break;
                        case HAGGLE_PROCESS_ALREADY_RUNNING: 
                                HAGGLE_ERR("PID file %s indicates that Haggle is already running.\n", PID_FILE.c_str());
                                break;
                        default:
                                HAGGLE_ERR("Unknown PID file error\n");

                }
                shouldCleanupPidFile = false;
                return -1;
        }
#if defined(OS_UNIX) && !defined(OS_ANDROID)
	setrawtty();
#endif
      
        /* Seed the random number generator */
	prng_init();

// SW: START CONFIG PATH (instead of hardcoded ~/.Haggle/config.xml),
    if (useMemoryDB) {
        kernel = new HaggleKernel(configFile, new MemoryDataStore(recreateDataStore));
    }
    else {
        kernel = new HaggleKernel(configFile, new SQLDataStore(recreateDataStore));
    }
// SW: END CONFIG PATH.

	if (!kernel || !kernel->init()) {
		fprintf(stderr, "Kernel initialization error!\n");
		return -1;
	}
	
	// Build a Haggle configuration
	am = new ApplicationManager(kernel);

	if (!am || !am->init()) {
		HAGGLE_ERR("Could not initialize application manager\n");
		goto finish;
	}

	dm = new DataManager(kernel, setCreateTimeOnBloomfilterUpdate);

	if (!dm || !dm->init()) {
		HAGGLE_ERR("Could not initialize data manager\n");
		goto finish;
	}

// SW: START: SendPriorityManager
    spm = new SendPriorityManager(kernel);
    if (!spm || !spm->init()) {
        HAGGLE_ERR("Could not initialize send priority manager\n");
        goto finish;
    }
// SW: END: SendPriorityManager

	nm = new NodeManager(kernel);

	if (!nm || !nm->init()) {
		HAGGLE_ERR("Could not initialize node manager\n");
		goto finish;
	}

	pm = new ProtocolManager(kernel);

	if (!pm || !pm->init()) {
		HAGGLE_ERR("Could not initialize protocol manager\n");
		goto finish;
	}

	fm = new ForwardingManager(kernel);

	if (!fm || !fm->init()) {
		HAGGLE_ERR("Could not initialize forwarding manager\n");
		goto finish;
	}

	sm = new SecurityManager(kernel, securityLevel);

	if (!sm || !sm->init()) {
		HAGGLE_ERR("Could not initialize security manager\n");
		goto finish;
	}
	fragmentationManager = new FragmentationManager(kernel);
	if(!fragmentationManager || !fragmentationManager->init()) {
	    HAGGLE_ERR("Could not initialize fragmentationManager\n");
	    goto finish;
	}

	networkCodingManager = new NetworkCodingManager(kernel);
	if(!networkCodingManager || !networkCodingManager->init()) {
	    HAGGLE_ERR("Could not initialize networkCodingManager \n");
	    goto finish;
	}

	//JM
	replicationManager = new ReplicationManager(kernel);
        if (!replicationManager || !replicationManager->init()) {
		HAGGLE_ERR("Could not initialize replication manager\n");
		goto finish;
    }
    
	lm = new LossEstimateManager(kernel);
	if(!lm || !lm->init()){
	    HAGGLE_ERR("Could not initialize LossEstimateManager \n");
	    goto finish;		
	}

#ifdef USE_UNIX_APPLICATION_SOCKET
	p = new ProtocolLOCAL(kernel->getStoragePath() + "/" + HAGGLE_LOCAL_SOCKET, pm);

	if (!p || !p->init()) {
		HAGGLE_ERR("Could not initialize LOCAL protocol\n");
		goto finish;
	}
	p->setFlag(PROT_FLAG_APPLICATION);
	p->registerWithManager();

#endif
	p = new ProtocolUDP("127.0.0.1", HAGGLE_SERVICE_DEFAULT_PORT, pm);
	/* Add ConnectivityManager last since it will start to
	* discover interfaces and generate events. At that
	* point the other managers should already be
	* running. */

	if (!p || !p->init()) {
		HAGGLE_ERR("Could not initialize UDP Application protocol\n");
		goto finish;
	}
	p->setFlag(PROT_FLAG_APPLICATION);
	p->registerWithManager();

// SW: start interest manager
    im = new InterestManager(kernel);

    if (!im || !im->init()) {
        HAGGLE_ERR("Could not initialize interest manager\n");
        goto finish;
    }
// SW: end interest manager

	/* MOS - disable resource mananager due 
	   high cpu utilization bug on Android

	rm = new ResourceManager(kernel);

	if (!rm || !rm->init()) {
		HAGGLE_ERR("Could not initialize resource manager\n");
		goto finish;
	}

	*/

	if (!isBenchmarking) {
		cm = new ConnectivityManager(kernel);

		if (!cm || !cm->init()) {
			HAGGLE_ERR("Could not initialize connectivity manager\n");
			goto finish;
		}

	} else {
		bm = new BenchmarkManager(kernel, Benchmark_DataObjects_Attr, Benchmark_Nodes_Attr, Benchmark_Attr_Num, Benchmark_DataObjects_Num, Benchmark_Test_Num);

		if (!bm || !bm->init()) {
			HAGGLE_ERR("Could not initialize benchmark manager\n");
			goto finish;
		}
	}
#if defined(ENABLE_DEBUG_MANAGER)
	// It seems as if there can be only one accept() per
	// thread... we need to make the DebugManager register
	// protocol or something with the ProtocolTCPServer
	// somehow
	db = new DebugManager(kernel, runAsInteractive);

	if (!db || !db->init()) {
		HAGGLE_ERR("Could not initialize debug manager\n");
		/* Treat as non critical error. */
	}
#endif

	HAGGLE_DBG("Starting Haggle...\n");

#ifdef OS_WINDOWS_MOBILE
	if (platform_type(current_platform()) == platform_windows_mobile_professional)
		tray_notification_add(g_hInstance, kernel);
#endif

	kernel->run();

	if(kernel->hasFatalError()) retval = EXIT_FAILURE;
	else retval = EXIT_SUCCESS;

	HAGGLE_DBG("Haggle finished...\n");

finish:
	if (bm)
		delete bm;
	if (lm)
		delete lm;

	if (fragmentationManager) {
	    delete fragmentationManager;
	    fragmentationManager = NULL;
	}
	if (networkCodingManager)
	    delete networkCodingManager;
	//JM
	if (replicationManager)
	  	delete replicationManager;
	if (cm)
		delete cm;
	if (sm)
		delete sm;
	if (fm)
		delete fm;
	if (pm)	
		delete pm;
	if (nm)
		delete nm;
	if (dm)
		delete dm;
// SW: START: SendPriorityManager
    if (spm)
        delete spm;
// SW: END: SendPriorityManager
	if (am)
		delete am;
// SW: start interest manager
    if (im)
        delete im;
// SW: end interest manager
#if defined(ENABLE_DEBUG_MANAGER)
	if (db)
		delete db;
#endif
	if (rm)
		delete rm;

#ifdef OS_WINDOWS_MOBILE
	tray_notification_remove();
#endif
	delete kernel;
	kernel = NULL;

	xmlCleanupParser(); // MOS

	return retval;
}

#if defined(OS_UNIX)

static struct {
	const char *cmd_short;
	const char *cmd_long;
	const char *cmd_desc;
} cmd[] = {
	{ "-b", "--benchmark", "run benchmark." },
	{ "-I", "--non-interactive", "turn off interactive information." },
	{ "-dd", "--delete-datastore", "delete database file before starting." },
	{ "-h", "--help", "print this help." },
	{ "-d", "--daemonize", "run in the background as a daemon." },
	{ "-f", "--filelog", "write debug output to a file (haggle.log)." },
	{ "-c", "--create-time-bloomfilter", "set create time in node description on bloomfilter update." },
	{ "-s", "--security-level", "set security level 0-3 (low, medium, high, very high)" }, // CBMEN, HL
// SW: START CONFIG PATH:
    { "-g", "--config", "specify xml configuration file." },
    { "-m", "--mem", "use in-memory db." }
// SW: END CONFIG PATH:
};

static void print_help()
{	
	unsigned int i;
	
	printf("Usage: ./haggle -[hbdfIcsg{dd}m]\n");
	
	for (i = 0; i < sizeof(cmd) / (3*sizeof(char *)); i++) {
		printf("\t%-4s %-20s %s\n", cmd[i].cmd_short, cmd[i].cmd_long, cmd[i].cmd_desc);
	}
}

static int check_cmd(char *input, unsigned int index)
{
	if (input == NULL || index >  (sizeof(cmd) / (3*sizeof(char *))))
		return 0;
	
	if (strcmp(input, cmd[index].cmd_short) == 0 || strcmp(input, cmd[index].cmd_long) == 0)
		return 1;
	
	return 0;	
}

#endif // OS_UNIX

#if !defined(OS_ANDROID)

#if defined(WINCE)
int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPTSTR lpCmdLine,    
    int nCmdShow)
{
	g_hInstance = hInstance;
	// Turn the following line on if you want to debug on windows mobile
	atexit(cleanup);
	
	setCreateTimeOnBloomfilterUpdate = true;

	return run_haggle();
}
#else
#if defined(OS_UNIX)
int main(int argc, char **argv)
#elif defined(OS_WINDOWS)
int main(void)
#endif
{
#if !defined(OS_WINDOWS_MOBILE)
	shutdown_counter = 0;
#endif
#if defined(OS_UNIX)
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
#endif
	
	// Set the handler function for unexpected exceptions
#if HAVE_EXCEPTION
        std::set_terminate(unexpected_exception_handler);
#endif

#if defined(OS_UNIX)
	// Do some command line parsing. Could use "getopt", but it
	// probably doesn't port well to Windows
	argv++;
	argc--;

	while (argc) {
		if (check_cmd(argv[0], 3)) {
			print_help();
			return EXIT_SUCCESS;
		} else if (check_cmd(argv[0], 1)) {
			runAsInteractive = false;
		} else if (check_cmd(argv[0], 0)) {
#ifdef BENCHMARK
			if (!(argv[1] && argv[2] && argv[3] && argv[4]
			      && argv[5])) {
				fprintf(stderr, "Bad number of arguments for benchmark option...\n");
				fprintf(stderr, "usage: -b doAttr nodeAttr numAttr numDataObject numNode\n");
				return -1;
			}
			printf("Haggle benchmarking...\n");
			isBenchmarking = true;
			Benchmark_DataObjects_Attr = atoi(argv[1]);
			Benchmark_Nodes_Attr = atoi(argv[2]);
			Benchmark_Attr_Num = atoi(argv[3]);
			Benchmark_DataObjects_Num = atoi(argv[4]);
			Benchmark_Test_Num = atoi(argv[5]);
#else
			fprintf(stderr, "-b: Unsupported: no benchmarking compiled in!\n");
			return EXIT_FAILURE;
#endif
			break;
		} else if (check_cmd(argv[0], 2)) {
			recreateDataStore = true;
		} else if (check_cmd(argv[0], 4)) {
			runAsInteractive = false;
			daemonize();
		} else if (check_cmd(argv[0], 5)) {
                        Trace::trace.enableFileTrace();
		} else if (check_cmd(argv[0], 6)) {
                        setCreateTimeOnBloomfilterUpdate = true;
		} else if (check_cmd(argv[0], 7)) {
			if (!argv[1] || atoi(argv[1]) < 0 || atoi(argv[1]) > 3) { // CBMEN, HL
				fprintf(stderr, "Bad security level, must be between 0-3\n"); // CBMEN, HL
				return -1;
			}
                        securityLevel = static_cast<SecurityLevel_t>(atoi(argv[1]));
			argv++;
			argc--;
// SW: START CONFIG PATH:
		} else if (check_cmd(argv[0], 8)) {
            if (!argv[1]) {
                fprintf(stderr, "Bad configuration path.\n");
                return -1;
            }
            configFile = argv[1];
            argv++;
            argc--;
// SW: END CONFIG PATH:
		} else if (check_cmd(argv[0], 9)) {
                        useMemoryDB = true;
		} else {
			fprintf(stderr, "Unknown command line option: %s\n", argv[0]);
			print_help();
			exit(EXIT_FAILURE);
		}
		argv++;
		argc--;
	}
#endif

#if defined(OS_WINDOWS)
	signal(SIGTERM, &signal_handler);
	signal(SIGINT, &signal_handler);
#elif defined(OS_UNIX)
	sigact.sa_handler = &signal_handler;
	//sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGPIPE, &sigact, NULL);
#endif

	atexit(cleanup);

	return run_haggle();
}

#endif

#endif /* !defined(OS_ANDROID) */
