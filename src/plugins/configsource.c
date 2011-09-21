/*
 *  configsource.c for Fwife
 *
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <glib.h>
#include <getopt.h>
#include <dirent.h>
#include <libintl.h>

#include "common.h"

static GList *mirrorlist;

static GtkWidget *viewserver;
char *PACCONF;

extern GtkWidget *assistant;

int run_net_config(GList **config);
int run_discs_config(GList **config);
int is_connected(char *host, int port, int timeouttime);

enum
{
	COLUMN_USE,
	COLUMN_STATUS,
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

plugin_t *info(void)
{
	return &plugin;
}

char *desc(void)
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
void add_mirror(GtkWidget *button, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)data;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	char* sName = NULL;

	sName = fwife_entry(_("Add a custom server"),
						_("You may specify a custom mirror (eg. LAN) so you can download packages faster.\nEnter server's address below :")
						, NULL);
	if(sName && strcmp(sName, "")) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
							COLUMN_USE, TRUE, COLUMN_NAME, sName, COLUMN_FROM, "CUSTOM", -1);
	}
	return;
}

/* delete a custom mirror to the list */
void remove_mirror(GtkWidget *widget, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)data;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GtkTreePath *path;
		gchar *from;

		gtk_tree_model_get (model, &iter, COLUMN_FROM, &from, -1);

		/* don't delete default mirrors */
		if(strcmp(from, "CUSTOM")) {
			fwife_error(_("Can't delete default mirrors"));
			return;
		}

		path = gtk_tree_model_get_path (model, &iter);
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		gtk_tree_path_free (path);
	}
	return;
}

/* Create the list of mirrors */
GtkWidget *mirrorview(void)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *view;

	store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_BOOLEAN, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	view = gtk_tree_view_new_with_model(model);
	g_object_unref (store);

	renderer = gtk_cell_renderer_toggle_new ();
  	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled), model);
  	col = gtk_tree_view_column_new_with_attributes (_("Use"), renderer, "active", COLUMN_USE, NULL);
	gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (col), GTK_TREE_VIEW_COLUMN_FIXED);
  	gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (col), 50);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	renderer = gtk_cell_renderer_pixbuf_new();
	col = gtk_tree_view_column_new_with_attributes (_("Status"), renderer, "pixbuf", COLUMN_STATUS, NULL);
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

GtkWidget *load_gtk_widget(void)
{
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;
	GtkWidget *addmirror, *delmirror, *buttonlist;
	GtkWidget *image, *info;

	GtkWidget *pVBox = gtk_vbox_new(FALSE, 0);

	/* top info label */
	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info),
						_("<b>You can choose one or more nearly mirrors to speed up package downloading.</b>"));

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

	// get the branch used
	PACCONF = data_get(*config, "pacconf");

	if(run_discs_config(config) == 1)
		return 0;

	while(run_net_config(config) == -1) {}

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
								COLUMN_USE, (gboolean)(GPOINTER_TO_INT(g_list_nth_data(mirrorlist, i+2))),
								COLUMN_STATUS, pixbad,
								COLUMN_NAME, (gchar*)g_list_nth_data(mirrorlist, i),
								COLUMN_FROM, (gchar*)g_list_nth_data(mirrorlist, i+1), -1);
			} else {
				gtk_list_store_set(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewserver))), &iter,
								COLUMN_USE, (gboolean)(GPOINTER_TO_INT(g_list_nth_data(mirrorlist, i+2))),
								COLUMN_STATUS, pixgood,
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

	if(data_get(*config, "srcdev") != NULL)
		return 0;

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

GtkWidget *load_help_widget(void)
{
	GtkWidget *labelhelp = gtk_label_new(_("Select one or more nearby mirrors to speed up package downloading.\nYou can add your own custom mirrors to increase download speed by using suitable button."));

	return labelhelp;
}
