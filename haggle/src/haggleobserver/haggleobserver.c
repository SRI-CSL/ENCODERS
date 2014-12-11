/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

/*

Observer Application for the Haggle API

*/


#include "libhaggle/haggle.h"
#include "libhaggle/attribute.h"
#include "libhaggle/list.h"
#include "../libhaggle/metadata.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "docopt.c"

#define LOG(...) { printf(__VA_ARGS__); }
#define ERROR(...) { fprintf(stderr, __VA_ARGS__); }

static haggle_handle_t haggle;
static const char *appName = "haggleobserver";
static int retcode;
static bool done = false;

static FILE *logfp = NULL;
static char *LOG_FILE_PATH = "haggleobserver.log";
static size_t DURATION = 30;
static int REDUCE_DISTRIBUTION_FREQUENCY_AFTER = -1;
static int SET_MAX_HOP_COUNT_AFTER = -1;
static int SET_MAX_HOP_COUNT_AFTER_AVERAGE = -1;

static char *OBSERVER_ATTRIBUTE_NAME = "ObserverDataObject";
static char *OBSERVER_ATTRIBUTE_VALUE = "true";
static struct attributelist *HOP_COUNTS;

bool registerWithHaggle() {

  LOG("Registering with Haggle daemon ...\n");
  retcode = haggle_handle_get(appName, &haggle);
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't register with Haggle daemon, error %d\n", retcode);
    return false;
  }

  int session = haggle_handle_get_session_id(haggle);
  LOG("Registered with sesion id %d\n", session);
  return true;

}

void unregisterWithHaggle() {

  LOG("Unregistering with Haggle daemon ...\n");
  haggle_handle_free(haggle);

}

int onHaggleShutdownEvent(haggle_event_t *e, void *ignore) {

  LOG("Received shutdown event!\n");
  done = true;
  return 0;

}

bool registerShutdownEventInterest() {

  LOG("Registering shutdown event interest ...\n");
  retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_SHUTDOWN, onHaggleShutdownEvent);
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't register shutdown event interest, error %d\n", retcode);
    return false;
  }
  return true;

}

bool reduceDistributionFrequency() {

  const char *config = "" \
"  <ApplicationManager reset_bloomfilter_at_registration=\"false\">"\
"      <Attr name=\"ObserverDataObject\">true</Attr>"\
"      <ObserverConfiguration>"\
"          <ObserveInterfaces>true</ObserveInterfaces>"\
"          <ObserveCacheStrategy>true</ObserveCacheStrategy>"\
"          <ObserveNodeDescription>true</ObserveNodeDescription>"\
"          <ObserveNodeStore>true</ObserveNodeStore>"\
"          <ObserveCertificates>true</ObserveCertificates>"\
"          <ObserveRoutingTable>true</ObserveRoutingTable>"\
"          <ObserveDataStoreDump>true</ObserveDataStoreDump>"\
"          <ObserveNodeMetrics>true</ObserveNodeMetrics>"\
"          <ObserveProtocols>true</ObserveProtocols>"\
"          <NotificationInterval>20</NotificationInterval>"\
"          <Attributes>"\
"              <Attr name=\"ObserverDataObject\">true</Attr>"\
"              <Attr name=\"CreationTime\" every=\"2\">%%replace_current_time%%</Attr>"\
"              <Attr name=\"ContentOrigin\">%%replace_current_node_name%%</Attr>"\
"              <Attr name=\"ContentOriginId\">%%replace_current_node_id%%</Attr>"\
"              <Attr name=\"ObserverDataObjectOrigin\">%%replace_current_node_name%%</Attr>"\
"          </Attributes>"\
"      </ObserverConfiguration>"\
"  </ApplicationManager>";
  LOG("Reducing Distribution Frequency ...\n");
  retcode = haggle_ipc_update_configuration_dynamic(haggle, config, strlen(config));
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't reduce distribution frequency, error %d\n", retcode);
    return false;
  }
  REDUCE_DISTRIBUTION_FREQUENCY_AFTER = -1;
  return true;

}

bool setMaxHopCount() {

  const char *config = "" \
"  <ApplicationManager reset_bloomfilter_at_registration=\"false\">"\
"      <Attr name=\"ObserverDataObject\">true</Attr>"\
"      <ObserverConfiguration>"\
"          <ObserveInterfaces>true</ObserveInterfaces>"\
"          <ObserveCacheStrategy>true</ObserveCacheStrategy>"\
"          <ObserveNodeDescription>true</ObserveNodeDescription>"\
"          <ObserveNodeStore>true</ObserveNodeStore>"\
"          <ObserveCertificates>true</ObserveCertificates>"\
"          <ObserveRoutingTable>true</ObserveRoutingTable>"\
"          <ObserveDataStoreDump>true</ObserveDataStoreDump>"\
"          <ObserveNodeMetrics>true</ObserveNodeMetrics>"\
"          <ObserveProtocols>true</ObserveProtocols>"\
"          <NotificationInterval>10</NotificationInterval>"\
"          <Attributes>"\
"              <Attr name=\"ObserverDataObject\">true</Attr>"\
"              <Attr name=\"CreationTime\" every=\"2\">%%replace_current_time%%</Attr>"\
"              <Attr name=\"ContentOrigin\">%%replace_current_node_name%%</Attr>"\
"              <Attr name=\"ContentOriginId\">%%replace_current_node_id%%</Attr>"\
"              <Attr name=\"ObserverDataObjectOrigin\">%%replace_current_node_name%%</Attr>"\
"              <Attr name=\"hopcount_metadata\">ttl</Attr>"\
"              <Attr name=\"hopcount_max\">1</Attr>"\
"          </Attributes>"\
"      </ObserverConfiguration>"\
"  </ApplicationManager>";
  LOG("Setting Max Hop Count for Observer Data Objects ...\n");
  retcode = haggle_ipc_update_configuration_dynamic(haggle, config, strlen(config));
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't set max hop count, error %d\n", retcode);
    return false;
  }
  SET_MAX_HOP_COUNT_AFTER = -1;
  return true;

}

