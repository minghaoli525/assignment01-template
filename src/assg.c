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
        schedule_path[strcspn(schedule_path, "\r\n")] = '\0';
        if (strlen(schedule_path) == 0) continue;

        char route[32];
        char *r_start = strstr(schedule_path, "route_") + 6;
        char *r_end = strchr(r_start, '_');               
        int r_len = r_end - r_start;
        strncpy(route, r_start, r_len);
        route[r_len] = '\0';

        char direction = *(r_end + 1);

        FILE *sched_file = fopen(schedule_path, "r");
        if (!sched_file) continue;

        char file_header[256];
        if (fgets(file_header, sizeof(file_header), sched_file) != NULL) {
            int num_stops = 0;

            char *comma = strchr(file_header, ',');
            if (comma) {
                num_stops = atoi(comma + 1);
            }

            printf("schedule #%d is route %s it has %d stops and is in the %c direction\n",
                   index, route, num_stops, direction);
        }

        fclose(sched_file);
        index++;
    }

    fclose(list_file);
    return 0;
}