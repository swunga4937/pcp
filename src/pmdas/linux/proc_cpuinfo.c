/*
 * Linux /proc/cpuinfo metrics cluster
 *
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.  All Rights Reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (c) 2001 Gilly Ran (gilly@exanet.com) for the
 * portions of the code supporting the Alpha platform.
 * All rights reserved.
 */

#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "pmapi.h"
#include "impl.h"
#include "pmda.h"

#include "proc_cpuinfo.h"

char *
cpu_name(proc_cpuinfo_t *proc_cpuinfo, int c)
{
    char name[1024];
    char *p;
    FILE *f;
    static int started = 0;

    if (!started) {
	refresh_proc_cpuinfo(proc_cpuinfo);

	proc_cpuinfo->machine = NULL;
	if ((f = fopen("/proc/sgi_prominfo/node0/version", "r")) != NULL) {
	    while (fgets(name, sizeof(name), f)) {
		if (strncmp(name, "SGI", 3) == 0) {
		    if ((p = strstr(name, " IP")) != NULL)
			proc_cpuinfo->machine = strndup(p+1, 4);
		    break;
		}
	    }
	    fclose(f);
	}
	if (proc_cpuinfo->machine == NULL)
	    proc_cpuinfo->machine = strdup("linux");

	started = 1;
    }

    snprintf(name, sizeof(name), "cpu%d", c);
    return strdup(name);
}

int
refresh_proc_cpuinfo(proc_cpuinfo_t *proc_cpuinfo)
{
    char buf[4096];
    FILE *fp;
    int cpunum;
    cpuinfo_t *info;
    char *val;
    char *p;
    static int started = 0;

    if (!started) {
	int need;
	if (proc_cpuinfo->cpuindom == NULL || proc_cpuinfo->cpuindom->it_numinst == 0)
	    abort();
	need = proc_cpuinfo->cpuindom->it_numinst * sizeof(cpuinfo_t);
	proc_cpuinfo->cpuinfo = (cpuinfo_t *)malloc(need);
	memset(proc_cpuinfo->cpuinfo, 0, need);
    	started = 1;
    }

    if ((fp = fopen("/proc/cpuinfo", "r")) == (FILE *)NULL)
    	return -errno;

#if defined(HAVE_ALPHA_LINUX)
    cpunum = 0;
#else	//intel
    cpunum = -1;
#endif
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	if ((val = strrchr(buf, '\n')) != NULL)
	    *val = '\0';
	if ((val = strchr(buf, ':')) == NULL)
	    continue;
	val += 2;

#if !defined(HAVE_ALPHA_LINUX)
	if (strncmp(buf, "processor", 9) == 0) {
	    cpunum++;
	    proc_cpuinfo->cpuinfo[cpunum].cpu_num = atoi(val);
	    continue;
	}
#endif

	info = &proc_cpuinfo->cpuinfo[cpunum];

	if (info->sapic == NULL && strncasecmp(buf, "sapic", 5) == 0)
	    info->sapic = strdup(val);
	if (info->model == NULL && strncasecmp(buf, "model name", 10) == 0)
	    info->model = strdup(val);
	if (info->model == NULL && strncasecmp(buf, "model", 5) == 0)
	    info->model = strdup(val);
	if (info->model == NULL && strncasecmp(buf, "cpu model", 9) == 0)
	    info->model = strdup(val);
	if (info->vendor == NULL && strncasecmp(buf, "vendor", 6) == 0)
	    info->vendor = strdup(val);
	if (info->stepping == NULL && strncasecmp(buf, "step", 4) == 0)
	    info->stepping = strdup(val);
	if (info->stepping == NULL && strncasecmp(buf, "revision", 8) == 0)
	    info->stepping = strdup(val);
	if (info->stepping == NULL && strncasecmp(buf, "cpu revision", 12) == 0)
	    info->stepping = strdup(val);
	if (info->clock == 0.0 && strncasecmp(buf, "cpu MHz", 7) == 0)
	    info->clock = atof(val);
	if (info->clock == 0.0 && strncasecmp(buf, "cycle frequency", 15) == 0) {
	    if ((p = strchr(val, ' ')) != NULL)
		*p = '\0';
	    info->clock = (atof(val))/1000000;
	}
	if (info->cache == 0 && strncasecmp(buf, "cache", 5) == 0)
	    info->cache = atoi(val);
	if (info->bogomips == 0.0 && strncasecmp(buf, "bogo", 4) == 0)
	    info->bogomips = atof(val);
	if (info->bogomips == 0.0 && strncasecmp(buf, "BogoMIPS", 8) == 0)
	    info->bogomips = atof(val);
    }
    fclose(fp);

#if defined(HAVE_ALPHA_LINUX)
    /* all processors are identical, therefore duplicate it to all the instances */
    for (cpunum=1; cpunum < proc_cpuinfo->cpuindom->it_numinst; cpunum++)
	memcpy(&proc_cpuinfo->cpuinfo[cpunum], info, sizeof(cpuinfo_t));
#endif

    /* success */
    return 0;
}
