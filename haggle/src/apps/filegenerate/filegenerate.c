#include "libhaggle/haggle.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
/*
 * Extended ians haggle file publisher
 * continually generates and publishes files
 * starts haggle if haggle not already started
 *
 */
haggle_handle_t haggle;

#define APPLICATION_NAME_FORMAT "filegenerate-test@%s"

#define N 10

char application_name[PATH_MAX];

void startHaggle() {
	unsigned long  pid = -1;
	int ret = haggle_daemon_pid(&pid);

	if (ret == HAGGLE_ERROR) {
		printf("Could not get Haggle pid\n");
		exit(EXIT_FAILURE);
	}
	if (ret == 0) {
		printf("Haggle not running, trying to spawn daemon... ");

		if (haggle_daemon_spawn(NULL) != 1) {
			printf("failed!\n");
			exit(EXIT_FAILURE);
		}
		printf("success starting haggle!\n");
	}
}

int on_neighbor_update(haggle_event_t *e, void* nix) {
	haggle_nodelist_t *nl = e->neighbors;
	int num_neighbors = haggle_nodelist_size(nl);

	fprintf(stderr, "number of neighbors is %d\n", num_neighbors);

	return 0;
}

int on_dataobject(haggle_event_t *e, void* nix) {
	if (e->dobj) {
		const unsigned char *xml = haggle_dataobject_get_raw(e->dobj);
		const char * filepath = haggle_dataobject_get_filepath(e->dobj);
		if (xml) {
			fprintf(stderr, "Received data object:\n%s\n", (const char *) xml);
		}
		if (filepath) {
			fprintf(stderr, "Received data object with file:\n%s\n",
					(const char *) filepath);
		}
	}
	return 0;
}

int on_shutdown(haggle_event_t *e, void* nix) {
	fprintf(stderr, "Shutdown Event\n");
	haggle_handle_free(haggle);
	fprintf(stderr, "Shutdown SUCCESS!\n");
	exit(0);
	return 0;
}

int on_interests(haggle_event_t *e, void* nix) {
	fprintf(stderr, "Received application interests event\n");
	return 0;
}

int register4events(haggle_handle_t *hh) {
	int retcode = haggle_ipc_register_event_interest(haggle,
			LIBHAGGLE_EVENT_SHUTDOWN, on_shutdown);
	if (retcode != HAGGLE_NO_ERROR) {
		fprintf(stderr, "Could not register shutdown event interest\n");
		return retcode;
	} else {
		fprintf(stderr, "Registered shutdown event interest\n");
	}
	retcode = haggle_ipc_register_event_interest(haggle,
			LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, on_neighbor_update);
	if (retcode != HAGGLE_NO_ERROR) {
		fprintf(stderr, "Could not register neighbor update event interest\n");
		return retcode;
	} else {
		fprintf(stderr, "Registered neighbor update event interest\n");
	}
	retcode = haggle_ipc_register_event_interest(haggle,
			LIBHAGGLE_EVENT_NEW_DATAOBJECT, on_dataobject);
	if (retcode != HAGGLE_NO_ERROR) {
		fprintf(stderr, "Could not register new data object event interest\n");
		return retcode;
	} else {
		fprintf(stderr, "Registered new data object event interest\n");
	}
	retcode = haggle_ipc_register_event_interest(haggle,
			LIBHAGGLE_EVENT_INTEREST_LIST, on_interests);
	if (retcode != HAGGLE_NO_ERROR) {
		fprintf(stderr, "Could not register interest list event interest\n");
		return retcode;
	} else {
		fprintf(stderr, "Registered interest list event interest\n");
	}
	if (haggle_event_loop_run_async(haggle) != HAGGLE_NO_ERROR) {
		fprintf(stderr, "Could not start event loop\n");
		return retcode;
	} else {
		fprintf(stderr, "Started event loop\n");
	}
	return HAGGLE_NO_ERROR;
}

int main(int argc, char *argv[]) {
	int retval = 0;
	if (argc < 3) {
		fprintf(stderr, "Usage: %s 'my name' 'their name' [active]\n", argv[0]);
		return 0;
	}

	startHaggle();

	int retcode = HAGGLE_NO_ERROR;
	char* me = argv[1];
	char* you = argv[2];
	int active = argc > 3;
	snprintf(application_name, PATH_MAX, APPLICATION_NAME_FORMAT, me);

	retcode = haggle_handle_get(application_name, &haggle);

	if (retcode != HAGGLE_NO_ERROR) {
		fprintf(stderr, "haggle_handle_get FAILED: %d\n", retcode);
		return retcode;
	}
	fprintf(stderr, "haggle_handle_get SUCCESS: %d\n", retcode);

	retcode = register4events(&haggle);

	if (retcode != HAGGLE_NO_ERROR) {
		fprintf(stderr, "register4events FAILED: %d\n", retcode);
		return retcode;
	}

	int count = 0;
	char countbuffer[1024];
	char message[1024];

	haggle_ipc_add_application_interest(haggle, "FileOwner", you);

	int x = 0;

	if (active) {
		for (x = 0; x < N; x++) {

			struct dataobject *dO;

			snprintf(countbuffer, 1024, "%d", count);
			// Create a new data object:
			memset(&message,0,1024);
			sprintf(message,"joshmessage |%d| |%s| ",count,me);
			//snprintf(&message,1024,"joshmessage |%d| |%s|",count,me);
			dO = haggle_dataobject_new_from_buffer(message, strlen(message));
			//const unsigned char* randomfile = "test";
			//dO = haggle_dataobject_new_from_buffer(randomfile, 4);

			// Add the correct attribute:
			haggle_dataobject_add_attribute(dO, "FileOwner", me);
			haggle_dataobject_add_attribute(dO, "Count", countbuffer);
			//haggle_dataobject_add_attribute(dO, "Name", filename);
			// Make sure the data object is permanent:
			haggle_dataobject_set_flags(dO, DATAOBJECT_FLAG_PERSISTENT);
			// Put it into haggle:
			fprintf(stderr, "Publishing NOW!\n");
			haggle_ipc_publish_dataobject(haggle, dO);

			fprintf(stderr, "About to publish file[%d] in %d seconds\n", count,
					N);
			sleep(N);

			count++;
		}
	}

	while(1){ sleep(10);};


	fprintf(stderr, "Falling out the bottom\n");

	haggle_handle_free(haggle);

	return retval;
}
