/**
 * macfand - hipuranyhou - 25.01.2021
 * 
 * Daemon for controlling fans on Linux systems using
 * applesmc and coretemp.
 * 
 * https://github.com/Hipuranyhou/macfand
 */

#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "fan.h"
#include "helper.h"
#include "logger.h"
#include "settings.h"

#define FAN_PATH_BASE "/sys/devices/platform/applesmc.768"
#define FAN_PATH_RD   "input"
#define FAN_PATH_WR   "output"
#define FAN_PATH_MAX  "max"
#define FAN_PATH_MIN  "min"
#define FAN_PATH_MOD  "manual"
#define FAN_PATH_LBL  "label"
#define FAN_PATH_FMT  FAN_PATH_BASE "/fan%d_%s"

/**
 * @brief Loads label of fan.
 * Loads label of given fan by reading the appropriate system file.
 * @param[in,out] fan Pointer to fan. 
 * @return int 0 on error, 1 on success.
 */
static int fan_load_lbl(t_fan *const fan);

/**
 * @brief Selects where should fan read speed end.
 * Changes *dest to proper location of where should be speed saved after reading
 * it for given fan based on path suffix.
 * @param[in]  fan  Pointer to fan which speed should be read.
 * @param[in]  suff Suffix of speed file from which we read speed.
 * @param[out] dest Pointer where should be read value destination saved.
 * @param[out] path Pointer where should be read path saved.
 * @return int 0 on error, 1 on success.
 */
static int fan_sel_spd_dest(const t_fan *const fan, const char *const suff, int **const dest, char **const path);

/**
 * @brief Loads given speed of fan.
 * Loads speed of given fan by reading the appropriate system file.
 * @param[in,out] fan  Pointer to fan which speed should be read.
 * @param[in]     suff Path end of wanted speed (one of FAN_PATH_MAX, FAN_PATH_MIN or FAN_PATH_RD).
 * @return int 0 on error, 1 on success.
 */ 
static int fan_read_spd(t_fan *const fan, const char *const suff);

/**
 * @brief Loads default values for given fan.
 * Loads max and min speed of given fan, calculates its step size based on these values, constructs its
 * reading, writing and mode setting paths and finally loads its label.
 * @param[in,out] fan Pointer to fan to be loaded.
 * @return int 0 on error, 1 on success.
 */
static int fan_load_def(t_fan *const fan);

/**
 * @brief Frees members of fan.
 * Calls free() on all allocated members of fan, but not fan itself.
 * @param[in] fan Pointer to fan which members should be freed.
 */
static void fan_load_free(t_fan *const fan);


static int fan_load_lbl(t_fan *const fan) {
    char    *path    = NULL;
    FILE    *file    = NULL;
    ssize_t get_ret  = 0;
    size_t  lbl_size = 0;

    if (!fan)
        return 0;

    fan->lbl = NULL;

    path = concat_fmt(FAN_PATH_FMT, fan->id, FAN_PATH_LBL);
    if (!path)
        return 0;

    file = fopen(path, "r");
    free(path);
    if (!file) {
        log_log(LOG_L_DEBUG, "Unable to open label file of fan %d", fan->id);
        return 0;
    }

    errno = 0;
    get_ret = getline(&(fan->lbl), &lbl_size, file);
    if (get_ret < 2 || errno != 0) {
        log_log(LOG_L_DEBUG, "Unable to load label of fan %d", fan->id);
        if (fclose(file) == EOF)
            log_log(LOG_L_DEBUG, "Unable to close label file of fan %d", fan->id);
        return 0;
    }

    // (get_ret - 2) because line ends with "'0x10''0x0A'"
    fan->lbl[get_ret-2] = '\0';

    if (fclose(file) == EOF)
        log_log(LOG_L_DEBUG, "Unable to close label file of fan %d", fan->id);
    return 1;
}


static int fan_sel_spd_dest(const t_fan *const fan, const char *const suff, int **const dest, char **const path) {
    if (!fan || !suff || !dest || !path)
        return 0;

    if (strcmp(FAN_PATH_MIN, suff) == 0) {
        *dest = &(fan->spd.min);
        *path = concat_fmt(FAN_PATH_FMT, fan->id, FAN_PATH_MIN);
    } else if (strcmp(FAN_PATH_MAX, suff) == 0) {
        *dest = &(fan->spd.max);
        *path = concat_fmt(FAN_PATH_FMT, fan->id, FAN_PATH_MIN);
    } else
        return 0;

    if (!(*path))
        return 0;
    return 1;
}


