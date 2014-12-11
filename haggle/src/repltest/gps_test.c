/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   James Mathewson (JM, JLM)
 */

#include <libhaggle/haggle.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define CONTENT_TYPE "ContentType"
#define CONTENT_ORIGINATOR "ContentOriginator"
#define TIME_STAMP "ContentCreationTime"
#define LOCATION "Location"

//needed for cntrl-c shutdown
static   haggle_handle_t hh, hh2;

static void cleanup(int signal)
{
	//kill children
	switch(signal) {
	case SIGKILL:
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		haggle_handle_free(hh);
		printf("Cleaned up\n");
		exit(EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

int onEventShutdown(haggle_event_t *e, void *arg)
{
    haggle_handle_free(hh);
    kill(0, SIGTERM);
    exit(1);
}

int onInterestList(haggle_event_t *e, void *arg)
{
        return 0;
}


int onNeighborUpdate(haggle_event_t *e, void *arg)
{
        return 0;
}

int
onDataObject (haggle_event_t * e, void *arg)
{
  int i, max_attrib;
  const char *name, *location, *timestamp;
  haggle_dobj_t *dobj = e->dobj;
  //attrib info
  max_attrib = haggle_dataobject_get_num_attributes (dobj);
  struct attribute *att;
  for (i = 0; i < max_attrib; i++)
    {
      att = haggle_dataobject_get_attribute_n (dobj, i);
      if (!strcmp (haggle_attribute_get_name (att), TIME_STAMP))
	{
	  timestamp = haggle_attribute_get_value (att);
	}
      else if (!strcmp (haggle_attribute_get_name (att), CONTENT_ORIGINATOR))
	{
	  name = haggle_attribute_get_value (att);
	}
      if (!strcmp (haggle_attribute_get_name (att), LOCATION))
	{
	  location = haggle_attribute_get_value (att);
	}
    }

  printf ("\n\n\tReceived GPS Beacon: %s is at %s at time %s\n",
	  name, location, timestamp);
  return 0;
}


int main() {

  unsigned long pid;
  haggle_dobj_t *dobj;
  struct sigaction sigact;

  set_trace_level(0);

	atexit((void *) &cleanup);

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = &cleanup;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGKILL, &sigact, NULL);

	//mutex_init(&mutex);

  int ret = haggle_daemon_pid(&pid);

   if (ret == 0) {
         printf("Haggle not running, trying to spawn daemon... ");
                
        if (haggle_daemon_spawn(NULL) != HAGGLE_NO_ERROR) {
                 printf("failed!\n");
                 exit(EXIT_FAILURE);
        }
        printf("success!\n");
   } else {
        printf("\n\nHaggle daemon is running with pid=%lu\n", pid);
   } 

   haggle_handle_get("gps_test", &hh);
   haggle_handle_get("gps_test2", &hh2);

   //haggle server shutdown
   haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_SHUTDOWN, onEventShutdown);
   //we found a file matching our interest list
   haggle_ipc_register_event_interest(hh2, LIBHAGGLE_EVENT_NEW_DATAOBJECT, onDataObject);
  //new even interest list
   haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_INTEREST_LIST, onInterestList);
   //new neighbor or update
   haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, onNeighborUpdate);

   printf("\nGen Gojira publishing beacon...\n");
   //You must specify full, absolute path.
   dobj = haggle_dataobject_new();
   haggle_dataobject_add_attribute(dobj, CONTENT_TYPE, "GPS" );
   haggle_dataobject_add_attribute(dobj, LOCATION, "here" );
   haggle_dataobject_add_attribute(dobj, CONTENT_ORIGINATOR, "Gen Gojira" );
   //add time
   char timeStr[100];
   //time_t expiration_time;
   //get current time, add time to live
   //we dont expire at 'x' time, since each node may be set differently
   //so we just have a count down timer
   //this will avoid setting the year to 1900 and keeping data we should (mal-capture)
   sprintf(timeStr, "%lu", (unsigned long) time(NULL));
   haggle_dataobject_add_attribute(dobj, TIME_STAMP, timeStr);
   // Make sure the data object is permanent:
   haggle_dataobject_set_flags(dobj, DATAOBJECT_FLAG_PERSISTENT);
   // copmpute hash over data object file (if any)
   haggle_dataobject_add_hash(dobj); // this is important to compute the right id
   haggle_dataobject_set_createtime(dobj,NULL); // this is important to compute the right id
   haggle_ipc_publish_dataobject(hh, dobj);

   //do this only if you want to time limit yourself on the original
   //registerTimeLimitedData(dobj, expiration_time);
   haggle_dataobject_free(dobj);

   sleep(1);

   //person #2
   printf("\nSgt Mothra publishing beacon...\n");
   dobj = haggle_dataobject_new();
   haggle_dataobject_add_attribute(dobj, CONTENT_TYPE, "GPS" );
   haggle_dataobject_add_attribute(dobj, LOCATION, "there" );
   haggle_dataobject_add_attribute(dobj, CONTENT_ORIGINATOR, "Sgt Mothra" );
   sprintf(timeStr, "%lu", (unsigned long) time(NULL));
   haggle_dataobject_add_attribute(dobj, TIME_STAMP, timeStr);
   // Make sure the data object is permanent:
   haggle_dataobject_set_flags(dobj, DATAOBJECT_FLAG_PERSISTENT);
   // copmpute hash over data object file (if any)
   haggle_dataobject_add_hash(dobj); // this is important to compute the right id
   haggle_dataobject_set_createtime(dobj,NULL); // this is important to compute the right id
   haggle_ipc_publish_dataobject(hh, dobj);
   haggle_dataobject_free(dobj);

   //request data
   char *str = CONTENT_TYPE "=GPS";
   haggle_attrlist_t *attrib_list = haggle_attributelist_new_from_string(str);
   haggle_ipc_add_application_interests(hh2, attrib_list);
   haggle_attributelist_free(attrib_list);

   haggle_ipc_get_application_interests_async(hh2);

   haggle_event_loop_run_async(hh);
   haggle_event_loop_run_async(hh2);
   sleep(2);
   
   //person #3
   int i;
   for(i=0; i<20; i++) {
     printf("\npvt Kong publishing beacon %d/20...\n", i);
     dobj = haggle_dataobject_new();
     haggle_dataobject_add_attribute(dobj, CONTENT_TYPE, "GPS" );
     haggle_dataobject_add_attribute(dobj, LOCATION, "elsewhere" );
     haggle_dataobject_add_attribute(dobj, CONTENT_ORIGINATOR, "pvt Kong" );
     sprintf(timeStr, "%lu", (unsigned long) time(NULL));
     haggle_dataobject_add_attribute(dobj, TIME_STAMP, timeStr);
     // Make sure the data object is permanent:
     haggle_dataobject_set_flags(dobj, DATAOBJECT_FLAG_PERSISTENT);
     // copmpute hash over data object file (if any)
     haggle_dataobject_add_hash(dobj); // this is important to compute the right id
     haggle_dataobject_set_createtime(dobj,NULL); // this is important to compute the right id
     haggle_ipc_publish_dataobject(hh, dobj);
     haggle_dataobject_free(dobj);
     sleep(5);

     if (!(i%5)) {  //every 5 tries, send OLD data
       printf("\npvt Kong publishing obsolete data (should not be received)...\n");
       dobj = haggle_dataobject_new();
       haggle_dataobject_add_attribute(dobj, CONTENT_TYPE, "GPS" );
       haggle_dataobject_add_attribute(dobj, LOCATION, "elsewhere" );
       haggle_dataobject_add_attribute(dobj, CONTENT_ORIGINATOR, "pvt Kong" );
       sprintf(timeStr, "%lu", (unsigned long) i+10);
       haggle_dataobject_add_attribute(dobj, TIME_STAMP, timeStr);
       // Make sure the data object is permanent:
       haggle_dataobject_set_flags(dobj, DATAOBJECT_FLAG_PERSISTENT);
       // copmpute hash over data object file (if any)
       haggle_dataobject_add_hash(dobj); // this is important to compute the right id
       haggle_dataobject_set_createtime(dobj,NULL); // this is important to compute the right id
       haggle_ipc_publish_dataobject(hh, dobj);
       haggle_dataobject_free(dobj);
       
       sleep(1);
     }
   }

  return 0;

}
