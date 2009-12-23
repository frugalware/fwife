/*
 *  configsource.c for Fwife
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
#include <unistd.h>
#include <stdlib.h>

#include <net/if.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwnetconfig.h>
#include <getopt.h>
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

static GList *mirrorlist = NULL;

static GtkWidget *viewserver = NULL;

extern GtkWidget *assistant;

int pre_net_config();
int post_net_config();
int run_net_config();

enum
{
  COLUMN_USE,
  COLUMN_NAME,
  COLUMN_FROM,
  NUM_COLUMNS
};

plugin_t plugin =
{
	"configsource",	
	desc,
	20,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
 	load_help_widget,
	prerun,
	run,
	NULL // dlopen handle
};

plugin_t *info()
{
	return &plugin;
}

char *desc()
{
	return (_("Selecting mirrors"));
}

GList *getmirrors(char *fn)
{
	FILE *fp;
	char line[PATH_MAX], *ptr = NULL, *country = NULL, *preferred = NULL;
	GList *mirrors=NULL;

	if ((fp = fopen(fn, "r"))== NULL) { //fopen error
		printf("Could not open output file for reading");
		return(NULL);
	}

	/* this string should be the best mirror for the given language from
	 * /etc/pacman-g2/repos */
	preferred = strdup(_("ftp://ftp5."));
	while(!feof(fp)) {
		if(fgets(line, PATH_MAX, fp) == NULL)
			break;
		if(line == strstr(line, "# - ")) { // country
			ptr = strrchr(line, ' ');
			*ptr = '\0';
			ptr = strrchr(line, ' ')+1;
			country = strdup(ptr);
		}
		if(line == strstr(line, "Server = ")) { // server
			ptr = strrchr(line, '/'); // drops frugalware-ARCH
			*ptr = '\0';
			ptr = strstr(line, "Server = ")+9; //drops 'Server = ' part
			mirrors = g_list_append(mirrors, strdup(ptr));
			mirrors = g_list_append(mirrors, strdup(country));
			if(!strncmp(ptr, preferred, strlen(preferred)))
				mirrors = g_list_append(mirrors, GINT_TO_POINTER(1));
			else
				mirrors = g_list_append(mirrors, GINT_TO_POINTER(0)); //unchecked by default in checkbox
		}
	}
	FREE(preferred);
	fclose(fp);
	return (mirrors);
}

int updateconfig(char *fn, GList *mirrors)
{
	FILE *fp;
	unsigned int i;

	if ((fp = fopen(fn, "w"))== NULL)
	{
		perror(_("Could not open output file for writing"));
		return(1);
	}
	fprintf(fp, "#\n# %s repository\n#\n\n[%s]\n\n", PACCONF, PACCONF);
	for (i=0; i<g_list_length(mirrors); i+=2) {
		// do not change the country style as it will cause getmirrors() misbehaviour
		fprintf(fp, "# - %s -\n", (char *)g_list_nth_data(mirrors, i+1));
		fprintf(fp, "Server = %s/frugalware-%s\n", (char *)g_list_nth_data(mirrors, i), ARCH);
	}
	fclose(fp);
	g_list_free(mirrors);
	return(0);
}

void fixed_toggled(GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean fixed;
	gchar *name, *from;

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, COLUMN_USE, &fixed, -1);
	gtk_tree_model_get (model, &iter, COLUMN_NAME, &name, -1);
	gtk_tree_model_get (model, &iter, COLUMN_FROM, &from, -1);

	/* set new value */
	fixed ^= 1;	
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_USE, fixed, -1);

	gtk_tree_path_free (path);
}

/* add a custom mirror to the list */
void add_mirror (GtkWidget *button, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)data;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	char* sName = NULL;

	sName = fwife_entry(_("Add a custom server"), 
						_("You may specify a custom mirror (eg. LAN) so you can download packages faster.\nEnter server's address below :")
						, NULL);
	if(sName) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							0, TRUE, 1, sName, 2, "CUSTOM", -1);
	}
	return;  
}

