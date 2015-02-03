/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Joshua Joy (JJ, jjoy)
 *   Jong-Seok Choi (JS)
 *   Hasnain Lakhani (HL)
 *   James Mathewson (JM, JLM)
 *   Sam Wood (SW)
 *   Hasanat Kazmi (HK)
 */

// test program for Haggle API

#include <stdbool.h>
#include "libhaggle/haggle.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX_ATTR_SIZE 1000
#define ORIGINATOR_ATTR "ContentOriginator"
#define TIMESTAMP_ATTR "ContentCreationTime"
#define SEQNUM_ATTR "SeqenceNumber"

#define CREATETIME_ASSTRING_FORMAT "%ld.%06ld"
static bool print_allowed=true;

#define ht_printf(...) if(print_allowed) {printf(__VA_ARGS__);}

static bool terminate = false;

static haggle_handle_t  haggle;
struct attributelist *cur_interests = NULL;
struct attributelist *cur_interests_policies = NULL; // IRD, HK

static int num_data_objects_received = 0;
static int num_data_objects_published = 0;

// SW: START: suppressing redundant receives 
static int wait = 0;
// SW: END: suppressing redundant receives 


char *strnew(char *s)
{
  char *r = (char*)malloc(strlen(s)+1);
  strcpy(r,s);
  return r;
}

char *filename_to_full_path(const char *filename){
  char *buf = (char*)malloc(1024);
  getcwd(buf,1024);
  strncat(buf, "/", 1024);
  strncat(buf, filename, 1024);
  return buf;
}

char *data_object_id_str(const struct dataobject *dobj)
{
  dataobject_id_t id;
  char idStr[2*sizeof(dataobject_id_t)+1];
  unsigned int i, len = 0;

  haggle_dataobject_calculate_id(dobj, &id);
  
  for (i = 0; i < sizeof(dataobject_id_t); i++) {
    len += sprintf(idStr + len, "%02x", id[i] & 0xff);
  }

  return strnew(idStr);
}

static bool done = false;

void sig_handler(int signum)
{
  done = true;
}

static struct timeval st;
static char *logfn = NULL;
static char *endfn = NULL;
static char *batchfn = NULL;
static FILE *logfp = NULL;
static FILE *endfp = NULL;
static char *doidlog = NULL;
static FILE *doidfp = NULL;
static FILE *batchfp = NULL;

static double cdelay_sum = 0;
static double rdelay_sum = 0;
static bool tag = false;
static char *tagid = NULL;
static char *tagvalue = NULL;
struct file_ll {
  char *filename;
  struct file_ll *next;
};
struct file_ll *head=NULL, *tail=NULL, *temp_ll;
bool copy = false;

double getTimeAsSeconds(struct timeval t)
{
  return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;  
}

int log_time(struct dataobject *dobj)
{
      char *objname = data_object_id_str(dobj);
      struct timeval *ct = haggle_dataobject_get_createtime(dobj);

      struct timeval at;

      gettimeofday(&at,NULL);
      double rdelay = getTimeAsSeconds(at) - getTimeAsSeconds(st);
      ht_printf("Delay for %s measured from initial request: %.3lf\n", objname, rdelay);
      rdelay_sum += rdelay;

      double cdelay = getTimeAsSeconds(at) - getTimeAsSeconds(*ct);
      ht_printf("Delay for %s measured from object creation: %.3lf\n", objname, cdelay);
      if(cdelay < 0) {
        ht_printf("Warning: receive time before create time !\n");
      }
      cdelay_sum += (cdelay < 0) ? 0 : cdelay;
      // SW: keep track of data len
      ssize_t datalen = haggle_dataobject_get_datalen(dobj);

      // JM: Get tag info for Rx packets
    if (tag) {
        tagvalue="";
        struct attribute *a;
        a=haggle_dataobject_get_attribute_by_name(dobj, tagid);
        tagvalue=haggle_attribute_get_value(a);
    }

      if(logfp) {
        fprintf(logfp, "Received,%s,", objname);
        fprintf(logfp, CREATETIME_ASSTRING_FORMAT, (long)ct->tv_sec, (long)ct->tv_usec);
        fprintf(logfp, ",");
        fprintf(logfp, CREATETIME_ASSTRING_FORMAT, (long)st.tv_sec, (long)st.tv_usec);
        fprintf(logfp, ",");
        fprintf(logfp, CREATETIME_ASSTRING_FORMAT, (long)at.tv_sec, (long)at.tv_usec);
        // SW: keep track of data len
        fprintf(logfp, ",%.3lf,%.3lf,%d,%s\n", rdelay, cdelay, (int)datalen, tag?tagvalue:"");
        fflush(logfp);
      }

      return 0;
}

int on_neighbor_update(haggle_event_t *e, void* nix){
  haggle_nodelist_t *nl = e->neighbors;
  int num_neighbors = haggle_nodelist_size(nl);

  ht_printf("\nReceived neighbor update event\n");
  ht_printf("Number of neighbors is %d\n", num_neighbors);
  
  return 0;
}


int on_dataobject(haggle_event_t *e, void* nix){
// SW: START: suppressing redundant receives 
  if(wait > 0 && num_data_objects_received >= wait) {
    return 0;
  }
// SW: END: suppressing redundant receives 
  ht_printf("\nReceived data object event\n");
  if (e->dobj) {
    const unsigned char *xml = haggle_dataobject_get_raw(e->dobj);
    if (xml) {
      ht_printf("%s\n", (const char *)xml);
    }
    //if -k is set for sub, copy the file local here
    if (copy) {
       char *path=haggle_dataobject_get_filepath(e->dobj);
       char *filezname=haggle_dataobject_get_filename(e->dobj);
       FILE *src, *dst;
       char data[1024];
       char filename2[1024];
       int nbytes;
       if (NULL != filezname) {
         strcpy(filename2, path);
         src = fopen(filename2,"rb");
         dst = fopen(filezname,"wb");
         //error check
         if (NULL == src || NULL == dst) {
           printf("unable to open src: %s and/or dst: %s\n", filename2, filezname);
           exit ( 1);
         }
         while (!feof(src) || ferror(src) ) {
           nbytes = fread(data, 1, 1024, src);
           fwrite(data, 1, nbytes, dst);
         }
         fclose(src);
         fclose(dst);
      }
    }
  }
  ht_printf("Last data object id received: %s\n", data_object_id_str(e->dobj));
  // SV. Log the times subscription request is fulfilled.
  log_time(e->dobj);
  ht_printf("Number of data objects received: %d\n", ++num_data_objects_received);
  return 0;
}

