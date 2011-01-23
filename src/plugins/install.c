/*
 *  install.c for Fwife
 *
 *  Copyright (c) 2008,2009,2010,2011 by Albar Boris <boris.a@cegetel.net>
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

#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pacman.h>

#include "common.h"

static GtkWidget *progress;
static GtkWidget *labelpkg;
static GtkWidget *dlinfo;
static long long compressedsize = 0;

float rate;
int offset;
int howmany;
int remains;
char reponame[PM_DLFNM_LEN+1];
int xferred1;
struct timeval t0, t;
int oldsize = 0;
int dlsize = 0;

plugin_t plugin =
{
	"install",
	desc,
	50,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_PROGRESS,
	FALSE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Installing packages");
}

plugin_t *info(void)
{
	return &plugin;
}

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *vbox;
	vbox = gtk_vbox_new (FALSE, 5);
	labelpkg = gtk_label_new("");
	progress = gtk_progress_bar_new();
	dlinfo = gtk_label_new("");
	gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), dlinfo, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), labelpkg, FALSE, FALSE, 5);
	return vbox;
}

void progress_conv(unsigned char event, void *data1, void *data2, void *data3, int *response)
{
	switch(event) {
		case PM_TRANS_CONV_CORRUPTED_PKG:
			*response = 1;
			break;
		default:
			break;
	}
}

int progress_update(PM_NETBUF *ctl, int xferred, void *arg)
{
	int size;
	float showedsize, totalsize;
	int per;
	char *name = NULL, *ptr = NULL, *dlinfostr = NULL;
	char rate_text[10];
	struct timeval t1;
	float  tdiff;

	while (gtk_events_pending())
		gtk_main_iteration ();

	ctl = NULL;
	size = *(int*)arg;
	if(oldsize == 0)
		oldsize = size;
	if(oldsize != size) {
		dlsize += oldsize;
		oldsize = size;
	}
	showedsize = ((float)((dlsize + (xferred+offset))/1024))/1024;
	totalsize = ((float)(compressedsize/1024))/1024;
	per = ((float)(xferred+offset) / size) * 100;
	gettimeofday (&t1, NULL);

	if (xferred+offset == size)
		t = t0;

	tdiff = t1.tv_sec-t.tv_sec + (float)(t1.tv_usec-t.tv_usec) / 1000000;
	if (xferred+offset == size) {
		rate = xferred / (tdiff * 1024);
	} else if (tdiff > 1) {
		rate = (xferred - xferred1) / (tdiff * 1024);
		xferred1 = xferred;
		gettimeofday (&t, NULL);
	}
	if(rate > 1000) {
		sprintf (rate_text, "%6.0fK/s", rate);
	} else {
		sprintf (rate_text, "%6.1fK/s", (rate>0)?rate:0);
	}

	name = strdup(reponame);
	ptr = g_strdup_printf(_("Downloading %s... (%d/%d)"), drop_version(name), remains, howmany);
	dlinfostr = g_strdup_printf("Download Infos : %.1fMo / %.1fMo At %s", showedsize, totalsize, rate_text);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), ptr);
	gtk_label_set_label(GTK_LABEL(dlinfo), dlinfostr);

	if(per>=0 && per <=100)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), (float)per/100);

	free(ptr);
	free(dlinfostr);
	free(name);

	while (gtk_events_pending())
		gtk_main_iteration ();

	return 1;
}

void progress_install(unsigned char event, char *pkgname, int percent, int howmany, int remain)
{
	char *main_text = NULL;

	if (!pkgname)
		return;
	if (percent < 0 || percent > 100)
		return;
	while (gtk_events_pending())
		gtk_main_iteration ();

	switch (event)
	{
		case PM_TRANS_PROGRESS_ADD_START:
			main_text = g_strdup_printf(_("Installing packages... (%d/%d)"),remain, howmany);
			break;
		case PM_TRANS_PROGRESS_UPGRADE_START:
			main_text = g_strdup (_("Upgrading packages..."));
			break;
		default:
			return;
	}
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), main_text);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), (float)percent/100);
	free(main_text);

	while (gtk_events_pending())
		gtk_main_iteration ();

	return;
}

void progress_event(unsigned char event, void *data1, void *data2)
{
	char *substr = NULL, *ptr = NULL;

	switch (event)
	{
		case PM_TRANS_EVT_ADD_START:
			if (data1 == NULL)
				break;
			substr = g_strdup_printf (_("Installing %s-%s"),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_NAME),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
			break;
		case PM_TRANS_EVT_ADD_DONE:
			if (data1 == NULL)
				break;
			substr = g_strdup_printf (_("Packet %s-%s installed"),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_NAME),
					(char*)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
			break;
		case PM_TRANS_EVT_RETRIEVE_START:
			if (data1 == NULL)
				break;
			substr = g_strdup_printf (_("Retrieving packages from %s"), (char*)data1);
			break;
		case PM_TRANS_EVT_RETRIEVE_LOCAL:
			if (data1 == NULL)
				break;
			ptr = g_strdup_printf(_("Retrieving packages from local... (%d/%d) : %s" ),remains, howmany, drop_version((char*)data1));
			gtk_progress_bar_set_text (GTK_PROGRESS_BAR(progress), ptr);
			while (gtk_events_pending())
				gtk_main_iteration ();
			free(ptr);
			substr = NULL;
			break;
		case PM_TRANS_EVT_INTEGRITY_START:
			gtk_label_set_label(GTK_LABEL(dlinfo), "");
			substr = g_strdup (_("Checking package integrity..."));
			break;
        case PM_TRANS_EVT_INTEGRITY_DONE:
            substr = g_strdup (_("Done"));
            break;
		default:
			return;
	}

	if(substr != NULL) {
		gtk_label_set_label(GTK_LABEL(labelpkg), substr);
		free(substr);
	}

	while (gtk_events_pending())
		gtk_main_iteration ();

	return;
}

int install_pkgs(GList *pkgs)
{
	int i = 0, questret;
	PM_LIST *pdata = NULL, *pkgsl;
	char *ptr, *file;

	/* nothing to install */
	if(pkgs == NULL)
		return 0;

	if(pacman_initialize(TARGETDIR) == -1) {
		LOG("Failed to initialize pacman library (%s)\n", pacman_strerror(pm_errno));
		return -1;
	}

	if (pacman_parse_config("/etc/pacman-g2.conf", NULL, "") == -1) {
			LOG("Failed to parse pacman-g2 configuration file (%s)", pacman_strerror(pm_errno));
			pacman_release();
			return(-1);
	}

	//* Set pacman options *//
