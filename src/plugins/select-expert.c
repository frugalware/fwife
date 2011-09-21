/*
 *  select-expert.c for Fwife
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

static GtkWidget *categories;
static GtkWidget *package_list;
static GtkWidget *package_info;

extern GList *syncs;
extern GList *all_packages;
extern GList *packages_current;
extern GList *cats;

enum
{
	USE_COLUMN,
 	CAT_COLUMN,
  	SIZE_COLUMN
};

//* A category as been toggled *//
static void fixed_toggled_cat (GtkCellRendererToggle *cell,gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean fixed;
	gchar *name;

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &fixed, -1);
	gtk_tree_model_get (model, &iter, 1, &name, -1);

	gint i = gtk_tree_path_get_indices (path)[0];

	/* do something with the value */
	fixed ^= 1;

	if(fixed == FALSE) {
		GList *elem = g_list_nth(cats, i*3+2);
		elem->data = GINT_TO_POINTER(0);
		g_object_set (package_list, "sensitive", FALSE, NULL);
	} else {
		GList *elem = g_list_nth(cats, i*3+2);
		elem->data = GINT_TO_POINTER(1);
		g_object_set (package_list, "sensitive", TRUE, NULL);
	}

	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, fixed, -1);

	/* clean up */
	gtk_tree_path_free (path);
}

//* Packet toggled *//
static void fixed_toggled_pack (GtkCellRendererToggle *cell,gchar *path_str, gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean fixed;
	gchar *name;
	gint i;

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &fixed, -1);
	gtk_tree_model_get (model, &iter, 1, &name, -1);

	/* do something with the value */
	fixed ^= 1;

	i = gtk_tree_path_get_indices (path)[0];

	GList *elem = g_list_nth(packages_current, i*4+3);

	if(fixed == TRUE)
		elem->data = GINT_TO_POINTER(1);
	else
		elem->data = GINT_TO_POINTER(0);

	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, fixed, -1);

	/* clean up */
	gtk_tree_path_free (path);
}

//* Packet selection have changed, update description *//
void package_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *selected;
	gint i;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		i = gtk_tree_path_get_indices (path)[0];
		gtk_tree_model_get (model, &iter, 1, &selected, -1);
		gtk_label_set_label(GTK_LABEL(package_info), (char*)g_list_nth_data(packages_current, i*4+2));
	}
}

GtkWidget *get_package_list(void)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;

	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);

	package_list = gtk_tree_view_new_with_model(model);
	g_object_unref (model);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (fixed_toggled_pack), model);
	col = gtk_tree_view_column_new_with_attributes (_("Install?"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(package_list), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Package Name"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(package_list), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Size"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(package_list), col);

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW(package_list)), GTK_SELECTION_SINGLE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (package_list));

	g_signal_connect (selection, "changed", G_CALLBACK (package_changed), NULL);

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), package_list);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	return pScrollbar;
}

void category_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *selected;
	gboolean checked = FALSE;
	int i;
	gtk_label_set_label(GTK_LABEL(package_info), "");

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 1, &selected, -1);
		gtk_tree_model_get (model, &iter, 0, &checked, -1);
		if(all_packages)
			packages_current = (GList*)data_get(all_packages, selected);
		if(packages_current)
		{
			gtk_list_store_clear(
				GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(package_list)))
			);

			for(i=0; i < g_list_length(packages_current); i+=4 ) {
				gtk_list_store_append(
					GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(package_list))),
					&iter
				);

				gtk_list_store_set(
					GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(package_list))),
					&iter,
					0, (gboolean)GPOINTER_TO_INT(g_list_nth_data(packages_current, i+3)),
					1, (char*)g_list_nth_data(packages_current, i),
					2, (char*)g_list_nth_data(packages_current, i+1), -1
				);
			}
		}

		/* disable package selection if category unselected */
		if(checked == FALSE)
			g_object_set (package_list, "sensitive", FALSE, NULL);
		else
			g_object_set (package_list, "sensitive", TRUE, NULL);
	}
}

void selection_clicked(GtkWidget *button, gpointer boolselect)
{
	int i;
	GtkTreeIter iter;

	if(packages_current) {
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(package_list))));

		for(i = 0; i < g_list_length(packages_current); i += 4)
		{
			g_list_nth(packages_current, i+3)->data = boolselect;
			gtk_list_store_append(
				GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(package_list))),
				&iter);
			gtk_list_store_set(
				GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(package_list))),
				&iter,
				0, (gboolean)GPOINTER_TO_INT(boolselect),
				1, (char*)g_list_nth_data(packages_current, i),
				2, (char*)g_list_nth_data(packages_current, i+1), -1);
		}
	}
}

