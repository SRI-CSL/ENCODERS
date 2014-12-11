/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   James Mathewson (JM, JLM)
 */

#include <stdlib.h>
#include <time.h>
#include <stdio.h>

//Due to problems with bash in android
//calling "now" by itself will release the current time
//but if you pass in an option, it will add those numeric values
//to the current time.
//A hack to resolve android bash not liking to add script values
int main(int argc, char **argv[])
{
   FILE *fp;
   int i;
   unsigned long now_l=0;
   fp = fopen("now.txt", "w");
   time_t now = time(NULL);
   now_l = (unsigned long) now;
   if (argc > 1) { //add the other parameters
     for(i=1; i<argc ; i++) {
        now_l+=  (unsigned long) atol(argv[i]);
     }
   } 

   fprintf(fp,"%ld", now_l);
   fclose(fp);
   printf("%ld", now_l);
}
