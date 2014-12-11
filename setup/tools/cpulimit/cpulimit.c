/**
 * This program is licensed under the GNU General Public License,
 * version 2. A copy of the license can be found in the accompanying
 * LICENSE file.
 *
 **********************************************************************
 *
 * Simple program to limit the cpu usage of a process
 * If you modify this code, send me a copy please
 *
 * Author:  Angelo Marletta
 * Date:    26/06/2005
 * Version: 1.1
 *
 * Modifications and updates by: Jesse Smith
 * Date: May 4, 2011
 * Version 1.2 
 * Date: Jan 29, 2013
 * Version 1.2 and newer
 *
 * Modifications and updates by: Hasnain Lakhani
 * Date: Mar 26, 2014
 * Version 2.1
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>    // for compatibility

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif


#ifdef FREEBSD
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#endif


//kernel time resolution (inverse of one jiffy interval) in Hertz
//i don't know how to detect it, then define to the default (not very clean!)
#define HZ 100

//some useful macro
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

// For platforms without PATH_MAX
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BEST_PRIORITY -10

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef VERSION
#define VERSION 2.1
#endif

//pid of the controlled process
pid_t pid = 0;
pid_t my_pid;     // this process's PID

//executable file name
char *program_name;
//verbose mode
int verbose = FALSE;
//lazy mode
int lazy = FALSE;
// is higher priority nice possible?
int nice_lim;

// number of CPUs we detected
int NCPU;

// quiet mode
int quiet = FALSE;

//reverse byte search
// void *memrchr(const void *s, int c, size_t n);



//return ta-tb in microseconds (no overflow checks!)
inline long timediff(const struct timespec *ta,const struct timespec *tb) {
    unsigned long us = (ta->tv_sec-tb->tv_sec)*1000000 + (ta->tv_nsec/1000 - tb->tv_nsec/1000);
    return us;
}



int Check_Us(pid_t target_pid)
{
   pid_t this_pid;

   this_pid = getpid();
   if (this_pid == target_pid)
   {
      fprintf(stderr, "We cannot throttle ourselves.\n");
      exit(7);
   }
   return TRUE;
}


int waitforpid(int pid) {
	//switch to low priority
	// if (setpriority(PRIO_PROCESS,getpid(),19)!=0) {
        /*
        if ( (nice_lim < INT_MAX) && 
             (setpriority(PRIO_PROCESS, my_pid, 19) != 0) ) {
		printf("Warning: cannot renice\n");
	}
        */
	int i=0;

	while(1) {

		DIR *dip;
		struct dirent *dit;

		//open a directory stream to /proc directory
		if ((dip = opendir("/proc")) == NULL) {
			perror("opendir");
			return -1;
		}

		//read in from /proc and seek for process dirs
		while ((dit = readdir(dip)) != NULL) {
			//get pid
			if (pid==atoi(dit->d_name)) {
				//pid detected
                                Check_Us(pid);
				if (kill(pid,SIGSTOP)==0 &&  kill(pid,SIGCONT)==0) {
					//process is ok!
                                        if (closedir(dip) == -1) {
                                           perror("closedir");
                                           return -1;
                                        }
					goto done;
				}
				else {
					fprintf(stderr,"Error: Process %d detected, but you don't have permission to control it\n",pid);
				}
			}
		}

		//close the dir stream and check for errors
		if (closedir(dip) == -1) {
			perror("closedir");
			return -1;
		}

		//no suitable target found
		if (i++==0) {
			if (lazy) {
				fprintf(stderr,"No process found\n");
				exit(2);
			}
			else {
				fprintf(stderr, "Warning: no target process found. Waiting for it...\n");
			}
		}

		//sleep for a while
		sleep(2);
	}

done:
    if (!quiet)
    	printf("Process %d detected\n",pid);
	//now set high priority, if possible
	// if (setpriority(PRIO_PROCESS,getpid(),-20)!=0) {
        /*
        if ( (nice_lim < INT_MAX) &&
             (setpriority(PRIO_PROCESS, my_pid, nice_lim) != 0) ) {
		printf("Warning: cannot renice.\nTo work better you should run this program as root.\n");
	}
        */
	return 0;

}

