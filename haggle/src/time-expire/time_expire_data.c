/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM, JLM)
 */

#undef DEBUG


#include <libhaggle/haggle.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

//needed for cntrl-c shutdown
static haggle_handle_t hh;
static char *unique_name = "godzilla";

//Timer parameter name
#define CONTENT_TYPE "ContentType"
#define CONTENT_TYPE_REL_VALUE "DelByRelTime"
#define CONTENT_TYPE_ABS_VALUE "DelByAbsTime"
#define TIME_STAMP "ContentDeleteTime"

//default values

static void
cleanup (int signal)
{
  //kill children
  switch (signal)
    {
    case SIGKILL:
    case SIGHUP:
    case SIGINT:
    case SIGTERM:
      haggle_handle_free (hh);
      printf ("Cleaned up\n");
      exit (EXIT_SUCCESS);
      break;
    default:
      break;
    }
}

int
onEventShutdown (haggle_event_t * e, void *arg)
{
  haggle_handle_free (hh);
  kill (0, SIGTERM);
  exit (1);
}

int
onInterestList (haggle_event_t * e, void *arg)
{
  return 0;
}


int
onNeighborUpdate (haggle_event_t * e, void *arg)
{
  return 0;
}

int
onDataObject (haggle_event_t * e, void *arg)
{
  int i, max_attrib;
  const char *name, *location, *timestamp;
  haggle_dobj_t *dobj = e->dobj;
  return 0;
}


int
main (int argc, char **argv)
{

  unsigned long pid;
  haggle_dobj_t *dobj;
  struct sigaction sigact;
  unsigned long long time_ll = 0, timer_interval = 0;
  int relative = 0, absolute = 0;

  set_trace_level (0);
  int plusTime = 0;
  int c;
  while ((c = getopt (argc, argv, "rat:n:")) != -1)
    {
      switch (c)
	{
	case 'r':		//relative
	  relative = 1;
	  break;
	case 't':		//time, +time is relative to now
	  if (optarg[0] != '+')
	    {
		plusTime = 0;
	      timer_interval = strtoull (optarg, NULL, 0);
	    }
	  else
	    {
		plusTime = 1;
	      timer_interval = strtoull (&optarg[1], NULL, 0);
	    }
	  break;
	case 'a':		//absolute
	  absolute = 1;
	  break;
	case 'n':		//name, must be unique
	  unique_name = optarg;
	  break;
	default:
	  printf ("Unknown option.   valid options are:\n");
	  printf ("\t-r\t Expire in relative to current time");
	  printf
	    ("\t-t <seconds>\t Number seconds (raw) to use to delete.\n");
	  printf ("\t-t +<seconds>\t Number seconds from now to delete\n");
	  printf
	    ("\t-a\t Expire in absolute time (seconds) from the Epoch.\n");
	  printf ("\t-n <name>\t unique name of individual or identifier\n");
	  exit (1);
	  break;
	}
    }

  if (relative && absolute)
    {
      printf ("You can not set both relative and absolute at same time\n");
      return 1;
    }
printf("absolute = %d, plusTime = %d\n", absolute, plusTime);
  if (absolute && plusTime) {
      timer_interval += time(NULL);
  }

  atexit ((void *) &cleanup);
  memset (&sigact, 0, sizeof (struct sigaction));
  sigact.sa_handler = &cleanup;
  sigaction (SIGHUP, &sigact, NULL);
  sigaction (SIGINT, &sigact, NULL);
  sigaction (SIGKILL, &sigact, NULL);

  //mutex_init(&mutex);

  int ret = haggle_daemon_pid (&pid);

  if (ret == 0)
    {
      printf ("Haggle not running, trying to spawn daemon... ");

      if (haggle_daemon_spawn (NULL) != HAGGLE_NO_ERROR)
	{
	  printf ("failed!\n");
	  exit (EXIT_FAILURE);
	}
      printf ("success!\n");
    }
  else
    {
      printf ("\n\nHaggle daemon is running with pid=%lu\n", pid);
    }

  haggle_handle_get (unique_name, &hh);

  //haggle server shutdown
  haggle_ipc_register_event_interest (hh, LIBHAGGLE_EVENT_SHUTDOWN,
				      onEventShutdown);
  //we found a file matching our interest list
  haggle_ipc_register_event_interest (hh, LIBHAGGLE_EVENT_NEW_DATAOBJECT,
				      onDataObject);
  //new even interest list
  haggle_ipc_register_event_interest (hh, LIBHAGGLE_EVENT_INTEREST_LIST,
				      onInterestList);
  //new neighbor or update
  haggle_ipc_register_event_interest (hh, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE,
				      onNeighborUpdate);

  //haggle_ipc_get_application_interests_async(hh);
  //start off async
  haggle_event_loop_run_async (hh);

  dobj = haggle_dataobject_new ();
  if (relative)
    {
      haggle_dataobject_add_attribute (dobj, CONTENT_TYPE,
				       CONTENT_TYPE_REL_VALUE);
      time_ll = timer_interval;
    }
  else
    {
      haggle_dataobject_add_attribute (dobj, CONTENT_TYPE,
				       CONTENT_TYPE_ABS_VALUE);
      time_ll = timer_interval;
    }

  //haggle_dataobject_add_attribute (dobj, CONTENT_ORIGINATOR, unique_name);
  //add current time
//relative time
  char timeStr[250];
  sprintf (timeStr, "%llu", time_ll);
  haggle_dataobject_add_attribute (dobj, TIME_STAMP, timeStr);
printf("%s = %s\n", TIME_STAMP, timeStr);

 //add a random trait, otherwise, sending the same info twice will be ignored
 sprintf (timeStr, "%llu", (unsigned long long) time(NULL));
  haggle_dataobject_add_attribute (dobj, "junk", timeStr);
  // Make sure the data object is permanent:
  haggle_dataobject_set_flags (dobj, DATAOBJECT_FLAG_PERSISTENT);
  // copmpute hash over data object file (if any)
  haggle_dataobject_add_hash (dobj);	// this is important to compute the right id
  haggle_dataobject_set_createtime (dobj, NULL);	// this is important to compute the right id
  haggle_ipc_publish_dataobject (hh, dobj);
  haggle_dataobject_free (dobj);


  time_t time_n = (time_t) time_ll;
if (relative)
	time_n += time(NULL);

  char *timeExp = ctime(&time_n);

printf("now is %s\n", timeStr);

  printf ("Data object[%s] : %s = %s, will be deleted at time %s\n",
	  unique_name, CONTENT_TYPE,
	  relative ? CONTENT_TYPE_REL_VALUE : CONTENT_TYPE_ABS_VALUE, timeExp);

  time_n = time(NULL);
  printf("now is %s", ctime(&time_n));
  sleep (100);
  return 0;
}
