/*
 *  layout.c for Fwife
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "common.h"

//* Define keymap directories *//
#define XKEYDIR "/usr/share/X11/xkb/symbols"
#define CKEYDIR "/usr/share/keymaps/i386"

//* Use for console keymap *//
static GList *consolekeymap = NULL;

//* Idem for x keymaps *//
static GList *xkeymap = NULL;

//* Main GTK WIdget *//
static GtkWidget *viewlayout = NULL;

extern GtkWidget *assistant;

plugin_t plugin =
{
	"layout",
	desc,
	15,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	NULL,
	run,
	NULL // dlopen handle
};

plugin_t *info(void)
{
	return &plugin;
}

char *desc(void)
{
	return _("Keyboard map selection");
}

int find_languages(char *dirname)
{
	struct dirent *ent = NULL;
	struct stat statbuf;
	char *fn = NULL;
	FILE *fp = NULL;
	char *line = NULL;
	size_t len = 0;
	int i;

	GtkTreeIter iter;
	GtkTreeModel *tree_model;

	/* parsing result */
	char *name = NULL;
	char *layout = NULL;
	char *consolemap = NULL;
	char *variante = NULL;

	MALLOC(line, 128);

	DIR *dir = opendir(dirname);
	if (!dir) {
		perror(dirname);
		return -1;
	}

	while ((ent = readdir(dir)) != NULL) {
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode)) {
			fp = fopen(fn, "r");
			/* skip file is we cannot open it */
			if (fp == NULL)
				continue;

			/* 4 lines in each file */
			for(i = 0; i < 4; i++) {
				line = NULL;
				if(getline(&line, &len, fp) != -1) {
					if(i == 0) {
						line[strlen(line)-1] = '\0';
						name = strdup(line);
					} else if(i == 1) {
						line[strlen(line)-1] = '\0';
						layout = strdup(line);
					} else if(i == 2) {
						line[strlen(line)-1] = '\0';
						consolemap = strdup(line);
					} else	{
						variante = strdup(line);
					}
				}
			}

			/* add the language to the gtk tree if the parsing is correct */
			if(name != NULL && layout != NULL && consolemap != NULL && variante != NULL) {
				tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));
				gtk_tree_store_append(GTK_TREE_STORE(tree_model), &iter, NULL);
				gtk_tree_store_set(GTK_TREE_STORE(tree_model), &iter,
							0, name, 1, layout, 2, variante, 3, consolemap, -1);
			}

			free(name);
			free(layout);
			free(consolemap);
			free(variante);
			free(fn);
			fclose(fp);
		}
	}

	free(line);
	closedir(dir);
	return(0);
}

int find_console_layout(char *dirname)
{
	struct dirent *ent;
	struct stat statbuf;
	char *ext, *fn;

	DIR *dir = opendir(dirname);
	if (!dir) {
		return -1;
	}

	while ((ent = readdir(dir)) != NULL) {
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISDIR(statbuf.st_mode)) {
			if(strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..") && strcmp(ent->d_name, "include"))
				find_console_layout(fn);
		}

		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode) && (ext = strrchr(ent->d_name, '.')) != NULL) {
			if (!strcmp(ext, ".gz"))
				consolekeymap = g_list_append(consolekeymap, g_strdup_printf("%s%s", strrchr(dirname, '/') + 1, fn+strlen(dirname)));
		}
		free(fn);
	}

	closedir(dir);
	return(0);
}

int find_x_layout(char *dirname)
{
	struct dirent *ent;
	struct stat statbuf;
	char *fn;

	DIR *dir = opendir(dirname);
	if (!dir) {
		return -1;
	}
	while ((ent = readdir(dir)) != NULL) {
		fn = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if(!stat(fn, &statbuf) && S_ISREG(statbuf.st_mode) && (strlen(ent->d_name) <= 3))
			xkeymap = g_list_append(xkeymap, strdup(ent->d_name));
		free(fn);
	}
	xkeymap = g_list_sort(xkeymap, cmp_str);

	closedir(dir);
	return(0);
}

void selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));
	char *ptr, *layout, *variante;

	if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		gtk_tree_model_get (model, &iter, 1, &layout, -1);
		gtk_tree_model_get (model, &iter, 2, &variante, -1);

		if(!strcmp(variante, "default")) {
			ptr = g_strdup_printf("setxkbmap -layout %s", layout);
			fw_system(ptr);
			free(ptr);
		} else {
			ptr = g_strdup_printf("setxkbmap -layout %s -variant %s", layout, variante);
			fw_system(ptr);
			free(ptr);
		}
	}
}

/* parse keyboard variants */
void model_changed(GtkWidget *combo, gpointer data)
{
	size_t len = 0;
	FILE * fp = NULL;
	char *file = NULL;
	char *line = NULL;

	MALLOC(line, 256);

	if(strcmp((char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)), "")) {
		file = g_strdup_printf("/usr/share/X11/xkb/symbols/%s", (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)));
		fp = fopen(file, "r");
		/* if opening fail - use default only */
		if(fp == NULL) {
			gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(data))));
			gtk_combo_box_append_text(GTK_COMBO_BOX(data), "default");
			gtk_combo_box_set_active (GTK_COMBO_BOX (data), 0);
			return;
		}

		gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(data))));
		gtk_combo_box_append_text(GTK_COMBO_BOX(data), "default");

		while (getline(&line, &len, fp) != -1) {
			if(strstr(line, "xkb_symbols \"")) {
				strchr(line+13, '"')[0] = '\0';
				gtk_combo_box_append_text(GTK_COMBO_BOX(data), line + 13);
			}
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (data), 0);
		free(file);
	}

	free(line);
	pclose(fp);
	return;
}

/* Dialog box for a personalized keyboard */
void add_keyboard(GtkWidget *button, gpointer data)
{
	GtkWidget* pBoite;
	GtkWidget *pComboCons, *pEntryName, *pComboModel, *pComboVar;
	char* sName, *sCons, *sModel, *sVar;
	GtkWidget *pVBox;
	GtkWidget *pFrame;
	GtkWidget *pVBoxFrame;
	GtkWidget *pLabel;
	GtkWidget *image;
	GtkTreeIter iter;
	int i = 0;

	pBoite = gtk_dialog_new_with_buttons(_("Personalized keyboard"),
					     NULL,
	  					GTK_DIALOG_MODAL,
   						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
   						GTK_STOCK_OK, GTK_RESPONSE_OK,
   						NULL);
	gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);

	pVBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pVBox, FALSE, FALSE, 5);

	char *imgpath = g_strdup_printf("%s/key48.png", IMAGEDIR);
	image = gtk_image_new_from_file(imgpath);
	gtk_box_pack_start(GTK_BOX(pVBox), image, FALSE, FALSE, 0);
	free(imgpath);

	pFrame = gtk_frame_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	pEntryName = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pEntryName, TRUE, TRUE, 0);

	pFrame = gtk_frame_new(_("Console Keymap"));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	/* Combo for console layouts */
	pComboCons = gtk_combo_box_new_text();

	/* add items to combo */
	for(i = 0; i < g_list_length(consolekeymap); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(pComboCons), g_list_nth_data(consolekeymap, i));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(pComboCons), 0);
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pComboCons, TRUE, TRUE, 0);

	pFrame = gtk_frame_new(_("X Keymap (Used only if X server installed)"));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 5);

	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	/* Combo for X keyboards models */
	pLabel = gtk_label_new(_("Model"));
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
	pComboModel = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pComboModel, TRUE, FALSE, 0);

	/* Combo for variants */
	pLabel = gtk_label_new(_("Variant"));
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pLabel, TRUE, FALSE, 0);
	pComboVar = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pComboVar, TRUE, FALSE, 0);

	g_signal_connect (pComboModel, "changed", G_CALLBACK (model_changed),(gpointer)pComboVar);

	/* add items to model combo */
	for(i = 0; i < g_list_length(xkeymap); i++) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(pComboModel), g_list_nth_data(xkeymap, i));
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(pComboModel), 0);

	gtk_widget_show_all(pBoite);

	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			sName = (char*)gtk_entry_get_text(GTK_ENTRY(pEntryName));
			sCons = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pComboCons));
			sModel = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pComboModel));
			sVar = (char*)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pComboVar));

			/* error if name is empty */
			if(!strcmp(sName, "")) {
				fwife_error(_("You must enter a name for this keyboard"));
				gtk_widget_destroy(pBoite);
				break;
			}

			// add the new keyboard to the list
			gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter, NULL);
			gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout))), &iter,
					   0, sName, 1, sModel, 2, sVar, 3, sCons, -1);

			gtk_widget_destroy(pBoite);
			break;
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			gtk_widget_destroy(pBoite);
			break;
	}
}

