/*
 *  select.c for Fwife
 *
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2008, 2009, 2010, 2011 by Albar Boris <boris.a@cegetel.net>
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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pacman.h>

#include "common.h"

extern GtkWidget *assistant;

//* Selected Mode : Expert 2 or Intermediate 1 or Basic 0
static GtkWidget *mode_widget;
static GtkWidget *mode_box;
static int selected_mode = 0;

// need some list to stock to manipulate packages more easily
GList *syncs;
GList *all_packages;
GList *packages_current;
GList *cats;

static char *PACCONF;

/* Expert Mode prototypes */
GtkWidget *get_expert_mode_widget(void);
void run_expert_mode(void);

/* Intermediate Mode prototypes */
GtkWidget *get_intermediate_mode_widget(void);
void run_intermediate_mode(void);
void configure_desktop_intermediate(void);

/* Basic Mode prototypes */
GtkWidget *get_basic_mode_widget(void);
void run_basic_mode(void);
void configure_desktop_basic(void);

plugin_t plugin =
{
	"select",
	desc,
	45,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

plugin_t *info(void)
{
	return &plugin;
}

char *desc(void)
{
	return _("Packages selection");
}

GList* group2pkgs(char *group)
{
	PM_GRP *grp;
	PM_PKG *pkg;
	PM_LIST *pmpkgs, *lp;
	GList *list = NULL;
	int i, optional=0, addpkg=1;
	char *ptr, *pkgname, *pkgfullname, *lang;
	double size;

	// add the core group to the start of the base list
	if(!strcmp(group, "base"))
		list = group2pkgs("core");

	// get language suffix
	lang = strdup(getenv("LANG"));
	ptr = rindex(lang, '_');
	*ptr = '\0';

	if(strlen(group) >= strlen(EXGRPSUFFIX) && !strcmp(group + strlen(group) - strlen(EXGRPSUFFIX), EXGRPSUFFIX))
		optional=1;

	for (i = 0; i < g_list_length(syncs); i++) {
		grp = pacman_db_readgrp(g_list_nth_data(syncs, i), group);
		if(grp) {
			pmpkgs = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
			for(lp = pacman_list_first(pmpkgs); lp; lp = pacman_list_next(lp)) {
				pkg = pacman_db_readpkg(g_list_nth_data(syncs, i), pacman_list_getdata(lp));
				pkgname = pacman_pkg_getinfo(pkg, PM_PKG_NAME);
				pkgfullname = g_strdup_printf("%s-%s", (char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
					(char*)pacman_pkg_getinfo(pkg, PM_PKG_VERSION));

				// enable by default the packages in the
				// frugalware repo + enable the
				// language-specific parts from
				// locale-extra
				addpkg = ((strcmp(getenv("LANG"), "en_US") &&
					!strcmp(group, "locale-extra") &&
					strlen(pkgname) >= strlen(lang) &&
					!strcmp(pkgname + strlen(pkgname) -
					strlen(lang), lang)) || !optional);

				// add the package to the list
				list = g_list_append(list, strdup(pkgname));
				size = (double)(long)pacman_pkg_getinfo(pkg, PM_PKG_SIZE);
				size = (double)(size/1048576.0);
				if(size < 0.1)
					size=0.1;
				list = g_list_append(list, g_strdup_printf("%6.1f MB", size ));
				list = g_list_append(list, strdup(pacman_pkg_getinfo(pkg, PM_PKG_DESC)));
				list = g_list_append(list, GINT_TO_POINTER(addpkg));

				free(pkgfullname);
			}
			break;
		}
	}

	return(list);
}

char* categorysize(char *category)
{
	int i;
	double size=0;
	PM_GRP *grp;
	PM_PKG *pkg;
	PM_LIST *pmpkgs, *lp;

	for (i = 0; i < g_list_length(syncs); i++) {
		grp = pacman_db_readgrp(g_list_nth_data(syncs, i), category);
		if(grp) {
			pmpkgs = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
			for(lp = pacman_list_first(pmpkgs); lp; lp = pacman_list_next(lp)) {
				pkg = pacman_db_readpkg(g_list_nth_data(syncs, i), pacman_list_getdata(lp));
				size += (long)pacman_pkg_getinfo(pkg, PM_PKG_SIZE);
			}
			break;
		}
	}

	size = (double)(size/1048576.0);
	if(size < 0.1)
		size=0.1;

	return(g_strdup_printf("%6.1f MB", size));
}

GList *getcat(PM_DB *db)
{
	char *ptr;
	GList *catlist=NULL;
	PM_LIST *lp;

	for(lp = pacman_db_getgrpcache(db); lp; lp = pacman_list_next(lp))
	{
		PM_GRP *grp = pacman_list_getdata(lp);

		ptr = (char *)pacman_grp_getinfo(grp, PM_GRP_NAME);

			if(!index(ptr, '-') && strcmp(ptr, "core"))
			{
				catlist = g_list_append(catlist, strdup(ptr));
				catlist = g_list_append(catlist, categorysize(ptr));
				catlist = g_list_append(catlist, GINT_TO_POINTER(1));
			}
			else if(strstr(ptr, EXGRPSUFFIX) || !strcmp(ptr, "lxde-desktop"))
			{
				catlist = g_list_append(catlist, strdup(ptr));
				catlist = g_list_append(catlist, categorysize(ptr));
				if(strcmp(ptr, "locale-extra"))
					catlist = g_list_append(catlist, GINT_TO_POINTER(0));
				else
					catlist = g_list_append(catlist, GINT_TO_POINTER(1));
			}
	}

	return catlist;
}

int prepare_pkgdb(char *srcdev)
{
	char *ptr, *pkgdb, *temp;
	int ret = 0;
	FILE *fp;
	PM_DB *db;

	// pacman can't lock & log without these
	ptr = g_strdup_printf("%s/tmp", TARGETDIR);
	makepath(ptr);
	free(ptr);
	ptr = g_strdup_printf("%s/var/log", TARGETDIR);
	makepath(ptr);
	free(ptr);
	pkgdb = g_strdup_printf("%s/var/lib/pacman-g2/%s", TARGETDIR, PACCONF);
	makepath(pkgdb);

	if (pacman_parse_config("/etc/pacman-g2.conf", NULL, "") == -1) {
		LOG("Failed to parse pacman-g2 configuration file (%s)", pacman_strerror(pm_errno));
		return(-1);
	}

	// register the database
	db = pacman_db_register(PACCONF);
	if(db == NULL) {
		LOG("could not register '%s' database (%s)\n",
			PACCONF, pacman_strerror(pm_errno));
		return(-1);
	} else {
		if(srcdev != NULL) {
			temp = g_strdup_printf("%s/frugalware-%s/%s.fdb", SOURCEDIR, ARCH, PACCONF);
			copyfile(temp, pkgdb);
			free(temp);

			if ((fp = fopen("/etc/pacman-g2.conf", "w")) == NULL) {
				LOG("could not open output file '/etc/pacman-g2.conf' for writing: %s", strerror(errno));
				return(1);
			}
			fprintf(fp, "[options]\n");
			fprintf(fp, "LogFile     = /var/log/pacman-g2.log\n");
			fprintf(fp, "MaxTries = 5\n");
			fprintf(fp, "[%s]\n", PACCONF);
			fprintf(fp, "Server = file://%s/frugalware-%s\n\n", SOURCEDIR, ARCH);
			fclose(fp);
		} else {
			LOG("updating the database");
			ret = pacman_db_update(1, db);
		}

		if(ret == 0) {
			LOG("database update done");
		} else if (ret == -1) {
			LOG("database update failed");
			if(pm_errno == PM_ERR_DB_SYNC) {
				LOG("Failed to synchronize %s", PACCONF);
				return(-1);
			} else {
				LOG("Failed to update %s (%s)", PACCONF, pacman_strerror(pm_errno));
				return(-1);
			}
		}
		syncs = g_list_append(syncs, db);
	}

	free(pkgdb);
	return(0);
}

//* Load main widget *//
GtkWidget *load_gtk_widget(void)
{
	mode_box = gtk_vbox_new(FALSE, 5);

	return mode_box;
}

int prerun(GList **config)
{
	int j;
	PM_DB *i;
	GList *pack = NULL;

	GtkWidget* pBoite;
	GtkWidget *progress, *vbox;

	// get the branch used
	PACCONF = data_get(*config, "pacconf");

	//* if previous loaded (previous button used) do nothing *//
	if(syncs == NULL && all_packages == NULL && cats == NULL) {
		pBoite = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
		gtk_window_set_default_size(GTK_WINDOW(pBoite), 320, 70);
		gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_modal(GTK_WINDOW(pBoite), TRUE);
		gtk_window_set_title(GTK_WINDOW(pBoite), _("Loading package's database"));
		gtk_window_set_deletable(GTK_WINDOW(pBoite), FALSE);

		vbox = gtk_vbox_new (FALSE, 5);

		progress = gtk_progress_bar_new();

		gtk_box_pack_start (GTK_BOX (vbox), progress, TRUE, FALSE, 5);

		gtk_container_add(GTK_CONTAINER(pBoite), vbox);

		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), _("Initializing pacman-g2"));
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.0);
		gtk_widget_show_all(pBoite);

		// Initialize pacman
		if(pacman_initialize(TARGETDIR) == -1) {
			LOG("failed to initialize pacman library (%s)\n", pacman_strerror(pm_errno));
			return(1);
		}

#ifndef NDEBUG
		pacman_set_option(PM_OPT_LOGMASK, -1);
#else
		pacman_set_option(PM_OPT_LOGMASK, PM_LOG_ERROR | PM_LOG_WARNING);
#endif
		pacman_set_option(PM_OPT_LOGCB, (long)cb_log);
		if(pacman_set_option(PM_OPT_DBPATH, (long)PM_DBPATH) == -1) {
			LOG("failed to set option DBPATH (%s)\n", pacman_strerror(pm_errno));
			return(1);
		}

		// Register with local database
		i = pacman_db_register("local");
		if(i == NULL) {
			LOG("could not register 'local' database (%s)\n", pacman_strerror(pm_errno));
			return(1);
		} else {
			syncs = g_list_append(syncs, i);
		}

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.3);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), _("Update and load database"));
		while (gtk_events_pending())
			gtk_main_iteration ();

		if(prepare_pkgdb((char*)data_get(*config, "srcdev")) == -1)
			return(-1);

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.5);
		while (gtk_events_pending())
			gtk_main_iteration ();

		// load categories of packages
		cats = getcat(g_list_nth_data(syncs, 1));
		if(cats == NULL) {
			return(-1);
		}
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(progress), 0.8);
		while (gtk_events_pending())
			gtk_main_iteration ();

		// preload package list for each categorie
		for(j=0;j < g_list_length(cats); j+=3) {
			pack = group2pkgs((char*)g_list_nth_data(cats, j));
			data_put(&all_packages, (char*)g_list_nth_data(cats, j), pack);
		}

		gtk_widget_destroy(pBoite);
	}

	/* Destroy the previous widget if their is one */
	if(mode_widget)
		gtk_widget_destroy(mode_widget);

	GtkWidget *label_msgbox = gtk_label_new(
_("Choose your preferred mode.\n\n \
The Basic mode is a mode designed for beginners \
and provide a default installation of Frugalware. \n \
It allows you only to choose between the differents desktop environments \
available in Frugalware. For each one, a short description and a picture is given.\n\n\
The Intermediate mode allows you to choose a desktop environment \
and some extra package groups. \n\
It is designed for intermediate users who wants \
to enable/disable some specifics groups of packages \
but don't want to deal with the Expert mode.\n\n\
The Expert mode gives you a complete control of the packages \
installed since it shows not only package groups but individual packages in each group.\n\
Obviously, you should know what you're doing if you use the Expert mode \
since it's possible to skip packages that are crucial to the functioning \
of your system.\n\n\
Users who wants a non-graphical installation should use the Intermediate or the Expert mode."));
    gtk_label_set_line_wrap(GTK_LABEL(label_msgbox), TRUE);


	GtkWidget *mode_msgbox = gtk_dialog_new_with_buttons(
		_("Package selection"),
		GTK_WINDOW(assistant),
		GTK_DIALOG_MODAL,
		_("Basic Mode"), GTK_RESPONSE_APPLY,
		_("Intermediate Mode"), GTK_RESPONSE_NO,
		_("Expert Mode"), GTK_RESPONSE_YES,
		NULL);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(mode_msgbox)->vbox), label_msgbox, TRUE, FALSE, 0);
	gtk_widget_show_all(GTK_DIALOG(mode_msgbox)->vbox);

	switch(gtk_dialog_run(GTK_DIALOG(mode_msgbox)))
	{
		case GTK_RESPONSE_YES:
			gtk_widget_destroy(mode_msgbox);
			mode_widget = get_expert_mode_widget();
			gtk_box_pack_start(GTK_BOX(mode_box), mode_widget, TRUE, TRUE, 0);
			gtk_widget_show_all(mode_box);
			selected_mode = 2;
			run_expert_mode();
			break;
		case GTK_RESPONSE_NO:
			gtk_widget_destroy(mode_msgbox);
			mode_widget = get_intermediate_mode_widget();
			gtk_box_pack_start(GTK_BOX(mode_box), mode_widget, TRUE, TRUE, 0);
			gtk_widget_show_all(mode_box);
			selected_mode = 1;
			run_intermediate_mode();
			break;
		case GTK_RESPONSE_APPLY:
			gtk_widget_destroy(mode_msgbox);
			mode_widget = get_basic_mode_widget();
			gtk_box_pack_start(GTK_BOX(mode_box), mode_widget, TRUE, TRUE, 0);
			gtk_widget_show_all(mode_box);
			selected_mode = 0;
			run_basic_mode();
			break;
	}

	return(0);
}

