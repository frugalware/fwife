/*
 *  timeconf.c for Fwife
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwtimeconfig.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libintl.h>
#include <sys/wait.h>

#include "common.h"

#define READZONE "/usr/share/zoneinfo/zone.tab"
#define ZONEFILE "/etc/localtime"

static GtkWidget *drawingmap=NULL;
static GdkPixbuf *image=NULL;
static GtkWidget *timeview=NULL;
static GtkWidget *localcheck=NULL;

static GList *zonetime = NULL;

extern GtkWidget *assistant;

enum
{
	COLUMN_TIME_NAME,
 	COLUMN_TIME_COMMENT
};

plugin_t plugin =
{
	"timeconfig",
	desc,
	60,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	FALSE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Timezone configuration");
}

plugin_t *info(void)
{
	return &plugin;
}

void parselatlong(char *latlong, float *latdec, float *longdec)
{
	int deg, min,sec;
	char *mark = latlong+1;
	while((*mark) != '+' && (*mark) != '-')
	{
		mark++;
		if((*mark) == '\0')
			return;
	}
	int longi = atoi(mark);
	int lati = atoi(latlong); //stop on error (so stop when find '+') -> return latitude
	if((lati>=100000) | (lati<=-100000))
	{
		sec = lati%100;
		min = (lati/100)%100;
		deg = (lati/10000);
	}
	else
	{
		sec = 0;
		min = lati%100;
		deg = lati/100;
	}
	*latdec = ((float)deg)+((float)min/60)+((float)sec/3600);

	if((longi>=1000000) | (longi <=-1000000))
	{
		sec = longi%100;
		min = (longi/100)%100;
		deg = (longi/10000);
	}
	else
	{
		sec = 0;
		min = longi%100;
		deg = longi/100;
	}
	*longdec = ((float)deg)+((float)min/60)+((float)sec/3600);
}

void getcartesiancoords(char *latlong, int *xcoord, int *ycoord)
{
	float x,y;
	float lat, lon;
	float x2 = (float)gdk_pixbuf_get_width(image);
	float y2 = (float)gdk_pixbuf_get_height(image);

	parselatlong(latlong, &lat, &lon);

	x = x2 / 2.0 + (x2 / 2.0) * lon / 180.0;
	y = y2 / 2.0 - (y2 / 2.0) * lat / 90.0;

	*xcoord = (int)x;
	*ycoord = (int)y;
}

//* /!\ Need free memory *//
char* get_country(char *elem)
{
	char* token;
	char *saveptr = NULL;
	char* str = elem;

	token = strtok_r(str, "/", &saveptr);
	return(strdup(token));
}

char* get_city(char *elem)
{
	char *saveptr = NULL;
	char* str = elem;

	strtok_r(str, "/", &saveptr);

	return(strdup(saveptr));
}

GtkTreePath *find_country_path(char *country)
{
	GtkTreeIter iter;
	GtkTreeView *treeview = (GtkTreeView *)timeview;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	char *name;

	if(gtk_tree_model_get_iter_first (model, &iter) == FALSE) {
		return(NULL);
	} else {
		do {
			gtk_tree_model_get (model, &iter, COLUMN_TIME_NAME, &name, -1);
			if(!strcmp(country, name))
				return(gtk_tree_model_get_path(model, &iter));
		} while(gtk_tree_model_iter_next(model, &iter));
	}
	return(NULL);
}

void selection_changed(GtkTreeSelection *selection, gpointer data)
{
	GtkTreeIter iter, iter_parent;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(timeview));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE) {
			set_page_incompleted();
			return;
		} else {
			set_page_completed();
		}
	} else {
		set_page_incompleted();
	}

	gtk_widget_queue_draw(drawingmap);
}