GtkWidget *load_gtk_widget(void)
{
	GtkTreeStore *store;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkWidget *pScrollbar, *pvbox;
	GtkTreeSelection *selection;

	/* Create a list for keyboards */
	store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	viewlayout = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes (_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_column_set_expand (col, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW (viewlayout), col);

	col = gtk_tree_view_column_new_with_attributes (_("X Layout"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (viewlayout), col);

	col = gtk_tree_view_column_new_with_attributes (_("X Variant"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (viewlayout), col);

	col = gtk_tree_view_column_new_with_attributes (_("Console keymap"), renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW (viewlayout), col);

	/* change tree selection mode */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (viewlayout));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
	g_signal_connect (selection, "changed", G_CALLBACK (selection_changed),NULL);

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), viewlayout);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	pvbox = gtk_vbox_new(FALSE, 5);

	/* top info label */
	GtkWidget *labelhelp = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(labelhelp), _("<b>You may select one of the following keyboard maps.</b>"));
	gtk_box_pack_start(GTK_BOX(pvbox), labelhelp, FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(pvbox), pScrollbar, TRUE, TRUE, 0);

	/* lower label and button for personalized keyboard */
	GtkWidget *hboxbas = gtk_hbox_new(FALSE, 5);
	GtkWidget *entrytest = gtk_entry_new();
	GtkWidget *labeltest = gtk_label_new(_("Test your keyboard here :"));
	GtkWidget *persokeyb = gtk_button_new_with_label(_("Personalized keyboard"));

	char *imgpath = g_strdup_printf("%s/key24.png", IMAGEDIR);
	GtkWidget *image = gtk_image_new_from_file(imgpath);
	free(imgpath);

	gtk_button_set_image(GTK_BUTTON(persokeyb), image);
	g_signal_connect (persokeyb, "clicked", G_CALLBACK (add_keyboard),NULL);

	gtk_box_pack_start(GTK_BOX(hboxbas), labeltest, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxbas), entrytest, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hboxbas), persokeyb, FALSE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(pvbox), hboxbas, FALSE, FALSE, 5);

	/* Load keymaps use for personalized keyboard */
	find_console_layout(CKEYDIR);
	find_x_layout(XKEYDIR);

	/* Loads keymaps from files */
	find_languages(KEYDIR);

	return pvbox;
}

int run(GList **config)
{
	char *fn;
	FILE* fp;
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	char *laycons, *xlayout, *xvariant;

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(viewlayout));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(viewlayout));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_tree_model_get (model, &iter, 1, &xlayout, 2, &xvariant, 3, &laycons, -1);
	}

	/* Create /sysconfig/keymap temporary file */
	fn = strdup("/tmp/setup_XXXXXX");
	mkstemp(fn);
	if ((fp = fopen(fn, "w")) == NULL) {
		perror(_("Could not open output file for writing"));
		return(1);
	}
	fprintf(fp, "# /etc/sysconfig/keymap\n\n"
		"# specify the keyboard map, maps are in "
		"/usr/share/keymaps\n\n");
	if(strstr(laycons, "/"))
		fprintf(fp, "keymap=%s\n", strstr(laycons, "/")+1);

	fclose(fp);

	/* Put data in config */
	data_put(config, "keymap", fn);
	data_put(config, "xlayout", strdup(xlayout));
	data_put(config, "xvariant", strdup(xvariant));

	return 0;
}
