/**
 * macfand - hipuranyhou - 21.06.2020
 * 
 * Daemon for controlling fans on Linux systems using
 * applesmc and coretemp.
 * 
 * https://github.com/Hipuranyhou/macfand
 */

#ifndef MACFAND_MONITOR_H_fajkdsfbua
#define MACFAND_MONITOR_H_fajkdsfbua

#include "config.h"
#include "linked.h"

/**
 * @brief 
 * 
 */
typedef struct monitor {
    int id;
    int temp;
    char *read_path;
} t_monitor;

/**
 * @brief Set the max temp object
 * 
 * @param settings 
 * @param cnt_hw 
 * @param cnt_mon 
 */
void settings_set_max(t_settings *settings, const int cnt_hw, const int cnt_mon);

/**
 * @brief 
 * 
 * @param cnt_hw 
 * @param cnt_temp 
 * @return int 
 */
int monitor_exists(const int cnt_hw, const int cnt_temp);

/**
 * @brief 
 * 
 * @param settings 
 * @return t_node* 
 */
t_node *monitors_load(t_settings *settings);

/**
 * @brief 
 * 
 * @param monitor 
 */
void monitor_free(t_monitor *monitor);

/**
 * @brief Get the current temp object
 * 
 * @param monitors 
 * @return int 
 */
int monitors_get_temp(const t_node *monitors);

#endif //MACFAND_MONITOR_H_fajkdsfbua
