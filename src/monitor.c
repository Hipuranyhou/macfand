/**
 * macfand - hipuranyhou - 21.06.2020
 * 
 * Daemon for controlling fans on Linux systems using
 * applesmc and coretemp.
 * 
 * https://github.com/Hipuranyhou/macfand
 */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "monitor.h"
#include "helper.h"

#define MONITOR_PATH_BASE "/sys/devices/platform/coretemp.0/hwmon/hwmon"
#define MONITOR_PATH_READ "_input"
#define MONITOR_PATH_MAX "_max"
#define MONITOR_PATH_LABEL "_label"
#define MONITOR_PATH_FORMAT "%s%d/temp%d%s" 


int monitor_load_label(t_monitor *monitor) {
    char *monitor_path_label = NULL;
    FILE *monitor_file_label = NULL;
    int getline_return = 0;
    size_t buffer_size = 0;

    if (!monitor)
        return 0;

    monitor->label = NULL;

    monitor_path_label = concatenate_format(MONITOR_PATH_FORMAT, MONITOR_PATH_BASE, monitor->id_hw, monitor->id, MONITOR_PATH_LABEL);
    if (!monitor_path_label)
        return 0;

    monitor_file_label = fopen(monitor_path_label, "r");
    free(monitor_path_label);
    if (!monitor_file_label)
        return 0;

    getline_return = getline(&(monitor->label), &buffer_size, monitor_file_label);
    if (getline_return == -1) {
        fclose(monitor_file_label);
        return 0;
    }

    // (getline_return-1) because line ends with "0x0A"
    monitor->label[getline_return-1] = '\0';

    // We silently ignore return value of fclose() when just reading from it
    fclose(monitor_file_label);
    return 1;
}


int monitor_load_max_temp(t_monitor *monitor) {
    char *monitor_path_max = NULL;
    FILE *monitor_file_max = NULL;

    if (!monitor)
        return 0;

    monitor_path_max = concatenate_format(MONITOR_PATH_FORMAT, MONITOR_PATH_BASE, monitor->id_hw, monitor->id, MONITOR_PATH_MAX);
    if (!monitor_path_max)
        return 0;

    monitor_file_max = fopen(monitor_path_max, "r");
    free(monitor_path_max);
    if (!monitor_file_max)
        return 0;

    if (fscanf(monitor_file_max, "%d", &(monitor->temp_max)) != 1) {
        fclose(monitor_file_max);
        return 0;
    }

    monitor->temp_max /= 1000; 

    // We silently ignore return value of fclose() when just reading from it
    fclose(monitor_file_max);
    return 1;
}

int monitor_load_defaults(t_monitor *monitor) {
    if (!monitor)
        return 0;

    if (!monitor_load_max_temp(monitor))
        return 0;

    monitor->path_read = concatenate_format(MONITOR_PATH_FORMAT, MONITOR_PATH_BASE, monitor->id_hw, monitor->id, MONITOR_PATH_READ);
    if (!monitor->path_read)
        return 0;

    if (!monitor_load_label(monitor))
        return 0;

    return 1;
}


int monitor_id_exists(const int cnt_hw, const int cnt_mon) {
    FILE *monitor_file_read = NULL;
    char *monitor_path_read = NULL;

    monitor_path_read = concatenate_format(MONITOR_PATH_FORMAT, MONITOR_PATH_BASE, cnt_hw, cnt_mon, MONITOR_PATH_READ);
    if (!monitor_path_read)
        return -1;

    monitor_file_read = fopen(monitor_path_read, "r");
    free(monitor_path_read);
    if (!monitor_file_read)
        return 0;

    // We silently ignore return value of fclose() when just checking if the file exists
    fclose(monitor_file_read);
    return 1;
}


t_node *monitors_load(void) {
    int cnt_mon = 0, monitor_exists = 0;
    t_monitor *monitor = NULL;
    t_node *monitors = NULL;

    monitor = (t_monitor*)malloc(sizeof(*monitor));
    if (!monitor)
        return NULL;

    monitor->temp_current = 0;

    for (int cnt_hw = 0; cnt_hw < 16; ++cnt_hw) {
        for (;;) {
            monitor->id_hw = cnt_hw;
            monitor->id = ++cnt_mon;
            monitor_exists = monitor_id_exists(cnt_hw, cnt_mon);

            if (monitor_exists == 0)
                break;

            if (monitor_exists == -1                        ||
                !monitor_load_defaults(monitor)             ||
                !list_push_front(&monitors, monitor, sizeof(*monitor)))
            {
                list_free(monitors, (void (*)(void *))monitor_free);
                monitors = NULL;
                free(monitor);
                break;
            }
        }
    }

    free(monitor);
    return monitors;
}


int monitors_get_temp(const t_node *monitors) {
    int temp = 0;
    FILE *monitor_file_read = NULL;
    t_monitor *monitor = NULL;

    if (!monitors)
        return 100;

    while (monitors) {
        monitor = monitors->data;

        monitor_file_read = fopen(monitor->path_read, "r");
        if (monitor_file_read == NULL) {
            monitors = monitors->next;
            continue;
        }

        if (fscanf(monitor_file_read, " %d", &(monitor->temp_current)) == EOF) {
            fclose(monitor_file_read);
            monitors = monitors->next;
            continue;
        }

        // We silently ignore return value of fclose() when just checking if the file exists
        fclose(monitor_file_read);
        temp = (monitor->temp_current > temp) ? monitor->temp_current : temp;
        monitors = monitors->next;
    }

    // Return 100 if failed to load at least one temperature to crank up the fans
    return (temp == 0) ? 100 : (temp / 1000);
}


void monitor_free(t_monitor *monitor) {
    if (!monitor)
        return;
    if (monitor->path_read)
        free(monitor->path_read);
    if (monitor->label)
        free(monitor->label);
    free(monitor);
}


void monitor_print(const t_monitor *monitor) {
    if (!monitor)
        return;
    printf("Monitor %d - %s\n", monitor->id, monitor->label);
    printf("Current temp: %d°C  Max temp: %d°C\n", monitor->temp_current, monitor->temp_max);
    printf("Read: %s\n", monitor->path_read);
}