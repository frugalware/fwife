/*
 *  netconf.c for Fwife
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

#include <gtk/gtk.h>
#include <stdio.h>
#include <net/if.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwnetconfig.h>
#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libintl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "common.h"

static GtkWidget *viewif=NULL;
static GList *iflist=NULL;
static GList *interfaceslist=NULL;

extern GtkWidget *assistant;

/* profile used do write configuration */
static fwnet_profile_t *newprofile=NULL;

enum
{
	COLUMN_NET_IMAGE,
	COLUMN_NET_NAME,
	COLUMN_NET_DESC,
	COLUMN_NET_TYPE
};

plugin_t plugin =
{
	"netconf",
	desc,
	65,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	load_help_widget,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Configuring your network");
}

plugin_t *info(void)
{
	return &plugin;
}

struct two_ptr {
	void *first;
	void *sec;
};

struct wifi_ap {
	char *address;
	char *essid;
	char *encmode;
	char *cypher;
	char *protocol;
	char *mode;
	int quality;
	int channel;
	int encryption;
};

void free_wifi_ap(struct wifi_ap *ap)
{
	if(ap != NULL) {
		free(ap->address);
		free(ap->essid);
		free(ap->encmode);
		free(ap->cypher);
		free(ap->protocol);
		free(ap->mode);
	}
	free(ap);
}

/* Parser for iwlist command */
GList *list_entry_points(char *ifacename)
{
	char *line = NULL;
	size_t len = 0;
	FILE * fp = NULL;
	char *command = NULL;
	struct wifi_ap *ap = NULL;
	char *tok = NULL;

	GList *entrys = NULL;

	MALLOC(line, 256);

	/* up interface */
	command = g_strdup_printf("ifconfig %s up", ifacename);
	system(command);
	free(command);
	/* scan APs */
	command = g_strdup_printf("iwlist %s scan", ifacename);
	fp = popen(command, "r");

	while (getline(&line, &len, fp) != -1) {
			if((tok = strstr(line, "Address: ")) != NULL) {
				if(ap != NULL)
					entrys = g_list_append(entrys, ap);

				MALLOC(ap, sizeof(struct wifi_ap));
				memset(ap, 0, sizeof(struct wifi_ap));
				strchr(tok+9, '\n')[0] = '\0';
				ap->address = strdup(tok+9);
				continue;
			}

			if((tok = strstr(line, "ESSID:\"")) != NULL) {
				strchr(tok+7, '"')[0] = '\0';
				ap->essid = strdup(tok+7);
				continue;
			}

			if((tok = strstr(line, "Protocol:")) != NULL) {
				strchr(tok+9, '\n')[0] = '\0';
				ap->protocol = strdup(tok+9);
				continue;
			}

			if((tok = strstr(line, "Encryption key:")) != NULL) {
				if(strstr(tok, "on") != NULL)
					ap->encryption = 1;
				else
					ap->encryption = 0;
				continue;
			}

			if((tok = strstr(line, "Channel:")) != NULL) {
				strchr(tok+8, '\n')[0] = '\0';
				ap->channel= atoi(tok+8);
				continue;
			}

			if((tok = strstr(line, "Mode:")) != NULL) {
				strchr(tok+5, '\n')[0] = '\0';
				ap->mode = strdup(tok+5);
				continue;
			}

			if((tok = strstr(line, "Quality=")) != NULL) {
				strchr(tok+8, '/')[0] = '\0';
				ap->quality = atoi(tok+8);
				continue;
			}

			if((tok = strstr(line, "IE: ")) != NULL && strstr(line, "Unknown") == NULL) {
				strchr(tok+4, '\n')[0] = '\0';
				ap->encmode = strdup(tok+4);
				continue;
			}

			if((tok = strstr(line, "Group Cipher : ")) != NULL) {
				strchr(tok+15, '\n')[0] = '\0';
				ap->cypher = strdup(tok+15);
				continue;
			}
		}

		if(ap != NULL)
			entrys = g_list_append(entrys, ap);

		free(line);
		free(command);
		pclose(fp);
		return entrys;
}

