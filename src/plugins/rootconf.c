/*
 *  rootconf.c for Fwife
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

#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>

#include "common.h"

static GtkWidget *rootpass, *rootverify;

plugin_t plugin =
{
	"rootconf",
	desc,
	54,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
 	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Configure root password");
}

plugin_t *info(void)
{
	return &plugin;
}

//* Verify if password are correct *//
void verify_password(GtkWidget *widget, gpointer data)
{
	char *pass1 = (char*)gtk_entry_get_text(GTK_ENTRY(widget));
	char *pass2 = (char*)gtk_entry_get_text(GTK_ENTRY(data));
	if(!strcmp(pass1, pass2))
		set_page_completed();
	else
		set_page_incompleted();
}

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *vboxp, *infolabl;
	//* For root entry end of page *//
	GtkWidget *hboxroot1, *hboxroot2;
	GtkWidget *rootlabel, *verifylabel;

	// main vbox
	vboxp = gtk_vbox_new(FALSE, 5);
	infolabl = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(infolabl), GTK_JUSTIFY_CENTER);
	gtk_label_set_markup(GTK_LABEL(infolabl), _("<b>You can set password on the system administrator account (root).</b>\n\
It is recommended that you set one now so that it is active the first time the machine is rebooted.\n\
This is especially important if your machine is on an internet connected LAN.\n\
If you don't want to set one, keep entry empty and go to the next step."));
	gtk_box_pack_start (GTK_BOX (vboxp), infolabl, FALSE, FALSE, 10);

	hboxroot1 = gtk_hbox_new(FALSE, 10);
	hboxroot2 = gtk_hbox_new(FALSE, 10);
	rootlabel = gtk_label_new(_("Enter root password :  "));
	verifylabel = gtk_label_new(_("Verify root password :  "));

	rootpass = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(rootpass), FALSE);
	rootverify = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(rootverify), FALSE);
	g_signal_connect (rootverify, "changed", G_CALLBACK (verify_password), rootpass);
	g_signal_connect (rootpass, "changed", G_CALLBACK (verify_password), rootverify);

	gtk_box_pack_start (GTK_BOX (hboxroot1), rootlabel, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hboxroot2), verifylabel, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hboxroot1), rootpass, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hboxroot2), rootverify, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (vboxp), hboxroot1, FALSE, FALSE, 20);
	gtk_box_pack_start (GTK_BOX (vboxp), hboxroot2, FALSE, FALSE, 5);

	return vboxp;
}

int run(GList **config)
{
	char *ptr = NULL, *pass;
	int ret;

	//* Set root password *//
	pass = strdup((char*)gtk_entry_get_text(GTK_ENTRY(rootpass)));
	if(strlen(pass)) {
		ptr = g_strdup_printf("echo '%s:%s' |chroot %s /usr/sbin/chpasswd", "root", pass, TARGETDIR);
		fw_system(ptr);
		free(ptr);
	} else {
		ret = fwife_question(_("The root password is empty. This is highly unsecure!\n\nDo you want to continue anyway ?"));
		if(ret == GTK_RESPONSE_NO)
			return 1;
    }

	free(pass);
	return 0;
}
