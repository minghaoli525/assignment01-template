#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The following function contains ideas from
// OpenAI ChatGPT.
// Subject: "What should I do if the information read from the first file is no longer accessible when processing the second file because it was read inside a while loop?"
// Prompted by Leon Li on 2026-10-22.
#define MAX_SCHEDULES 300
#define MAX_STOPS 128
#define MAX_STOP_NAME 128

typedef struct {
    int stop_id;
    char stop_name[MAX_STOP_NAME];
}Stop;

typedef struct {
    char route_id[32];
    char direction;
    int num_stops;
    Stop stops[MAX_STOPS];
} Schedule;

typedef struct {
    char date[20];
    char time[20];
    char type;
    int stop_id;
} Tap;

typedef struct {
    int entry_stop;
    int exit_stop;

    char entry_date[20];
    char entry_time[20];

    char exit_date[20];
    char exit_time[20];
} Journey;

int main(int argc, char *argv[]) {
    if (argc != 3) {
    return 1;
}

    FILE *list_file = fopen(argv[1], "r");
    if (!list_file) return 1;

    Schedule schedules[MAX_SCHEDULES];// 这个是创建了一个数组
    int schedule_count = 0;

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
            strcpy(schedules[schedule_count].route_id, route);//这里 strcpy() 把 route 中的 "101" 复制到了 route_id 中。这是因为在 C 语言中，不能使用 = 给一个已经声明的字符数组整体赋值。
            schedules[schedule_count].direction = direction;
            schedules[schedule_count].num_stops = num_stops;
            char stop_line[256];

            for (int k = 0; k < num_stops; k++) {

                fgets(stop_line, sizeof(stop_line), sched_file);

                sscanf(stop_line, "%d,%127[^\r\n]",//读取后面的字符串，直到遇到换行或回车，但最多只读取 127 个字符。
                    &schedules[schedule_count].stops[k].stop_id,
                    schedules[schedule_count].stops[k].stop_name);
            }

            schedule_count++;  
        }

        fclose(sched_file);

        index++;
    }

    fclose(list_file);

    /*starting reading second file*/
    FILE *tap_file = fopen(argv[2], "r");

if (tap_file == NULL) {
    printf("Cannot open tap history file.\n");
    return 1;
}

char tap_line[256];
int num_taps;

// 读取第一行：Tap 记录总数
fgets(tap_line, sizeof(tap_line), tap_file);
num_taps = atoi(tap_line);

// 创建数组，保存所有 Tap
Tap taps[num_taps];

// 按文件中的顺序读取所有 Tap
for (int i = 0; i < num_taps; i++) {

if (fgets(tap_line, sizeof(tap_line), tap_file) == NULL) {
    printf("Invalid tap record\n");
    fclose(tap_file);
    return 1;
}

    taps[i].stop_id = -1;

    int fields_read = sscanf(
        tap_line,
        "%19[^,],%19[^,],%c,%d",
        taps[i].date,
        taps[i].time,
        &taps[i].type,
        &taps[i].stop_id
    );
    if (fields_read < 3) {

    printf("Invalid tap record\n");

}
else if (taps[i].type != 'E' &&
         taps[i].type != 'X' &&
         taps[i].type != 'M') {

    printf("Invalid tap record\n");

}
else if ((taps[i].type == 'E' || taps[i].type == 'X')
         && fields_read != 4) {

    printf("Invalid tap record\n");
}
}

fclose(tap_file);
Journey journeys[num_taps];
int journey_count = 0;
int entry_index = -1;

for (int i = num_taps - 1; i >= 0; i--) {

    if (taps[i].type == 'E') {

        entry_index = i;

        printf("Found Entry at stop %d\n",
               taps[i].stop_id);
    }

    else if (taps[i].type == 'X' && entry_index != -1) {
        journeys[journey_count].entry_stop =
    taps[entry_index].stop_id;

        journeys[journey_count].exit_stop =
            taps[i].stop_id;
            strcpy(journeys[journey_count].entry_date,
       taps[entry_index].date);

            strcpy(journeys[journey_count].entry_time,
                taps[entry_index].time);

            strcpy(journeys[journey_count].exit_date,
                taps[i].date);

            strcpy(journeys[journey_count].exit_time,
                taps[i].time);

        journey_count++;

        printf("Journey: %d -> %d\n",
               taps[entry_index].stop_id,
               taps[i].stop_id);
               int entry_stop = taps[entry_index].stop_id;
                int exit_stop = taps[i].stop_id;
                int candidate_count = 0;//这个变量专门统计有多少条路线满足要求。
                int candidates[MAX_SCHEDULES];
                for (int s = 0; s < schedule_count; s++) {

            int entry_pos = -1;
            int exit_pos = -1;

                    for (int k = 0; k < schedules[s].num_stops; k++) {

                        if (schedules[s].stops[k].stop_id == entry_stop) {
                            entry_pos = k;
                        }

                        if (schedules[s].stops[k].stop_id == exit_stop) {
                            exit_pos = k;
                        }
                    }

                    if (entry_pos != -1 &&
                        exit_pos != -1 &&
                        entry_pos < exit_pos) {
                            candidates[candidate_count] = s;
                            candidate_count++;
                        printf("Possible route: %s %c\n",
                            schedules[s].route_id,
                            schedules[s].direction);
                    }
                }
                if (candidate_count == 0) {
                        printf("No matching route found.\n");
                    }
                    else if (candidate_count == 1) {
                        int selected = candidates[0];

                        printf("Unique route: %s %c\n",
                            schedules[selected].route_id,
                            schedules[selected].direction);
                    }
                    else {
                        printf("Ambiguous trip: %d possible routes.\n",
                            candidate_count);
                    }
                        entry_index = -1;
                    }
                }

    return 0;
}