/* delete a custom mirror to the list */
void remove_mirror (GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)data;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gint i;
		GtkTreePath *path;
		gchar *old_text, *from;

		gtk_tree_model_get (model, &iter, 1, &old_text, -1);
		gtk_tree_model_get (model, &iter, 2, &from, -1);
      
		/* don't delete default mirrors */
		if(strcmp(from, "CUSTOM")) {
			fwife_error(_("Can't delete default mirrors"));
			return;
		}

		path = gtk_tree_model_get_path (model, &iter);
		i = gtk_tree_path_get_indices (path)[0];
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		gtk_tree_path_free (path);
	}
	return;
}

/* Create the list of mirrors */
GtkWidget *mirrorview()
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *view;
	
	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);
	
	renderer = gtk_cell_renderer_toggle_new ();
  	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), model);
  	col = gtk_tree_view_column_new_with_attributes (_("Use"), renderer, "active", 0, NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (col), GTK_TREE_VIEW_COLUMN_FIXED);
  	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col), 50);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Mirrors"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Froms"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	return view;
}

GtkWidget *load_gtk_widget()
{	
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	GtkWidget *addmirror, *delmirror, *buttonlist;
	GtkWidget *image, *info;

	GtkWidget *pVBox = gtk_vbox_new(FALSE, 0);

	/* top info label */
	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), 
						_("<span face=\"Courier New\"><b>You can choose one or more nearly mirrors to speed up package downloading.</b></span>"));

	gtk_box_pack_start (GTK_BOX (pVBox), info, FALSE, FALSE, 5);

	/* list of mirrors */
	viewserver = mirrorview();
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (viewserver));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), viewserver);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (pVBox), pScrollbar, TRUE, TRUE, 0);
		
	buttonlist = gtk_hbox_new(TRUE, 10);
			
	/* Set up buttons for custom mirrors */
	addmirror = gtk_button_new_with_label(_("Add custom mirror"));
	/* TOFIX : need 2 creation to be show lol (why???) */
	delmirror = gtk_button_new_with_label(NULL);
	delmirror = gtk_button_new_with_label(_("Remove custom mirror"));
		
	/* Set images */
	image = gtk_image_new_from_stock (GTK_STOCK_ADD, 2);
	gtk_button_set_image(GTK_BUTTON(addmirror), image);
	image = gtk_image_new_from_stock (GTK_STOCK_CANCEL, 2);
	gtk_button_set_image(GTK_BUTTON(delmirror), image);
		
	/* connect button to the select root part function */
	g_signal_connect (addmirror, "clicked",G_CALLBACK (add_mirror), viewserver);
	g_signal_connect (delmirror, "clicked", G_CALLBACK (remove_mirror), viewserver);		
	
	/* Add them to the box */
	gtk_box_pack_start (GTK_BOX (buttonlist), addmirror, TRUE, FALSE, 10);	
	gtk_box_pack_start (GTK_BOX (buttonlist), delmirror, TRUE, FALSE, 10);
	
	gtk_box_pack_start (GTK_BOX (pVBox), buttonlist, FALSE, FALSE, 5);
	
	return pVBox;
}

int prerun(GList **config)
{
	GtkTreeIter iter;
	char *fn;
	int i;
	
	switch(fwife_question(_("You need an active internet connection ,\n do you want to configure your network now?")))
	{
		case GTK_RESPONSE_YES:
			run_net_config();
			break;
		case GTK_RESPONSE_NO:
			break;
	}
	
	if(mirrorlist == NULL)
	{
		fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);
		mirrorlist = getmirrors(fn);
		FREE(fn);

		for(i=0; i < g_list_length(mirrorlist); i+=3) 
		{		
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter);
		
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter,
										0, (gboolean)(GPOINTER_TO_INT(g_list_nth_data(mirrorlist, i+2))),
										1, (gchar*)g_list_nth_data(mirrorlist, i), 
										2, (gchar*)g_list_nth_data(mirrorlist, i+1), -1);	
		
		}
	}
	return 0;
}