//this function periodically scans process list and looks for executable path names
//it should be executed in a low priority context, since precise timing does not matter
//if a process is found then its pid is returned
//process: the name of the wanted process, can be an absolute path name to the executable file
//         or simply its name
//return: pid of the found process
int getpidof(const char *process) {

	//set low priority
	// if (setpriority(PRIO_PROCESS,getpid(),19)!=0) {
        /*
        if ( (nice_lim < INT_MAX) &&
             (setpriority(PRIO_PROCESS, my_pid, 19) != 0) ) {
		printf("Warning: cannot renice\n");
	}
        */
	char exelink[20];
	char exepath[PATH_MAX+1];
	int pid=0;
	int i=0;

	while(1) {

		DIR *dip;
		struct dirent *dit;

		//open a directory stream to /proc directory
		if ((dip = opendir("/proc")) == NULL) {
			perror("opendir");
			return -1;
		}

		//read in from /proc and seek for process dirs
		while ((dit = readdir(dip)) != NULL) {
			//get pid
			pid=atoi(dit->d_name);
			if (pid>0) {
				sprintf(exelink,"/proc/%d/exe",pid);
				int size=readlink(exelink,exepath,sizeof(exepath));
				if (size>0) {
					int found=0;
					if (process[0]=='/' && strncmp(exepath,process,size)==0 && size==strlen(process)) {
						//process starts with / then it's an absolute path
						found=1;
					}
					else {
						//process is the name of the executable file
						if (strncmp(exepath+size-strlen(process),process,strlen(process))==0) {
							found=1;
						}
					}
					if (found==1) {
                                        Check_Us(pid);
						if (kill(pid,SIGSTOP)==0 &&  kill(pid,SIGCONT)==0) {
							//process is ok!
                                                        if (closedir(dip) == -1) {
                                                          perror("closedir");
                                                          return -1;
                                                        }
							goto done;
						}
						else {
							fprintf(stderr,"Error: Process %d detected, but you don't have permission to control it\n",pid);
						}
					}
				}
			}
		}

		//close the dir stream and check for errors
		if (closedir(dip) == -1) {
			perror("closedir");
			return -1;
		}

		//no suitable target found
		if (i++==0) {
			if (lazy) {
				fprintf(stderr,"No process found\n");
				exit(2);
			}
			else {
				fprintf(stderr, "Warning: no target process found. Waiting for it...\n");
			}
		}

		//sleep for a while
		sleep(2);
	}

done:
    if (!quiet)
    	printf("Process %d detected\n",pid);
	//now set high priority, if possible
	// if (setpriority(PRIO_PROCESS,getpid(),-20)!=0) {
        /*
        if ( (nice_lim < INT_MAX) &&
             (setpriority(PRIO_PROCESS, my_pid, nice_lim) != 0) ) {
		printf("Warning: cannot renice.\nTo work better you should run this program as root.\n");
	}
        */
	return pid;

}

//SIGINT and SIGTERM signal handler
void quit(int sig) {
	//let the process continue if it's stopped
	kill(pid,SIGCONT);
	printf("Exiting...\n");
	exit(0);
}


#ifdef FREEBSD
//get jiffies count from /proc filesystem
int getjiffies(int pid)
{
   kvm_t *my_kernel = NULL;
   struct kinfo_proc *process_data = NULL;
   int processes;
   int my_jiffies = -1;

   my_kernel = kvm_open(0, 0, 0, O_RDONLY, "kvm_open");
   if (! my_kernel)
   {
      fprintf(stderr, "Error opening kernel vm. You should be running as root.\n");
      return -1;
   }

   process_data = kvm_getprocs(my_kernel, KERN_PROC_PID, pid, &processes);
   if ( (process_data) && (processes >= 1) )
       my_jiffies = process_data->ki_runtime;
   
   kvm_close(my_kernel);
   if (my_jiffies >= 0)
     my_jiffies /= 1000;
   return my_jiffies;
}

#endif

