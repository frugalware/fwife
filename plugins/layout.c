/*
 *  layout.c for Fwife
 * 
 *  Copyright (c) 2005 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2008 by Albar Boris <boris.a@cegetel.net>
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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "common.h"

static GList *layoutl = NULL;
static GtkWidget *viewlayout = NULL;

plugin_t plugin =
{
	"layout",
	desc,
	15,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	FALSE,
	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

plugin_t *info()
{
	return &plugin;
}

char *desc()
{
	return _("Keyboard map selection");
}

int find(char *dirname)
{
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;
	char *ext, *fn;

	dir = opendir(dirname);
	if (!dir)
	{
		perror(dirname);
		return(1);
	}
	while ((ent = readdir(dir)) != NULL)
	{
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISDIR(statbuf.st_mode))
			if(strcmp(ent->d_name, ".") &&
				strcmp(ent->d_name, "..") &&
				strcmp(ent->d_name, "include"))
				find(fn);
		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode) &&
			(ext = strrchr(ent->d_name, '.')) != NULL)
			if (!strcmp(ext, ".gz"))
					layoutl = g_list_append(layoutl, g_strdup_printf("%s%s", strrchr(dirname, '/')+1, fn+strlen(dirname)));
	}
	closedir(dir);
	return(0);
}

//* /!\ Need free memory *//
char* get_type(char *elem)
{
	char* token;
	char *saveptr = NULL;
	char* str = elem;
	
	token = strtok_r(str, "/", &saveptr);
	return(strdup(token));
}
	
char* get_layout(char *elem)
{
	char* token;
	char *saveptr = NULL;
	char* str = elem;
	
	token = strtok_r(str, "/", &saveptr);
	
	return(strdup(saveptr));
}

GtkTreePath *find_type_path(char *type)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)viewlayout;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	char *name;

	if(gtk_tree_model_get_iter_first (model, &iter) == FALSE)
	{
		return(NULL);
	}
	else
	{
		do
		{
			gtk_tree_model_get (model, &iter, 0, &name, -1);
			if(!strcmp(type, name))
				return(gtk_tree_model_get_path(model, &iter));
			
		} while(gtk_tree_model_iter_next(model, &iter));
	}
	return(NULL);
}

//* Need to have one selected - disable next button if unselected *//
void selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter, iter_parent;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE)
		{	
			set_page_incompleted();
			return;
		}
		else
		{
			set_page_completed();
		}		
	}
	else
	{
		set_page_incompleted();
	}
}

GtkWidget *load_gtk_widget()
{
	int i;
	char *type = NULL, *layout = NULL, *elem = NULL;
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkTreeIter iter, child_iter;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar, *pvbox;
	GtkTreeSelection *selection;
	GtkTreePath *path = NULL;

	if (layoutl)
	{
		g_list_free(layoutl);
		layoutl = NULL;
	}
	//* Loads keymaps from file *//
	find("/usr/share/keymaps/i386");
	layoutl = g_list_sort(layoutl, cmp_str);
	
	store = gtk_tree_store_new(1, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);
	
	viewlayout = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(viewlayout), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(viewlayout), FALSE);
	
	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_title(col, "Code");
	gtk_tree_view_append_column(GTK_TREE_VIEW(viewlayout), col);	
	
	if(layoutl)
	{
		for(i=0; i < g_list_length(layoutl); i++) 
		{		
			elem = strdup((char*)g_list_nth_data(layoutl, i));
			type = get_type(elem);
			FREE(elem);
			elem = strdup((char*)g_list_nth_data(layoutl, i));
			layout = get_layout(elem);
			FREE(elem);
			if((path = find_type_path(type)) == NULL)
			{
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter, NULL);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter,
					   0, type, -1);
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &child_iter,
						0, layout, -1);
				
			}
			else
			{
				if(gtk_tree_model_get_iter(model, &iter, path) == FALSE)
					LOG("Error when add layout to the treeview");
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &child_iter,
						0, layout, -1);
			}
			FREE(type);
			FREE(layout);
		}
	}

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (viewlayout));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      
        g_signal_connect (selection, "changed", G_CALLBACK (selection_changed),NULL);
	
	pScrollbar = gtk_scrolled_window_new(NULL, NULL);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), viewlayout);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	pvbox = gtk_vbox_new(FALSE, 5);

	GtkWidget *labelhelp = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelhelp), _("<span face=\"Courier New\"><b>You may select one of the following keyboard maps.</b></span>\n"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelhelp, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(pvbox), pScrollbar, TRUE, TRUE, 0);

	return pvbox;
}

int run(GList **config)
{
	char *fn, *ptr;
	FILE* fp;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter, iter_parent;
	char *type, *layout, *selected;
	
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewlayout));	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, 0, &layout, -1);
	  
					
		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE)
		{	
			return(-1);
		}
		else
		{
			gtk_tree_model_get (model, &iter_parent, 0, &type, -1);
		}	
	}

	selected = g_strdup_printf("%s/%s", type, layout);
	FREE(type)
	FREE(layout);
	
	if(strlen(selected) >= 7)
		selected[strlen(selected)-7]='\0';
	
	LOG("selected layout '%s'", selected);
	ptr = g_strdup_printf("loadkeys /usr/share/keymaps/i386/%s.map.gz", selected);
	fw_system(ptr);
	FREE(ptr);

	fn = strdup("/tmp/setup_XXXXXX");
	mkstemp(fn);
	if ((fp = fopen(fn, "w")) == NULL)
	{
		perror(_("Could not open output file for writing"));
		return(1);
	}
	fprintf(fp, "# /etc/sysconfig/keymap\n\n"
		"# specify the keyboard map, maps are in "
		"/usr/share/keymaps\n\n");
	if(strstr(selected, "/"))
		fprintf(fp, "keymap=%s\n", strstr(selected, "/")+1);
	FREE(selected);
	fclose(fp);

	data_put(config, "keymap", fn);

	return 0;
}
 
 