char *select_entry_point(fwnet_interface_t *interface)
{
	GtkWidget* pBoite;

	GtkWidget *viewif;
	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GdkPixbuf *connectimg;
	int i;
	char *essidap;

	GList *listaps = NULL;
	struct wifi_ap *ap = NULL;

	pBoite = gtk_dialog_new_with_buttons(_("Select your access point :"),
        GTK_WINDOW(assistant),
        GTK_DIALOG_MODAL,
        GTK_STOCK_REFRESH, GTK_RESPONSE_APPLY,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        NULL);

	 gtk_window_set_default_size(GTK_WINDOW(pBoite), 320, 200);
	 gtk_window_set_position(GTK_WINDOW (pBoite), GTK_WIN_POS_CENTER);

	store = gtk_list_store_new(7, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	viewif = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref (store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewif));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes (_("Signal"), renderer, "pixbuf", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Address"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("ESSID"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Mode"), renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Protocol"), renderer, "text", 4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Encryption"), renderer, "text", 5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Cypher"), renderer, "text", 6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), viewif, TRUE, TRUE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	while(1) {
		if((listaps = list_entry_points(interface->name)) == NULL)
			return NULL;

		for(i = 0; i < g_list_length(listaps); i++) {
			ap = (struct wifi_ap*)g_list_nth_data(listaps, i);
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter);

			/* set image according to quality signal */
			if(ap->quality < 25)
				connectimg = gtk_image_get_pixbuf(GTK_IMAGE(gtk_image_new_from_file(g_strdup_printf("%s/signal-25.png", IMAGEDIR))));
			else if(ap->quality >= 25 && ap->quality < 50)
				connectimg = gtk_image_get_pixbuf(GTK_IMAGE(gtk_image_new_from_file(g_strdup_printf("%s/signal-50.png", IMAGEDIR))));
			else if(ap->quality >= 50 && ap->quality < 75)
				connectimg = gtk_image_get_pixbuf(GTK_IMAGE(gtk_image_new_from_file(g_strdup_printf("%s/signal-75.png", IMAGEDIR))));
			else if(ap->quality >= 75)
				connectimg = gtk_image_get_pixbuf(GTK_IMAGE(gtk_image_new_from_file(g_strdup_printf("%s/signal-100.png", IMAGEDIR))));

			if(ap->encryption == 1) {
				gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter,
					0, connectimg, 1, ap->address, 2, ap->essid,
					3, ap->mode, 4, ap->protocol,
					5, ap->encmode, 6, ap->cypher,
					-1);
			} else {
				gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter,
					0, connectimg, 1, ap->address, 2, ap->essid,
					3, ap->mode, 4, ap->protocol,
					5, _("No encryption"), 6, "",
					-1);
			}
			free_wifi_ap(ap);
			g_object_unref(connectimg);
		}

		int ret = gtk_dialog_run(GTK_DIALOG(pBoite));
		if(ret == GTK_RESPONSE_OK) {
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(viewif)));
			if(gtk_tree_selection_get_selected(selection, &model, &iter))
				gtk_tree_model_get (model, &iter, 2, &essidap, -1);
			else
				essidap = "";
			gtk_widget_destroy(pBoite);
			g_list_free(listaps);
			return strdup(essidap);
		} else if(ret == GTK_RESPONSE_APPLY) {
			gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))));
			g_list_free(listaps);
		} else {
			gtk_widget_destroy(pBoite);
			g_list_free(listaps);
			return NULL;
		}
	}

	return NULL;
}

char *ask_nettype(void)
{
	char *str = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *combo;
	GtkListStore *store;
	int i;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Select network type"),
								GTK_WINDOW(assistant),
								GTK_DIALOG_MODAL,
								GTK_STOCK_OK,GTK_RESPONSE_OK,
								NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Now we need to know how your machine connects to the network.\n"
			"If you have an internal network card and an assigned IP address, gateway, and DNS,\n use 'static' "
			"to enter these values.\n"
			"If your IP address is assigned by a DHCP server (commonly used by cable modem services),\n select 'dhcp'. \n"));

	char *types[] =
	{
		"dhcp", _("Use a DHCP server"),
		"static", _("Use a static IP address")
	};
	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));
	gtk_widget_set_size_request(combo, 350, 40);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);

	for (i = 0; i < 4; i+=2) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, types[i], 1, types[i+1], -1);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), combo, FALSE, FALSE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
			gtk_tree_model_get (model, &iter, 0, &str, -1);
			break;
		/* user cancel */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	/* destroy dialogbox */
	gtk_widget_destroy(pBoite);
	return str;
}

void set_access_point(GtkWidget *widget, gpointer data)
{
	struct two_ptr *tptr = (struct two_ptr*)data;
	char *result = select_entry_point(tptr->first);
	gtk_entry_set_text(GTK_ENTRY(tptr->sec), result);
	free(result);
}