int run(GList **config)
{
	char *fn, *name, *from;
	gboolean toggled;
	GList *newmirrorlist = NULL;
	
	fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);

	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)viewserver;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);

	if(gtk_tree_model_get_iter_first (model, &iter) == FALSE)
	{
		return(0);
	}
	else
	{
		do
		{
			gtk_tree_model_get (model, &iter, COLUMN_USE, &toggled, COLUMN_NAME, &name, COLUMN_FROM, &from, -1);
			if(toggled == TRUE)
			{
				newmirrorlist = g_list_prepend(newmirrorlist, strdup(from));
				newmirrorlist = g_list_prepend(newmirrorlist, strdup(name));
			}
			else
			{
				newmirrorlist = g_list_append(newmirrorlist, strdup(name));
				newmirrorlist = g_list_append(newmirrorlist, strdup(from));
			}
		} while(gtk_tree_model_iter_next(model, &iter));
	}	

	updateconfig(fn, newmirrorlist);
	FREE(fn); 
	return(0);
}

GtkWidget *load_help_widget()
{
	GtkWidget *labelhelp = gtk_label_new(_("Select one or more nearly mirrors to speed up package downloading.\nYou can add your own custom mirrors to increase downloadind speed by using suitable button."));
	
	return labelhelp;
}





/* ------------------------------- Pre Network configuration part ------------------------------ */
/* ------------------  TODO : Most of code has been taken from netconf plugin -------------------*/
/* ------------------  TODO : This code can be used for detected down mirrors -------------------*/



static GtkWidget *viewif=NULL;
static GList *iflist=NULL;
static GList *interfaceslist=NULL;

/* profile used do write configuration */
static fwnet_profile_t *newprofile=NULL;

/* Used to timeout a connect */
static jmp_buf timeout_jump;

static GtkWidget *pWindow;

enum
{
	COLUMN_NET_IMAGE,
	COLUMN_NET_NAME,
	COLUMN_NET_DESC,
	COLUMN_NET_TYPE
};

void timeout(int sig)
{
    longjmp( timeout_jump, 1 ) ;
}

int is_connected()
{
	int sControl;
	char *host = "www.frugalware.org";
	int port = 80;
	int timeouttime = 2;
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
		close(sControl);
		return 0;
    } else {
		if (connect(sControl, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
			close(sControl);
			alarm(0);
			return -1;
		}
    }

    /* connected */
	alarm(0);
	close(sControl);
	return 1;
}

int testdhcp(char *iface)
{
	system(g_strdup_printf("ifconfig %s up", iface));
	system(g_strdup_printf("dhcpcd -n -t 2 %s", iface));
	
	if(is_connected() < 1) {
		system(g_strdup_printf("dhcpcd --release %s", iface));
		system(g_strdup_printf("ifconfig %s down", iface));
		return -1;
	}
	
	return 1;
}

GtkWidget *getNettypeCombo()
{
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

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


	for (i = 0; i < 4; i+=2)
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, types[i], 1, types[i+1], -1);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

	return combo;
}

