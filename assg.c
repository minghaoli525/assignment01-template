#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char*argv[]){

    if (argc!=2){
        fprintf(stderr,"Usage: %s <list_file>\n",argv[0]);
        return 1;
    }

FILE* list_file = fopen(argv[1],"r");
if (list_file == NULL){
    perror("Error opening list file");
    return 1;
}
int total_files = 0;
char temp_line[256];
while (fgets(temp_line, sizeof(temp_line), list_file)!= NULL){
    temp_line[strcspn(temp_line,"\r\n")] = '\0';
    if (strlen(temp_line)>0){
        total_files++;
    }
}
rewind(list_file);
printf("Processing %d schedule files\n",total_files);
int index = 1;

char schedule_path[256]; 
    while (fgets(schedule_path, sizeof(schedule_path), list_file) != NULL) {
        schedule_path[strcspn(schedule_path, "\r\n")] = '\0';
        char direction = '\0';
        char *last_underscore = strrchr(schedule_path,'_');
        if(last_underscore != NULL){
            direction = *(last_underscore +1);
        }
        if (strlen(schedule_path) == 0) continue;
    FILE*sched_file = fopen(schedule_path,"r");
    if(sched_file ==NULL){
        perror("Could not open schedule file");
        continue;
    }

char file_header[256];
    if(fgets(file_header,sizeof(file_header),sched_file) != NULL){
        file_header[strcspn(file_header,"\r\n")] = '\0';
        int route_number = 0;
        int num_stops = 0;
        
        if (sscanf(file_header,"%d %*[^,], %d", &route_number, &num_stops)== 2){
            printf("Schedule #%d is route %d it has %d stops amd is in the %c direction\n",index, route_number, num_stops, direction);
        }
    }
    fclose(sched_file);
    index++;
}
    fclose(list_file);
    return 0;
}