static int fan_read_spd(t_fan *const fan, const char *const suff) {
    char    *str     = NULL;
    size_t  str_size = 0;
    ssize_t get_ret  = 0;
    FILE    *file    = NULL;
    int     *rd_dest = &(fan->spd.real);
    char    *rd_path = fan->path.rd;

    if (!fan || (suff && !fan_sel_spd_dest(fan, suff, &rd_dest, &rd_path)))
        return 0;

    // Open file for reading
    file = fopen(rd_path, "r");
    if (!file) {
        log_log(LOG_L_DEBUG, "Unable to open speed file of fan %d", fan->id);
        return 0;
    }

    // Read line from speed file and remove trailing '\n'
    errno = 0;
    get_ret = getline(&str, &str_size, file);
    if (get_ret < 2 || errno != 0) {
        log_log(LOG_L_DEBUG, "Invalid speed file of fan %d", fan->id);
        if (fclose(file) == EOF)
            log_log(LOG_L_DEBUG, "Unable to close speed file of fan %d", fan->id);
        return 0;
    }
    str[get_ret-1] = '\0';

    // Convert read line to integer
    if (str_to_int(str, rd_dest, 10, NULL) < 1) {
        log_log(LOG_L_DEBUG, "Invalid speed of fan %d", fan->id);
        if (fclose(file) == EOF)
            log_log(LOG_L_DEBUG, "Unable to close speed file of fan %d", fan->id);
        return 0;
    }

    if (fclose(file) == EOF)
        log_log(LOG_L_DEBUG, "Unable to close speed file of fan %d", fan->id);
    return 1;
}


static int fan_load_def(t_fan *const fan) {
    int temp_max  = settings_get_value(SET_TEMP_MAX);
    int temp_high = settings_get_value(SET_TEMP_HIGH);

    if (!fan)
        return 0;

    // Load all paths of given fan
    fan->path.rd = concat_fmt(FAN_PATH_FMT, fan->id, FAN_PATH_RD);
    fan->path.wr = concat_fmt(FAN_PATH_FMT, fan->id, FAN_PATH_WR);
    fan->path.mod = concat_fmt(FAN_PATH_FMT, fan->id, FAN_PATH_MOD);
    if (!fan->path.rd || !fan->path.wr || !fan->path.mod) {
        log_log(LOG_L_DEBUG, "Unable to load read, write or mode path of fan %d", fan->id);
        return 0;
    }

    // Load min and max speed of given fan
    if (!fan_read_spd(fan, FAN_PATH_MIN) || !fan_read_spd(fan, FAN_PATH_MAX)) {
        log_log(LOG_L_DEBUG, "Unable to load max or min speed of fan %d", fan->id);
        return 0;
    }

    fan->spd.real = 0;
    fan->spd.tgt = 0;

    // Calculate size of one unit of fan speed change
    fan->spd.step = (fan->spd.max - fan->spd.min) / ((temp_max - temp_high) * (temp_max - temp_high + 1) / 2);

    // Load fan label
    if (!fan_load_label(fan)) {
        log_log(LOG_L_DEBUG, "Unable to load label of fan %d", fan->id);
        return 0;
    }

    return 1;
}


