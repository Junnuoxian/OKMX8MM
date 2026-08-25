#ifndef __MYSQL_CLEANER_H

#define __MYSQL_CLEANER_H 1

#include "mysql-config/mysql-config.h"

typedef struct mysql_cleaner_config_struct {

    // mysql configuration
    mysql_config_t *mysql_config;

    // minimize free space in bytes
    int64_t rolling_free_space;

    // check interval (ms), normally we just check 1/2 hours per time
    int check_interval;

} mysql_cleaner_config_t;

int start_mysql_cleaner(mysql_cleaner_config_t *mysql_cleaner_config);

#endif