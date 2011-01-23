/*
 *  select-basic.c for Fwife
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"

static GtkWidget *radio_kde;
static GtkWidget *desktop_image;
static GtkWidget *desktop_label;

extern GList *syncs;
extern GList *all_packages;
extern GList *packages_current;
extern GList *cats;

/* Some prototypes from intermediate mode */
void selectcat(char *name, int bool);
void selectallfiles(char *cat, char *exception, int bool);
void selectfile(char *cat, char* name, int bool);

void desktop_changed(GtkWidget *button, gpointer data)
{
	char *image_path = g_strdup_printf("%s/%s.png", IMAGEDIR, (char*)data);

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
		gtk_image_set_from_file(GTK_IMAGE(desktop_image), image_path);

	if(!strcmp((char*)data, "kde4desktop")) {
			gtk_label_set_label(GTK_LABEL(desktop_label),
_("KDE is an international free software community producing \
an integrated set of cross-platform applications designed to run on Linux. \
KDE was formed to create a beautiful, functional and free desktop \
computing environment."));
	} else if(!strcmp((char*)data, "gnomedesktop")) {
		gtk_label_set_label(GTK_LABEL(desktop_label),
_("GNOME is an international project that includes creating software development frameworks, \
selecting application software for the desktop, and working on the programs which manage \
application launching, file handling, and window and task management. \
The GNOME desktop environment is designed to be an intuitive and attractive desktop for users"));
	} else if(!strcmp((char*)data, "xfcedesktop")) {
		gtk_label_set_label(GTK_LABEL(desktop_label),
_("Xfce is a free software desktop environment designed to be fast and lightweight, \
while still being visually appealing and easy to use."));
	} else if(!strcmp((char*)data, "lxdedesktop")) {
		gtk_label_set_label(GTK_LABEL(desktop_label),
_("LXDE is a free software desktop environment designed to work well with computers \
on the low end of the performance spectrum such as older resource-constrained machines, \
new generation netbooks, and other small computers."));
	} else if(!strcmp((char*)data, "e17desktop")) {
		gtk_label_set_label(GTK_LABEL(desktop_label),
_("Enlightenment DR17 (or E17) is an highly flexible and customizable window manager. \
Enlightement is designed to work well with both old and modern computers."));
	}

	free(image_path);
}

GtkWidget *get_basic_mode_widget(void)
{
	GtkWidget *pvboxp = gtk_vbox_new(FALSE,0);
	GtkWidget *pvbox, *hboxdesktop, *hboxtemp;
	GtkWidget *logo;

	GtkWidget *labelenv = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelenv), _("<b>Choose your desktop environment :</b>"));
	gtk_box_pack_start(GTK_BOX(pvboxp), labelenv, FALSE, TRUE, 15);
	hboxdesktop = gtk_hbox_new(FALSE, 5);

	/* Desktop radio button */
	pvbox = gtk_vbox_new(FALSE,2);
	radio_kde = gtk_radio_button_new_with_label(NULL, _("KDE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/kdelogo.png", IMAGEDIR));
	gtk_signal_connect(GTK_OBJECT(radio_kde), "toggled", G_CALLBACK(desktop_changed), (gpointer)"kde4desktop");
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), radio_kde, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *radio_gnome =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_kde), _("GNOME"));
	gtk_signal_connect(GTK_OBJECT(radio_gnome), "toggled", G_CALLBACK(desktop_changed), (gpointer)"gnomedesktop");
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/gnomelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), radio_gnome, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *radio_xfce =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_kde), _("XFCE"));
	gtk_signal_connect(GTK_OBJECT(radio_xfce), "toggled", G_CALLBACK(desktop_changed), (gpointer)"xfcedesktop");
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/xfcelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), radio_xfce, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *radio_lxde =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_kde), _("LXDE"));
	gtk_signal_connect(GTK_OBJECT(radio_lxde), "toggled", G_CALLBACK(desktop_changed), (gpointer)"lxdedesktop");
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/lxdelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), radio_lxde, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *radio_e17 =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_kde), _("E17"));
	gtk_signal_connect(GTK_OBJECT(radio_e17), "toggled", G_CALLBACK(desktop_changed), (gpointer)"e17desktop");
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/e17logo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), radio_e17, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(pvboxp), hboxdesktop, FALSE, TRUE, 15);

	hboxdesktop = gtk_hbox_new(FALSE, 5);

	desktop_image = gtk_image_new_from_file(g_strdup_printf("%s/kde4desktop.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(hboxdesktop), desktop_image, TRUE, TRUE, 5);

	desktop_label = gtk_label_new(_("KDE is an international free software community producing \
an integrated set of cross-platform applications designed to run on Linux. \
KDE was formed to create a beautiful, functional and free desktop \
computing environment."));
	gtk_label_set_line_wrap(GTK_LABEL(desktop_label), TRUE);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), desktop_label, TRUE, TRUE, 5);

	gtk_box_pack_start(GTK_BOX(pvboxp), hboxdesktop, FALSE, TRUE, 10);

	return pvboxp;
}

