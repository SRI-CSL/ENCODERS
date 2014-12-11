#include <stdio.h>
#include <string.h>

int main()
{
   FILE *movement, *squadformation, *environ, *output_imn, *output_scen;
   int meters_per_sec_speed;
   int num_members_squad;
   char *outputname="IED_patrol";
   char *names[]={"1st-squad","2nd-squad", "3rd-squad","uav", NULL};
   char filename[100]; 

   strcpy(filename, outputname);
   strcat(filename, ".imn");
   output_imn = fopen(filename, "w");
   strcpy(filename, outputname);
   strcat(filename, ".scen");
   output_scen = fopen(filename, "w");

   int i;
   int tag;
   int j=1;
   int scale_pix_100m = 321;
   float scale = 1.0/3.21; //how many pixels = 1 meter
   int sizex = 755;
   int sizey = 615;
   char *wallname="update-paper-bg.jpg";
   float x,y,x_offset,y_offset;
   int range=45*scale; //45 meters, but range is in pixels
   //create nodes first
   for(i=0;NULL != names[i]; i++) {
       fprintf(output_imn,"comments {%s}\n", names[i]);
       tag = 0;
       strcpy(filename, names[i]);
       strcat(filename, ".mov");
       movement = fopen(filename, "r");
       fscanf(movement,"%f %f", &x, &y);
       fclose(movement);

       //file in format of +x +y meters from movement file
       strcpy(filename, names[i]);
       strcat(filename, ".form");
       movement = fopen(filename, "r");
      
       //read scale file, but hardcode for now
       while (EOF != fscanf(movement,"%f %f", &x_offset, &y_offset)) 
       {
        
       fprintf(output_imn,"node n%d {\n", j);
       fprintf(output_imn,"    type router\n    model PC\n");
       fprintf(output_imn,"    network-config {\n");
       if (tag) {
         fprintf(output_imn,"        hostname n%d\n", j);
       } else {
         fprintf(output_imn,"        hostname %s\n", names[i]);
       }
       fprintf(output_imn,"        !\n        interface eth0\n");
       fprintf(output_imn,"          ip address 10.0.1.%d/16\n", j);
       fprintf(output_imn,"          ipv6 address a:%d::1/64\n", j);
       fprintf(output_imn,"        !\n    }\n");
       fprintf(output_imn,"    canvas c0\n");
       fprintf(output_imn,"    iconcoords {%f %f}\n", x+x_offset*scale, y+y_offset*scale);
       fprintf(output_imn,"    labelcoords {%f %f}\n", x+x_offset*scale+14.0, y+y_offset*scale+43.0);
       if (tag) {
          fprintf(output_imn,"    hidden 1\n");
       } else {
          if (strcmp(names[i], "uav")) {
          fprintf(output_imn,"    custom-image platoon-dismount-small.jpg\n");
          } else {
          fprintf(output_imn,"    custom-image UAV.jpg\n");
          }
       }
       fprintf(output_imn,"    services {DefaultRoute HaggleService}\n");
       fprintf(output_imn,"}\n\n");
       tag = 1;
       j++;
       }
   }

   //wireless router
   fprintf(output_imn,"node n%d {\n", j);
   fprintf(output_imn,"    delay 20000\n");
   fprintf(output_imn,"    tbandwidth 54000000\n");
   fprintf(output_imn,"    type wlan\n");
   fprintf(output_imn,"    range %d\n", range);
   fprintf(output_imn,"    network-config {\n");
   fprintf(output_imn,"        hostname wlan%d\n", j);
   fprintf(output_imn,"        !\n        interface wireless\n");
   fprintf(output_imn,"        ip address 10.0.0.0/16\n");
   fprintf(output_imn,"        ipv6 address a:0::1/64\n");
   fprintf(output_imn,"        !\n        scriptfile\n");
   fprintf(output_imn,"        %s%s\n        !\n",outputname, ".scen");
   fprintf(output_imn,"    mobmodel\n        coreapi\n         basic_range\n        !\n    }\n");
   //custom-config
   //fprintf(output_imn,"    custom-config {\n    custom-config-id basic_range\n");
   //fprintf(output_imn,"    custom-command {3 3 9 9 9}\n    config {\n");
   //fprintf(output_imn,"     %d\n     54000000\n     0\n     20000\n     0\n     }\n    }\n", range);
   //fprintf(output_imn,"    canvas c0\n");
   //fprintf(output_imn,"    iconcoords {%f %f}\n", 15.0, 15.0);
   //fprintf(output_imn,"    labelcoords {%f %f}\n", 29.0, 58.0);
   fprintf(output_imn,"}\n\n");
   int k;
   for(k=0; k<j; k++)
   {
     // fprintf(output_imn,"    interface-peer {e%d n%d}\n", k, k+1);
   }

   
   for(k=1; k<j; k++) 
   {
      fprintf(output_imn,"link l%d {\n\tnodes {n%d n%d}\n}\n", k, j, k);
   }

   fprintf(output_imn, "canvas c0 {\n    name {Canvas0}\n");
   fprintf(output_imn, "    refpt {0 0 47.58 -122.13 2.0}\n"); 
   fprintf(output_imn,"     scale {%d}\n", (int) (scale_pix_100m));
   fprintf(output_imn,"     wallpaper-style {centered}\n");
   fprintf(output_imn,"     wallpaper {%s}\n", wallname);
   fprintf(output_imn,"     size {%d %d}\n", sizex, sizey);
   fprintf(output_imn,"}\n");
   //option global
   fprintf(output_imn,"\noption global {\ninterface_names no\n    ip_adresses yes\n");
   fprintf(output_imn,"    ipv6_adresses no\n    node_labels yes\n    link_labels yes\n");
   fprintf(output_imn,"    ipsec_configs yes\n    exec_errors yes\n    show_api no\n");
   fprintf(output_imn,"    background_images no\n    annotations yes\n    grid no\n    traffic_start 0\n}\n");


}