#ifndef NDEBUG
	pacman_set_option(PM_OPT_LOGMASK, -1);
#else
	pacman_set_option(PM_OPT_LOGMASK, PM_LOG_ERROR | PM_LOG_WARNING);
#endif
	pacman_set_option(PM_OPT_LOGCB, (long)cb_log);
	pacman_set_option (PM_OPT_DLCB, (long)progress_update);
	pacman_set_option (PM_OPT_DLOFFSET, (long)&offset);
	pacman_set_option (PM_OPT_DLRATE, (long)&rate);
	pacman_set_option (PM_OPT_DLFNM, (long)reponame);
	pacman_set_option (PM_OPT_DLHOWMANY, (long)&howmany);
	pacman_set_option (PM_OPT_DLREMAIN, (long)&remains);
	pacman_set_option (PM_OPT_DLT0, (long)&t0);
	pacman_set_option (PM_OPT_DLT0, (long)&t);
	pacman_set_option (PM_OPT_DLXFERED1, (long)&xferred1);

	PM_DB *db_local = pacman_db_register("local");
	if(db_local == NULL) {
		LOG("Could not register 'local' database (%s)\n", pacman_strerror(pm_errno));
		pacman_release();
		return -1;
	}

retry:	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_FORCE|PM_TRANS_FLAG_NODEPS, progress_event, progress_conv, progress_install) == -1) {
		if (pm_errno == PM_ERR_HANDLE_LOCK) {
			file = g_strdup_printf("%s/tmp/pacman-g2.lck", TARGETDIR);
			g_remove (file);
			free(file);
			goto retry;
		}
		LOG("Failed to initialize transaction %s\n", pacman_strerror (pm_errno));
		pacman_release();
		return -1;
	}

	for (i = 0; i<g_list_length(pkgs); i++) {
		ptr = strdup((char*)g_list_nth_data(pkgs, i));
		if(pacman_trans_addtarget(strdup(drop_version(ptr))) == -1)
			LOG("Error adding packet %s", pacman_strerror (pm_errno));
		free(ptr);
	}

	//* prepare transaction *//
	if(pacman_trans_prepare(&pdata) == -1) {
		LOG("Failed to prepare transaction (%s)", pacman_strerror (pm_errno));
		pacman_list_free(pdata);
		pacman_trans_release();
		pacman_release();
		return -1;
	}

	pkgsl = pacman_trans_getinfo (PM_TRANS_PACKAGES);
	if (pkgsl == NULL) {
		LOG("Error getting transaction info %s\n", pacman_strerror (pm_errno));
		pacman_trans_release();
		pacman_release();
		return -1;
	}

	/* commit transaction */
	if (pacman_trans_commit(&pdata) == -1) {
		switch(pm_errno) {
			case PM_ERR_DISK_FULL:
				fwife_error(_("Disk full, cannot install more packages"));
				break;
			case PM_ERR_PKG_CORRUPTED:
				questret = fwife_question(_("Some packages seems corrupted, do you want to download them again?"));
				if(questret == GTK_RESPONSE_YES) {
					pacman_list_free(pdata);
					pacman_trans_release();
					goto retry;
				}
				break;
			case PM_ERR_RETRIEVE:
				fwife_error(_("Failed to retrieve packages"));
				break;
			default:
				break;
		}

		LOG("Failed to commit transaction (%s)\n", pacman_strerror (pm_errno));
		pacman_list_free(pdata);
		pacman_trans_release();
		pacman_release();
		return -1;
	}

	/* release the transaction */
	pacman_trans_release();
	pacman_release();

	return 0;
}

