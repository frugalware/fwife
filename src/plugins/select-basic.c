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

	/* Disable devel as well in this mode */
	selectcat("devel", 0);

	/* Disable some xapps as well */
	selectallfiles("xapps", NULL, 0);
	selectallfiles("apps", NULL, 0);
	selectallfiles("xlib", NULL, 0);

	selectfile("xapps", "dvdauthor", 1);
	selectfile("xapps", "emacs", 1);
	selectfile("xapps", "gfpm", 1);
	selectfile("xapps", "gnetconfig", 1);
	selectfile("xapps", "gnokii", 1);
	selectfile("xapps", "gvfs", 1);
	selectfile("xapps", "icedtea-web", 1);
	selectfile("xapps", "libreoffice", 1);
	selectfile("xapps", "vte", 1);
	selectfile("xapps", "system-config-printer", 1);
	selectfile("xapps", "startup-notification", 1);
	selectfile("xapps", "plymouth", 1);
	selectfile("xapps", "xpdf", 1);
	selectfile("xapps", "udisks", 1);

	selectfile("apps", "bash-completion", 1);
	selectfile("apps", "bc", 1);
	selectfile("apps", "bluez", 1);
	selectfile("apps", "cdrtools", 1);
	selectfile("apps", "sudo", 1);
	selectfile("apps", "irqbalance", 1);
	selectfile("apps", "logrotate", 1);
	selectfile("apps", "man-pages", 1);
	selectfile("apps", "nano", 1);
	selectfile("apps", "pm-utils", 1);
	selectfile("apps", "pmount", 1);
	selectfile("apps", "recode", 1);
	selectfile("apps", "unarj", 1);
	selectfile("apps", "unzip", 1);
	selectfile("apps", "unarj", 1);
	selectfile("apps", "vim", 1);
	selectfile("apps", "upower", 1);
	selectfile("apps", "memtest86+", 1);

	selectfile("network", "apache", 0);
	selectfile("network", "samba", 0);
	selectfile("network", "procmail", 0);
	selectfile("network", "postfix", 0);

	selectfile("xlib", "flashplugin", 1);

	if(strcmp(lang ,"en")) {
		ptr = g_strdup_printf("firefox-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr); ptr = g_strdup_printf("thunderbird-%s", lang);
		selectfile("locale-extra", ptr, 0);
		free(ptr);
	}

	/* Add Frugalware-Tweaks */
	selectcat("xapps-extra", 1);
	selectfile("xapps-extra", "frugalware-tweak", 1);

	/* Disable all default desktop categories */
	selectcat("kde", 0);
	selectallfiles("gnome", NULL, 0);
	selectcat("xfce4", 0);

	if(!strcmp(seldesk, _("KDE")))
	{
		selectcat("kde", 1);
		ptr = g_strdup_printf("koffice-l10n-%s", lang);
		selectfile("locale-extra", ptr, 1);
		free(ptr); ptr = g_strdup_printf("kde-l10n-%s", lang);
		selectfile("locale-extra", ptr, 1);
		selectfile("locale-extra", "k3b-i18n", 1);

		selectcat("kde-extra", 1);
		selectfile("kde-extra", "rekonq", 1);
		selectfile("kde-extra", "krita", 1);
		selectfile("kde-extra", "kmymoney2", 1);
		selectfile("apps", "attica", 1);
		selectfile("xapps", "akonadi", 1);
		selectfile("xapps-extra", "qtparted", 1);
	}
	else if(!strcmp(seldesk, _("GNOME")))
	{
		selectallfiles("gnome", NULL, 1);
		selectcat("gnome-extra", 1);
		selectfile("gnome-extra", "epiphany", 1);
		selectfile("gnome-extra", "empathy", 1);
		selectfile("gnome-extra", "gnucash", 1);
		selectfile("xapps-extra", "transmission", 1);
		selectfile("xapps", "gimp", 1);
		selectfile("xapps", "gftp", 1);
		selectfile("xapps", "xchat", 1);
		selectfile("xmultimedia", "phonon", 0);
		selectfile("xapps-extra", "gparted", 1);
	}
	else if(!strcmp(seldesk, _("XFCE")))
	{
		selectcat("xfce4", 1);
		selectcat("xfce4-extra", 1);
		selectfile("xfce4-extra", "squeeze", 1);
		selectfile("xfce4-extra", "eatmonkey", 1);
		selectfile("xfce4-extra", "parole", 1);
		selectfile("xfce4-extra", "xfburn", 1);
		selectfile("xfce4-extra", "xfce4-notifyd", 1);
		selectfile("xfce4-extra", "xfce4-power-manager", 1);
		selectfile("xfce4-extra", "xfce4-volumed", 1);
		selectfile("xfce4-extra", "thunar-thumbnailers", 1);
		selectfile("xapps-extra", "midori", 1);
		selectfile("xapps-extra", "xarchiver", 1);
		selectfile("xapps-extra", "claws-mail", 1);
		selectfile("xapps", "gimp", 1);
		selectfile("xapps", "gftp", 1);
		selectfile("xapps", "pidgin", 1);
		selectfile("gnome", "gksu-frugalware", 1);
		selectfile("xapps", "xchat", 1);
		selectfile("xmultimedia", "phonon", 0);
		selectfile("xapps-extra", "epdfview", 1);
		selectfile("xapps-extra", "gparted", 1);
		selectfile("xapps-extra", "grisbi", 1);
	}
	else if(!strcmp(seldesk, _("LXDE")))
	{
		selectcat("lxde-desktop", 1);
		selectallfiles("lxde-desktop", NULL, 1);
		selectfile("xapps-extra", "midori", 1);
		selectfile("xapps-extra", "leafpad", 1);
		selectfile("xapps-extra", "claws-mail", 1);
		selectfile("xapps-extra", "xarchiver", 1);
		selectfile("xapps", "gimp", 1);
		selectfile("xapps", "gftp", 1);
		selectfile("xapps", "pidgin", 1);
		selectfile("gnome", "gksu-frugalware", 1);
		selectfile("xapps", "xchat", 1);
		selectfile("xmultimedia", "phonon", 0);
		selectfile("xapps-extra", "epdfview", 1);
		selectfile("xapps-extra", "gparted", 1);
		selectfile("xapps-extra", "grisbi", 1);
		selectfile("xapps-extra", "recorder", 1);
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
