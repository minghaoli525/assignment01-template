/*Milestone 就是让你写一个简易版的 C 程序，读入一个路线列表文件，
把各个时刻表文件的基本信息读出来并按格式打印在屏幕上，
然后推送到 Git 的 milestone-submission 分支并填表。*/
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
char schedule_path[256]; /*create a set which size is 256 */
    while (fgets(schedule_path, sizeof(schedule_path), list_file) != NULL) {
        schedule_path[strcspn(schedule_path, "\r\n")] = '\0';
        if (strlen(schedule_path) == 0) continue;
        /*printf("%s",schedule_path);*/
    FILE*sched_file = fopen(schedule_path,"r");
    if(sched_file ==NULL){
        perror("Could not open schedule file%s\n");
        continue;
    }
char file_header[256];
    if(fgets(file_header,sizeof(file_header),sched_file) != NULL){
        file_header[strcspn(file_header,"\r\n")] = '\0';
        printf("path:%s -> the first line:%s\n", schedule_path,file_header);
    }
    fclose(sched_file);
    }
    fclose(list_file);
    return 0;
}


