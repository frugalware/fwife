/*
 *  configsource-network.c for Fwife
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
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <net/if.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwnetconfig.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libintl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "common.h"

extern GtkWidget *assistant;

static GList *iflist = NULL;

/* Used to timeout a connect */
static jmp_buf timeout_jump;

void timeout(int sig)
{
    longjmp( timeout_jump, 1 ) ;
}

int is_connected(char *host, int port, int timeouttime)
{
	int sControl;
	struct sockaddr_in sin;
	struct hostent* phe;

	memset(&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if ((signed)(sin.sin_addr.s_addr = inet_addr(host)) == -1) {
		if ((phe = gethostbyname(host)) == NULL)
			return -1;
		memcpy((char *)&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	sControl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sControl == -1)
		return -1;

	signal(SIGALRM, timeout) ;
    alarm(timeouttime) ;
    /* timed out */
    if (setjmp(timeout_jump) == 1) {
		alarm(0);
		close(sControl);
		return 0;
    } else {
		if (connect(sControl, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
			alarm(0);
			close(sControl);
			return -1;
		}
    }

    /* connected */
    alarm(0);
	close(sControl);
	return 1;
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
				if(ap != NULL && ap->essid != NULL && strcmp(ap->essid, ""))
					entrys = g_list_append(entrys, ap);
				else
					free_wifi_ap(ap);

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

		if(ap != NULL && ap->essid != NULL && strcmp(ap->essid, ""))
			entrys = g_list_append(entrys, ap);
		else
			free_wifi_ap(ap);

		free(line);
		free(command);
		pclose(fp);
		return entrys;
}

char *select_entry_point(fwnet_interface_t *interface)
{
	GtkWidget* pBoite;

	GtkWidget *viewif;
	GtkWidget *pScrollbar;
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

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), viewif);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pScrollbar, TRUE, TRUE, 5);

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
	gint i;

	char *types[] =
	{
		"dhcp", _("Use a DHCP server"),
		"static", _("Use a static IP address")
	};

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Select network type"),
								GTK_WINDOW(assistant),
								GTK_DIALOG_MODAL,
								GTK_STOCK_OK,GTK_RESPONSE_OK,
								NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Now we need to know how your machine connects to the network.\n"
			"If you have an internal network card and an assigned IP address, gateway, and DNS,\n use 'static' "
			"to enter these values.\n"
			"If your IP address is assigned by a DHCP server (commonly used by cable modem services),\n select 'dhcp'. \n"));

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
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, types[i], 1, types[i+1], -1);
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
	GtkWidget *butaps = gtk_button_new_with_label(_("Scan Access Points"));
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
		default:
			break;
	}

	gtk_widget_destroy(pBoite);
	return 0;
}

int dsl_config(fwnet_profile_t *newprofile, fwnet_interface_t *interface)
{
	GtkWidget *phboxtemp, *labeltemp;
	char *uname, *passwd, *passverify;
	char *iface = interface->name;
	char *msgptr = g_strdup_printf(_("Do you want to configure a DSL connection associated with the interface %s?"), iface);

	switch(fwife_question(msgptr))
	{
		case GTK_RESPONSE_YES:
			break;
		case GTK_RESPONSE_NO:
			return 0;
	}
	free(msgptr);

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
				return -2;
			} else {
				snprintf(newprofile->adsl_username, PATH_MAX, uname);
				snprintf(newprofile->adsl_password, PATH_MAX, passwd);
				snprintf(newprofile->adsl_interface, IF_NAMESIZE, iface);
			}
			break;
		default:
			break;
	}

	gtk_widget_destroy(pBoite);
	return 0;
}

int post_net_config(fwnet_profile_t *newprofile, fwnet_interface_t *interface)
{
	sprintf(newprofile->name, "default");
	newprofile->interfaces = g_list_append(newprofile->interfaces, interface);

	if(fwnet_is_dhcp(interface) == 0)
		while(dsl_config(newprofile, interface) == -2) {}

	char *host = strdup("install.frugalware");
	fwnet_writeconfig(newprofile, host);
	fw_system("netconfig start");
	free(host);

	if(is_connected("www.google.org", 80, 5) < 1) {
		int ret = fwife_question(_("Fwife cannot connect to internet with this configuration, do you want to apply this configuration anyway?"));
		if(ret == GTK_RESPONSE_YES) {
			return 0;
		} else {
			return -1;
		}
	}

	return 0;
}

