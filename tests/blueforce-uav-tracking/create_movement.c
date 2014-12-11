#include <stdio.h>
#include <string.h>
#include <math.h>

int main()
{
   FILE *movement, *squadformation, *environ, *output_imn, *output_scen, *output_scen2;
   int meters_per_sec_speed;
   int num_members_squad;
   char *outputname="IED_patrol";
   //char *names[]={"1st-squad","2nd-squad", "3rd-squad","uav", NULL};
   char *names[]={"1st-squad","2nd-squad", "3rd-squad",NULL};
   char filename[100]; 

   strcpy(filename, outputname);
   strcat(filename, ".scen");
   output_scen = fopen(filename, "w");
   strcat(filename,"2");
   output_scen2 = fopen(filename, "w");

   int i;
   int tag;
   int j=1;
   int scale_pix_100m = 321;
   float scale =  1.0/3.21; //how many pixels = 1 meter
   int sizex = 755;
   int sizey = 615;
   char *wallname="update-paper-bg.jpg";
   float x,y,x_offset,y_offset;
   int range=45*scale; //45 meters, but range is in pixels
   float x_movements[5][50];
   float y_movements[5][50];
   float t_wait[5][50];
   float z_time[5][50];
   int num_movements[5];
   float xdiff, ydiff;
   float diff;
   //create nodes first
   for(i=0;NULL != names[i]; i++) {
       tag = 0;
       strcpy(filename, names[i]);
       strcat(filename, ".mov");
       movement = fopen(filename, "r");
       j=0; 
       //read scale file, but hardcode for now
       while (EOF != fscanf(movement,"%f %f %f", &x_movements[i][j], &y_movements[i][j], &t_wait[i][j])) 
       {
        //calculate how long to get to destination
        if (0 == j) {
            z_time[i][j] = t_wait[i][0];
        } else {
            diff = sqrtf(pow((x_movements[i][j]-x_movements[i][j-1]),2)+pow((y_movements[i][j]-y_movements[i][j-1]),2));
            //convert pixels to meters, adjust for walking
            diff = (diff/scale)/1.4;
            //how long to walk that distance? (3.2 mph = 1.4 m/s)
            if(i==4)  diff=diff*50;  //uav is faster
            z_time[i][j]=z_time[i][j-1]+diff;
            if (i!=2 && j!=0) z_time[i][j] +=t_wait[i][j-1];
printf("squad %i To walk from %f,%f to %f,%f (with a %f wait) should take %f seconds to walk %f meters\n",i+1, x_movements[i][j-1], y_movements[i][j-1], x_movements[i][j], y_movements[i][j], t_wait[i][j], z_time[i][j], diff*1.4);    
        }

        j++;
       }
       fclose(movement);
       num_movements[i]=j-1;
   }

for (i=0; NULL != names[i]; i++) {
    for(j=0; j<9; j++) {
       fprintf(output_scen,"$node_(%d) set X_ %f\n", j+1+i*9, x_movements[i][0]);
       fprintf(output_scen,"$node_(%d) set Y_ %f\n", j+1+i*9, y_movements[i][0]);
       }
} 
int z;
float speed;
float time_for_opord = 0.0; //-380.0;
   for(i=0;NULL != names[i]; i++) {
      
       //read scale file, but hardcode for now
       for(j=1;j<num_movements[i]+1; j++){
	//each squad member
        strcpy(filename, names[i]);
        strcat(filename, ".form");
        squadformation = fopen(filename,"r");
//fix 50 to 1
        z=0;
speed = 1.4*scale;
        while (EOF != fscanf(squadformation,"%f %f", &xdiff, &ydiff)) {
       if (i != 2) {
       fprintf(output_scen2,"$ns_ at %08.2f \"$node_(%d) setdest %f %f %f\"\n", z_time[i][j-1]+time_for_opord, i*9+1+z, x_movements[i][j]+xdiff, y_movements[i][j]+ydiff, speed); } else {

       fprintf(output_scen2,"$ns_ at %08.2f \"$node_(%d) setdest %f %f %f\"\n", z_time[i][j-1]+time_for_opord, i*9+1+z, x_movements[i][j]+xdiff, y_movements[i][j]+ydiff, speed); 
        }
            z++;
         }
         fclose(squadformation);
       }
   }
    
   fclose(output_scen);
   fclose(output_scen2);

}


