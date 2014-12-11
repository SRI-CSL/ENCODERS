/* Copyright 2008 Uppsala University
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
#include <sys/inotify.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/select.h>
#include <libhaggle/haggle.h>

#define EVENT_BUFLEN 1000
#define MAX_DIRPATH_LEN 512

#define DEFAULT_DIRNAME "File drop"

static int fd = -1;
static char *prog_name;
static int daemonize = 0;
static char dirpath[MAX_DIRPATH_LEN];
static uint32_t wd = 0;
static haggle_handle_t hh;

static void cleanup(int signal)
{
	switch(signal) {
	case SIGKILL:
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		inotify_rm_watch(fd, wd);
		close(fd);
		haggle_handle_free(hh);
		printf("Cleaned up\n");
		exit(EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

void print_usage()
{
	printf("usage:\n");
}

const char *filename_to_full_path(const char *filename)
{
	static char buf[1024];

	sprintf(buf, "%s/%s", dirpath, filename);

	return buf;
}

int neighbor_update_event(haggle_event_t *e, void *arg)
{
	list_t *pos;

	printf("Neighbor update event!\n");
	
	list_for_each(pos, &e->neighbors->nodes) {
		list_t *ppos;
		haggle_node_t *node = (haggle_node_t *)pos;

		printf("Neighbor: %s\n", haggle_node_get_name(node));

		list_for_each(ppos, &node->interfaces) {
			haggle_interface_t *iface = (haggle_interface_t *)ppos;
			printf("\t%s : [%s]\n", haggle_interface_get_type_name(iface), 
			       haggle_interface_get_identifier_str(iface));
		}
	}
        return 0;
}

int new_dataobject_event(haggle_event_t *e, void *arg)
{
	printf("New data object!\n");
	
        return 0;
}

int haggle_shutdown_event(haggle_event_t *e, void *arg)
{
	printf("Haggle was shut down!\n");
	exit(0);

        return 0;
}

char *filepath_to_filename(char *filepath)
{
	char *token, *filename, *save_ptr = NULL;

	token = strtok_r(filepath, "/", &save_ptr); 

	filename = token;

	while (token) {
		filename = token;
		token = strtok_r(NULL, "/", &save_ptr); 
	}

	return filename;
}

void add_application_interests(haggle_handle_t hh, char *str)
{
	haggle_attrlist_t *al = haggle_attributelist_new_from_string(str);

	if (!al)
		return;

	haggle_ipc_add_application_interests(hh, al);

	haggle_attributelist_free(al);
}

int main(int argc, char **argv)
{
	struct stat dir_stat;
	struct sigaction sigact;
	int ret;
        unsigned long pid;

	strncpy(dirpath, getenv("HOME"), MAX_DIRPATH_LEN);
	dirpath[strlen(dirpath)] = '/';
	strncpy(dirpath+strlen(dirpath), DEFAULT_DIRNAME, MAX_DIRPATH_LEN-strlen(dirpath));

	prog_name = argv[0];
	argv++;
	argc--;

	/* Parse commands */
	while (argc) {
		if (strcmp("-d", argv[0]) == 0 ||
		    strcmp("--daemon", argv[0]) == 0) {
			daemonize = 1;
		} else if (strcmp("-p", argv[0]) == 0 ||
			   strcmp("--path", argv[0]) == 0) {
			if (!argv[1]) {
				print_usage();
				return -1;
			}
			strcpy(dirpath, argv[1]);
			argc--;
			argv++;
		} else if (strcmp("-h", argv[0]) == 0 ||
			   strcmp("--help", argv[0]) == 0) {
			print_usage();
			return 0;
		} 
		argc--;
		argv++;
	}
	
	ret = stat(dirpath, &dir_stat);

	if (ret < 0) {
		if (errno == ENOENT) {
			ret = mkdir(dirpath, 0755);
			printf("Created directory \'%s\'\n", dirpath);
			if (ret < 0) {
				fprintf(stderr, "Could not create directory \'%s\' - %s\n", 
					dirpath, strerror(errno));
				return EXIT_FAILURE;	
			}
		} else {
			fprintf(stderr, "Bad directory - %s\n", strerror(errno));
			return EXIT_FAILURE;	
		}	
	}
	
	/* Make sure we cleanup at exit... */
	atexit((void *) &cleanup);

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = &cleanup;
	sigaction(SIGHUP, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGKILL, &sigact, NULL);

        ret = haggle_daemon_pid(&pid);

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
                printf("success!\n");
        } 

        printf("Haggle running, trying to get handle\n");

        ret = haggle_handle_get("Filedrop Linux", &hh);
        
	if (ret != HAGGLE_NO_ERROR) {
		fprintf(stderr, "Could not get haggle handle, error: %d\n", ret);
                exit(EXIT_FAILURE);                
	}

        printf("Haggle daemon pid is %lu\n", pid);

	haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, neighbor_update_event);
	haggle_ipc_register_event_interest(hh, LIBHAGGLE_EVENT_NEW_DATAOBJECT, new_dataobject_event);
        
        // For debug purposes
        haggle_ipc_add_application_interest(hh, "Picture", "dilbert");

        /* Setup inotify */
        fd = inotify_init();

	if (fd < 0) {
		fprintf(stderr, "Inotify init error - %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	ret = inotify_add_watch(fd, dirpath, IN_ATTRIB|IN_CREATE|IN_DELETE|IN_MOVED_TO|IN_MOVED_FROM|IN_MOVE_SELF);

	if (ret < 0) {
		fprintf(stderr, "Could not watch directory %s - %s\n", 
			dirpath, strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	
	wd = (uint32_t)ret;

	printf("Watching %s, watch descriptor=%u\n", dirpath, wd);
        
	ret = haggle_event_loop_run_async(hh);

	if (ret < 0) {
		fprintf(stderr, "Haggle event loop error: %d\n", ret);
		exit(-1);
	}

	if (daemonize) {
		/* Detach from terminal */
		if (fork() != 0)
			exit(0);
		/* Close stdin, stdout and stderr... */
		/*  close(0); */
		close(1);
		close(2);
		setsid();
	}
	
	for (;;) {
		char buffer[EVENT_BUFLEN];
		struct inotify_event *ievent;
		int len;
		haggle_dobj_t *dobj;

		len = read(fd, buffer, EVENT_BUFLEN);
		
		if (len < 0) {
			fprintf(stderr, "Could not read event - %s\n", strerror(errno));
			continue;
		}
		ievent = (struct inotify_event *)buffer;
		
		if (ievent->wd == wd) {
			if (ievent->mask & IN_ATTRIB) {
				printf("attribute changed : %s\n", ievent->name);
			}
			if (ievent->mask & IN_CREATE) {
				printf("file/directory created: %s\n", ievent->name);
				
				dobj = haggle_dataobject_new_from_file(filename_to_full_path(ievent->name));
				
				haggle_dataobject_add_attribute(dobj, "Picture", "dilbert");
				
                                haggle_dataobject_add_hash(dobj);

				ret = haggle_ipc_publish_dataobject(hh, dobj);

				if (ret < 0) {
					fprintf(stderr, "Could not publish dataobject\n");
				}
				haggle_dataobject_free(dobj);
				
			}
			if (ievent->mask & IN_DELETE) {
				printf("file/directory deleted: %s\n", ievent->name);
			}
			if (ievent->mask & IN_MOVED_FROM) {
				printf("file moved from directory: %s\n", ievent->name);
			}
			if (ievent->mask & IN_MOVED_TO) {
				printf("file moved to directory: %s\n", ievent->name);
				
				dobj = haggle_dataobject_new_from_file(filename_to_full_path(ievent->name));
				
				//haggle_dataobject_add_attribute(dobj, "Picture", ievent->name);
				haggle_dataobject_add_attribute(dobj, "Picture", "banana");

                                haggle_dataobject_add_hash(dobj);

				haggle_ipc_publish_dataobject(hh, dobj);

				if (ret < 0) {
					fprintf(stderr, "Could not publish dataobject\n");
				}

				haggle_dataobject_free(dobj);

			}
			if (ievent->mask & IN_MOVE_SELF) {
				printf("directory was moved\n");
			}
		}
	}

	inotify_rm_watch(fd, wd);

	close(fd);
	haggle_handle_free(hh);

	return EXIT_SUCCESS;
}
