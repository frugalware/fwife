/*
 *  configsource-discs.c for Fwife
 *
 *  Copyright (c) 2011 by Albar Boris <boris.a@cegetel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <blkid/blkid.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>

#include "common.h"

#define BLKGETSIZE64 _IOR(0x12,114,size_t)

GList *extract_drives(char *line)
{
	char *p, *s;
	GList *drives=NULL;

	for (p=line+12;p!='\0';p=strstr(p+1, "\t"))
	{
		s = strdup(p+1);
		drives = g_list_append(drives, s);
		while(!isspace(*s))
			s++;
		*s='\0';
	}
	return(drives);
}

GList *grep_drives(char *file)
{
	FILE *fp;
	char line[PATH_MAX];

	if ((fp = fopen(file, "r")) == NULL) {
		perror(_("Could not open output file for writing"));
		return(NULL);
	}

	while(!feof(fp)) {
		if(fgets(line, 256, fp) == NULL)
			break;
		if(line == strstr(line, "drive name:")) {
			return(extract_drives(line));
		}
	}
	fclose(fp);
	return(NULL);
}

int is_netinstall(char *path)
{
	struct stat statbuf;
	char *ptr;
	int ret;

	ptr = g_strdup_printf("%s/frugalware-%s", path, ARCH);
	if(!stat(ptr, &statbuf)
		&& S_ISDIR(statbuf.st_mode))
		ret = 0;
	else
		ret = 1;

	free(ptr);
	return(ret);
}

static char* get_blkid(char *device)
{
	int fd;
	blkid_probe pr = NULL;
	uint64_t size;
	const char *label;
	char *ret;
	char path[PATH_MAX];

	if(!device || !strlen(device))
		return NULL;

	snprintf(path, PATH_MAX, "/dev/%s", device);

	fd = open(path, O_RDONLY);
	if(fd<0)
		return NULL;
	pr = blkid_new_probe ();
	blkid_probe_set_request (pr, BLKID_PROBREQ_LABEL);
	ioctl(fd, BLKGETSIZE64, &size);
	blkid_probe_set_device (pr, fd, 0, size);
	blkid_do_safeprobe (pr);
	blkid_probe_lookup_value(pr, "LABEL", &label, NULL);
	ret = strdup(label);
	blkid_free_probe (pr);
	close(fd);
	return ret;
}

int run_discs_detection(GList **config)
{
	GList *drives=NULL;
	int i;
	int found = 0;
	char *ptr;

	umount_if_needed(SOURCEDIR);
    makepath(SOURCEDIR);

	drives = grep_drives("/proc/sys/dev/cdrom/info");
	for (i = 0; i < g_list_length(drives); i++) {
		ptr = get_blkid((char*)g_list_nth_data(drives, i));
		if(ptr && !strcmp(ptr, "Frugalware Install")) {
			LOG("install medium found in %s", (char*)g_list_nth_data(drives, i));
			free(ptr);
			ptr = g_strdup_printf("mount -o ro -t iso9660 /dev/%s %s",
				(char*)g_list_nth_data(drives, i),
				SOURCEDIR);
			fw_system(ptr);
			free(ptr);

			if(!is_netinstall(SOURCEDIR)) {
				data_put(config, "srcdev", (char*)g_list_nth_data(drives, i));
				found = 1;
				break;
			}
		} else {
			LOG("skipping non-install medium in %s", (char*)g_list_nth_data(drives, i));
        }
	}

	return found;
}

int run_discs_config(GList **config)
{
	int ret = fwife_question(_("Do you want to search for a installation CD/DVD?\n\n"
			"If you want an installation from a CD/DVD, you should answer 'Yes'.\n"
			"Otherwise if you want a network installation, you should answer 'No'"));

	/* if the user whant a cd/dvd install */
	if(ret == GTK_RESPONSE_YES) {
		if(run_discs_detection(config)) {
			char *pacbindir = g_strdup_printf("%s/frugalware-%s", SOURCEDIR, ARCH);
			disable_cache(pacbindir);
			FREE(pacbindir);
			skip_to_next_plugin();
            return 1;
		} else {
			fwife_error("No package database found.\nPerforming a network installation.");
		}
	}

	return 0;
}