int on_shutdown(haggle_event_t *e, void* nix){
  ht_printf("\nReceived shutdown event\n");
  done = true;
  return 0;
}

int on_interests(haggle_event_t *e, void* nix){
  ht_printf("\nReceived application interests event\n");
  if (e->interests) {
    cur_interests = haggle_attributelist_copy(e->interests);
    int i = 0;
    while(true){
      struct attribute *a = haggle_attributelist_get_attribute_n(cur_interests,i);
      if(a == NULL) break;
      ht_printf(" attribute %s=%s:%lu\n",  
       haggle_attribute_get_name(a), 
       haggle_attribute_get_value(a), 
       haggle_attribute_get_weight(a));
      i++;
    }
    if(i == 0){
      ht_printf(" no interest attributes registered\n");
    }
  }
  done = true;
  return 0;
}

// IRD, HK, Begin
int on_interestsPolicies(haggle_event_t *e, void* nix){
  ht_printf("\nReceived application interests policies event\n");
  if (e->interests_policies) {
    cur_interests_policies = haggle_attributelist_copy(e->interests_policies);
    int i = 0;
    while(true){
      struct attribute *a = haggle_attributelist_get_attribute_n(cur_interests_policies,i);
      if(a == NULL) break;
      ht_printf(" attribute %s, policy %s\n",  
       haggle_attribute_get_name(a), 
       haggle_attribute_get_value(a));
      i++;
    }
    if(i == 0){
      ht_printf(" no interest policies attributes registered\n");
    }
  }
  done = true;
  return 0;
}
// IRD, HK, End

int is_subscribe(char *s)
{
  return strcmp(s,"subscribe") == 0 || strcmp(s,"sub") == 0;
}

int is_resubscribe(char *s)
{
  return strcmp(s,"resubscribe") == 0 || strcmp(s,"resub") == 0;
}

int is_unsubscribe(char *s)
{
  return strcmp(s,"unsubscribe") == 0 || strcmp(s,"unsub") == 0;
}

int is_publish(char *s)
{
  return strcmp(s,"publish") == 0 || strcmp(s,"pub") == 0;
}

int is_interest(char *s)
{
  return strcmp(s,"interest") == 0 || strcmp(s,"int") == 0;
}

int is_nop(char *s)
{
  return strcmp(s,"nop") == 0;
}

// IRD, HK, Begin
int is_setpolicy(char *s)
{
  return strcmp(s,"setPolicy") == 0 || strcmp(s, "sp") == 0;
}

int is_listpolicy(char *s)
{
  return strcmp(s,"listPolicy") == 0 || strcmp(s, "lp") == 0;
}
// IRD, HK, ENd

// CBMEN, HL, Begin
int is_add_role_shared_secrets(char *s)
{
  return strcmp(s, "addRoleSharedSecrets") == 0;
}

int is_add_node_shared_secrets(char *s)
{
  return strcmp(s, "addNodeSharedSecrets") == 0;
}

int is_add_authorities(char *s)
{
  return strcmp(s, "addAuthorities") == 0;
}

int is_authorize_nodes_for_certification(char *s)
{
  return strcmp(s, "authorizeNodesForCertification") == 0;
}

int is_authorize_role_for_attributes(char *s)
{
  return strcmp(s, "authorizeRoleForAttributes") == 0;
}
// CBMEN, HL, End

int is_cmd(char *s)
{
  if(s == NULL) return 0;
  return is_subscribe(s) || is_resubscribe(s) || is_unsubscribe(s) || is_publish(s) || is_interest(s) || is_nop(s) || is_add_role_shared_secrets(s) || is_add_node_shared_secrets(s) || is_add_authorities(s) || is_authorize_nodes_for_certification(s) || is_authorize_role_for_attributes(s); // CBMEN, HL
}

static int argc = 0;
static char **argv = 0;

char *arg(int i)
{
  return argc > i ? argv[i] : NULL;
}

