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
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>

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

int run_net_config(GList **config);
int is_connected(char *host, int port, int timeouttime);

enum
{
	COLUMN_STATUS,
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
	free(preferred);
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
							COLUMN_USE, TRUE, COLUMN_NAME, sName, COLUMN_FROM, "CUSTOM", -1);
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
		gchar *from;

		gtk_tree_model_get (model, &iter, COLUMN_FROM, &from, -1);

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

	store = gtk_list_store_new(NUM_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (store);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes (_("Status"), renderer, "pixbuf", COLUMN_STATUS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_toggle_new ();
  	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), model);
  	col = gtk_tree_view_column_new_with_attributes (_("Use"), renderer, "active", COLUMN_USE, NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (col), GTK_TREE_VIEW_COLUMN_FIXED);
  	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col), 50);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Mirrors"), renderer, "text", COLUMN_NAME, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Froms"), renderer, "text", COLUMN_FROM, NULL);
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
						_("<span font=\"11\"><b>You can choose one or more nearly mirrors to speed up package downloading.</b></span>"));

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
	char *fn, *testurl;
	int i;

	if(is_connected("www.google.com", 80, 2) < 1) {
		switch(fwife_question(_("You need an active internet connection ,\n do you want to configure your network now?")))
		{
			case GTK_RESPONSE_YES:
				while(run_net_config(config) == -1) {}
				break;
			case GTK_RESPONSE_NO:
				break;
		}
	}

	if(mirrorlist == NULL) {
		fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);
		mirrorlist = getmirrors(fn);
		free(fn);

		GtkWidget *cellview = gtk_cell_view_new ();
		GdkPixbuf *pixgood = gtk_widget_render_icon(cellview, GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON, NULL);
		GdkPixbuf *pixbad = gtk_widget_render_icon(cellview, GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON, NULL);

		for(i=0; i < g_list_length(mirrorlist); i+=3) {
			gtk_list_store_append(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter);

			testurl = strdup(((char*)g_list_nth_data(mirrorlist, i)) + 6);
			strchr(testurl, '/')[0] = '\0';

			if(is_connected(testurl, 80, 1) < 1) {
				gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter,
											COLUMN_STATUS, pixbad,
											COLUMN_USE, (gboolean)(GPOINTER_TO_INT(g_list_nth_data(mirrorlist, i+2))),
											COLUMN_NAME, (gchar*)g_list_nth_data(mirrorlist, i),
											COLUMN_FROM, (gchar*)g_list_nth_data(mirrorlist, i+1), -1);
			} else {
				gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter,
											COLUMN_STATUS, pixgood,
											COLUMN_USE, (gboolean)(GPOINTER_TO_INT(g_list_nth_data(mirrorlist, i+2))),
											COLUMN_NAME, (gchar*)g_list_nth_data(mirrorlist, i),
											COLUMN_FROM, (gchar*)g_list_nth_data(mirrorlist, i+1), -1);
			}
			free(testurl);
		}
	}
	return 0;
}

int run(GList **config)
{
	char *fn, *name, *from;
	gboolean toggled;
	GList *newmirrorlist = NULL;

	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)viewserver;
  	GtkTreeModel *model = gtk_tree_view_get_model (treeview);

	fn = g_strdup_printf("%s/%s", PACCONFPATH, PACCONF);

	if(gtk_tree_model_get_iter_first (model, &iter) == FALSE) {
		return(0);
	} else {
		do {
			gtk_tree_model_get (model, &iter, COLUMN_USE, &toggled, COLUMN_NAME, &name, COLUMN_FROM, &from, -1);
			if(toggled == TRUE) {
				newmirrorlist = g_list_prepend(newmirrorlist, strdup(from));
				newmirrorlist = g_list_prepend(newmirrorlist, strdup(name));
			} else {
				newmirrorlist = g_list_append(newmirrorlist, strdup(name));
				newmirrorlist = g_list_append(newmirrorlist, strdup(from));
			}
		} while(gtk_tree_model_iter_next(model, &iter));
	}

	updateconfig(fn, newmirrorlist);
	free(fn);
	return(0);
}

GtkWidget *load_help_widget()
{
	GtkWidget *labelhelp = gtk_label_new(_("Select one or more nearly mirrors to speed up package downloading.\nYou can add your own custom mirrors to increase downloadind speed by using suitable button."));

	return labelhelp;
}

/* ----------------------------------------------------------------- Pre Network configuration part ---------------------------------------------------------------- */

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
	char *line = malloc(256);
	size_t len = 0;
	FILE * fp = NULL;
	char *command = NULL;
	struct wifi_ap *ap = NULL;
	char *tok = NULL;

	GList *entrys = NULL;

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

				ap = malloc(sizeof(struct wifi_ap));
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
	GtkWidget *cellview;
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

	cellview = gtk_cell_view_new ();

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

char *ask_nettype()
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
	GtkWidget *butaps = gtk_button_new_with_label(_("Scan for Acess Points"));
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
	char *msgptr = g_strdup_printf(_("Do you want to configure a DSL connexion associated with the interface %s?"), iface);

	switch(fwife_question(msgptr))
	{
		case GTK_RESPONSE_YES:
			break;
		case GTK_RESPONSE_NO:
			return 0;
	}
	free(msgptr);

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

	while(dsl_config(newprofile, interface) == -2) {}

	char *host = strdup("frugalware");
	fwnet_writeconfig(newprofile, host);
	free(host);	

	system("netconfig start");

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
	fwnet_interface_t *newinterface = NULL;

	/* profile used do write configuration */
	fwnet_profile_t *newprofile=NULL;

	if((newprofile = (fwnet_profile_t*)malloc(sizeof(fwnet_profile_t))) == NULL)
		return -1;
	memset(newprofile, 0, sizeof(fwnet_profile_t));

	if((newinterface = (fwnet_interface_t*)malloc(sizeof(fwnet_interface_t))) == NULL)
		return -1;
	memset(newinterface, 0, sizeof(fwnet_interface_t));

	if(select_interface(newinterface) == -1)
		return -1;

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

	if(post_net_config(newprofile, newinterface) == -1) {
		free(newprofile);
		free(newinterface);
		return -1;
	}
	
	/* save network profile for later usage in netconf*/
	data_put(config, "netprofile", newprofile);

	return 0;
}