gboolean affiche_dessin(GtkWidget *dessin, GdkEventExpose *event, gpointer data)
{
	GdkColormap *colormap;
	GdkColor couleur_fond;
	int i, xcoord, ycoord;
	char *coords;

	GtkTreeModel *model;
	GtkTreeIter iter, iter_parent;
	char *country, *city;
	GtkTreeSelection *selection;

	colormap = gdk_drawable_get_colormap(dessin->window);

	// set up background color
	couleur_fond.red=0xFFFF;
	couleur_fond.green=0xFFFF;
	couleur_fond.blue=0;

	gdk_colormap_alloc_color(colormap,&couleur_fond,FALSE,FALSE);

	// draw the map image
	gdk_draw_pixbuf(dessin->window, dessin->style->fg_gc[GTK_WIDGET_STATE (dessin)], image, 0, 0, 0, 0,
        gdk_pixbuf_get_width(image), gdk_pixbuf_get_height(image), GDK_RGB_DITHER_MAX, 0, 0);

	gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
			      &couleur_fond);

	if(zonetime) {
		for(i=0; i<g_list_length(zonetime); i+=4) {
			coords = (char*)g_list_nth_data(zonetime, i+1);
			getcartesiancoords(coords, &xcoord, &ycoord);
			gdk_draw_rectangle(drawingmap->window, drawingmap->style->fg_gc[GTK_WIDGET_STATE(drawingmap)], TRUE, xcoord, ycoord, 3, 3);
		}
	}

	couleur_fond.red=0xFFFF;
	couleur_fond.green=0;
	couleur_fond.blue=0;

	gdk_colormap_alloc_color(colormap,&couleur_fond,FALSE,FALSE);

	gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
			      &couleur_fond);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (timeview));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_TIME_NAME, &city, -1);

		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE) {
			gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
					      &dessin->style->black);
			return TRUE;
		} else {
			gtk_tree_model_get (model, &iter_parent, COLUMN_TIME_NAME, &country, -1);
		}

		char *total = g_strdup_printf("%s/%s",country, city);

		gint pos =  g_list_position(zonetime, g_list_find_custom(zonetime, total, cmp_str));
		if(pos>0) {
			getcartesiancoords((char*)g_list_nth_data(zonetime, pos-1), &xcoord, &ycoord);
			gdk_draw_rectangle(drawingmap->window, drawingmap->style->fg_gc[GTK_WIDGET_STATE(drawingmap)], TRUE, xcoord, ycoord, 3, 3);
		}
		free(total);
	}

	// reset default color for text
	gdk_gc_set_foreground(dessin->style->fg_gc[GTK_WIDGET_STATE(dessin)],
			      &dessin->style->black);

	return TRUE;
}

//* Parsing file to get zones, description of each zone, etc
int fwifetime_find(void)
{
    	char ligne[256];
    	FILE * fin;
	char *saveptr = NULL;
	char* token, *mark;
	char *str;
	int j = 0;

	if ((fin = fopen(READZONE, "r")) == NULL)
		return EXIT_FAILURE;

	while (fgets(ligne, sizeof ligne, fin)) {
		if (ligne[0] != '#') {
			for (str = ligne, j=0;; j++,str = NULL) {
				token = strtok_r(str, "\t", &saveptr);
				if(token) {
					mark = token;
					while((*mark) != '\0') {
						mark++;
						if((*mark) == '\n')
							(*mark) = '\0';
					}
					zonetime = g_list_append(zonetime, strdup(token));
				} else if(j<4) {
					zonetime = g_list_append(zonetime, strdup(""));
					break;
				} else {
					break;
				}
			}
		}
	}

    fclose(fin);

    return EXIT_SUCCESS;
}

gboolean resize_cb (GtkWindow *window, GdkEvent *event, gpointer data)
{
	gint decal = (event->configure.width - gdk_pixbuf_get_width(image))/2;
	gtk_box_set_child_packing(GTK_BOX(data), drawingmap, FALSE, TRUE, decal, GTK_PACK_START);
	// continue signal propagation (TRUE for stopping)
	return FALSE;
}