/* Set up desktop configuration */
void configure_desktop_basic(void)
{
	GSList *pList;
	char *seldesk = NULL;

	char *lang = strdup(getenv("LANG"));
	char *ptr = rindex(lang, '_');
	*ptr = '\0'; ptr = NULL;

	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_kde));

	/* Parcours de la liste */
	while(pList) {
		/* button selected? */
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pList->data))) {
			/* get button label */
			seldesk = strdup((char*)gtk_button_get_label(GTK_BUTTON(pList->data)));
			/* Get out */
			pList = NULL;
		} else {
			/* next button */
			pList = g_slist_next(pList);
		}
	}

	/* Disable lib in basic mode - lib used are just add as dependencies */
	selectcat("lib", 0);
	selectcat("xlib", 0);

	/* Disable devel as well in this mode */
	selectcat("devel", 0);

	/* Disable all default desktop categories */
	selectcat("kde", 0);
	selectcat("gnome", 0);
	selectcat("xfce4", 0);

	if(!strcmp(seldesk, _("KDE")))
	{
		selectcat("kde", 1);
		ptr = g_strdup_printf("koffice-l10n-%s", lang);
		selectfile("locale-extra", ptr, 1);
		free(ptr); ptr = g_strdup_printf("kde-l10n-%s", lang);
		selectfile("locale-extra", ptr, 1);
		selectfile("locale-extra", "k3b-i18n", 1);
	}
	else if(!strcmp(seldesk, _("GNOME")))
	{
		selectcat("gnome", 1);
	}
	else if(!strcmp(seldesk, _("XFCE")))
	{
		selectcat("xfce4", 1);
	}
	else if(!strcmp(seldesk, _("LXDE")))
	{
		selectcat("lxde-desktop", 1);
		selectallfiles("lxde-desktop", NULL, 1);
	}
	else if(!strcmp(seldesk, _("E17")))
	{
		selectcat("e17-extra", 1);
		selectallfiles("e17-extra", NULL, 1);
		selectcat("x11-extra", 1);
		selectfile("x11-extra", "lxdm", 1);
	}

	free(lang);
	return;
}

void run_basic_mode(void)
{
	/* Fix locale dependency */
	char *lang = strdup(getenv("LANG"));
	char *ptr = rindex(lang, '_');
	*ptr = '\0'; ptr = NULL;
	if(strcmp(lang ,"en")) {
		ptr = g_strdup_printf("eric-i18n-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr); ptr = g_strdup_printf("eric4-i18n-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr); ptr = g_strdup_printf("hunspell-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr); ptr = g_strdup_printf("koffice-l10n-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr); ptr = g_strdup_printf("mbrola-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr); ptr = g_strdup_printf("kde-l10n-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr);
	}
	free(lang);
}