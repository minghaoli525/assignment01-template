#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) return 1;

    FILE *list_file = fopen(argv[1], "r");
    if (!list_file) return 1;

    int total_files = 0;
    char line[256];

    while (fgets(line, sizeof(line), list_file) != NULL) {
        if (strlen(line) > 1) {
            total_files++;
        }
    }
    rewind(list_file);
    printf("Processing %d schedule files...\n", total_files);

    int index = 1;
    char schedule_path[256];

while (fgets(schedule_path, sizeof(schedule_path), list_file) != NULL) {

       int i = 0;

        while (schedule_path[i] != '\0') {

            if (schedule_path[i] == '\n' ||
                schedule_path[i] == '\r') {

                schedule_path[i] = '\0';
                break;
            }

            i++;
        }

        if (schedule_path[0] == '\0') {
            continue;
        }


int start = 0;
        i = 0;

        while (schedule_path[i] != '\0') {

            if (schedule_path[i] == '/') {
                start = i + 1;
            }

            i++;
        }

        /* 文件名格式：route_101_EB.txt */
        i = start + 6;

        char route[32];
        int j = 0;

        /* 复制路线编号 */
        while (schedule_path[i] != '_' &&
               schedule_path[i] != '\0' &&
               j < 31) {

            route[j] = schedule_path[i];

            i++;
            j++;
        }

        route[j] = '\0';

        if (schedule_path[i] != '_' ||
            schedule_path[i + 1] == '\0') {
            continue;
        }

        char direction = schedule_path[i + 1];

       FILE *sched_file = fopen(schedule_path, "r");

        if (sched_file == NULL) {
            continue;
        }

        char file_header[256];
        int num_stops = 0;

        if (fgets(file_header, sizeof(file_header), sched_file) != NULL) {

            /* 找到逗号 */
            i = 0;

            while (file_header[i] != ',' &&
                   file_header[i] != '\0') {

                i++;
            }

            /* 逗号后面的内容包含 stop 数量 */
            if (file_header[i] == ',') {
                num_stops = atoi(file_header + i + 1);
            }

            printf(
                "schedule #%d is route %s it has %d stops and is in the %c direction\n",
                index, route, num_stops, direction
            );
        }

        fclose(sched_file);

        index++;
    }

    fclose(list_file);

    return 0;
}