t_node* fans_load(void) {
    struct dirent *dirent    = NULL;
    DIR           *dir       = NULL;
    char          *fname     = NULL;
    char          inv        = 0;
    int           id_prev    = 0;
    int           to_int_ret = 0;
    t_fan         fan;
    t_node        *fans      = NULL;

    // Open fans directory
    dir = opendir(FAN_PATH_BASE);
    if (!dir) {
        log_log(LOG_L_DEBUG, "Unable to open system fans directory");
        return NULL;
    }

    // Walk through fans directory
    while (dirent = readdir(dir)) {
        fname = basename(dirent->d_name);

        if (!fname || strncmp(fname, "fan", 3) != 0)
            continue;

        // Get id of fan
        to_int_ret = str_to_int(fname+3, &(fan.id), 10, &inv);
        if (to_int_ret < 0 || inv != '_') {
            list_free(fans, fan_free);
            fan_load_free(&fan);
            log_log(LOG_L_DEBUG, "Invalid fan filename encountered.");
            return NULL;
        }

        // Skip already finished fan
        if (fan.id == id_prev)
            continue;
        id_prev = fan.id;

        // Load fan defaults and append it to linked list of fans
        if (!fan_load_def(&fan) || !list_push_front(&fans, &fan, sizeof(fan))) {
            list_free(fans, fan_free);
            fan_load_free(&fan);
            log_log(LOG_L_DEBUG, "Unable to load defaults of fan %d", fan.id);
            return NULL;
        }
    }

    if (closedir(dir) < 0)
        log_log(LOG_L_DEBUG, "Unable to close system fans directory");
    return fans;
}


int fans_write_mod(const t_node *fans, const enum fan_mode mod) {
    int   state = 1;
    FILE  *file = NULL;
    t_fan *fan  = NULL;

    if (!fans || mod < FAN_M_AUTO || mod > FAN_M_MAN)
        return 0;

    while (fans) {
        fan = fans->data;

        file = fopen(fan->path.mod, "w+");
        if (!file) {
            log_log(LOG_L_DEBUG, "Unable to open mode file of fan %d", fan->id);
            state = 0;
            fans = fans->next;
            continue;
        }

        if (fprintf(file, "%d\n", mod) < 0) {
            log_log(LOG_L_DEBUG, "Unable to write mode of fan %d", fan->id);
            state = 0;
        }

        if (fclose(file) == EOF) {
            log_log(LOG_L_DEBUG, "Unable to close mode file of fan %d", fan->id);
            state = 0;
        }

        fans = fans->next;
    }

    return state;
}


int fan_write_spd(t_fan *const fan) {
    FILE *file = NULL;

    if (!fan)
        return 0;

    // Check current fan speed
    if (!fan_read_spd(fan, FAN_PATH_RD)) {
        log_log(LOG_L_DEBUG, "Unable to read speed of fan %d", fan->id);
        return 0;
    }

    // Skip writing speed if already same
    if (fan->spd.real == fan->spd.tgt)
        return 1;

    file = fopen(fan->path.wr, "w+");
    if (!file) {
        log_log(LOG_L_DEBUG, "Unable to open output speed file of fan %d", fan->id);
        return 0;
    }

    // Write new fan speed
    if (fprintf(file, "%d\n", fan->spd.tgt) < 0) {
        log_log(LOG_L_DEBUG, "Unable to write speed of fan %d", fan->id);
        if (fclose(file) == EOF)
            log_log(LOG_L_DEBUG, "Unable to close output speed file of fan %d", fan->id);
        return 0;
    }

    if (fclose(file) == EOF) {
        log_log(LOG_L_DEBUG, "Unable to close output speed file of fan %d", fan->id);
        return 0;
    }

    return 1;
}


static void fan_load_free(t_fan *const fan) {
    if (!fan)
        return;

    if (fan->path.wr)
        free(fan->path.wr);
    if (fan->path.mod)
        free(fan->path.mod);
    if (fan->path.rd)
        free(fan->path.rd);
    if (fan->lbl)
        free(fan->lbl);
}


void fan_free(t_fan *fan) {
    if (!fan)
        return;

    if (fan->lbl)
        free(fan->lbl);
    if (fan->path.rd)
        free(fan->path.rd);
    if (fan->path.wr)
        free(fan->path.wr);
    if (fan->path.mod)
        free(fan->path.mod);
    free(fan);
}


void fan_print(const t_fan *const fan, FILE *const file) {
    if (!fan || !file)
        return;

    fprintf(file, "Fan %d - %s\n", fan->id, fan->lbl);
    fprintf(file, "Min speed: %d    Max speed: %d    Step: %d\n", fan->spd.min, fan->spd.max, fan->spd.step);
    fprintf(file, "Read: %s\n", fan->path.rd);
    fprintf(file, "Write: %s\n", fan->path.wr);
    fprintf(file, "Mode: %s\n", fan->path.mod);
}