#ifdef LINUX
int getjiffies(int pid) {
	static char stat[20];
	static char buffer[1024];
        char *p;
	sprintf(stat,"/proc/%d/stat",pid);
	FILE *f=fopen(stat,"r");
	if (f==NULL) return -1;
	p = fgets(buffer,sizeof(buffer),f);
	fclose(f);
	// char *p=buffer;
        if (p)
        {
	  p=memchr(p+1,')',sizeof(buffer)-(p-buffer));
	  int sp=12;
	  while (sp--)
		p=memchr(p+1,' ',sizeof(buffer)-(p-buffer));
	  //user mode jiffies
	  int utime=atoi(p+1);
	  p=memchr(p+1,' ',sizeof(buffer)-(p-buffer));
	  //kernel mode jiffies
	  int ktime=atoi(p+1);
	  return utime+ktime;
        }
        // could not read info
        return -1;
}
#endif


//process instant photo
struct process_screenshot {
	struct timespec when;	//timestamp
	int jiffies;	//jiffies count of the process
	int cputime;	//microseconds of work from previous screenshot to current
};

//extracted process statistics
struct cpu_usage {
	float pcpu;
	float workingrate;
};

//this function is an autonomous dynamic system
//it works with static variables (state variables of the system), that keep memory of recent past
//its aim is to estimate the cpu usage of the process
//to work properly it should be called in a fixed periodic way
//perhaps i will put it in a separate thread...
int compute_cpu_usage(int pid,int last_working_quantum,struct cpu_usage *pusage) {
	#define MEM_ORDER 10
	//circular buffer containing last MEM_ORDER process screenshots
	static struct process_screenshot ps[MEM_ORDER];
	//the last screenshot recorded in the buffer
	static int front=-1;
	//the oldest screenshot recorded in the buffer
	static int tail=0;

	if (pusage==NULL) {
		//reinit static variables
		front=-1;
		tail=0;
		return 0;
	}

	//let's advance front index and save the screenshot
	front=(front+1)%MEM_ORDER;
	int j=getjiffies(pid);
	if (j>=0) ps[front].jiffies=j;
	else return -1;	//error: pid does not exist

        #ifdef __APPLE__
        // OS X does not have clock_gettime, use clock_get_time
        clock_serv_t cclock;
        mach_timespec_t mts;
        host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
        clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);
        ps[front].when.tv_sec = mts.tv_sec;
        ps[front].when.tv_nsec = mts.tv_nsec;

        #else
        // Linux and BSD can use real time
	clock_gettime(CLOCK_REALTIME,&(ps[front].when));
	ps[front].cputime=last_working_quantum;
        #endif
	//buffer actual size is: (front-tail+MEM_ORDER)%MEM_ORDER+1
	int size=(front-tail+MEM_ORDER)%MEM_ORDER+1;

	if (size==1) {
		//not enough samples taken (it's the first one!), return -1
		pusage->pcpu=-1;
		pusage->workingrate=1;
		return 0;
	}
	else {
		//now we can calculate cpu usage, interval dt and dtwork are expressed in microseconds
		long dt=timediff(&(ps[front].when),&(ps[tail].when));
		long dtwork=0;
		int i=(tail+1)%MEM_ORDER;
		int max=(front+1)%MEM_ORDER;
		do {
			dtwork+=ps[i].cputime;
			i=(i+1)%MEM_ORDER;
		} while (i!=max);
		int used=ps[front].jiffies-ps[tail].jiffies;
		float usage=(used*1000000.0/HZ)/dtwork;
		pusage->workingrate=1.0*dtwork/dt;
		pusage->pcpu=usage*pusage->workingrate;
		if (size==MEM_ORDER)
			tail=(tail+1)%MEM_ORDER;
		return 0;
	}
	#undef MEM_ORDER
}

void print_caption() {
	printf("\n%%CPU\twork quantum\tsleep quantum\tactive rate\n");
}


