/**
 * macfand - hipuranyhou - 21.06.2020
 * 
 * Daemon for controlling fans on Linux systems using
 * applesmc and coretemp.
 * 
 * https://github.com/Hipuranyhou/macfand
 */

#include <stdio.h>

#include "widget.h"
#include "settings.h"
#include "logger.h"
#include "fan.h"


void widget_write(const t_node *fans) {
    FILE *widget_file = NULL;
    t_fan *fan = NULL;

    widget_file = fopen(settings_get_value_string(SET_WIDGET_FILE_PATH), "w");
    if (!widget_file) {
        logger_log(LOG_L_ERROR, "%s", "Unable to open widget file");
        return;
    }

    while (fans) {
        fan = fans->data;
        if (fprintf(widget_file, "%d(f%d)%c", fan->speed, fan->id, (fans->next) ? ' ' : '\0') < 0)
            logger_log(LOG_L_ERROR, "%s %d %s", "Unable to write speed of fan", fan->id, "to widget file");
        fans = fans->next;
    }

    if (fflush(widget_file) == EOF)
        logger_log(LOG_L_ERROR, "%s", "Unable to write widget file");

    if (fclose(widget_file) == EOF) {
        logger_log(LOG_L_ERROR, "%s", "Unable to close widget file");
        return;
    }

}