int configure_wireless(fwnet_interface_t *interface)
{
	GtkWidget *phboxtemp, *labeltemp;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Configure Wireless"),
							GTK_WINDOW(assistant),
							GTK_DIALOG_MODAL,
							GTK_STOCK_OK,GTK_RESPONSE_OK,
							GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
							NULL);

	phboxtemp = gtk_hbox_new(FALSE, 5);
	GtkWidget *labelinfo = gtk_label_new(_("In order to use wireless, you must set your extended netwok name (ESSID).\n"
						"If you have a WEP encryption key, then please enter it below.\n"
						"Examples: 4567-89AB-CD or s:password\n"
						"If you have a WPA passphrase, then please enter it below.\n"
						"Example: secret\n"
						"If you want to use a custom driver (not the generic one, called 'wext'), then please enter it below.\n"
						"If unsure enter nothing"));
	GtkWidget *imagewifi = gtk_image_new_from_file(g_strdup_printf("%s/wifi.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(phboxtemp), imagewifi, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(phboxtemp), labelinfo, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("Essid : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryEssid = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryEssid, FALSE, FALSE, 0);
	GtkWidget *butaps = gtk_button_new_with_label(_("Scan for Access Points"));
	gtk_box_pack_start(GTK_BOX(phboxtemp), butaps, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("WEP encryption key : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryWepKey = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryWepKey, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("WPA encryption passphrase : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryWpaPass = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryWpaPass, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("WPA Driver : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryWpaDriver = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryWpaDriver, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	struct two_ptr tptr = {interface, pEntryEssid};
	g_signal_connect (butaps, "clicked", G_CALLBACK (set_access_point), &tptr);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			snprintf(interface->essid, FWNET_ESSID_MAX_SIZE, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryEssid)));
			snprintf(interface->key, FWNET_ENCODING_TOKEN_MAX, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryWepKey)));
			snprintf(interface->wpa_psk, PATH_MAX, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryWpaPass)));
			snprintf(interface->wpa_driver, PATH_MAX, (char*)gtk_entry_get_text(GTK_ENTRY(pEntryWpaDriver)));
			break;
		/* user cancel */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	/* destroy dialogbox */
	gtk_widget_destroy(pBoite);
	return 0;
}

int configure_static(fwnet_interface_t *interface)
{
	GtkWidget *phboxtemp, *labeltemp;
	char option[50];
	char *ipaddr, *netmask, *gateway;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Configure static network"),
			GTK_WINDOW(assistant),
			GTK_DIALOG_MODAL,
      			GTK_STOCK_OK,GTK_RESPONSE_OK,
       			NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Enter static network parameters :"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("IP Address : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryIP = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryIP, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("Network Mask : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryNetmask = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryNetmask, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("Gateway : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryGateway = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryGateway, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			ipaddr = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryIP));
			netmask = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryNetmask));

			if(strlen(ipaddr))
				snprintf(option, 49, "%s netmask %s", ipaddr, netmask);

			interface->options = g_list_append(interface->options, strdup(option));

			gateway = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryGateway));

			if(strlen(gateway))
				snprintf(interface->gateway, FWNET_GW_MAX_SIZE, "default gw %s", gateway);

			break;
		/* user cancel */
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	/* destroy dialogbox */
	gtk_widget_destroy(pBoite);
	return 0;
}

int dsl_config(GtkWidget *button, gpointer data)
{
	GtkWidget *phboxtemp, *labeltemp;
	char *uname, *passwd, *passverify, *iface;

	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewif));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(viewif)));

	/* check if an interface has been selected */
	if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_NET_NAME, &iface, -1);
	} else {
		fwife_error(_("You must select an interface in the above list."));
		return 0;
	}

	switch(fwife_question(g_strdup_printf(_("Do you want to configure a DSL connection associated with the interface %s?"), iface)))
	{
		case GTK_RESPONSE_YES:
			break;
		case GTK_RESPONSE_NO:
			return 0;
	}

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Configure DSL connection"),
								GTK_WINDOW(assistant),
								GTK_DIALOG_MODAL,
								GTK_STOCK_OK,GTK_RESPONSE_OK,
								GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
								NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Enter DSL parameters"));
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("PPPOE username : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryName = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryName, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("PPPOE password : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryPass = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryPass, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	phboxtemp = gtk_hbox_new(FALSE, 0);
	labeltemp = gtk_label_new(_("Re-enter PPPOE password : "));
	gtk_box_pack_start(GTK_BOX(phboxtemp), labeltemp, FALSE, FALSE, 5);
	GtkWidget *pEntryVerify = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(phboxtemp), pEntryVerify, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), phboxtemp, FALSE, FALSE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			uname = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryName));
			passwd = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryPass));
			passverify = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryVerify));

			if(strcmp(passverify, passwd)) {
				fwife_error(_("Passwords do not match! Try again."));
				gtk_widget_destroy(pBoite);
				dsl_config(button, data);
				return 0;
			} else {
				snprintf(newprofile->adsl_username, PATH_MAX, uname);
				snprintf(newprofile->adsl_password, PATH_MAX, passwd);
				snprintf(newprofile->adsl_interface, IF_NAMESIZE, iface);
			}
			break;
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
	}

	gtk_widget_destroy(pBoite);
	return 0;
}

