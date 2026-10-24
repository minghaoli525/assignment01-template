#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// AI assistance: OpenAI ChatGPT, prompted by Leon Li, October 22-24, 2026.
// Topics: storing schedule data, checking input, matching journeys,
// missing-tap prediction, time conversion, and formatted file output.
// This version keeps Leon's structs, arrays, and main loop as its base.
// Check the course AI citation rules before submitting.
#define MAX_SCHEDULES 300
#define MAX_STOPS 128
#define MAX_STOP_NAME 128
#define MAX_TAPS 20000

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

    char route_id[32];
    char direction;
    int schedule_index; // Position of selected route in schedules[]
    int guessed;        // 1 for a Missing Tap, 0 for a real Exit
} Journey;

/* Check whether a character is a digit. */
int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int leap_year(int year) {
    return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

int days_in_month(int year, int month) {
    int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && leap_year(year)) return 29;
    return days[month - 1];
}

/* The tap file uses MM-DD-YYYY and HH:MM AM/PM. */
int valid_date_time(const char date[], const char time[]) {
    if (strlen(date) != 10 || strlen(time) != 8) return 0;
    if (date[2] != '-' || date[5] != '-' ||
        time[2] != ':' || time[5] != ' ') return 0;

    for (int i = 0; i < 10; i++) {
        if (i != 2 && i != 5 && !is_digit(date[i])) return 0;
    }
    for (int i = 0; i < 5; i++) {
        if (i != 2 && !is_digit(time[i])) return 0;
    }
    if ((time[6] != 'A' && time[6] != 'P') || time[7] != 'M') return 0;

    int month = (date[0] - '0') * 10 + date[1] - '0';
    int day = (date[3] - '0') * 10 + date[4] - '0';
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 +
               (date[8] - '0') * 10 + date[9] - '0';
    int hour = (time[0] - '0') * 10 + time[1] - '0';
    int minute = (time[3] - '0') * 10 + time[4] - '0';

    return year > 0 && day > 0 && day <= days_in_month(year, month) &&
           hour >= 1 && hour <= 12 && minute >= 0 && minute <= 59;
}

/* Convert an ASCII number to int; reject extra characters. */
int read_stop_id(const char text[], int *answer) {
    if (text[0] == '\0') return 0;
    long number = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (!is_digit(text[i])) return 0;
        int digit = text[i] - '0';
        if (number > (2147483647L - digit) / 10) return 0;
        number = number * 10 + digit;
    }
    if (number <= 0) return 0;
    *answer = (int) number;
    return 1;
}

/* Check the ID against all stops in all schedules. */
int stop_exists(Schedule schedules[], int schedule_count, int stop_id) {
    for (int s = 0; s < schedule_count; s++) {
        for (int k = 0; k < schedules[s].num_stops; k++) {
            if (schedules[s].stops[k].stop_id == stop_id) return 1;
        }
    }
    return 0;
}

/* 12:xx AM = 00:xx, 01:xx PM = 13:xx. */
int hour_24(const char time[]) {
    int hour = (time[0] - '0') * 10 + time[1] - '0';
    if (hour == 12) hour = 0;
    if (time[6] == 'P') hour += 12;
    return hour;
}

/* Convert date + time to one number, so subtraction gives a duration. */
long long time_in_minutes(const char date[], const char time[]) {
    int month = (date[0] - '0') * 10 + date[1] - '0';
    int day = (date[3] - '0') * 10 + date[4] - '0';
    int year = (date[6] - '0') * 1000 + (date[7] - '0') * 100 +
               (date[8] - '0') * 10 + date[9] - '0';
    int minute = (time[3] - '0') * 10 + time[4] - '0';

    long long previous_years = year - 1;
    long long days = previous_years * 365 + previous_years / 4 -
                     previous_years / 100 + previous_years / 400;
    for (int m = 1; m < month; m++) {
        days += days_in_month(year, m);
    }
    days += day - 1;
    return days * 24 * 60 + hour_24(time) * 60 + minute;
}