GtkWidget *load_gtk_widget(void)
{
	GtkTreeStore *store;
	GtkTreeModel *model;
	GtkTreeViewColumn *col;
	GtkTreeSelection *selection;
	GtkCellRenderer *renderer;
	GtkWidget *pVbox, *pScrollbar;
	GtkWidget *hboximg;

	pVbox = gtk_vbox_new(FALSE, 5);
	hboximg = gtk_hbox_new(FALSE, 5);

	drawingmap=gtk_drawing_area_new();

	image = gdk_pixbuf_new_from_file(g_strdup_printf("%s/timemap.png", IMAGEDIR), NULL);
	gtk_widget_set_size_request(drawingmap, gdk_pixbuf_get_width(image), gdk_pixbuf_get_height(image));

	g_signal_connect(G_OBJECT(drawingmap),"expose_event", (GCallback)affiche_dessin, NULL);

	// drawing map need to be center manually
	gint width, height;
	gtk_widget_get_size_request(assistant, &width, &height);
	gint decal = (width - gdk_pixbuf_get_width(image))/2;

	gtk_box_pack_start(GTK_BOX(hboximg), drawingmap, FALSE, TRUE, decal);
	gtk_box_pack_start(GTK_BOX(pVbox), hboximg, FALSE, TRUE, 5);

	g_signal_connect(G_OBJECT(assistant),"configure-event", (GCallback)resize_cb, hboximg);

	store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	model = GTK_TREE_MODEL(store);

	timeview = gtk_tree_view_new_with_model(model);
	g_object_unref (model);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(timeview), TRUE);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_TIME_NAME, NULL);
	gtk_tree_view_column_set_title(col, _("Continent/Main City"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);

	col = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_attributes(col, renderer, "text", COLUMN_TIME_COMMENT, NULL);
	gtk_tree_view_column_set_title(col, _("Comments"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(timeview), col);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (timeview));

	g_signal_connect (selection, "changed",  G_CALLBACK (selection_changed), NULL);

	pScrollbar = gtk_scrolled_window_new(NULL, NULL);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScrollbar), timeview);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_box_pack_start(GTK_BOX(pVbox), pScrollbar, TRUE, TRUE, 0);

	localcheck = gtk_check_button_new_with_label(_("Set clock to locale time coordinates"));
	gtk_box_pack_start(GTK_BOX (pVbox), localcheck, FALSE, FALSE, 3);

	return pVbox;
}

int prerun(GList **config)
{
	int i;
	GtkTreeIter iter;
	GtkTreeIter child_iter;
	GtkTreePath *path = NULL;

	GtkTreeView *treeview = (GtkTreeView *)timeview;
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	char *country, *city, *elem;

	fwifetime_find();

	if(zonetime) {
		for(i=0; i < g_list_length(zonetime); i+=4) {
			elem = strdup((char*)g_list_nth_data(zonetime, i+2));
			country = get_country(elem);
			FREE(elem);
			elem = strdup((char*)g_list_nth_data(zonetime, i+2));
			city = get_city(elem);
			FREE(elem);
			if((path = find_country_path(country)) == NULL) {
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter, NULL);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &iter,
					   0, country, -1);
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter,
						0, city,1,(char*)g_list_nth_data(zonetime, i+3), -1);
			} else {
				if(gtk_tree_model_get_iter(model, &iter, path) == FALSE)
					return(-1);
				gtk_tree_store_append(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter, &iter);
				gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(timeview))), &child_iter,
						0, city,1,(char*)g_list_nth_data(zonetime, i+3), -1);
			}
			free(country);
			free(elem);
		}
	}

	return 0;
}

int run(GList **config)
{
	int ret;
	char* city = NULL, *country = NULL, *ptr=NULL;
	GtkTreeIter iter, iter_parent;
  	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(timeview));
  	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(timeview));

	gboolean localchecked = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(localcheck));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_tree_model_get (model, &iter, COLUMN_TIME_NAME, &city, -1);
		if(gtk_tree_model_iter_parent(model, &iter_parent, &iter) == FALSE) {
			return(-1);
		} else {
			gtk_tree_model_get (model, &iter_parent, COLUMN_TIME_NAME, &country, -1);
		}
	}

	pid_t pid = fork();

	if(pid == -1) {
		LOG("Error when forking process in grubconf plugin.");
	} else if(pid == 0) {
		chroot(TARGETDIR);

		if(localchecked == TRUE)
			fwtime_hwclockconf("localtime");
		else
			fwtime_hwclockconf("UTC");

		ptr = g_strdup_printf("/usr/share/zoneinfo/%s/%s",country, city);
		if(ptr)
			symlink(ptr, ZONEFILE);
		free(ptr);
		exit(0);
    	 } else {
		 wait(&ret);
	 }

	return 0;
}

