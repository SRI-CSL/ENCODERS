/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
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
static int listen_gps;
static char *unique_name;

//Timer parameter name
#define CONTENT_TYPE "ContentType"
#define CONTENT_TYPE_VALUE "GPS"
#define CONTENT_ORIGINATOR "ContentOriginator"
#define TIME_STAMP "ContentCreationTime"
#define LOCATION "Location"
#define LOCATION_VALUE "Nowhere"

//default values
#define NAME "Godzilla"
#define MAX_GPS_BCASTS 1000
#define TIME_BETWEEN_BCASTS 10
#define JITTER_TIME 2
#define LISTEN_DEFAULT 1

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

 switch (listen_gps)
    {
    case 1:
      printf ("\n\n\tReceived GPS Beacon: %s is at %s at time %s\n",
	      name, location, timestamp);
      break;
    case 2:
      if (strcmp (name, unique_name) != 0)
	{
	  printf ("\n\n\tReceived GPS Beacon: %s is at %s at time %s\n",
		  name, location, timestamp);
	}
      break;
    default:
      break;
    }
  return 0;
}


int
main (int argc, char **argv)
{

  unsigned long pid;
  haggle_dobj_t *dobj;
  struct sigaction sigact;


  //parameter parsing/defaults
  char *gps_location = LOCATION_VALUE;
  char timeStr[100];
  int timer_interval;
  int jitter;
  int max_bcasts;
  int time_to_sleep;

  set_trace_level(0);

  max_bcasts = MAX_GPS_BCASTS;
  listen_gps = LISTEN_DEFAULT;
  timer_interval = TIME_BETWEEN_BCASTS;
  jitter = JITTER_TIME;

  int c;
  while ((c = getopt (argc, argv, "r:t:j:m:l:n:")) != -1)
    {
      switch (c)
	{
	case 'r':
	  listen_gps = (int) strtoul (optarg, NULL, 0);
	  break;
	case 't':
	  timer_interval = (int) strtoul (optarg, NULL, 0);
	  break;
	case 'j':
	  jitter = (int) strtoul (optarg, NULL, 0);
	  break;
	case 'm':
	  max_bcasts = (int) strtoul (optarg, NULL, 0);
	  break;
	case 'l':
	  gps_location = optarg;
	  break;
	case 'n':
	  unique_name = optarg;
	  break;
	default:
	  printf ("Unknown option.   valid options are:\n");
	  printf
	    ("\t-r 0/1/2\t 0=no receive GPS beacons, 1=receive all, 2=recieve all but your own\n");
	  printf
	    ("\t-t <seconds>\t Number seconds (base) between GPS broadcasts\n");
	  printf
	    ("\t-j <seconds>\t +-value for jitter off base GPS broadcast time\n");
	  printf ("\t-m <number\t maximum gps beacon broadcasts\n");
	  printf ("\t-l <string>\t location in ascii\n");
	  printf ("\t-n <name>\t unique name of individual or identifier\n");
	  exit(1);
	  break;
	}
    }
  atexit ((void *) &cleanup);
//printf("-r %d, -t %d, -j %d, -m %d, -l %s, -n %s\n", listen_gps, timer_interval, jitter, max_bcasts, gps_location, unique_name);
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

  //request GPS Beacon data
  char *str = "ContentType=GPS";
  haggle_attrlist_t *attrib_list = haggle_attributelist_new_from_string (str);
  haggle_ipc_add_application_interests (hh, attrib_list);
  haggle_attributelist_free (attrib_list);

  //haggle_ipc_get_application_interests_async(hh);
  //start off async
  haggle_event_loop_run_async (hh);

  int i;
  for (i = 0; i < max_bcasts; i++)
    {
      //Create a new GPS Beacon
      dobj = haggle_dataobject_new ();
      haggle_dataobject_add_attribute (dobj, CONTENT_TYPE,
				       CONTENT_TYPE_VALUE);
      haggle_dataobject_add_attribute (dobj, LOCATION, gps_location);
      haggle_dataobject_add_attribute (dobj, CONTENT_ORIGINATOR, unique_name);
      //add current time
      sprintf (timeStr, "%llu", (unsigned long long) time (NULL));
      haggle_dataobject_add_attribute (dobj, TIME_STAMP, timeStr);
      // Make sure the data object is permanent:
      haggle_dataobject_set_flags(dobj, DATAOBJECT_FLAG_PERSISTENT);
      // copmpute hash over data object file (if any)
      haggle_dataobject_add_hash(dobj); // this is important to compute the right id
      haggle_dataobject_set_createtime(dobj,NULL); // this is important to compute the right id
      haggle_ipc_publish_dataobject (hh, dobj);
      haggle_dataobject_free (dobj);
      time_to_sleep = timer_interval + (random () % (2 * jitter)) - jitter;
      //printf ("%d :: sleep time is %d\n", i, time_to_sleep);
      sleep (time_to_sleep);
    }
  return 0;
}