void usage()
{
    ht_printf("Usage: %s [<options>] [<app>] pub|publish [file <file-name>] <attribute-name>[=<value>]\n", argv[0]);
    ht_printf("       %s [<options>] [<app>] sub|subscribe <attribute-name>[=<value>[:<weight>]]\n", argv[0]);
    ht_printf("       %s [<options>] [<app>] unsub|unsubscribe <attribute-name>[=<value>[:<weight>]]\n", argv[0]);
    ht_printf("       %s [<options>] [<app>] resub|resubscribe <attribute-name>[=<value>[:<weight>]]\n", argv[0]);
    ht_printf("       %s [<options>] [<app>] setPolicy|sp <attribute-name>=<\"policy\">\n", argv[0]); // IRD, HK
    ht_printf("       %s [<options>] [<app>] listPolicy|lp\n", argv[0]); // IRD, HK
    ht_printf("       %s [<options>] [<app>] addRoleSharedSecrets <role-name>=<shared-secret>\n", argv[0]); // CBMEN, HL
    ht_printf("       %s [<options>] [<app>] addNodeSharedSecrets <node-id>=<shared-secret>\n", argv[0]); // CBMEN, HL
    ht_printf("       %s [<options>] [<app>] addAuthorities <auth-name>=<auth-id>\n", argv[0]); // CBMEN, HL
    ht_printf("       %s [<options>] [<app>] authorizeNodesForCertification <node-id>\n", argv[0]); // CBMEN, HL
    ht_printf("       %s [<options>] [<app>] authorizeRoleForAttributes <role-name> <encryption-count> <decryption-count> <attribute-name>\n", argv[0]); // CBMEN, HL
    ht_printf("       %s [<options>] [<app>] int|interest\n", argv[0]);
    ht_printf("       %s [<options>] [<app>] nop\n", argv[0]);
    ht_printf("Note: All operations will register/unregister automatically with the Haggle daemon using\n");
    ht_printf("         <app> as application name or the name of this command as default.\n");
    ht_printf("      Default values are true for the <value> and 1 for <weight>.\n");
    ht_printf("Options: -p <pause> (in sec) and -r <rate> (in Hz) to specify publish frequency\n");
    ht_printf("         -c means clear, i.e. unsubscribe from all current interests before main operation\n");
    ht_printf("         -d means delete the application node state and bloomfilter from the database on deregistration\n");
    ht_printf("         -q means quit, i.e. quit after sub/pub (instead of waiting/looping)\n");
    ht_printf("         -l means loop, i.e. publish an infinite sequence of data objects\n");
    ht_printf("         -b <bound> means bound, i.e. publish a finite sequence of data objects up to a given bound\n");
    ht_printf("         -k means copy, i.e. make a fresh copy for each published data object or make a local copy (sub) (recommended)\n");
    ht_printf("         -m means maximum, i.e. maximum number of data objects returned per match\n");
    ht_printf("         -t means threshold, i.e. number between 0-100 to set matching threshold\n");
    ht_printf("         -n means neighbors, i.e. register for neighbor update events\n");
    ht_printf("         -s <timeout> (in sec) means stop, i.e. terminate after given timeout\n");
    ht_printf("         -a means automatic, i.e. automatically unsubscribe at termination\n");
    ht_printf("         -z means zero, i.e. reset Bloom filter and request data\n");
    ht_printf("         -f <file-name> means file, i.e. optional log file\n");
    ht_printf("         -i means interest, i.e. get current interest before executing main command\n");
    ht_printf("         -u means update, i.e. push current node descriptions to all peers\n");
    ht_printf("         -w <count> means wait, i.e. wait for count data objects to be received before exiting\n");
    //JLM
    ht_printf("         -e <filename> means write Elements (stats) to a file, to be processed by external programs\n");
    ht_printf("         -g <filename> means execute the specified file {# relative seconds sleep, pub, \"keyword=val;keyword2=val2\",\"filename\" (used for -f)\n");
    ht_printf("         -x Means terminate haggle server\n");
    ht_printf("         -j <keyword> Means print the value (keyword=value) in the log\n");
    ht_printf("         -v means to disable extra printing.   Useful for automated testing\n");
    ht_printf("         -y <filename> means to write all DO ID's to the specified file (useful to track DO's)\n");
}

int exists(int i)
{
  return i < argc;
}