void increase_priority()
{
        //find the best available nice value
        int old_priority = getpriority(PRIO_PROCESS, 0);
        int priority = old_priority;
        while ( (setpriority(PRIO_PROCESS, 0, priority-1) == 0) &&
                (priority > BEST_PRIORITY) )
        {
                priority--;
        }
        if (priority != old_priority) {
                if (verbose) printf("Priority changed to %d\n", priority);
        }
        else {
                if (verbose) printf("Warning: Cannot change priority. Run as root or renice for best results.\n");
        }


}



void print_usage(FILE *stream,int exit_code) {
        fprintf(stream, "CPUlimit version %1.1f\n", VERSION);
	fprintf(stream, "Usage: %s TARGET [OPTIONS...] [-- PROGRAM]\n",program_name);
	fprintf(stream, "   TARGET must be exactly one of these:\n");
	fprintf(stream, "      -p, --pid=N        pid of the process\n");
	fprintf(stream, "      -e, --exe=FILE     name of the executable program file\n");
        fprintf(stream, "                         The -e option only works when\n");
        fprintf(stream, "                         cpulimit is run with admin rights.\n");
	fprintf(stream, "      -P, --path=PATH    absolute path name of the\n");
        fprintf(stream, "                         executable program file\n");
	fprintf(stream, "   OPTIONS\n");
        fprintf(stream, "      -b  --background   run in background\n");
        fprintf(stream, "      -c  --cpu=N        override the detection of CPUs on the machine.\n");
	fprintf(stream, "      -l, --limit=N      percentage of cpu allowed from 1 up.\n");
        fprintf(stream, "                         Usually 1 - %d00, but can be higher\n", NCPU);
        fprintf(stream, "                         on multi-core CPUs (mandatory)\n");
        fprintf(stream, "      -q, --quiet        run in quiet mode (only print errors).\n");
        fprintf(stream, "      -k, --kill         kill processes going over their limit\n");
        fprintf(stream, "                         instead of just throttling them.\n");
        fprintf(stream, "      -r, --restore      Restore processes after they have\n");
        fprintf(stream, "                         been killed. Works with the -k flag.\n");

	fprintf(stream, "      -v, --verbose      show control statistics\n");
	fprintf(stream, "      -z, --lazy         exit if there is no suitable target process,\n");
        fprintf(stream, "                         or if it dies\n");
        fprintf(stream, "          --             This is the final CPUlimit option. All following\n");
        fprintf(stream, "                         options are for another program we will launch.\n");
	fprintf(stream, "      -h, --help         display this help and exit\n");
	exit(exit_code);
}



// Get the number of CPU cores on this machine.
int get_ncpu()
{
        int ncpu = 1;
#ifdef _SC_NPROCESSORS_ONLN
        ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#endif
        return ncpu;
}




