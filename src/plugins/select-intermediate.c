/*
 *  select-intermediate.c for Fwife
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

static GtkWidget *pRadioKDE;

extern GList *syncs;
extern GList *all_packages;
extern GList *packages_current;
extern GList *cats;

//* select a categorie *//
void selectcat(char *name, int bool)
{
	int i;
	for(i=0; i<g_list_length(cats); i+=3)
	{
		if(!strcmp(g_list_nth_data(cats, i), name))
		{
			g_list_nth(cats, i+2)->data = GINT_TO_POINTER(bool);
			return;
		}
	}
}

//* Select all file in a categorie *//
void selectallfiles(char *cat, char *exception, int bool)
{
	int i;
	GList *lispack;

	if(all_packages) {
		lispack = (GList*)data_get(all_packages, cat);

		if(exception == NULL) {
			for(i = 0; i < g_list_length(lispack); i += 4) {
				g_list_nth(lispack, i+3)->data = GINT_TO_POINTER(bool);
			}
		} else {
			for(i = 0; i < g_list_length(lispack); i += 4) {
				if(strcmp(g_list_nth_data(lispack, i), exception))
					g_list_nth(lispack, i+3)->data = GINT_TO_POINTER(bool);
			}
		}
	}
}

//* select (or unselect) a file 'name' in categorie 'cat' *//
void selectfile(char *cat, char* name, int bool)
{
	int i;
	GList *lispack;

	if(all_packages) {
		lispack = (GList*)data_get(all_packages, cat);

		if(lispack) {
			for(i = 0; i < g_list_length(lispack); i += 4) {
				if(!strcmp(name, (char *)g_list_nth_data(lispack, i))) {
					g_list_nth(lispack, i+3)->data = GINT_TO_POINTER(bool);
					return;
				}
			}
		}
	}
}

//* set up group when selected or unselected *//
void group_changed(GtkWidget *button, gpointer data)
{
	char *lang = strdup(getenv("LANG"));
	char *ptr = rindex(lang, '_');
	*ptr = '\0'; ptr = NULL;

	if(!strcmp((char*)data, "NET"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
			selectcat("network", 1);
		else
			selectcat("network", 0);
	}
	else if(!strcmp((char*)data, "NETEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE)
			selectcat("network-extra", 1);
		else
			selectcat("network-extra", 0);
	}
	else if(!strcmp((char*)data, "MUL"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("multimedia", 1);
			selectcat("xmultimedia", 1);
		} else {
			selectcat("multimedia", 0);
			selectcat("xmultimedia", 0);
		}
	}
	else if(!strcmp((char*)data, "MULEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("multimedia-extra", 1);
			selectcat("xmultimedia-extra", 1);
		} else {
			selectcat("multimedia-extra", 0);
			selectcat("xmultimedia-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "DEV"))
	{
		ptr = g_strdup_printf("php-docs-%s", lang);
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("devel", 1);
			selectfile("locale-extra", ptr, 1);
		} else {
			selectcat("devel", 0);
			selectfile("locale-extra", ptr, 0);
		}
		free(ptr);
	}
	else if(!strcmp((char*)data, "DEVEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("devel-extra", 1);
		} else {
			selectcat("devel-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "CONS"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("apps", 1);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("vim-spell-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr); ptr = g_strdup_printf("aspell-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr);
			}
		} else {
			selectcat("apps", 0);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("vim-spell-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr); ptr = g_strdup_printf("aspell-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr);
			}
		}
	}
	else if(!strcmp((char*)data, "CONSEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("apps-extra", 1);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("hunspell-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr); ptr = g_strdup_printf("mbrola-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr);
			}
		} else {
			selectcat("apps-extra", 0);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("hunspell-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr); ptr = g_strdup_printf("mbrola-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr);
			}
		}
	}
	else if(!strcmp((char*)data, "XAPP"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectallfiles("xapps", "libreoffice", 1);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("firefox-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr); ptr = g_strdup_printf("thunderbird-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr);
			}
		} else {
			selectallfiles("xapps", "libreoffice", 0);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("firefox-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr); ptr = g_strdup_printf("thunderbird-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr);
			}
		}
	}
	else if(!strcmp((char*)data, "XAPPEX"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("xapps-extra", 1);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("eric-i18n-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr); ptr = g_strdup_printf("eric4-i18n-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr);
			}
		} else {
			selectcat("xapps-extra", 0);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("eric-i18n-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr); ptr = g_strdup_printf("eric4-i18n-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr);
			}
		}
	}
	else if(!strcmp((char*)data, "GAME"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectcat("games-extra", 1);
		} else {
			selectcat("games-extra", 0);
		}
	}
	else if(!strcmp((char*)data, "BUR"))
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE) {
			selectfile("xapps", "libreoffice", 1);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("libreoffice-l10n-%s", lang);
				selectfile("locale-extra", ptr, 1);
				free(ptr);
			}
		} else {
			selectfile("xapps", "libreoffice", 0);
			if(strcmp(lang ,"en")) {
				ptr = g_strdup_printf("libreoffice-l10n-%s", lang);
				selectfile("locale-extra", ptr, 0);
				free(ptr);
			}
		}
	}

	free(lang);
	return;
}