/* Convert MM-DD-YYYY + HH:MM AM/PM to YYYY-MM-DDTHH:MM:SS. */
void make_iso(char iso[20], const char date[], const char time[]) {
    int hour = hour_24(time);
    for (int i = 0; i < 4; i++) iso[i] = date[i + 6];
    iso[4] = '-';
    iso[5] = date[0];
    iso[6] = date[1];
    iso[7] = '-';
    iso[8] = date[3];
    iso[9] = date[4];
    iso[10] = 'T';
    iso[11] = (char) ('0' + hour / 10);
    iso[12] = (char) ('0' + hour % 10);
    iso[13] = ':';
    iso[14] = time[3];
    iso[15] = time[4];
    iso[16] = ':';
    iso[17] = '0';
    iso[18] = '0';
    iso[19] = '\0';
}

/* Look up the name on the route the user selected. */
const char *find_stop_name(Schedule schedules[], int route_index, int stop_id) {
    for (int k = 0; k < schedules[route_index].num_stops; k++) {
        if (schedules[route_index].stops[k].stop_id == stop_id) {
            return schedules[route_index].stops[k].stop_name;
        }
    }
    return "Unknown_Stop";
}

/* Missing Tap: count previous real exits with the same entry stop and hour. */
int guess_exit(Journey journeys[], int journey_count,
               int entry_stop, const char entry_time[], int *guessed_stop) {
    int best_stop = -1;
    int best_count = 0;
    for (int i = 0; i < journey_count; i++) {
        if (journeys[i].guessed || journeys[i].entry_stop != entry_stop ||
            hour_24(journeys[i].entry_time) != hour_24(entry_time)) continue;

        int stop = journeys[i].exit_stop;
        int count = 0;
        for (int j = 0; j < journey_count; j++) {
            if (!journeys[j].guessed &&
                journeys[j].entry_stop == entry_stop &&
                hour_24(journeys[j].entry_time) == hour_24(entry_time) &&
                journeys[j].exit_stop == stop) count++;
        }
        if (count > best_count || (count == best_count && stop < best_stop)) {
            best_count = count;
            best_stop = stop;
        }
    }
    if (best_count == 0) return 0;
    *guessed_stop = best_stop;
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
    printf("Usage: %s <list_file> <tap_history>\n", argv[0]);
    return 1;
}

    FILE *list_file = fopen(argv[1], "r");
    if (!list_file) {
        printf("Cannot open schedule list file.\n");
        return 1;
    }

    static Schedule schedules[MAX_SCHEDULES];// 这个是创建了一个数组
    int schedule_count = 0;

    // We already keep the actual number of schedules in schedule_count.
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
        if (schedule_count >= MAX_SCHEDULES) {
            printf("Too many schedule files.\n");
            fclose(list_file);
            return 1;
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
            if (num_stops < 1 || num_stops > MAX_STOPS) {
                printf("Invalid schedule file.\n");
                fclose(sched_file);
                fclose(list_file);
                return 1;
            }
            strcpy(schedules[schedule_count].route_id, route);//这里 strcpy() 把 route 中的 "101" 复制到了 route_id 中。这是因为在 C 语言中，不能使用 = 给一个已经声明的字符数组整体赋值。
            schedules[schedule_count].direction = direction;
            schedules[schedule_count].num_stops = num_stops;
            char stop_line[256];

            for (int k = 0; k < num_stops; k++) {

                if (fgets(stop_line, sizeof(stop_line), sched_file) == NULL ||
                    sscanf(stop_line, "%d,%127[^\r\n]",//读取后面的字符串，直到遇到换行或回车，但最多只读取 127 个字符。
                           &schedules[schedule_count].stops[k].stop_id,
                           schedules[schedule_count].stops[k].stop_name) != 2) {
                    printf("Invalid schedule file.\n");
                    fclose(sched_file);
                    fclose(list_file);
                    return 1;
                }
            }

            schedule_count++;  
        }

        fclose(sched_file);

    }

    fclose(list_file);

    /*starting reading second file*/
    FILE *tap_file = fopen(argv[2], "r");

if (tap_file == NULL) {
    printf("Cannot open tap history file.\n");
    return 1;
}

char tap_line[256];
int num_taps = 0;

