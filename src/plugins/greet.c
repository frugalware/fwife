/*
 *  greet.c for Fwife
 * 
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

#include "common.h"

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

char *desc()
{
	return _("Welcome");
}

plugin_t *info()
{
	return &plugin;
}

GtkWidget *load_gtk_widget()
{
	GtkWidget *widget = gtk_label_new (NULL);
	gtk_label_set_markup(GTK_LABEL(widget), _("<b>Welcome among the users of Frugalware!\n\n</b>\n"
							"The aim of creating Frugalware was to help you to do your work faster and simpler.\n"
							"We hope that you will like our product.\n\n"
        					"<span style=\"italic\" font=\"10\" foreground=\"#0000FF\">The Frugalware Developer Team</span>\n"));
	
	return widget;
}

int run(GList **config)
{
	return 0;
}