int countOneHopNeighbors(const char *fileContents) {

  int count = 0;
  char *current = NULL;
  char tagBuffer[128];
  char *tag = NULL;
  char *blockStart = NULL;
  char *blockEnd = NULL;
  size_t blockSize = 0;
  char *tmp = NULL;
  const char *xmlStartTag = "<?xml version=\"1.0\"?>\n";
  const size_t xmlStartTagLen = strlen(xmlStartTag);
  metadata_t *m = NULL;
  metadata_t *node = NULL;
  metadata_t *interfaces = NULL;
  metadata_t *interface = NULL;
  char nodeName[128];
  struct attribute *attr;

  memset(nodeName, 0, sizeof(nodeName));

  current = fileContents;
  while (true) {
    memset(tagBuffer, 0, sizeof(tagBuffer));
    blockStart = strstr(current, xmlStartTag);
    if (blockStart == NULL) {
      break;
    }

    tmp = blockStart + xmlStartTagLen;
    tmp++;
    tagBuffer[0] = '<';
    tagBuffer[1] = '/';
    tag = tagBuffer + 2;
    while (*tmp) {
      *tag = *tmp;
      if (*tag == '>') {
        tmp++;
        break;
      }
      tag++;
      tmp++;
    }

    blockEnd = strstr(tmp, tagBuffer);
    if (!blockEnd) {
      ERROR("Couldn't find end tag %s!\n", tagBuffer);
      break;
    }

    blockSize = ((size_t) blockEnd - (size_t) blockStart) + strlen(tagBuffer) + 1;
    tmp = malloc((blockSize + 1) * sizeof(char));
    if (!tmp) {
      ERROR("Couldn't malloc tmp buffer of size %lu - (%p, %p)!\n", (blockSize + 1), blockEnd, blockStart);
      break;
    }

    strncpy(tmp, current, blockSize);
    tmp[blockSize] = '\0';
    current = blockEnd + strlen(tagBuffer) + 1;

    m = metadata_new_from_raw(tmp, blockSize);
    if (!m) {
      free(tmp);
      break;
    }

    if (strcmp(metadata_get_name(m), "ObserveNodeStore") == 0) {
      node = metadata_get(m, "Node");
      do {
        if (!node) {
          break;
        }
        if (strcmp(metadata_get_parameter(node, "type"), "local_device") == 0) {
          strncpy(nodeName, metadata_get_parameter(node, "name"), sizeof(nodeName)-1);
          continue;
        }
        if (strcmp(metadata_get_parameter(node, "type"), "peer") != 0) {
          continue;
        }
        interfaces = metadata_get(node, "Interfaces");
        if (!interfaces) {
          continue;
        }
        interface = metadata_get(interfaces, "Interface");
        do {
          if (!interface) {
            break;
          }
          if (strcmp(metadata_get_parameter(interface, "is_up"), "true") == 0) {
            count++;
            break;
          }
        } while ((interface = metadata_get_next(interfaces)));
      } while ((node = metadata_get_next(m)));
    }

    metadata_free(m);
    free(tmp);
  }

  attr = haggle_attributelist_get_attribute_by_name(HOP_COUNTS, nodeName);
  if (attr) {
    haggle_attributelist_detach_attribute(HOP_COUNTS, attr);
    haggle_attribute_free(attr);
    attr = NULL;
  }

  attr = haggle_attribute_new_weighted(nodeName, nodeName, count);
  if (!attr) {
    ERROR("Couldn't allocate memory to save hop count!\n");
    return count;
  }
  haggle_attributelist_add_attribute(HOP_COUNTS, attr);

  return count;

}

int computeAverageHopCount() {

  list_t *pos;
  int sum = 0;
  int count = 0;

  list_for_each(pos, &HOP_COUNTS->attributes) {
    struct attribute *attr = (struct attribute *) pos;
    count++;
    sum += haggle_attribute_get_weight(attr);
  }

  return (sum / count);
}