int main(int argc_, char *argv_[]){
  int retcode = HAGGLE_NO_ERROR;

  argc = argc_; 
  argv = argv_;

  set_trace_level(0);
  signal(SIGINT, sig_handler);

  bool quit = false;
  bool seq = false;
  int max_seqnum = -1;
  bool clear = false;
  int pause = 1000;
  int wait = 0;
  long threshold = -1;
  long max_dataobjects = -1;
  bool neighbors = false;
  bool interests = false;
  int stop = 0;
  bool zero = false;
  bool update = false;
  bool retFile = false;
  bool batchExe = false;
  bool automatic = false;
  bool dereg_delete = false;

  opterr = 0;
  int opt;
  while ((opt = getopt (argc, argv, "lb:qh?cr:p:t:m:w:ne:g:j:xvr:is:zf:uky:ad")) != -1) {
    switch (opt)
    {
      case 'h':
      case '?':
        usage(); exit(0);
      case 'p':
        pause = atoi(optarg) * 1000;
      break;
      case 'r':
        pause = 1000/(double)atoi(optarg);
      break;
      case 'c':
        clear = true;
      break;
      case 'q':
        quit = true;
      break;
      case 'l':
        seq = true;
      break;
      case 'b':
        seq = true;
        if(optarg) max_seqnum = atoi(optarg);
      break;
      case 'e':
        retFile = true;
        endfn = optarg;
        max_seqnum = 1;
        if ((endfp = fopen(endfn, "w")) == 0) {
          ht_printf("Cannot open %s\n", endfn);
          exit(-1);
        }
      break;
      case 'g':
        batchExe = true;
        batchfn = optarg;
        if ((batchfp = fopen(batchfn, "r")) == 0) {
          ht_printf("Cannot open %s\n", batchfn);
          exit(-1);
        }
        break;
      case 't':
        threshold = atoi(optarg);
      break;
      case 'm':
        max_dataobjects = atoi(optarg);
      break;
      case 'w':
        wait = atoi(optarg);
      break;
      case 'n':
        neighbors = true;
      break;
      case 'i':
        interests = true;
      break;
      case 'v':
        print_allowed = false;
      break;
      case 'x':
        terminate = true;
      break;
      case 'j':
        tag = true;
        tagid = malloc (strlen(optarg)+1);
        strcpy(tagid, optarg);
      break;
      case 'z':
        zero = true;
      break;
      case 'u':
        update = true;
      break;
      case 's':
        stop = atoi(optarg);
      break;
      case 'a':
        automatic = true;
      break;
      case 'd':
        dereg_delete = true;
      break;
      case 'f':
        logfn = optarg;
        if ((logfp = fopen(logfn, "w")) == 0) {
          ht_printf("Cannot open %s\n", logfn);
          exit(-1);
        }
          // SW: keep track of data len
         // jm: add attribute tracking
        fprintf(logfp, "Action,ObjectId,CreateTime,ReqestTime,ArrivalTime,DelayRelToRequest,DelayRelToCreation,DataLen,tags\n");
      break;
      case 'y':
        doidlog = optarg;
        if ((doidfp = fopen(doidlog, "a")) == 0) {
          ht_printf("Cannot open %s\n", doidlog);
          exit(-1);
        }
      break;
      case 'k':
        copy = true;
      break;
      default:
        fprintf(stderr, "error: unknown option %d \n", opt);
        exit(-1);
    }
  }
  char* app = arg(0);
  int argindex = optind;
  char* cmd = arg(argindex);
  if(!is_cmd(cmd) && !batchExe){
      app = arg(optind);
      argindex = optind+1;
      cmd = arg(argindex);
      if(!is_cmd(cmd)) { usage(); exit(-1); }
  }

  if (batchExe) {
     app = arg(optind); 
     //if a batchfile is used, we ALWAYS PUB, and use options -c -d
     cmd = "pub";
     dereg_delete = true;
     clear = true;
     copy = true;
  } 
 
  ht_printf("Register %s with Haggle daemon ...\n", app);
  retcode = haggle_handle_get(app, &haggle);
  if(retcode != HAGGLE_NO_ERROR){
    fprintf(stderr, "Could not register %s with Haggle deamon (error %d)\n", app, retcode);
    exit(-1);
  }
  int session = haggle_handle_get_session_id(haggle);
  ht_printf("OK (session id = %d)\n\n", session);
   
  if (terminate)
  haggle_ipc_shutdown(haggle); 
  /*
    retcode = haggle_event_loop_register_start_stop_events(haggle, on_event_loop_start, on_event_loop_stop, haggle);
      
    if (retcode != HAGGLE_NO_ERROR) {
    fprintf(stderr, "Could not register start and stop events\n");
    exit(-1);
    }
  */
    
  ht_printf("Register shutdown event interest ...\n");
  retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_SHUTDOWN, on_shutdown);
      
  if (retcode != HAGGLE_NO_ERROR) {
    fprintf(stderr, "Could not register shutdown event interest\n");
    exit(-1);
  } else {
    ht_printf("OK\n\n");
  }
  
  if(neighbors) {
    ht_printf("Register neighbor update event interest ...\n");
    retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, on_neighbor_update);  
    if (retcode != HAGGLE_NO_ERROR) {
      fprintf(stderr, "Could not register neighbor update event interest\n");
      exit(-1);
    } else {
      ht_printf("OK\n\n");   
    }
  }
  
  ht_printf("Register new data object event interest ...\n");
  retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_NEW_DATAOBJECT, on_dataobject);  
  if (retcode != HAGGLE_NO_ERROR) {
    fprintf(stderr, "Could not register new data object event interest\n");
    exit(-1);
  } else {
    ht_printf("OK\n\n");
  }
  
  ht_printf("Register interest list event interest ...\n");
  retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_INTEREST_LIST, on_interests);  
  if (retcode != HAGGLE_NO_ERROR) {
    fprintf(stderr, "Could not register interest list event interest\n");
    exit(-1);
  } else {
    ht_printf("OK\n\n");
  }

  // IRD, HK, Begin  
  ht_printf("Register interests policies list event ...\n");
  retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_INTERESTS_POLICIES_LIST, on_interestsPolicies);  
  if (retcode != HAGGLE_NO_ERROR) {
    fprintf(stderr, "Could not register interest list event interest\n");
    exit(-1);
  } else {
    ht_printf("OK\n\n");
  }
  // IRD, HK, End 
  
  ht_printf("Start event loop ...\n");
  if (haggle_event_loop_run_async(haggle) != HAGGLE_NO_ERROR) {
    fprintf(stderr, "Could not start event loop\n");
    exit(-1);
  } else {
    ht_printf("OK\n\n");
  }
  ht_printf("\n");
  // ht_printf("Waiting 10s ...\n");
  // sleep(10);

  if(dereg_delete){
    haggle_ipc_set_delete_app_state_on_deregister(haggle, 1);
  }

  if(clear){
    done = false;
    ht_printf("Requesting current interests ...\n");
    haggle_ipc_get_application_interests_async(haggle);
    while(!done) usleep(100000);

    int i;
    for(i = 0; i < cur_interests->num_attributes; i++){
      struct attribute *a = haggle_attributelist_get_attribute_n(cur_interests,i);
      ht_printf("Unsubscribing from %s=%s ...\n",
        haggle_attribute_get_name(a), haggle_attribute_get_value(a));
    }

    retcode = haggle_ipc_remove_application_interests(haggle, cur_interests);
    if (retcode != HAGGLE_NO_ERROR) {
      fprintf(stderr, "Could not remove application interest\n");
      exit(-1);
    } else {
      ht_printf("OK\n\n");
    }
  }

  done = false;

  if(is_interest(cmd)) {
    ht_printf("Requesting current interests ...\n");
    haggle_ipc_get_application_interests_async(haggle);
    while(!done) usleep(100000);
  }
  else if((is_subscribe(cmd) || is_resubscribe(cmd) || is_unsubscribe(cmd) || is_publish(cmd) || is_nop(cmd) || is_add_role_shared_secrets(cmd) || is_add_node_shared_secrets(cmd) || is_add_authorities(cmd) || is_authorize_nodes_for_certification(cmd) || is_authorize_role_for_attributes(cmd)) && !batchExe){ // CBMEN, HL
    bool ok = true;
    argindex++;

    char *filename = exists(argindex) && strcmp(arg(argindex),"file") == 0 ? arg(argindex+1) : NULL;
    if(filename != NULL){      
      argindex +=2 ;
      if(filename[0] != '/') filename = filename_to_full_path(filename);
    }

    // CBMEN, HL, Begin
    char *roleName = NULL;
    unsigned long encryptionCount = 0;
    unsigned long decryptionCount = 0;

    if (is_authorize_role_for_attributes(cmd)) {
      roleName = exists(argindex) ? arg(argindex) : NULL;
      if (!roleName) {
        fprintf(stderr, "error: <role-name> not specified\n");
        ok = false;
      }

      argindex++;

      if (exists(argindex)) {
        int n = sscanf(arg(argindex), "%lu", &encryptionCount);
        if (n <= 0) {
          fprintf(stderr, "error: <encryption-count> must be a nonnegative integer\n");
          ok = false;
        }
      }
      argindex++;

      if (exists(argindex)) {
        int n = sscanf(arg(argindex), "%lu", &decryptionCount);
        if (n <= 0) {
          fprintf(stderr, "error: <decryption-count> must be a nonnegative integer\n");
          ok = false;
        }
      }
      argindex++;
    }
    // CBMEN, HL, Begin

    struct attributelist *al = haggle_attributelist_new();
    
    while(arg(argindex) != NULL){

      char av[MAX_ATTR_SIZE];
      char attr[MAX_ATTR_SIZE];
      char value[MAX_ATTR_SIZE] = "true";
      unsigned long weight = 1;
      
      if(strlen(arg(argindex)) >= MAX_ATTR_SIZE){
        fprintf(stderr, "error: attribute %s too long for test app\n", arg(argindex));
        ok = false;
        break;
      }
      else {
        int n = sscanf(arg(argindex),"%[^:]:%lu",av,&weight);
        if(n <= 0){
          fprintf(stderr, "error: attribute %s must be of the form <attribute-name>[=<value>[:<weight>]]\n", arg(argindex));  
          ok = false;
          break;
        }
        else{
          sscanf(av,"%[^=]=%[^:]",attr,value);
        }

        // ht_printf("parsed attribute %s=%s:%lu\n",attr,value,weight);

        struct attribute *a = haggle_attribute_new(strnew(attr),strnew(value));
        if (tag) {
          if (!strcmp(attr,tagid)) {
             tagvalue=(char *) malloc(strlen(value));
             strcpy(tagvalue, value);
          } 
        }
        haggle_attribute_set_weight(a,weight);
        haggle_attributelist_add_attribute(al,a);
      }
      argindex++;
    }

    if(!is_nop(cmd) && al->num_attributes == 0){
      // CBMEN, HL
      if(is_add_role_shared_secrets(cmd) || is_add_node_shared_secrets(cmd))
        fprintf(stderr, "error: no shared secrets specified\n");
      else if(is_add_authorities(cmd))
        fprintf(stderr, "error: no authorities specified\n");
      else if(is_authorize_nodes_for_certification(cmd))
        fprintf(stderr, "error: no nodes specified\n");
      else if(is_setpolicy(cmd)) // IRD, HK
        fprintf(stderr, "error: no policy specified\n"); // IRD, HK
      else
        fprintf(stderr, "error: no attributes specified\n");
    }
    else if(ok){
      
      int i;
      if(is_unsubscribe(cmd) || is_resubscribe(cmd)){
        for(i = 0; i < al->num_attributes; i++){
          struct attribute *a = haggle_attributelist_get_attribute_n(al,i);
          ht_printf("Unsubscribing from %s=%s ...\n",
            haggle_attribute_get_name(a), haggle_attribute_get_value(a));
        }

        retcode = haggle_ipc_remove_application_interests(haggle, al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not remove application interest\n");
          exit(-1);
        } else {
          ht_printf("OK\n\n");
        }

      }

      if(threshold >= 0){
        ht_printf("Setting matching threshold for %s to %lu ...\n",app,threshold); 
        retcode = haggle_ipc_set_matching_threshold(haggle,threshold);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not set matching threshold\n");
          exit(-1);
        } else {
          ht_printf("OK\n\n");
        }
      }

      if(max_dataobjects >= 0){
        ht_printf("Setting maximum data objects per match for %s to %lu ...\n",app,max_dataobjects); 
        retcode = haggle_ipc_set_max_data_objects_in_match(haggle,max_dataobjects);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not set max data obejcts\n");
          exit(-1);
        } else {
          ht_printf("OK\n\n");
        }
      }

      // SV. Note the time the request is made.
      gettimeofday(&st,NULL); 
      ht_printf("Setting time of request to current time: ");
      ht_printf(CREATETIME_ASSTRING_FORMAT, (long)st.tv_sec, (long)st.tv_usec);
      ht_printf("\n");


      if(zero) {
        ht_printf("Resetting application Bloom filter and requesting matching data objects ...\n");
        haggle_ipc_get_data_objects_async(haggle);
      }

      if(is_subscribe(cmd) || is_resubscribe(cmd)){
        for(i = 0; i < al->num_attributes; i++){
          struct attribute *a = haggle_attributelist_get_attribute_n(al,i);
          ht_printf("Subscribing to %s=%s:%lu ...\n", 
            haggle_attribute_get_name(a), haggle_attribute_get_value(a), haggle_attribute_get_weight(a));
        }
        retcode = haggle_ipc_add_application_interests(haggle,al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not add application interest\n");
          exit(-1);
        } else {
          ht_printf("OK\n\n");
        }
      }

      // CBMEN, HL, Begin
      if (is_add_role_shared_secrets(cmd)) {
        for(i = 0; i < al->num_attributes; i++) {
          struct attribute *a = haggle_attributelist_get_attribute_n(al, i);
          ht_printf("Adding role %s with shared secret %s\n",
                    haggle_attribute_get_name(a), haggle_attribute_get_value(a));
        }
        retcode = haggle_ipc_add_role_shared_secrets(haggle, al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not add role shared secrets\n");
          exit(-1);
        } else {
          ht_printf("Successfully added role shared secrets.\n");
        }
      }

      if (is_add_node_shared_secrets(cmd)) {
        for(i = 0; i < al->num_attributes; i++) {
          struct attribute *a = haggle_attributelist_get_attribute_n(al, i);
          ht_printf("Adding node %s with shared secret %s\n",
                    haggle_attribute_get_name(a), haggle_attribute_get_value(a));
        }
        retcode = haggle_ipc_add_node_shared_secrets(haggle, al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not add node shared secrets\n");
          exit(-1);
        } else {
          ht_printf("Successfully added node shared secrets.\n");
        }
      }

      if (is_add_authorities(cmd)) {
        for(i = 0; i < al->num_attributes; i++) {
          struct attribute *a = haggle_attributelist_get_attribute_n(al, i);
          ht_printf("Adding authority %s with id %s\n",
                    haggle_attribute_get_name(a), haggle_attribute_get_value(a));
        }
        retcode = haggle_ipc_add_authorities(haggle, al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not add authorities\n");
          exit(-1);
        } else {
          ht_printf("Successfully added authorities.\n");
        }
      }

      if (is_authorize_nodes_for_certification(cmd)) {
        for(i = 0; i < al->num_attributes; i++) {
          struct attribute *a = haggle_attributelist_get_attribute_n(al, i);
          ht_printf("Authorizing node %s for certification\n",
                    haggle_attribute_get_name(a));
        }
        retcode = haggle_ipc_authorize_nodes_for_certification(haggle, al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not authorize nodes for certification\n");
          exit(-1);
        } else {
          ht_printf("Successfully authorized nodes for certification.\n");
        }
      }

      if (is_authorize_role_for_attributes(cmd)) {
        if (haggle_attributelist_size(al) < (encryptionCount + decryptionCount)) {
          fprintf(stderr, "Not enough attributes specified on command line: %lu < %lu\n", haggle_attributelist_size(al), encryptionCount+decryptionCount);
          exit(1);
        }
        struct attributelist *encryption = haggle_attributelist_new();
        if (!encryption) {
          fprintf(stderr, "Could not allocate memory for encryption attribute list\n");
          exit(1);
        }
        for (i = 0; i < encryptionCount; i++) {
          struct attribute *a = haggle_attribute_copy(haggle_attributelist_get_attribute_n(al, i));
          if (!a) {
            fprintf(stderr, "Could not allocate memory for attribute\n");
            exit(1);
          }
          ht_printf("Authorizing role %s for encryption attribute %s\n", roleName, haggle_attribute_get_name(a));
          haggle_attributelist_add_attribute(encryption, a);
        }
        struct attributelist *decryption = haggle_attributelist_new();
        if (!decryption) {
          fprintf(stderr, "Could not allocate memory for decryption attribute list\n");
          exit(1);
        }
        for (i = 0; i < decryptionCount; i++) {
          struct attribute *a = haggle_attribute_copy(haggle_attributelist_get_attribute_n(al, i+encryptionCount));
          if (!a) {
            fprintf(stderr, "Could not allocate memory for attribute\n");
            exit(1);
          }
          ht_printf("Authorizing role %s for decryption attribute %s\n", roleName, haggle_attribute_get_name(a));
          haggle_attributelist_add_attribute(decryption, a);
        }
        retcode = haggle_ipc_authorize_role_for_attributes(haggle, roleName, encryption, decryption);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not authorize role for attributes\n");
          exit(-1);
        } else {
          ht_printf("Successfully authorized role for attributes.\n");
        }
        if (encryption)
          haggle_attributelist_free(encryption);
        if (decryption)
          haggle_attributelist_free(decryption);
      }
      // CBMEN, HL, End

      // IRD, HK, Begin
      if (is_setpolicy(cmd)) {
        for(i = 0; i < al->num_attributes; i++) {
          struct attribute *a = haggle_attributelist_get_attribute_n(al, i);
          ht_printf("Helo Adding role %s with shared secret %s\n",
                    haggle_attribute_get_name(a), haggle_attribute_get_value(a));
        }
        retcode = haggle_ipc_add_interests_policies(haggle, al);
        if (retcode != HAGGLE_NO_ERROR) {
          fprintf(stderr, "Could not add interests policies.\n");
          exit(-1);
        } else {
          ht_printf("Successfully added interests policies.\n");
        }
      }

      // if (is_listpolicy(cmd)) {
      //   retcode = haggle_ipc_list_interests_policies(haggle);
      //   if (retcode != HAGGLE_NO_ERROR) {
      //     fprintf(stderr, "Could not list interests policies.\n");
      //     exit(-1);
      //   }
      // }

      // IRD, HK, End

      done = false;
      if(interests) {
        ht_printf("Requesting current interests ...\n");
        haggle_ipc_get_application_interests_async(haggle);
        int interestCount =0;
        for(interestCount = 0; stop == 0 || interestCount < stop * 10; interestCount++) {
          if(done) break;
          usleep(100000);
        }
        if(!done) { 
          fprintf(stderr, "Could not obtain application interest (timeout)\n");   
          exit(-1);
        }
      }

      if(update) {
        ht_printf("Pushing local node description(s) to all peers ...\n");
        haggle_ipc_send_node_description(haggle);
      }

      done = false;
      if(is_publish(cmd)){
        int seqnum = 1;
        while(!done){
          if(max_seqnum >= 0 && seqnum > max_seqnum) break;

          struct timeval ct;
          char ctbuffer[1024];
          libhaggle_gettimeofday(&ct, NULL);
          snprintf(ctbuffer, 20, CREATETIME_ASSTRING_FORMAT, (long)ct.tv_sec, (long)ct.tv_usec);

          char buffer[1024];
          // Create a new data object:
          struct dataobject *dO = NULL;
          if(filename != NULL){
            char hfilename[1024];
            strcpy(hfilename, filename);
            if(copy) {
              sprintf(hfilename, "%s-%s", filename, ctbuffer);
              ht_printf("\nCopying file %s to %s ...\n", filename, hfilename);
              sprintf(buffer,"cp %s %s", filename, hfilename);
              int res = system(buffer);
              if(res < 0) {
                fprintf(stderr, "Could not make copy of file\n");   
                exit(-1);
              }
            }
            ht_printf("\nPreparing file object %s ...\n", hfilename);
            dO = haggle_dataobject_new_from_file(hfilename);
          } else {
            ht_printf("\nPreparing empty object ...\n");
            dO = haggle_dataobject_new();
          }
          if(dO == NULL){
            fprintf(stderr, "Could not create data object or access file\n");
            retcode = HAGGLE_ERROR;
            break;
          }

          // Add the correct attributes:     
          ht_printf("Adding attribute %s=%s\n",ORIGINATOR_ATTR,app);
          haggle_dataobject_add_attribute(dO, ORIGINATOR_ATTR, app);
          snprintf(buffer, 1024, "%llu", (unsigned long long)time(NULL));
          ht_printf("Adding attribute %s=%s\n",TIMESTAMP_ATTR,buffer);
          haggle_dataobject_add_attribute(dO, TIMESTAMP_ATTR, buffer);

          for(i = 0; i < al->num_attributes; i++){
            struct attribute *a = haggle_attributelist_get_attribute_n(al,i);
            ht_printf("Adding attribute %s=%s:%lu ...\n", 
              haggle_attribute_get_name(a), haggle_attribute_get_value(a), haggle_attribute_get_weight(a));
            haggle_dataobject_add_attribute_weighted(dO,
              haggle_attribute_get_name(a), haggle_attribute_get_value(a), haggle_attribute_get_weight(a));
          }

          if(seq){
            snprintf(buffer, 1024, "%d", seqnum);
            ht_printf("Adding attribute %s=%d\n",SEQNUM_ATTR,seqnum);
            haggle_dataobject_add_attribute(dO, SEQNUM_ATTR, buffer);
          }

          // haggle_attributelist_print(haggle_dataobject_get_attributelist(dO));

          // Make sure the data object is permanent:
          haggle_dataobject_set_flags(dO, DATAOBJECT_FLAG_PERSISTENT);

          // copmpute hash over data object file (if any)
          haggle_dataobject_add_hash(dO); // this is important to compute the right id
          haggle_dataobject_set_createtime(dO,&ct); // this is important to compute the right id
          ht_printf("Adding create time metadata create_time=%s\n",ctbuffer);

          const char *fn = haggle_dataobject_get_filename(dO);
          if(fn) {
            if(!seq) snprintf(buffer, 1024, "%s", fn); else snprintf(buffer, 1024, "%s-%d", fn, seqnum);
            ht_printf("Adding file name metadata FileName=%s\n",buffer);
            haggle_dataobject_set_filename(dO,buffer);
          }

          // Put it into haggle:
          ht_printf("Publishing object ...\n");
          retcode = haggle_ipc_publish_dataobject(haggle, dO);
          if (retcode != HAGGLE_NO_ERROR) {
            fprintf(stderr, "Could not publish data object\n");
            break;
          } else {
            ht_printf("OK\n");
            ht_printf("Last data object id published: %s\n", data_object_id_str(dO));
            if(logfp) {
              ssize_t datalen = haggle_dataobject_get_datalen(dO);
              // SW: keep track of data len
              // JM: Get tag data
              if (tag) {
                tagvalue=""; 
                struct attribute *a;
                a=haggle_dataobject_get_attribute_by_name(dO, tagid);
                tagvalue=haggle_attribute_get_value(a);
              }

              fprintf(logfp, "Published,%s,%s,0,0,0,0,%d,%s\n", data_object_id_str(dO), ctbuffer, (int)datalen,tag?tagvalue:"");
              fflush(logfp);
            }
            if (doidfp) {
              fprintf(doidfp, "%s\n", data_object_id_str(dO));
              fflush(doidfp);
            }
            ht_printf("Number of data objects published: %d\n", ++num_data_objects_published);
          }

          seqnum++;

          if(quit || !seq) break;
          usleep(pause * 1000);   
        }
      }

      if(is_publish(cmd) || is_subscribe(cmd) || is_resubscribe(cmd) || is_unsubscribe(cmd) || zero) {
        int counter = 0;
        for(counter = 0; stop == 0 || counter < stop * 10; counter++) {
          if(done || quit) break;
          if(stop == 0 && (is_publish(cmd) || is_unsubscribe(cmd))) break;
          if(wait > 0 && num_data_objects_received >= wait) break;
          usleep(100000);   
        }

        if(automatic && (is_subscribe(cmd) || is_resubscribe(cmd))){
          int j;
          for(j = 0; j < al->num_attributes; j++){
            struct attribute *a = haggle_attributelist_get_attribute_n(al,j);
            ht_printf("\nUnsubscribing from %s=%s ...\n",
                haggle_attribute_get_name(a), haggle_attribute_get_value(a));
          }
          
          retcode = haggle_ipc_remove_application_interests(haggle, al);
          if (retcode != HAGGLE_NO_ERROR) {
            fprintf(stderr, "Could not remove application interest\n");
            exit(-1);
          } else {
            ht_printf("OK\n\n");
          }
        }
      }
    }
  } else if (batchExe) {
    //char val[256], pubcmd[256], attrstr[256];
    float sleeptime;  
    char *sleeptimestr, *pubcmd, *attrstr, *sendfilename;
    #define MAXSIZE 1024
    char buffertok[MAXSIZE];

    float sleep_count  = 0;

    while (fgets(buffertok, MAXSIZE-1, batchfp)) {
      sleeptimestr = strtok(buffertok, ",");
      pubcmd = strtok(NULL, ",");
      attrstr = strtok(NULL, ",");
      sendfilename = strtok(NULL, ",");
      //remove ""
      int k=strlen(++sendfilename);
      while (sendfilename[k] != '"') {
          k--;
      }
      sendfilename[k]=0;
      if(strlen(sendfilename) ){      
        if(sendfilename[0] != '/') sendfilename = filename_to_full_path(sendfilename);
      }
      //make copy if -k option is specified
      char hfilename[1024];
      struct timeval ct;
      char ctbuffer[1024];
      char buffer[1024];

      sleeptime=atof(sleeptimestr);
      usleep(sleeptime*1000000.0);
      sleep_count += sleeptime;

      if (strlen(sendfilename)  && copy) {
        libhaggle_gettimeofday(&ct, NULL);
        snprintf(ctbuffer, 20, CREATETIME_ASSTRING_FORMAT, (long)ct.tv_sec, (long)ct.tv_usec);
        sprintf(hfilename, "%s-%s", sendfilename, ctbuffer);
        ht_printf("\nCopying file %s to %s ...\n", sendfilename, hfilename);
        sprintf(buffer,"ln -s %s %s", sendfilename, hfilename);
        int res = system(buffer);
        if(res < 0) {
          fprintf(stderr, "Could not make copy of file\n");   
          exit(-1);
        }
        temp_ll = (struct file_ll *) malloc(sizeof(struct file_ll));
        temp_ll->next = NULL;
        temp_ll->filename = (char *) malloc(strlen(hfilename)+1);
        strcpy(temp_ll->filename, hfilename);
        if (NULL == head) {  //first time
           head=temp_ll;
           tail=temp_ll;
        } else {
            tail->next = temp_ll;
            tail = temp_ll;
        }
      } else {
         strcpy(hfilename, sendfilename);
      }

      //create DO
      // Create a new data object:
      struct dataobject *dO = NULL;

      if(strlen(hfilename) ){
        dO = haggle_dataobject_new_from_file(hfilename);
      }
      else{
        ht_printf("\nPreparing empty object ...\n");
        dO = haggle_dataobject_new();
      }
      if(dO == NULL){
        fprintf(stderr, "Could not create data object or access file\n");
        retcode = HAGGLE_ERROR;
        break;
      }
      //add attributes
      attrstr[strlen(attrstr)-1]=0; //remove the ""
      char *attrpairstr=strtok(&attrstr[1],";");
      unsigned long attrweight;
      char av[MAX_ATTR_SIZE];
      char attr[MAX_ATTR_SIZE];
      char value[MAX_ATTR_SIZE];
      tagvalue="";
      while(NULL != attrpairstr) {
        attrweight=1;
        sscanf(attrpairstr,"%[^:]:%lu", av, &attrweight);
        sscanf(av,"%[^=]=%[^:]",attr,value);
        struct attribute *a = haggle_attribute_new(strnew(attr),strnew(value));
        haggle_attribute_set_weight(a,attrweight);
        haggle_dataobject_add_attribute_weighted(dO, attr, value, attrweight);
        if (tag) {
          if (!strcmp(attr,tagid)) {
            tagvalue=(char *) malloc(strlen(value));
            strcpy(tagvalue, value);
          } 
        }
        attrpairstr=strtok(NULL,";");
        ht_printf("Adding attribute %s=%s\n",attr, value);
      }
      // Add the correct attributes:     
      ht_printf("Adding attribute %s=%s\n",ORIGINATOR_ATTR,app);
      haggle_dataobject_add_attribute(dO, ORIGINATOR_ATTR, app);
      snprintf(buffer, 1024, "%llu", (unsigned long long)time(NULL));
      ht_printf("Adding attribute %s=%s\n",TIMESTAMP_ATTR,buffer);
      haggle_dataobject_add_attribute(dO, TIMESTAMP_ATTR, buffer);
      // Make sure the data object is permanent:
      haggle_dataobject_set_flags(dO, DATAOBJECT_FLAG_PERSISTENT);

      // copmpute hash over data object file (if any)
      haggle_dataobject_add_hash(dO); // this is important to compute the right id
      haggle_dataobject_set_createtime(dO,&ct); // this is important to compute the right id
      ht_printf("Adding create time metadata create_time=%s\n",ctbuffer);

      // Put it into haggle:
      ht_printf("Publishing object ...\n");
      retcode = haggle_ipc_publish_dataobject(haggle, dO);
      if (retcode != HAGGLE_NO_ERROR) {
        fprintf(stderr, "Could not publish data object\n");
        break;
      } else {
        ht_printf("OK\n");
        ht_printf("Last data object id published: %s\n", data_object_id_str(dO));
        if(logfp) {
          ssize_t datalen = haggle_dataobject_get_datalen(dO);
          // SW: keep track of data len
          fprintf(logfp, "Published,%s,%s,0,0,0,0,%d,%s\n", data_object_id_str(dO), ctbuffer, (int)datalen,tag?tagvalue:"");
          fflush(logfp);
        }
        tagvalue="";
        ht_printf("Number of data objects published: %d\n", ++num_data_objects_published);
      }
    }
    //shuting down, remove all files
    float shutdown_pause = pause*1000.0 - sleep_count*1000000;
    if (shutdown_pause > 0) {
        //wait -p time before deleting all files
        usleep(shutdown_pause); 
    }
    while (head != NULL) {
      printf("deleting %s\n", head->filename);
      unlink(head->filename);  
      free(head->filename);
      temp_ll = head;
      head = head->next;
      free(temp_ll);

    }

  }

  ht_printf("\nUnregistering %s from Haggle daemon\n", app);
  haggle_handle_free(haggle);

  ht_printf("Number of data objects published: %d\n", num_data_objects_published);
  ht_printf("Number of data objects received: %d\n", num_data_objects_received);

  if(num_data_objects_received >= 1) {
    ht_printf("Average delay measured from initial request: %.3lf\n", rdelay_sum/(double)num_data_objects_received);
    ht_printf("Average delay measured from object creation: %.3lf\n", cdelay_sum/(double)num_data_objects_received);
  }

  //JLM
  if (retFile) {
    fprintf(endfp, "Num objects publishd:%d\n", num_data_objects_published);
    fprintf(endfp, "Num objects received:%d\n", num_data_objects_received);
    fflush(endfp);
    fclose(endfp);
  }
  if (doidfp) {
    fclose(doidfp);
  } 
  return (retcode == HAGGLE_NO_ERROR ? 0 : -1);
  
}