int run(GList **config)
{
	int i, j;
	PM_LIST *lp, *junk, *sorted, *x;
	char *ptr;
	GList *allpkgs = NULL;
	long long pkgsize=0, freespace;
	long long *compressedsize = NULL;
	MALLOC(compressedsize, sizeof(long long));
	*compressedsize = 0;

	if(cats == NULL)
		return -1;

	if(selected_mode == 0)
		configure_desktop_basic();
	else if(selected_mode == 1)
		configure_desktop_intermediate();

	//* Check for missing dependency *//
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_NOCONFLICTS, NULL, NULL, NULL) == -1)
		return(-1);

	//* For each checked package, add it in list *//
	for(i = 0; i < g_list_length(cats); i += 3) {
		if(GPOINTER_TO_INT(g_list_nth_data(cats, i+2)) == 1) {
			GList *pkgs = (GList*)data_get(all_packages, (char*)g_list_nth_data(cats, i));
			for(j = 0; j < g_list_length(pkgs); j += 4)	{
				if(GPOINTER_TO_INT(g_list_nth_data(pkgs, j+3)) == 1) {
					ptr = strdup((char*)g_list_nth_data(pkgs, j));
					pacman_trans_addtarget(ptr);
					free(ptr);
				}
			}
		}
	}

	if(pacman_trans_prepare(&junk) == -1) {
		LOG("pacman-g2 error: %s", pacman_strerror(pm_errno));

		// Well well well, LOG pacman deps error at tty4
		for(x = pacman_list_first(junk); x; x = pacman_list_next(x)) {
			PM_DEPMISS *miss = pacman_list_getdata(x);
			LOG(":: %s: %s %s",
				(char*)pacman_dep_getinfo(miss, PM_DEP_TARGET),
				(long)pacman_dep_getinfo(miss, PM_DEP_TYPE) == PM_DEP_TYPE_DEPEND ? "requires" : "is required by",
			    (char*)pacman_dep_getinfo(miss, PM_DEP_NAME));
		}
		pacman_list_free(junk);
		pacman_trans_release();
		return(-1);
	}

	fw_system("ln -sf /proc/self/mounts /etc/mtab");
	freespace = get_freespace();

	sorted = pacman_trans_getinfo(PM_TRANS_PACKAGES);
	for(lp = pacman_list_first(sorted); lp; lp = pacman_list_next(lp)) {
		PM_SYNCPKG *sync = pacman_list_getdata(lp);
		PM_PKG *pkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
		ptr = g_strdup_printf("%s-%s", (char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
				      (char*)pacman_pkg_getinfo(pkg, PM_PKG_VERSION));
		pkgsize += (long)pacman_pkg_getinfo(pkg, PM_PKG_USIZE);
		// remember that packages are on the disk too
		*compressedsize += (long)pacman_pkg_getinfo(pkg, PM_PKG_SIZE);
		allpkgs = g_list_append(allpkgs, ptr);
	}

	pacman_trans_release();
	data_put(config, "compsizepkg", compressedsize);

	if((pkgsize + (*compressedsize)) > freespace) {
		LOG("freespace : %lld, packages space : %lld", freespace, pkgsize);
		fwife_error(_("No enough disk space available to install all packages"));
		g_list_foreach(allpkgs, (GFunc)g_free, NULL);
		g_list_free(allpkgs);
		return 1;
	}

	data_put(config, "packages", allpkgs);

	pacman_release();
	return 0;
}