char *ask_nettype()
{
	char *str = NULL;
	GtkTreeIter iter;
	GtkTreeModel *model;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Select network type"),
								GTK_WINDOW(assistant),
								GTK_DIALOG_MODAL,
								GTK_STOCK_OK,GTK_RESPONSE_OK,
								NULL);

	GtkWidget *labelinfo = gtk_label_new(_("Now we need to know how your machine connects to the network.\n"
			"If you have an internal network card and an assigned IP address, gateway, and DNS,\n use 'static' "
			"to enter these values.\n"
			"If your IP address is assigned by a DHCP server (commonly used by cable modem services),\n select 'dhcp'. \n"));

	GtkWidget *combotype = getNettypeCombo();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), combotype, FALSE, FALSE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combotype), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(combotype));
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

	switch(fwife_question(g_strdup_printf(_("Do you want to configure a DSL connexion associated with the interface %s?"), iface)))
	{
		case GTK_RESPONSE_YES:
			break;
		case GTK_RESPONSE_NO:
			return 0;
	}

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Configure DSL connexion"),
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

			if(strcmp(passverify, passwd))
			{
				fwife_error(_("Passwords do not match! Try again."));
				gtk_widget_destroy(pBoite);
				dsl_config(button, data);
				return 0;
			}
			else
			{
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

	for(i=0;i<g_list_length(interfaceslist); i+=2)
	{
		if(!strcmp((char*)g_list_nth_data(interfaceslist, i), iface))
		{
			int retquest = fwife_question(_("This interface has been already configured! Do you want to configure it again?"));
			if(retquest == GTK_RESPONSE_YES) {
				free(g_list_nth_data(interfaceslist, i));
				free(g_list_nth_data(interfaceslist, i+1));
				interfaceslist =  g_list_delete_link (interfaceslist, g_list_nth(interfaceslist, i));
				interfaceslist =  g_list_delete_link (interfaceslist, g_list_nth(interfaceslist, i));
				break;
			} else {
				return -1;
			}
		}
	}

	if((newinterface = (fwnet_interface_t*)malloc(sizeof(fwnet_interface_t))) == NULL)
		return(-1);
	memset(newinterface, 0, sizeof(fwnet_interface_t));

	snprintf(newinterface->name, IF_NAMESIZE, iface);

	nettype = ask_nettype();
	if(nettype == NULL)
		return -1;

	if(strcmp(nettype, "lo"))
	{
		interfaceslist = g_list_append(interfaceslist, strdup(iface));
		interfaceslist = g_list_append(interfaceslist, newinterface);
	}

	if(strcmp(nettype, "lo") && fwnet_is_wireless_device(iface))
	{
		switch(fwife_question(_("It seems that this network card has a wireless extension.\n\nConfigure your wireless now?")))
		{
			case GTK_RESPONSE_YES:
				configure_wireless(newinterface);
				break;
			default:
				break;
		}
	}

	if(!strcmp(nettype, "dhcp"))
	{
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
	}
	else if(!strcmp(nettype, "static"))
	{
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

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewif));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(GTK_TREE_VIEW(viewif)));

	if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_NET_NAME, &nameif, -1);
		GList * elem = g_list_find_custom(interfaceslist, (gconstpointer) nameif, cmp_str);
		gint i = g_list_position(interfaceslist, elem);
		interfaceslist =  g_list_delete_link (interfaceslist, g_list_nth(interfaceslist, i));
		interfaceslist =  g_list_delete_link (interfaceslist, g_list_nth(interfaceslist, i));
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_NET_TYPE, "", -1);
	}

	return 0;
}

int try_autodetect()
{
	int i;
	char *iface;
	int detected = 0;	
	
	for(i=0; i<g_list_length(iflist); i+=2)
	{
		iface = (char*)g_list_nth_data(iflist, i);
		if(testdhcp(iface) == 1) {
			int ret = fwife_question(g_strdup_printf(_("A valid dhcp connection has been dicovered on interface %s. Do you want to use this configuration?"), iface));
			if(ret == GTK_RESPONSE_YES) {
				detected = 1;		
				break;				
			} else {
				system(g_strdup_printf("dhcpcd --release %s", iface));
				system(g_strdup_printf("ifconfig %s down", iface));
			}
		}
	}
			
	return detected;
}

GtkWidget *load_netconf_widget()
{
	GtkWidget *pVBox, *phboxbut;
	GtkWidget *hboxview;
	GtkWidget *info;

	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;

	pVBox = gtk_vbox_new(FALSE, 0);
	phboxbut = gtk_hbox_new(FALSE, 0);
	hboxview = gtk_hbox_new(FALSE, 0);

	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), _("<span face=\"Courier New\"><b>You can configure all network interfaces you need</b></span>"));
	gtk_box_pack_start(GTK_BOX(pVBox), info, FALSE, FALSE, 5);

	store = gtk_list_store_new(5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);

	viewif = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(viewif), TRUE);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes ("", renderer, "pixbuf", COLUMN_NET_IMAGE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Device"), renderer, "text", COLUMN_NET_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Description"), renderer, "text", COLUMN_NET_DESC, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewif), col);

	renderer = gtk_cell_renderer_text_new();
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
	GtkWidget *btnapply = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	gtk_box_pack_start(GTK_BOX(phboxbut), btnapply, FALSE, FALSE, 10);
	
	gtk_box_pack_start(GTK_BOX(hboxview), viewif, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(pVBox), hboxview, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(pVBox), phboxbut, FALSE, FALSE, 5);

	g_signal_connect(G_OBJECT(btnsave), "clicked", G_CALLBACK(add_interface), NULL);
	g_signal_connect(G_OBJECT(btndel), "clicked", G_CALLBACK(del_interface), NULL);
	g_signal_connect(G_OBJECT(btndsl), "clicked", G_CALLBACK(dsl_config), NULL);
	g_signal_connect(G_OBJECT(btnapply), "clicked", G_CALLBACK(post_net_config), NULL);

	return pVBox;
}