int logObserverDataObject(haggle_event_t *e, void *ignore) {
  struct dataobject *dobj = e->dobj;

  if (!dobj) {
    return -1;
  }

  size_t size;
  int retcode;

  retcode = haggle_dataobject_get_data_size(dobj, &size);
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't get data object size, error %d\n", retcode);
    return 0;
  }

  // haggle_dataobject_print(stdout, dobj);

  void *data = haggle_dataobject_get_data_all(dobj);
  if (!data) {
    ERROR("Couldn't get data object data, error %d\n", retcode);
    return 0;
  }

  fwrite(data, size, 1, logfp);
  fflush(logfp);

  int oneHopNeighbors = countOneHopNeighbors((const char *) data);
  if ((SET_MAX_HOP_COUNT_AFTER > 0) &&
       (oneHopNeighbors >= SET_MAX_HOP_COUNT_AFTER)) {
    LOG("One hop neighbors: %d >= %d\n", oneHopNeighbors, SET_MAX_HOP_COUNT_AFTER);
    setMaxHopCount();
  } else {
    // FIXME: Should probably purge stale entries (i.e. those from neighbors we have not seen for a long time)
    // to keep the average hop count more accurate.
    int oneHopNeighborsAverage = computeAverageHopCount();
    if ((SET_MAX_HOP_COUNT_AFTER_AVERAGE > 0) &&
         (oneHopNeighborsAverage >= SET_MAX_HOP_COUNT_AFTER_AVERAGE)) {
      LOG("Average One hop neighbors: %d >= %d\n", oneHopNeighborsAverage, SET_MAX_HOP_COUNT_AFTER_AVERAGE);
      setMaxHopCount();
    }
  }

  free(data);

  return 0;
}

bool registerObserverDataObjectEventInterest() {

  LOG("Registering observer data object event interest ...\n");
  retcode = haggle_ipc_register_event_interest(haggle, LIBHAGGLE_EVENT_NEW_DATAOBJECT, logObserverDataObject);
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't register observer data object event interest, error %d\n", retcode);
    return false;
  }

  return true;

}

bool runHaggleEventLoop() {

  LOG("Starting event loop ...\n");
  retcode = haggle_event_loop_run_async(haggle);
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't run haggle event loop, error %d\n", retcode);
    return false;
  }
  return true;

}

bool configureObserver() {

  struct attributelist *al = haggle_attributelist_new();
  LOG("Configuring observer ...\n");

  if (!al) {
    ERROR("Couldn't create attribute list!\n");
    return false;
  }

  struct attribute *a = haggle_attribute_new(OBSERVER_ATTRIBUTE_NAME, OBSERVER_ATTRIBUTE_VALUE);
  if (!a) {
    ERROR("Couldn't add attribute %s=%s!\n", OBSERVER_ATTRIBUTE_NAME, OBSERVER_ATTRIBUTE_VALUE);
    return false;
  }

  haggle_attributelist_add_attribute(al, a);

  retcode = haggle_ipc_add_application_interests(haggle, al);
  if (retcode != HAGGLE_NO_ERROR) {
    ERROR("Couldn't subscribe to observer attribute, error %d\n", retcode);
    return false;
  }

  return true;
}

bool init(int argc, char *argv[]) {
  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "1.0");
  LOG_FILE_PATH = args.logfile;
  DURATION = strtoul(args.duration, NULL, 10);
  OBSERVER_ATTRIBUTE_NAME = args.attribute_name;
  OBSERVER_ATTRIBUTE_VALUE = args.attribute_value;
  REDUCE_DISTRIBUTION_FREQUENCY_AFTER = atoi(args.reduce_dist_frequency_after);
  SET_MAX_HOP_COUNT_AFTER = atoi(args.set_max_hop_count_after);
  SET_MAX_HOP_COUNT_AFTER_AVERAGE = atoi(args.set_max_hop_count_after_average);

  if ((logfp = fopen(LOG_FILE_PATH, "w")) == 0) {
    ERROR("Couldn't open %s\n", LOG_FILE_PATH);
    return false;
  }

  HOP_COUNTS = haggle_attributelist_new();
  if (!HOP_COUNTS) {
    ERROR("Couldn't allocate memory!\n");
    return false;
  }

  return true;
}

bool cleanup() {
  if (logfp) {
    fclose(logfp);
  }
  if (HOP_COUNTS) {
    haggle_attributelist_free(HOP_COUNTS);
  }
  return true;
}

int main(int argc, char *argv[]) {

  // Mute libhaggle
  set_trace_level(0);

  if (!init(argc, argv)) {
    exit(1);
  }

  if (!registerWithHaggle()) {
    exit(2);
  }

  if (!registerShutdownEventInterest()) {
    exit(3);
  }

  if (!registerObserverDataObjectEventInterest()) {
    exit(4);
  }

  if (!configureObserver()) {
    exit(5);
  }

  if (!runHaggleEventLoop()) {
    exit(6);
  }

  int ticks = 0;
  while ((ticks <= DURATION) && !done) {
    if (REDUCE_DISTRIBUTION_FREQUENCY_AFTER > 0
        && ticks == REDUCE_DISTRIBUTION_FREQUENCY_AFTER)
    {
      if (!reduceDistributionFrequency()) {
        break;
      }
    }
    usleep(1000000);
    ticks++;
  }

  unregisterWithHaggle();

  if (!cleanup()) {
    exit(7);
  }

}