int select_interface(fwnet_interface_t *interface)
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
	GtkWidget *cellview;
	int i;

	char *iface = NULL;

	pBoite = gtk_dialog_new_with_buttons(_("Select your network interface :"),
        GTK_WINDOW(assistant),
        GTK_DIALOG_MODAL,
        GTK_STOCK_OK,GTK_RESPONSE_OK,
        NULL);

	 gtk_window_set_default_size(GTK_WINDOW(pBoite), 320, 200);
	 gtk_window_set_position(GTK_WINDOW (pBoite), GTK_WIN_POS_CENTER);

	store = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	viewif = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref (store);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewif));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes ("", renderer, "pixbuf", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Device"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	col = gtk_tree_view_column_new_with_attributes (_("Description"), renderer, "text", 2, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	cellview = gtk_cell_view_new ();
	connectimg = gtk_widget_render_icon (cellview, GTK_STOCK_NETWORK, GTK_ICON_SIZE_BUTTON, NULL);

	if(iflist == NULL) {
		iflist = fwnet_iflist();
	}

	for(i=0; i<g_list_length(iflist); i+=2) {
		gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter);
		gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter,
			0, connectimg,
			1, (char*)g_list_nth_data(iflist, i),
			2, (char*)g_list_nth_data(iflist, i+1),
			-1);
	}

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), viewif, TRUE, TRUE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
    {
        case GTK_RESPONSE_OK:
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(viewif)));
			if(gtk_tree_selection_get_selected(selection, &model, &iter))
				gtk_tree_model_get (model, &iter, 1, &iface, -1);

			snprintf(interface->name, IF_NAMESIZE, iface);
            break;
		default:
			gtk_widget_destroy(pBoite);
			return -1;
    }

    gtk_widget_destroy(pBoite);
    return 0;
}

int run_net_config(GList **config)
{
	char *nettype = NULL;
	char *ptr = NULL;
	char *lastprof = NULL;
	fwnet_interface_t *newinterface = NULL;
	struct dirent *ent = NULL;
	struct stat info;
	DIR *dir;
	int brk = 0;

	/* profile used do write configuration */
	fwnet_profile_t *newprofile = NULL;

	MALLOC(newprofile, sizeof(fwnet_profile_t));
	memset(newprofile, 0, sizeof(fwnet_profile_t));

	MALLOC(newinterface, sizeof(fwnet_interface_t));
	memset(newinterface, 0, sizeof(fwnet_interface_t));

	if(is_connected("www.google.com", 80, 2) == 1) {
		// seems we got a connection, can we infer the origin?
		// installation from fw? in this case we've got a netconfig profile
		lastprof = fwnet_lastprofile();
		if(lastprof != NULL) {
			ptr = g_strdup_printf(_("A netconfig profile (\"%s\") has been found on your current installation.\nDo you want to use it?"), lastprof);
			switch(fwife_question(ptr))
			{
				case GTK_RESPONSE_YES:
					free(newprofile);
					if((newprofile = fwnet_parseprofile(lastprof)) != NULL) {
						sprintf(newprofile->name, "default");
						data_put(config, "netprofile", newprofile);
						free(ptr);
						free(lastprof);
						return 0;
					}
				case GTK_RESPONSE_NO:
					brk = 1;
					break;
			}
			free(ptr);
			free(lastprof);
		}

		// maybe a dhcp connection without netconfig, look at generated resolv.conf
		dir=opendir("/var/run/dhcpcd/resolv.conf/");
		if(dir != NULL) {
			while((ent = readdir(dir))) {
				if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
					ptr = g_strdup_printf("/var/run/dhcpcd/resolv.conf/%s", ent->d_name);
					if(stat(ptr, &info))
						continue;
					free(ptr);
					if(S_ISREG(info.st_mode)) {
						ptr = g_strdup_printf(_("An active connection seems to have been found on interface %s using DHCP.\nDo you want to use it?"), ent->d_name);
						switch(fwife_question(ptr))
						{
						case GTK_RESPONSE_YES:
							snprintf(newinterface->name, IF_NAMESIZE, ent->d_name);
							newinterface->dhcp_opts[0]='\0';
							newinterface->options = g_list_append(newinterface->options, strdup("dhcp"));
							sprintf(newprofile->name, "default");
							newprofile->interfaces = g_list_append(newprofile->interfaces, newinterface);
							data_put(config, "netprofile", newprofile);
							free(ptr);
							return 0;
						case GTK_RESPONSE_NO:
							brk = 1;
							break;
						}
						free(ptr);
					}
				}
			}
			closedir(dir);
		}

		// ask only if previous detection fail
		if(brk == 0) {
			switch(fwife_question(_("An active connection seems to have been found but the connection's type have not been found.\nDo you want to use it anyway?")))
			{
				case GTK_RESPONSE_YES:
					return 0;
				case GTK_RESPONSE_NO:
					break;
			}
		}
	}

	if(select_interface(newinterface) == -1)
		return 0;

	nettype = ask_nettype();
	if(nettype == NULL) {
		free(newprofile);
		free(newinterface);
		return -1;
	}

	if(fwnet_is_wireless_device(newinterface->name)) {
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
		free(ptr);
	} else if(!strcmp(nettype, "static")) {
		configure_static(newinterface);
	} else {
		free(newprofile);
		free(newinterface);
		return -1;
	}

	fw_system("netconfig stop");
	if(post_net_config(newprofile, newinterface) == -1) {
		free(newprofile);
		free(newinterface);
		return -1;
	}

	/* save network profile for later usage in netconf*/
	data_put(config, "netprofile", newprofile);

	return 0;
}