int add_interface(GtkWidget *button, gpointer data)
{
	fwnet_interface_t *newinterface = NULL;
	char *ptr = NULL;
	char *nettype = NULL;
	char *iface = NULL;
	unsigned int i;

	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewif));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(viewif)));

	if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_NET_NAME, &iface, -1);
	} else {
		fwife_error(_("You must select an interface in the above list."));
		return 0;
	}

	for(i=0;i<g_list_length(interfaceslist); i++) {
		if(!strcmp(((fwnet_interface_t*)g_list_nth_data(interfaceslist, i))->name, iface)) {
			int retquest = fwife_question(_("This interface has been already configured! Do you want to configure it again?"));
			if(retquest == GTK_RESPONSE_YES) {
				free(g_list_nth_data(interfaceslist, i));
				interfaceslist =  g_list_delete_link (interfaceslist, g_list_nth(interfaceslist, i));
				break;
			} else {
				return -1;
			}
		}
	}

	MALLOC(newinterface, sizeof(fwnet_interface_t));
	memset(newinterface, 0, sizeof(fwnet_interface_t));

	snprintf(newinterface->name, IF_NAMESIZE, iface);

	nettype = ask_nettype();
	if(nettype == NULL)
		return -1;

	if(strcmp(nettype, "lo")) {
		interfaceslist = g_list_append(interfaceslist, newinterface);
	}

	if(strcmp(nettype, "lo") && fwnet_is_wireless_device(iface)) {
		switch(fwife_question(_("It seems that this network card has a wireless extension.\n\nConfigure your wireless now?")))
		{
			case GTK_RESPONSE_YES:
				configure_wireless(newinterface);
				break;
			default:
				break;
		}
	}

	if(!strcmp(nettype, "dhcp")) {
		ptr = fwife_entry(_("Set DHCP hostname"), _("Some network providers require that the DHCP hostname be\n"
			"set in order to connect.\n If so, they'll have assigned a hostname to your machine.\n If you were"
			"assigned a DHCP hostname, please enter it below.\n If you do not have a DHCP hostname, just"
			"hit enter."), NULL);

		if(ptr != NULL && strlen(ptr))
			snprintf(newinterface->dhcp_opts, PATH_MAX, "-t 10 -h %s\n", ptr);
		else
			newinterface->dhcp_opts[0]='\0';

		newinterface->options = g_list_append(newinterface->options, strdup("dhcp"));
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_TYPE, "dhcp", -1);
		free(ptr);
	} else if(!strcmp(nettype, "static")) {
		if(configure_static(newinterface) != -1)
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_TYPE, "static", -1);
	}

	return 0;
}