//* get a gtk tree list for categories *//
GtkWidget *get_categories_list(void)
{
	GtkListStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar;
	GtkTreeSelection *selection;

	store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);

	categories = gtk_tree_view_new_with_model(model);
	g_object_unref(model);

	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect(renderer, "toggled", G_CALLBACK (fixed_toggled_cat), model);
	col = gtk_tree_view_column_new_with_attributes(
		_("Install?"), renderer, "active", USE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(
		_("Groups"), renderer, "text", CAT_COLUMN, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes(
		_("Size"), renderer, "text", SIZE_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(categories), col);

	gtk_tree_selection_set_mode(
		gtk_tree_view_get_selection(GTK_TREE_VIEW(categories)),
		GTK_SELECTION_SINGLE
	);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(categories));
	g_signal_connect(selection, "changed", G_CALLBACK (category_changed), NULL);

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(pScrollbar),
		categories
	);
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(pScrollbar),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC
	);

	return pScrollbar;
}

GtkWidget *get_expert_mode_widget(void)
{
	GtkWidget *hsepa1, *hsepa2;
	GtkWidget *image;

	GtkWidget *phbox = gtk_hbox_new(TRUE,8);

	/* Groups list */
	GtkWidget *pvbox = gtk_vbox_new(FALSE,5);

	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Choose groups you want to install :</b>"));

	GtkWidget *categl = get_categories_list();

	gtk_box_pack_start(GTK_BOX(pvbox), label, FALSE, TRUE, 7);
	gtk_box_pack_start(GTK_BOX(pvbox), categl, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(phbox), pvbox, TRUE, TRUE, 0);

	/* Packages list */
	GtkWidget *pkglist = get_package_list();
	GtkWidget *phbox2 = gtk_hbox_new(FALSE,8);
	pvbox = gtk_vbox_new(FALSE,5);

	//* Two button selecting packages *//
	GtkWidget *hboxbuttons = gtk_hbox_new(TRUE,5);
	GtkWidget *buttonselect = gtk_button_new_with_label(_("Select all"));
	GtkWidget *buttonunselect = gtk_button_new_with_label(_("Unselect all"));
	g_signal_connect(buttonselect, "clicked", G_CALLBACK (selection_clicked), GINT_TO_POINTER(1));
	g_signal_connect(buttonunselect, "clicked", G_CALLBACK (selection_clicked), GINT_TO_POINTER(0));
	gtk_box_pack_start(GTK_BOX(hboxbuttons), buttonselect, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxbuttons), buttonunselect, FALSE, TRUE, 0);

	//* Two separators for package description *//
	hsepa1 = gtk_hseparator_new();
	hsepa2 = gtk_hseparator_new();

	// description of each package
	package_info = gtk_label_new(NULL);
	gtk_label_set_justify(GTK_LABEL(package_info), GTK_JUSTIFY_LEFT);
	// load a nice image
	image = gtk_image_new_from_file(g_strdup_printf("%s/package.png", IMAGEDIR));
	gtk_label_set_line_wrap(GTK_LABEL(package_info), TRUE);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Choose packages into selected group :</b>"));

	gtk_box_pack_start(GTK_BOX(pvbox), label, FALSE, TRUE, 7);
	gtk_box_pack_start(GTK_BOX(pvbox), pkglist, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxbuttons, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hsepa1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(phbox2), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(phbox2), package_info, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), phbox2, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pvbox), hsepa2, FALSE, FALSE, 0);

	//* Put the two box into one big *//
	gtk_box_pack_start(GTK_BOX(phbox), pvbox, TRUE, TRUE, 0);

	return phbox;
}

void run_expert_mode(void)
{
		int j;
		GtkTreeIter iter;

		gtk_list_store_clear(
			GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories)))
		);

		// add cats in treeview list
		for(j = 0; j < g_list_length(cats); j += 3) {
			gtk_list_store_append(
				GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))),
				&iter
			);

			gtk_list_store_set(
				GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(categories))),
				&iter,
				0, (gboolean)GPOINTER_TO_INT(g_list_nth_data(cats, j+2)),
				1, (char*)g_list_nth_data(cats, j),
				2, (char*)g_list_nth_data(cats, j+1), -1
			);
		}
}