int pre_net_config()
{
	int i, detected = 0;
	GdkPixbuf *connectimg;
	GtkWidget *cellview;
	GtkTreeIter iter;

	cellview = gtk_cell_view_new ();
	connectimg = gtk_widget_render_icon (cellview, GTK_STOCK_NETWORK,
					GTK_ICON_SIZE_BUTTON, NULL);

	if(iflist == NULL)
	{
		iflist = fwnet_iflist();

		for(i=0; i<g_list_length(iflist); i+=2)
		{
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter);
			gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewif))), &iter, COLUMN_NET_IMAGE, connectimg,
																				COLUMN_NET_NAME, (char*)g_list_nth_data(iflist, i),
																				COLUMN_NET_DESC, (char*)g_list_nth_data(iflist, i+1),
																				COLUMN_NET_TYPE, "",
																				-1);
		}
	}
	
	free(newprofile);
	if((newprofile = (fwnet_profile_t*)malloc(sizeof(fwnet_profile_t))) == NULL)
		return(1);
	memset(newprofile, 0, sizeof(fwnet_profile_t));
	
	switch(fwife_question(_("Do you want to try an automatic detection of your network configuration?")))
	{
		case GTK_RESPONSE_YES:
			detected = try_autodetect();
			if(detected == 0) {
				fwife_error(_("No valid network configuration have been detected, if you have one configure it manually."));
			}
			break;
		case GTK_RESPONSE_NO:
			break;
	}

	return detected;
}

int post_net_config()
{
	int i;	
	
	if((newprofile = (fwnet_profile_t*)malloc(sizeof(fwnet_profile_t))) == NULL)
		return -1;
	memset(newprofile, 0, sizeof(fwnet_profile_t));
	
	sprintf(newprofile->name, "default");
	for(i = 1; i<g_list_length(interfaceslist); i+=2)
	{
		newprofile->interfaces = g_list_append(newprofile->interfaces, (fwnet_interface_t *) g_list_nth_data(interfaceslist, i));
	}
	char *host = strdup("frugalware.installer");

	fwnet_writeconfig(newprofile, host);

	system("netconfig start");
	
	if(is_connected() < 1) {
		int ret = fwife_question(_("Fwife cannot connect to internet with this configuration, do you want to apply this configuration anyway?"));
		if(ret == GTK_RESPONSE_YES) {
			free(host);
			gtk_widget_destroy(pWindow);
			return 0;
		} else {
			free(newprofile);
			free(host);			
			return 0;
		}
	}
		
	free(host);
	gtk_widget_destroy(pWindow);
	return 0;
}

int run_net_config()
{
	set_page_incompleted();
	pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(pWindow), _("Network Configuration"));
    gtk_window_set_default_size(GTK_WINDOW(pWindow), 320, 200);
	gtk_window_set_position(GTK_WINDOW (pWindow), GTK_WIN_POS_CENTER);
	gtk_window_set_transient_for(GTK_WINDOW (pWindow), GTK_WINDOW (assistant));
	gtk_signal_connect (GTK_OBJECT (pWindow), "destroy", G_CALLBACK(set_page_completed), NULL);
 

    gtk_container_add(GTK_CONTAINER(pWindow), load_netconf_widget());
	
	if(pre_net_config() == 1) {
		gtk_widget_destroy(pWindow);
		return 0;
	}

    gtk_widget_show_all(pWindow);
	
	return 0;   
}
 