// 读取第一行：Tap 记录总数
if (fgets(tap_line, sizeof(tap_line), tap_file) == NULL) {
    printf("Invalid data format\n");
    fclose(tap_file);
    return 1;
}
for (int p = 0; tap_line[p] != '\0'; p++) {
    if (tap_line[p] == '\r' || tap_line[p] == '\n') {
        tap_line[p] = '\0';
        break;
    }
}
if (!read_stop_id(tap_line, &num_taps) || num_taps > MAX_TAPS) {
    printf("Invalid data format\n");
    fclose(tap_file);
    return 1;
}

// 创建数组，保存所有 Tap
static Tap taps[MAX_TAPS];

// 按文件中的顺序读取所有 Tap
for (int i = 0; i < num_taps; i++) {

if (fgets(tap_line, sizeof(tap_line), tap_file) == NULL) {
    printf("Invalid data format\n");
    fclose(tap_file);
    return 1;
}

    // Remove the end-of-line character before checking Stop ID.
    for (int p = 0; tap_line[p] != '\0'; p++) {
        if (tap_line[p] == '\r' || tap_line[p] == '\n') {
            tap_line[p] = '\0';
            break;
        }
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
    // Check the three commas and make sure Type is exactly one letter.
    int commas[3] = {-1, -1, -1};
    int comma_count = 0;
    for (int p = 0; tap_line[p] != '\0'; p++) {
        if (tap_line[p] == ',') {
            if (comma_count < 3) commas[comma_count] = p;
            comma_count++;
        }
    }

    int format_ok = comma_count == 3 &&
                    commas[2] == commas[1] + 2 &&
                    fields_read >= 3;
    if (format_ok) {
        format_ok = valid_date_time(taps[i].date, taps[i].time) &&
                    (taps[i].type == 'E' ||
                     taps[i].type == 'X' || taps[i].type == 'M');
    }
    if (format_ok && (taps[i].type == 'E' || taps[i].type == 'X' ||
                      tap_line[commas[2] + 1] != '\0')) {
        // The location must be a complete, positive integer.
        format_ok = read_stop_id(tap_line + commas[2] + 1,
                                 &taps[i].stop_id);
    }
    if (!format_ok ||
        ((taps[i].type == 'E' || taps[i].type == 'X') &&
         fields_read != 4)) {
        printf("Invalid data format\n");
        fclose(tap_file);
        return 1;
    }

    if (taps[i].type != 'M' &&
        !stop_exists(schedules, schedule_count, taps[i].stop_id)) {
        printf("Invalid stop ID\n");
        fclose(tap_file);
        return 1;
    }
}

fclose(tap_file);
static Journey journeys[MAX_TAPS];
int journey_count = 0;
int entry_index = -1;

for (int i = num_taps - 1; i >= 0; i--) {

    if (taps[i].type == 'E') {
        // Remember the Entry until an X or M closes this trip.
        if (entry_index != -1) {
            printf("Invalid data format\n");
            return 1;
        }
        entry_index = i;
    }
    else if (taps[i].type == 'X' || taps[i].type == 'M') {
        if (entry_index == -1) {
            printf("Invalid data format\n");
            return 1;
        }

        // Keep using the Journey struct and arrays from Leon's code.
        journeys[journey_count].entry_stop = taps[entry_index].stop_id;
        journeys[journey_count].exit_stop = taps[i].stop_id;

        strcpy(journeys[journey_count].entry_date, taps[entry_index].date);
        strcpy(journeys[journey_count].entry_time, taps[entry_index].time);
        strcpy(journeys[journey_count].exit_date, taps[i].date);
        strcpy(journeys[journey_count].exit_time, taps[i].time);

        journeys[journey_count].route_id[0] = '\0';
        journeys[journey_count].direction = '\0';
        journeys[journey_count].schedule_index = -1;
        journeys[journey_count].guessed = (taps[i].type == 'M');

        // Task 7: guess a Missing Tap from earlier real trips.
        if (taps[i].type == 'M') {
            int guessed_stop = -1;
            if (!guess_exit(journeys, journey_count,
                            journeys[journey_count].entry_stop,
                            journeys[journey_count].entry_time,
                            &guessed_stop)) {
                printf("Unable to guess missing tap.\n");
                return 1;
            }
            journeys[journey_count].exit_stop = guessed_stop;
        }

        journey_count++;

        // Leon's original candidate[] route-matching method.
        int entry_stop = journeys[journey_count - 1].entry_stop;
        int exit_stop = journeys[journey_count - 1].exit_stop;
        int candidate_count = 0;
        int candidates[MAX_SCHEDULES];

        for (int s = 0; s < schedule_count; s++) {

            int entry_pos = -1;
            int exit_pos = -1;

            for (int k = 0; k < schedules[s].num_stops; k++) {
                int current_stop = schedules[s].stops[k].stop_id;
                if (entry_pos == -1 && current_stop == entry_stop) {
                    entry_pos = k;
                }
                else if (entry_pos != -1 && current_stop == exit_stop) {
                    exit_pos = k;
                    break;
                }
            }

            if (entry_pos != -1 && exit_pos != -1) {
                candidates[candidate_count] = s;
                candidate_count++;
            }
        }

        if (candidate_count == 0) {
            printf("No matching route found.\n");
            return 1;
        }
        else if (candidate_count == 1) {
            int selected = candidates[0];
            strcpy(journeys[journey_count - 1].route_id,
                   schedules[selected].route_id);
            journeys[journey_count - 1].direction = schedules[selected].direction;
            journeys[journey_count - 1].schedule_index = selected;
        }
        else {
            // Task 6: follow the exact interface from the assignment.
            printf("Ambiguous trip detected:\n");
            printf("Entry: %d (%s)\n", entry_stop,
                   find_stop_name(schedules, candidates[0], entry_stop));
            printf("Exit: %d (%s)\n", exit_stop,
                   find_stop_name(schedules, candidates[0], exit_stop));
            printf("Possible routes:\n");
            for (int c = 0; c < candidate_count; c++) {
                int s = candidates[c];
                printf("%d. %s %c\n", c + 1,
                       schedules[s].route_id, schedules[s].direction);
            }

            int choice = 0;
            char input[128];

            while (1) {
                printf("Select route (1-%d): \n", candidate_count);
                fflush(stdout);

                if (fgets(input, sizeof(input), stdin) == NULL) return 1;

                // Reject a line that is too long to fit in input[].
                if (strchr(input, '\n') == NULL && !feof(stdin)) {
                    int ch;
                    while ((ch = getchar()) != '\n' && ch != EOF) { }
                    printf("Invalid selection\n");
                    continue;
                }

                char tail;
                int fields_read = sscanf(input, " %d %c", &choice, &tail);

                if (fields_read != 1 || choice < 1 || choice > candidate_count) {
                    printf("Invalid selection\n");
                    continue;
                }
                break;
            }

            int selected = candidates[choice - 1];
            strcpy(journeys[journey_count - 1].route_id,
                   schedules[selected].route_id);
            journeys[journey_count - 1].direction = schedules[selected].direction;
            journeys[journey_count - 1].schedule_index = selected;
        }

        entry_index = -1; // The next trip starts with a new Entry.
    }
}

if (entry_index != -1) {
    printf("Invalid data format\n");
    return 1;
}

// Task 8: generate the final file instead of using debugging printf().
FILE *output_file = fopen("output.txt", "w");
if (output_file == NULL) {
    printf("Cannot create output.txt.\n");
    return 1;
}

for (int j = 0; j < journey_count; j++) {

    char entry_iso[20];
    char exit_iso[20];

    make_iso(entry_iso, journeys[j].entry_date, journeys[j].entry_time);
    make_iso(exit_iso, journeys[j].exit_date, journeys[j].exit_time);

    long long duration = time_in_minutes(journeys[j].exit_date,
                                         journeys[j].exit_time) -
                         time_in_minutes(journeys[j].entry_date,
                                         journeys[j].entry_time);

    if (duration < 0) {
        printf("Invalid data format\n");
        fclose(output_file);
        return 1;
    }

    int route_index = journeys[j].schedule_index;

    fprintf(output_file, "%s %c,%s,%s,%s,%s,%lld:%02lld:00%s\n",
            journeys[j].route_id,
            journeys[j].direction,
            find_stop_name(schedules, route_index, journeys[j].entry_stop),
            find_stop_name(schedules, route_index, journeys[j].exit_stop),
            entry_iso, exit_iso,
            duration / 60, duration % 60,
            journeys[j].guessed ? ",Guessed" : "");
}

fclose(output_file);
return 0;
} // end main