int install_pkgs_discs(GList *pkgs, char *srcdev)
{
	int i, first = 1, ret;

	while(pkgs) {
		GList *list = NULL;
		struct stat buf;
		char *ptr;

		if(first) {
			// for the first time the volume is already loaded
			first = 0;
		} else {
			eject(srcdev, SOURCEDIR);
			ret = fwife_question(_("Please insert the next Frugalware install disc and press "
								"ENTER to continue installing packages. If you don't "
								"have more discs, choose NO, and then you can finish  "
								"the installation. Have you inserted the next disc?"));
			if(ret == GTK_RESPONSE_NO)
				return 0;

            ptr = g_strdup_printf("mount -o ro -t iso9660 /dev/%s %s",
				srcdev,
				SOURCEDIR);
			fw_system(ptr);
            free(ptr);
        }

		// see what packages can be useful from this volume
		for(i = 0; i < g_list_length(pkgs); i++) {
			ptr = g_strdup_printf("%s/frugalware-%s/%s-%s.fpm", SOURCEDIR, ARCH,
				(char*)g_list_nth_data(pkgs, i), ARCH);
			if(!stat(ptr, &buf))
				list = g_list_append(list, strdup((char*)g_list_nth_data(pkgs, i)));
			free(ptr);
		}

		// remove them from the full list
		for(i = 0; i < g_list_length(list); i++)
			pkgs = g_list_strremove(pkgs, (char*)g_list_nth_data(list, i));

		// install them
		if(install_pkgs(list) == -1)
			return -1;
	}

	return 0;
}

int prerun(GList **config)
{
	// fix gtk graphical bug : forward button is clicked in
	set_page_completed();
	set_page_incompleted();
	long long *compsize = (long long*)data_get(*config,"compsizepkg");
	if(compsize != NULL)
		compressedsize = *compsize;

	makepath(TARGETDIR "/dev");
	fw_system("mount /dev -o bind " TARGETDIR "/dev");

	if(data_get(*config, "srcdev") != NULL) {
		if(install_pkgs_discs((GList*)data_get(*config, "packages"), (char*)data_get(*config, "srcdev")) == -1) {
			fwife_error(_("An error occurs during packages installation (see /var/log/fwife.log for more details)"));
			return -1;
		}
	} else {
		if(install_pkgs((GList*)data_get(*config, "packages")) == -1) {
			fwife_error(_("An error occurs during packages installation (see /var/log/fwife.log for more details)"));
			return -1;
		}
	}

	gtk_label_set_label(GTK_LABEL(labelpkg), _("Packages installation completed"));
	set_page_completed();
	return 0;
}

int run(GList **config)
{
	char *ptr;
	//mount system point to targetdir
	ptr = g_strdup_printf("mount /proc -o bind %s/proc", TARGETDIR);
	fw_system(ptr);
	free(ptr);
	ptr = g_strdup_printf("mount /sys -o bind %s/sys", TARGETDIR);
	fw_system(ptr);
	free(ptr);

	return 0;
}
