/*
** libzbxpgsql - A PostgreSQL monitoring module for Zabbix
** Copyright (C) 2015 - Ryan Armstrong <ryan@cavaliercoder.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "libzbxpgsql.h"

#define MAX_QUERY_LEN			4096
#define MAX_CLAUSE_LEN          64

#define PGSQL_GET_BACKENDS		"SELECT COUNT(pid) FROM pg_stat_activity"

#define PGSQL_GET_LONGEST_QUERY "\
SELECT \
  EXTRACT(EPOCH FROM NOW()) - EXTRACT(EPOCH FROM query_start) AS duration \
FROM pg_stat_activity \
WHERE state = 'active' \
ORDER BY duration DESC \
LIMIT 1"

/*
 * Custom key pg.backends.count
 *
 * Returns statistics for connected backends (remote clients)
 *
 * Parameter [0-4]:     	<host,port,db,user,passwd>
 *
 * Parameter <database>:	OID or name of the connected database
 *
 * Parameter <user>:		OID or name of the connected user
 *
 * Parameter <application>:	name of the connected application
 *
 * Parameter <client>: 		hostname or IP address of the connected host
 *
 * Parameter <waiting>:     return only waiting backends
 *
 * Parameter <state>:       return only backends in the matching state
 *
 * Parameter <query>:       the SQL query being executed
 *
 * Returns: u
 */
 int    PG_BACKENDS_COUNT(AGENT_REQUEST *request, AGENT_RESULT *result)
 {
    int         ret = SYSINFO_RET_FAIL;                  // Request result code
    const char  *__function_name = "PG_BACKENDS_COUNT";  // Function name for log file
	char        query[MAX_QUERY_LEN];
	char        *p = &query[0];
	int         i = 0;
	char        *param = NULL;
	char        *clause = PG_WHERE;

    zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);

    // Build the sql query
    memset(query, 0, MAX_QUERY_LEN);
    zbx_strlcpy(p, PGSQL_GET_BACKENDS,MAX_QUERY_LEN);
    p += strlen(p);

    // iterate over the available parameters
    for(i = 0; i < 8; i++) {
    	param = get_rparam(request, PARAM_FIRST + i);
    	if(NULL != param && '\0' != *param) {
    		switch(i) {
    			case 0: // <database>
    				if(isdigit(*param))
    					zbx_snprintf(p, MAX_CLAUSE_LEN, " %s datid=%s", clause, param);
    				else
    					zbx_snprintf(p, MAX_CLAUSE_LEN, " %s datname='%s'", clause, param);
    				break;

    			case 1: // <user>
    			    if(isdigit(*param))
    			    	zbx_snprintf(p, MAX_CLAUSE_LEN, " %s usesysid=%s", clause, param);
    				else
    					zbx_snprintf(p, MAX_CLAUSE_LEN, " %s usename='%s'", clause, param);
    				break;

    			case 2: // <application>
    				zbx_snprintf(p, MAX_CLAUSE_LEN, " %s application_name='%s'", clause, param);
    				break;

    			case 3: // <client>
    			    if(isdigit(*param) || ':' == *param)
    			    	zbx_snprintf(p, MAX_CLAUSE_LEN, " %s client_addr = inet '%s'", clause, param);
    				else
    					zbx_snprintf(p, MAX_CLAUSE_LEN, " %s client_hostname='%s'", clause, param);
    				break;

    			case 4: // <waiting>
    				if(0 == strncmp("true", param, 4)) {
                        zbx_snprintf(p, MAX_CLAUSE_LEN, " %s waiting=TRUE", clause);
                    } else if(0 == strncmp("false", param, 5)) {
                        zbx_snprintf(p, MAX_CLAUSE_LEN, " %s waiting=FALSE", clause);
                    } else {
                        zabbix_log(LOG_LEVEL_ERR, "Unsupported 'Waiting' parameter: %s in %s()", param, __function_name);
                        goto out;
                    }
                    
    				break;

    			case 5: // <state>
    				zbx_snprintf(p, MAX_CLAUSE_LEN, " %s state='%s'", clause, param);
    				break;

    			case 6: // <query>
    				zbx_snprintf(p, MAX_CLAUSE_LEN, " %s query='%s'", clause, param);
    				break;
    		}

    		p += strlen(p);
    		clause = PG_AND;
    	}
    }

    ret = pg_get_int(request, result, query);

out:  
    zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
    return ret;
}

/*
 * Custom key pg.queries.longest
 *
 * Returns the duration in seconds of the longest running current query
 *
 * Parameter [0-4]:     <host,port,db,user,passwd>
 *
 * Returns: d
 */
int    PG_QUERIES_LONGEST(AGENT_REQUEST *request, AGENT_RESULT *result)
{
    int         ret = SYSINFO_RET_FAIL;                     // Request result code
    const char  *__function_name = "PG_QUERIES_LONGEST";    // Function name for log file
    
    zabbix_log(LOG_LEVEL_DEBUG, "In %s()", __function_name);
    
    ret = pg_get_dbl(request, result, PGSQL_GET_LONGEST_QUERY);
    
    zabbix_log(LOG_LEVEL_DEBUG, "End of %s()", __function_name);
    return ret;
}