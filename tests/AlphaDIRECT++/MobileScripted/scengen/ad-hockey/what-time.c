/* what-time.c
   $Id$
   */

#include <sys/time.h>
#include <stdio.h>

int
main()
{
  char buf[10];
  struct timeval t;

  for(;;) {
    fgets(buf, sizeof(buf), stdin);
    gettimeofday(&t,NULL);
    printf("%f\n",t.tv_sec + (double) t.tv_usec / 1.0e+6);
    fflush(stdout);
    }
  return 0;
}
