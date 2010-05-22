/*
 *  fwife.h for Fwife
 *
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2008, 2009, 2010 by Albar Boris <boris.a@cegetel.net>
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

#ifndef FWIFE_H_INCLUDED
#define FWIFE_H_INCLUDED

#include <glib.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include "util.h"

#define LOGDEV "/dev/tty4"
#define LOGFILE "/var/log/fwife.log"
#define SOURCEDIR "/mnt/source"
#define TARGETDIR "/mnt/target"

#define MKSWAP "/sbin/mkswap"
#define SWAPON "/sbin/swapon"

#define PACCONFPATH "/etc/pacman-g2/repos/"

#define EXGRPSUFFIX "-extra"

#define SHARED_LIB_EXT ".so"

/* Structure of a plugins */

typedef struct {
	char *name;
	char* (*desc)(void);
	int priority;
	GtkWidget* (*load_gtk_widget)(void);
	GtkAssistantPageType type;
	gboolean complete;
	GtkWidget* (*load_help_widget)(void);
	int (*prerun)(GList **config);
	int (*run)(GList **config);
	void *handle;
} plugin_t;


/* A structure for a plugin page */
typedef struct {
	GtkWidget *widget;
	gint index;
	const gchar *title;
	GtkAssistantPageType type;
	gboolean complete;
} PageInfo;

/* Functions to grant/deny next page access */
void set_page_completed(void);
void set_page_incompleted(void);

/* Force fwife to quit */
void fwife_exit(void);

/* Go to next plugin in special case */
int skip_to_next_plugin(void);

#endif /* FWIFE_H_INCLUDED */