int del_interface(GtkWidget *button, gpointer data)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char *nameif;
	int i;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewif));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(viewif)));

	if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_NET_NAME, &nameif, -1);
		for(i = 0;i < g_list_length(interfaceslist); i++) {
			if(!strcmp(((fwnet_interface_t*)g_list_nth_data(interfaceslist, i))->name, nameif)) {
				free(g_list_nth_data(interfaceslist, i));
				interfaceslist =  g_list_delete_link (interfaceslist, g_list_nth(interfaceslist, i));
			}
		}
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_TYPE, "", -1);
	}

	return 0;
}

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *pVBox, *phboxbut;
	GtkWidget *hboxview;
	GtkWidget *info;

	GtkListStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	pVBox = gtk_vbox_new(FALSE, 0);
	phboxbut = gtk_hbox_new(FALSE, 0);
	hboxview = gtk_hbox_new(FALSE, 0);

	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), _("<b>You can configure all network interfaces you need</b>"));
	gtk_box_pack_start(GTK_BOX(pVBox), info, FALSE, FALSE, 5);

	store = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	viewif = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref (store);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes ("", renderer, "pixbuf", COLUMN_NET_IMAGE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Device"), renderer, "text", COLUMN_NET_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Description"), renderer, "text", COLUMN_NET_DESC, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Configuration"), renderer, "text", COLUMN_NET_TYPE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	GtkWidget *image = gtk_image_new_from_file(g_strdup_printf("%s/configure24.png", IMAGEDIR));
	GtkWidget *btnsave = gtk_button_new_with_label(_("Configure"));
	gtk_button_set_image(GTK_BUTTON(btnsave), image);
	gtk_box_pack_start(GTK_BOX(phboxbut), btnsave, FALSE, FALSE, 10);
	GtkWidget *btndel = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	gtk_box_pack_start(GTK_BOX(phboxbut), btndel, FALSE, FALSE, 10);
	image = gtk_image_new_from_file(g_strdup_printf("%s/dsl24.png", IMAGEDIR));
	GtkWidget *btndsl = gtk_button_new_with_label(_("DSL Configuration"));
	gtk_button_set_image(GTK_BUTTON(btndsl), image);
	gtk_box_pack_start(GTK_BOX(phboxbut), btndsl, FALSE, FALSE, 10);

	gtk_box_pack_start(GTK_BOX(hboxview), viewif, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(pVBox), hboxview, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(pVBox), phboxbut, FALSE, FALSE, 5);

	g_signal_connect(G_OBJECT(btnsave), "clicked", G_CALLBACK(add_interface), NULL);
	g_signal_connect(G_OBJECT(btndel), "clicked", G_CALLBACK(del_interface), NULL);
	g_signal_connect(G_OBJECT(btndsl), "clicked", G_CALLBACK(dsl_config), NULL);

	return pVBox;
}

int prerun(GList **config)
{
	int i;
	GdkPixbuf *connectimg;
	GtkWidget *cellview;
	GtkTreeIter iter;

	fwnet_profile_t *oldprofile = (fwnet_profile_t*)data_get(*config, "netprofile");
	if(oldprofile != NULL) {
		switch(fwife_question(_("You can now reconfigure your network if you want to configure others interfaces or just apply current network configuration.\n\n\
Do you want to apply the current configuration ?")))
		{
			case GTK_RESPONSE_YES:
				newprofile = oldprofile;
				skip_to_next_plugin();
				return 0;
			case GTK_RESPONSE_NO:
				break;
		}
	}

	cellview = gtk_cell_view_new();
	connectimg = gtk_widget_render_icon (cellview, GTK_STOCK_NETWORK,
					GTK_ICON_SIZE_BUTTON, NULL);

	if(iflist == NULL) {
		iflist = fwnet_iflist();

		for(i=0; i<g_list_length(iflist); i+=2) {
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter);
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter, COLUMN_NET_IMAGE, connectimg,
					COLUMN_NET_NAME, (char*)g_list_nth_data(iflist, i),
					COLUMN_NET_DESC, (char*)g_list_nth_data(iflist, i+1),
					COLUMN_NET_TYPE, "",
					-1);
		}
	}

	free(newprofile);
	MALLOC(newprofile, sizeof(fwnet_profile_t));
	memset(newprofile, 0, sizeof(fwnet_profile_t));

	return 0;
}

int run(GList **config)
{
	int i, ret;

	sprintf(newprofile->name, "default");
	for(i = 0; i<g_list_length(interfaceslist); i++) {
		newprofile->interfaces = g_list_append(newprofile->interfaces, (fwnet_interface_t *) g_list_nth_data(interfaceslist, i));
	}

	char *host = fwife_entry(_("Hostname"), _("We'll need the name you'd like to give your host.\nThe full hostname is needed, such as:\n\nfrugalware.example.net\n\nEnter full hostname:"), "frugalware.example.net");

	pid_t pid = fork();

	if(pid == -1) {
		LOG("Error when forking process in grubconf plugin.");
	} else if(pid == 0) {
		chroot(TARGETDIR);
		fwnet_writeconfig(newprofile, host);
		exit(0);
	} else {
		wait(&ret);
	}

	free(newprofile);
	newprofile = NULL;
	free(host);
	return 0;
}

GtkWidget *load_help_widget(void)
{
	GtkWidget *helplabel = gtk_label_new(_("Select network interface you want to configure then click on add button and follow instructions..."));
	return helplabel;
}
