/*
 *  finish.c for Fwife
 *
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2008, 2009 by Albar Boris <boris.a@cegetel.net>
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
#include <string.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwmouseconfig.h>
#include <pacman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <libintl.h>
#include <unistd.h>

#include "common.h"

plugin_t plugin =
{
	"Completed",
	desc,
	100,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONFIRM,
	TRUE,
	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Installation completed");
}

plugin_t *info(void)
{
	return &plugin;
}

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *widget = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL(widget), _("<b>Installation completed! We hope that you enjoy Frugalware!</b>"));

	return widget;
}

int run(GList **config)
{
	return 0;
}