int main(int argc, char **argv) {

	//get program name
	// char *p=(char*)memrchr(argv[0],(unsigned int)'/',strlen(argv[0]));
	// program_name = p==NULL?argv[0]:(p+1);
        program_name = argv[0];
        int run_in_background = FALSE;
	//parse arguments
	int next_option;
	/* A string listing valid short options letters. */
	const char* short_options="p:e:P:l:c:bqkrvzh";
	/* An array describing valid long options. */
	const struct option long_options[] = {
		{ "pid", required_argument, NULL, 'p' },
		{ "exe", required_argument, NULL, 'e' },
		{ "path", required_argument, NULL, 'P' },
		{ "limit", required_argument, NULL, 'l' },
                { "background", no_argument, NULL, 'b' },
        { "quiet", no_argument, NULL, 'q' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "lazy", no_argument, NULL, 'z' },
		{ "help", no_argument, NULL, 'h' },
                { "cpu", required_argument, NULL, 'c'},
		{ NULL, 0, NULL, 0 }
	};
	//argument variables
	const char *exe=NULL;
	const char *path=NULL;
	int perclimit=0;
	int pid_ok = FALSE;
	int process_ok = FALSE;
	int limit_ok = FALSE;
        int last_known_argument = 0;
        int kill_process = FALSE;   // kill process instead of stopping it
        int restore_process = FALSE;  // restore killed process
        // struct rlimit maxlimit;

        NCPU = get_ncpu();

        opterr = 0;      // avoid unwanted error messages for unknown parameters
	do {
		next_option = getopt_long (argc, argv, short_options,long_options, NULL);
		switch(next_option) {
                        case 'b':
                                run_in_background = TRUE;
                                last_known_argument++;
                                break;
			case 'p':
				pid=atoi(optarg);
                                if (pid)   // valid PID
                                {
				  pid_ok = TRUE;
                                  lazy = TRUE;
                                }
                                last_known_argument += 2;
				break;
			case 'e':
				exe=optarg;
				process_ok = TRUE;
                                last_known_argument += 2;
				break;
			case 'P':
				path=optarg;
				process_ok = TRUE;
                                last_known_argument += 2;
				break;
			case 'l':
				perclimit=atoi(optarg);
				limit_ok = TRUE;
                                last_known_argument += 2;
				break;
                        case 'c':
                                NCPU = atoi(optarg);
                                last_known_argument += 2;
                                break;
                        case 'k':
                                kill_process = TRUE;
                                last_known_argument++;
                                break;
                        case 'r':
                                restore_process = TRUE;
                                last_known_argument++;
                                break;

			case 'v':
				verbose = TRUE;
                                last_known_argument++;
				break;
            case 'q':
                quiet = TRUE;
                                last_known_argument++;
                break;
			case 'z':
				lazy = TRUE;
                                last_known_argument++;
				break;
			case 'h':
				print_usage (stdout, 1);
                                last_known_argument++;
				break;
                        case 'o':
                                last_known_argument++;
                                next_option = -1;
                                break;
			case '?':
				print_usage (stderr, 1);
                                last_known_argument++;
				break;
			case -1:
				break;
			// default:
			//	abort();
		}
	} while(next_option != -1);


        // try to launch a program passed on the command line
        // But only if we do not already have a PID to watch
        if ( (last_known_argument + 1 < argc) && (pid_ok == FALSE) )
        {
           last_known_argument++;
           // if we stopped on "--" jump to the next parameter
           if ( (last_known_argument + 1 < argc) && (! strcmp(argv[last_known_argument], "--") ) )
               last_known_argument++;
           pid_t forked_pid;
           // try to launch remaining arguments
           if (verbose)
           {
               int index = last_known_argument;
               printf("Launching %s", argv[index]);
               for (index = last_known_argument + 1; index < argc; index++)
                    printf(" %s", argv[index]);
               printf(" with limit %d\n", perclimit);
           }
           forked_pid = fork();
           if (forked_pid == -1)  // error
           {
               printf("Failed to launch specified process.\n");
               exit(1);
           }
           else if (forked_pid == 0)   // target child
           {
              execvp(argv[last_known_argument],
                     &(argv[last_known_argument]) );
              exit(2);
           }
           else     // parent who will now fork the throttler
           {
              pid_t limit_pid;
              // if we are planning to kill a process, give it
              // a running head start to avoid death at start-up
              if (kill_process)
                 sleep(5);
     
              limit_pid = fork();
              if (limit_pid == 0)   // child
              {
                 pid = forked_pid;    // the first child
                 lazy = TRUE;
                 pid_ok = TRUE;
                 if (verbose)
                   printf("Throttling process %d\n", (int) pid);
              }
              else    // parent
                exit(0);
           }

           /*
           else if (forked_pid == 0)   // child
           {
               lazy = TRUE;
               pid_ok = TRUE;
               pid = getppid();
               if (verbose)
                  printf("Throttling process %d\n", (int) pid);
           }
           else // parent
           {
               execvp(argv[last_known_argument], 
                      &(argv[last_known_argument]));
               
               // we should never return  
               exit(2);
           }
           */
           
        }      // end of launching child process

	if (!process_ok && !pid_ok) {
		fprintf(stderr,"Error: You must specify a target process\n");
		print_usage (stderr, 1);
		exit(1);
	}
	if ((exe!=NULL && path!=NULL) || (pid_ok && (exe!=NULL || path!=NULL))) {
		fprintf(stderr,"Error: You must specify exactly one target process\n");
		print_usage (stderr, 1);
		exit(1);
	}
	if (!limit_ok) {
		fprintf(stderr,"Error: You must specify a cpu limit\n");
		print_usage (stderr, 1);
		exit(1);
	}
	float limit=perclimit/100.0;
	if ( (limit <= 0.00) || (limit > NCPU) )
        {
		fprintf(stderr,"Error: limit must be in the range of 1 to %d00\n", NCPU);
		print_usage (stderr, 1);
		exit(1);
	}

        // check to see if we should fork
        if (run_in_background)
        {
             pid_t process_id;
             process_id = fork();
             if (! process_id)
                exit(0);
             else
             {
                setsid();
                process_id = fork();
                if (process_id)
                  exit(0);
             }
        }

	//parameters are all ok!
	signal(SIGINT,quit);
	signal(SIGTERM,quit);

        my_pid = getpid();
        if (verbose)
           printf("%d CPUs detected.\n", NCPU);

        increase_priority();
/*
Instead of all this big block of code to detect and change
priority settings, let us just use the increase_priority()
function. It is a little more simple and takes a more
gradual approach, rather than "all or nothing".
-- Jesse

	if (setpriority(PRIO_PROCESS, my_pid,-20)!=0) {
	//if that failed, check if we have a limit 
        // by how much we can raise the priority
#ifdef RLIMIT_NICE 
//check if non-root can even make changes 
// (ifdef because it's only available in linux >= 2.6.13)
		nice_lim=getpriority(PRIO_PROCESS, my_pid);
		getrlimit(RLIMIT_NICE, &maxlimit);

//if we can do better then current
		if( (20 - (signed)maxlimit.rlim_cur) < nice_lim &&  
		    setpriority(PRIO_PROCESS, my_pid,
                    20 - (signed)maxlimit.rlim_cur)==0 //and it actually works
		  ) {

			//if we can do better, but not by much, warn about it
			if( (nice_lim - (20 - (signed)maxlimit.rlim_cur)) < 9) 
                        {
			printf("Warning, can only increase priority by %d.\n",                                nice_lim - (20 - (signed)maxlimit.rlim_cur));
			}
                        //our new limit
			nice_lim = 20 - (signed)maxlimit.rlim_cur; 

		} else 
// otherwise don't try to change priority. 
// The below will also run if it's not possible 
// for non-root to change priority
#endif
		{
			printf("Warning: cannot renice.\nTo work better you should run this program as root, or adjust RLIMIT_NICE.\nFor example in /etc/security/limits.conf add a line with: * - nice -10\n\n");
			nice_lim=INT_MAX;
		}
	} else {
		nice_lim=-20;
	}
*/


	//time quantum in microseconds. it's splitted in a working period and a sleeping one
	int period=100000;
	struct timespec twork,tsleep;   //working and sleeping intervals
	memset(&twork,0,sizeof(struct timespec));
	memset(&tsleep,0,sizeof(struct timespec));

wait_for_process:

	//look for the target process..or wait for it
	if (exe != NULL)
		pid=getpidof(exe);
	else if (path != NULL)
		pid=getpidof(path);
	else 
		waitforpid(pid);

	
	//process detected...let's play

	//init compute_cpu_usage internal stuff
	compute_cpu_usage(0,0,NULL);
	//main loop counter
	int i=0;

	struct timespec startwork,endwork;
	long workingtime=0;		//last working time in microseconds

	if (verbose) print_caption();

	float pcpu_avg=0;

	//here we should already have high priority, for time precision
	while(1) {

		//estimate how much the controlled process is using the cpu in its working interval
		struct cpu_usage cu;
		if (compute_cpu_usage(pid,workingtime,&cu)==-1) {
            if (!quiet)
    			fprintf(stderr,"Process %d dead!\n",pid);
			if (lazy) exit(2);
			//wait until our process appears
			goto wait_for_process;		
		}

		//cpu actual usage of process (range 0-1)
		float pcpu=cu.pcpu;
		//rate at which we are keeping active the process (range 0-1)
		float workingrate=cu.workingrate;

		//adjust work and sleep time slices
		if (pcpu>0) {
			twork.tv_nsec=min(period*limit*1000/pcpu*workingrate,period*1000);
		}
		else if (pcpu==0) {
			twork.tv_nsec=period*1000;
		}
		else if (pcpu==-1) {
			//not yet a valid idea of cpu usage
			pcpu=limit;
			workingrate=limit;
			twork.tv_nsec=min(period*limit*1000,period*1000);
		}
		tsleep.tv_nsec=period*1000-twork.tv_nsec;

		//update average usage
		pcpu_avg=(pcpu_avg*i+pcpu)/(i+1);

		if (verbose && i%10==0 && i>0) {
			printf("%0.2f%%\t%6ld us\t%6ld us\t%0.2f%%\n",pcpu*100,twork.tv_nsec/1000,tsleep.tv_nsec/1000,workingrate*100);
                        if (i%200 == 0)
                           print_caption();
		}

		// if (limit<1 && limit>0) {
                // printf("Comparing %f to %f\n", pcpu, limit);
                if (pcpu < limit)
                {
                        // printf("Continue\n");
			//resume process
			if (kill(pid,SIGCONT)!=0) {
                if (!quiet)
    				fprintf(stderr,"Process %d dead!\n",pid);
				if (lazy) exit(2);
				//wait until our process appears
				goto wait_for_process;
			}
		}

                #ifdef __APPLE_
                // OS X does not have clock_gettime, use clock_get_time
                clock_serv_t cclock;
                mach_timespec_t mts;
                host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
                clock_get_time(cclock, &mts);
                mach_port_deallocate(mach_task_self(), cclock);
                startwork.tv_sec = mts.tv_sec;
                startwork.tv_nsec = mts.tv_nsec;

                #else
		clock_gettime(CLOCK_REALTIME,&startwork);
                #endif

		nanosleep(&twork,NULL);		//now process is working
                #ifdef __APPLE__
                // OS X does not have clock_gettime, use clock_get_time
                // clock_serv_t cclock;
                // mach_timespec_t mts;
                host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
                clock_get_time(cclock, &mts);
                mach_port_deallocate(mach_task_self(), cclock);
                endwork.tv_sec = mts.tv_sec;
                endwork.tv_nsec = mts.tv_nsec;

                #else
		clock_gettime(CLOCK_REALTIME,&endwork);
                #endif
		workingtime=timediff(&endwork,&startwork);

		// if (limit<1) {
                // printf("Checking %f vs %f\n", pcpu, limit);
                if (pcpu > limit)
                {
                     // When over our limit we may run into
                     // situations where we want to kill
                     // the offending process, then restart it
                     if (kill_process)
                     {
                         kill(pid, SIGKILL);
                         if (!quiet)
                             fprintf(stderr, "Process %d killed.\n", pid);
                         if ( (lazy) && (! restore_process) ) 
                              exit(2);
                         // restart killed process
                         if (restore_process)
                         {
                             pid_t new_process;
                             new_process = fork();
                             if (new_process == -1)
                             {
                              fprintf(stderr, "Failed to restore killed process.\n");
                             }
                             else if (new_process == 0)
                             {
                                // child which becomes new process
                                if (verbose)
                                   printf("Relaunching %s\n",
                                          argv[last_known_argument]);
                                execvp(argv[last_known_argument],
                                       &(argv[last_known_argument]) ); 
                             }
                             else // parent
                             {
                                // we need to track new process
                                pid = new_process;
                                // avoid killing child process
                                sleep(5);
                             }
                         }
                     }
                     // do not kll process, just throttle it
                     else
                     {

                        // printf("Stop\n");
			//stop process, it has worked enough
			if (kill(pid,SIGSTOP)!=0) {
                if (!quiet)
    				fprintf(stderr,"Process %d dead!\n", pid);
				if (lazy) exit(2);
				//wait until our process appears
				goto wait_for_process;
			}
			nanosleep(&tsleep,NULL);	//now process is sleeping
                      }   // end of throttle process
		}         // end of process using too much CPU
		i++;
	}

   return 0;
}
