/*
 *  grubconf.c for Fwife
 *
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <glib.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libfwgrubconfig.h>
#include <libfwutil.h>

#include "common.h"

static GtkWidget *pRadio1 = NULL;

plugin_t plugin =
{
	"grubconfig",
	desc,
	52,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	load_help_widget,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Configuring GRUB");
}

plugin_t *info(void)
{
	return &plugin;
}

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *pVBox = gtk_vbox_new(FALSE, 5);
	GtkWidget *pLabelInfo=gtk_label_new(NULL);

	/* top info label */
	gtk_label_set_markup(GTK_LABEL(pLabelInfo), _("<b>Installing GRUB bootloader</b>"));
	gtk_box_pack_start(GTK_BOX(pVBox), pLabelInfo, FALSE, FALSE, 6);

	GtkWidget *pLabel = gtk_label_new(_("Choose install type :"));
	gtk_box_pack_start(GTK_BOX(pVBox), pLabel, FALSE, FALSE, 5);

	/* Set up radio buttons */
	pRadio1 = gtk_radio_button_new_with_label(NULL, _("MBR  -  The Master Boot Record of your first hard drive"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio1, FALSE, FALSE, 0);

	GtkWidget *pRadio2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("MBR  -  The Master Boot Record of your root hard drive"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio2, FALSE, FALSE, 0);

	GtkWidget *pRadio3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadio1), _("Skip  -  Skip the installation of GRUB"));
	gtk_box_pack_start(GTK_BOX (pVBox), pRadio3, FALSE, FALSE, 0);

	return pVBox;
}

int run(GList **config)
{
	GSList *pList;
	const char *sLabel = "";
	int needrelease, ret;
	enum fwgrub_install_mode mode;

	/* get button list */
	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(pRadio1));

	while(pList)
	{
		/* selected? */
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data)))
		{
			/* yes -> get label */
			sLabel = gtk_button_get_label(GTK_BUTTON(pList->data));
			/* get out */
			pList = NULL;
		}
		else
		{
			/* no -> next button */
			pList = g_slist_next(pList);
		}
	}

	if(!strcmp(sLabel, _("MBR  -  The Master Boot Record of your first hard drive")))
		mode = FWGRUB_INSTALL_MBR_FIRST;
	else if(!strcmp(sLabel, _("MBR  -  The Master Boot Record of your root hard drive")))
		mode = FWGRUB_INSTALL_MBR_ROOT;
	else
		mode = -1;

	pid_t pid = fork();

	if(pid == -1)
		LOG("Error when fork process in grubconf plugin.");
	else if(pid == 0)
	{
		chroot(TARGETDIR);
		//* Install grub and menu *//
		if(mode != -1)
		{
			needrelease = fwutil_init();
			fwgrub_install(mode);
			fwgrub_make_config();
			if(needrelease)
				fwutil_release();
		}
		exit(0);
	}
	else
	{
		wait(&ret);
	}

	return 0;
}

GtkWidget *load_help_widget(void)
{
	GtkWidget* help = gtk_label_new(_("GRUB can be installed to a variety of places:\n\n\t1. The Master Boot Record of your first hard drive.\n\t2. The Master Boot Record of your root hard drive."));

	return help;
}