GtkWidget *get_intermediate_mode_widget(void)
{
	GtkWidget *pvboxp = gtk_vbox_new(FALSE,0);
	GtkWidget *pvbox, *hboxdesktop, *hboxpackage, *hboxtemp;
	GtkWidget *logo;

	GtkWidget *labelenv = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelenv), _("<b>Choose your desktop environment :</b>"));
	gtk_box_pack_start(GTK_BOX(pvboxp), labelenv, FALSE, TRUE, 15);
	hboxdesktop = gtk_hbox_new(FALSE, 5);

	//* Desktop radio button *//
	pvbox = gtk_vbox_new(FALSE,2);
	pRadioKDE = gtk_radio_button_new_with_label(NULL, _("KDE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/kdelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioKDE, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioGNOME =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("GNOME"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/gnomelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioGNOME, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioXFCE =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("XFCE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/xfcelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioXFCE, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioLXDE =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("LXDE"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/lxdelogo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioLXDE, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioE17 =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("E17"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/e17logo.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioE17, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

    pvbox = gtk_vbox_new(FALSE,2);
	GtkWidget *pRadioXorg =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON (pRadioKDE), _("No X Server"));
	logo =  gtk_image_new_from_file(g_strdup_printf("%s/nox.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pvbox), logo, FALSE, FALSE, 0);
	hboxtemp = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxtemp), pRadioXorg, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxtemp, FALSE, FALSE, 8);
	gtk_box_pack_start(GTK_BOX(hboxdesktop), pvbox, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(pvboxp), hboxdesktop, FALSE, TRUE, 15);

	//* Other check button for other packages *//

	hboxpackage = gtk_hbox_new(FALSE, 0);

	//* Basic packages *//
	pvbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *labelgroup = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelgroup), _("<b>Choose groups you want to install :</b>"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelgroup, TRUE, TRUE, 0);

	GtkWidget *phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleApp =  gtk_check_button_new_with_label(_("Graphical Application (Firefox, Thunderbird,...)"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleApp) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleApp), "toggled", G_CALLBACK(group_changed), (gpointer)"XAPP");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleApp, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleCons =  gtk_check_button_new_with_label(_("Console Applications (Nano, Vim...)"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleCons) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleCons), "toggled", G_CALLBACK(group_changed), (gpointer)"CONS");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleCons, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleBur =  gtk_check_button_new_with_label(_("LibreOffice Suite"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleBur) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleBur), "toggled", G_CALLBACK(group_changed), (gpointer)"BUR");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleBur, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleNet =  gtk_check_button_new_with_label(_("Network"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleNet) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleNet), "toggled", G_CALLBACK(group_changed), (gpointer)"NET");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleNet, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleDev =  gtk_check_button_new_with_label(_("Development"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleDev) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleDev), "toggled", G_CALLBACK(group_changed), (gpointer)"DEV");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleDev, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleMul =  gtk_check_button_new_with_label(_("Multimedia"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(pToggleMul) , TRUE);
	gtk_signal_connect(GTK_OBJECT(pToggleMul), "toggled", G_CALLBACK(group_changed), (gpointer)"MUL");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleMul, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hboxpackage), pvbox, TRUE, TRUE, 0);

	//* Extra packages *//
	pvbox = gtk_vbox_new(FALSE, 5);
	GtkWidget *labelgroupex = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelgroupex), _("<b>You can choose extra groups :</b>"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelgroupex, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleAppex =  gtk_check_button_new_with_label(_("Extra - Graphical Applications"));
	gtk_signal_connect(GTK_OBJECT(pToggleAppex), "toggled", G_CALLBACK(group_changed), (gpointer)"APPEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleAppex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleConsex =  gtk_check_button_new_with_label(_("Extra - Console Applications"));
	gtk_signal_connect(GTK_OBJECT(pToggleConsex), "toggled", G_CALLBACK(group_changed), (gpointer)"CONSEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleConsex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleGame =  gtk_check_button_new_with_label(_("Extra - Games"));
	gtk_signal_connect(GTK_OBJECT(pToggleGame), "toggled", G_CALLBACK(group_changed), (gpointer)"GAME");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleGame, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleNetex =  gtk_check_button_new_with_label(_("Extra - Network"));
	gtk_signal_connect(GTK_OBJECT(pToggleNetex), "toggled", G_CALLBACK(group_changed), (gpointer)"NETEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleNetex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleDevex =  gtk_check_button_new_with_label(_("Extra - Development"));
	gtk_signal_connect(GTK_OBJECT(pToggleDevex), "toggled", G_CALLBACK(group_changed), (gpointer)"DEVEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleDevex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	phbox = gtk_hbox_new(FALSE,5);
	GtkWidget *pToggleMulex =  gtk_check_button_new_with_label(_("Extra - Multimedia"));
	gtk_signal_connect(GTK_OBJECT(pToggleMulex), "toggled", G_CALLBACK(group_changed), (gpointer)"MULEX");
	gtk_box_pack_start(GTK_BOX(phbox), pToggleMulex, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox, TRUE, TRUE, 0);

	//* Put into hbox *//
	gtk_box_pack_start(GTK_BOX(hboxpackage), pvbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvboxp), hboxpackage, TRUE, TRUE, 0);

	return pvboxp;
}

//* set up desktop configuration *//
void configure_desktop_intermediate(void)
{
	GSList *pList;
	char *seldesk = NULL;

	char *lang = strdup(getenv("LANG"));
	char *ptr = rindex(lang, '_');
	*ptr = '\0'; ptr = NULL;

	pList = gtk_radio_button_get_group(GTK_RADIO_BUTTON(pRadioKDE));

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

	/* Disable lib in intermediate mode - lib used are just add as dependencies */
	selectcat("lib", 0);
	selectcat("xlib", 0);

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
	} else if(!strcmp(seldesk, _("XFCE"))) {
		selectcat("xfce4", 1);
	} else if(!strcmp(seldesk, _("LXDE"))) {
		selectcat("lxde-desktop", 1);
		selectallfiles("lxde-desktop", NULL, 1);
	} else if(!strcmp(seldesk, _("E17"))) {
		selectcat("e17-extra", 1);
		selectallfiles("e17-extra", NULL, 1);
		selectcat("x11-extra", 1);
		selectfile("x11-extra", "lxdm", 1);
	} else if(!strcmp(seldesk, _("No X Server"))) {
		selectcat("x11", 0);
	}

	free(lang);
	return;
}

void run_intermediate_mode(void)
{
	//* Fix locale dependency *//
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
