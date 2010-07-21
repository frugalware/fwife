/*
 *  greet.c for Fwife
 *
 *  Copyright (c) 2008,2009,2010 by Albar Boris <boris.a@cegetel.net>
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
#include <gtk/gtk.h>
#include <pacman.h>

#include "common.h"

static char *PACCONF = NULL;

plugin_t plugin =
{
	"greet",
	desc,
	10,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_INTRO,
	TRUE,
	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Welcome");
}

plugin_t *info(void)
{
	return &plugin;
}

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *widget = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL(widget), _("<b>Welcome among the users of Frugalware!\n\n</b>\n"
				"The aim of creating Frugalware was to help you to do your work faster and simpler.\n"
				"We hope that you will like our product.\n\n"
				"<span style=\"italic\" foreground=\"#0000FF\">The Frugalware Developer Team</span>\n"));

	return widget;
}

void cb_db_register(const char *section, PM_DB *db)
{
	// the first repo find is used
	if(!strcmp(section, "frugalware-current") || !strcmp(section, "frugalware"))
		PACCONF = strdup(section);
}

int run(GList **config)
{
	// find the fw branch that'll be used
	pacman_release();
	if(pacman_initialize("/") == -1)
		return -1;

	if (pacman_parse_config("/etc/pacman-g2.conf", cb_db_register, "") == -1) {
		LOG("Failed to parse pacman-g2 configuration file (%s)", pacman_strerror(pm_errno));
		return(-1);
	}

	pacman_release();

	if(PACCONF == NULL) {
		LOG("No usable pacman-g2 database for installation");
		return -1;
	}
	data_put(config, "pacconf", PACCONF);

	return 0;
}
