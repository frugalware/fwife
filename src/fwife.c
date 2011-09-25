/*
 *  fwife.c for Fwife
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

#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>
#include "fwife.h"
#include "fwife-message.h"

/* list of all loaded plugins */
GList *plugin_list = NULL;

/* used by plugins for get/put config */
GList *config = NULL;

/* current loaded plugin */
plugin_t *plugin_active = NULL;

/* Widget for the assistant */
GtkWidget *assistant = NULL;

/* Pages data */
PageInfo *pages = NULL;

/* Function used for sort plugins with priorities */
int sort_plugins(gconstpointer a, gconstpointer b)
{
	const plugin_t *pa = a;
	const plugin_t *pb = b;
	return (pa->priority - pb->priority);
}

/* Open a file plugin and add it into list */
int fwife_add_plugins(char *filename)
{
	void *handle;
	void *(*infop) (void);

	if ((handle = dlopen(filename, RTLD_NOW)) == NULL)
	{
		fprintf(stderr, "%s", dlerror());
		return(1);
	}

	if ((infop = dlsym(handle, "info")) == NULL)
	{
		fprintf(stderr, "%s", dlerror());
		return(1);
	}
	plugin_t *plugin = infop();
	plugin->handle = handle;
	plugin_list = g_list_append(plugin_list, plugin);

	return(0);
}

/* Find dynamic plugins and add them */
int fwife_load_plugins(char *dirname)
{
	char *filename, *ext;
	DIR *dir;
	struct dirent *ent;
	struct stat statbuf;

	dir = opendir(dirname);
	if (!dir)
	{
		perror(dirname);
		return(1);
	}
	/* remember that asklang is now special plugin */
	while ((ent = readdir(dir)) != NULL)
	{
		filename = g_strdup_printf("%s/%s", dirname, ent->d_name);
		if (!stat(filename, &statbuf) && S_ISREG(statbuf.st_mode) &&
				(ext = strrchr(ent->d_name, '.')) != NULL)
			if (!strcmp(ext, SHARED_LIB_EXT) && strcmp(ent->d_name, "asklang.so"))
				fwife_add_plugins(filename);
		free(filename);

	}
	closedir(dir);
	return(0);
}

/* Unload dynamic plugins */
int cleanup_plugins(void)
{
	int i;
	plugin_t *plugin;

	for (i=0; i<g_list_length(plugin_list); i++)
	{
		plugin = g_list_nth_data(plugin_list, i);
		dlclose(plugin->handle);
	}
	return(0);
}

/* Quit fwife */
void fwife_exit(void)
{
	char *ptr;

	/* unmout system directories */
	ptr = g_strdup_printf("umount %s/sys", TARGETDIR);
	fw_system(ptr);
	free(ptr);
	ptr = g_strdup_printf("umount %s/proc", TARGETDIR);
	fw_system(ptr);
	free(ptr);
	ptr = g_strdup_printf("umount %s/dev", TARGETDIR);
	fw_system(ptr);
	free(ptr);
	ptr = g_strdup_printf("umount %s", TARGETDIR);
	fw_system(ptr);
	free(ptr);

	free(pages);
   	cleanup_plugins();
   	gtk_main_quit();
}

/* Dialog box for quit the installation */
void cancel_install(GtkWidget *w, gpointer user_data)
{
	int result = fwife_question(_("Are you sure you want to exit from the installer?\n"));
	if(result == GTK_RESPONSE_YES) {
		fwife_exit();
	}
	return;
}

/* Finish the installation and quit */
void close_install(GtkWidget *w, gpointer user_data)
{
	fwife_info(_("Frugalware installation completed.\n You can now reboot your computer"));
	plugin_active->run(&config);
	fwife_exit();

	return;
}

/* Load next plugin */
int plugin_next(GtkWidget *w, gpointer user_data)
{
	int plugin_number = g_list_index(plugin_list, (gconstpointer)plugin_active) + 1;
	gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), plugin_number - 1);
	int ret = plugin_active->run(&config);
	if( ret == -1) {
		LOG("Error when running plugin %s\n", plugin_active->name);
		fwife_error(_("Error when running plugin. Please report"));
		fwife_exit();
		return -1;
	} else if(ret == 1) {
		return -1;
	}
	gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), plugin_number);

	/* load next plugin an call his prerun function */
	plugin_active = g_list_nth_data(plugin_list, plugin_number);

	if(plugin_active->prerun) {
		if(plugin_active->prerun(&config) == -1) {
			LOG("Error when running plugin %s\n", plugin_active->name);
			fwife_error(_("Error when running plugin. Please report"));
			fwife_exit();
			return -1;
		}
	}
	return 0;
}

/* load previous plugin */
int plugin_previous(GtkWidget *w, gpointer user_data)
{
	int plugin_number = g_list_index(plugin_list, (gconstpointer)plugin_active) - 1;
	plugin_active = g_list_nth_data(plugin_list, plugin_number);
	gtk_assistant_set_current_page(GTK_ASSISTANT(assistant), plugin_number);

	/* call prerun when back to a previous plugin */
	if(plugin_active->prerun) {
		if(plugin_active->prerun(&config) == -1) {
			LOG("Error when running plugin %s\n", plugin_active->name);
			fwife_error(_("Error when running plugin. Please report"));
			fwife_exit();
			return -1;
		}
	}

	return 0;
}

/* Go to next plugin in special case */
int skip_to_next_plugin(void)
{
	return plugin_next(NULL,NULL);
}

/* Load next plugin */
int show_help(GtkWidget *w, gpointer user_data)
{
	if((plugin_active->load_help_widget) != NULL)
	{
		GtkWidget *helpwidget = plugin_active->load_help_widget();

		GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Plugin help"),
								GTK_WINDOW(assistant),
								GTK_DIALOG_MODAL,
								GTK_STOCK_OK,GTK_RESPONSE_OK,
								NULL);

		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), helpwidget, TRUE, TRUE, 5);
		gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);
		gtk_dialog_run(GTK_DIALOG(pBoite));
		gtk_widget_destroy(pBoite);
	}
	else
	{
		fwife_error(_("No help available for this plugin"));
	}

	return 0;
}

/* Asklang is now a special plugin loaded before all others plugins */
int ask_language(int width, int height)
{
	void *handle;
	void *(*infop) (void);

	char *ptr = g_strdup_printf("%s/%s", PLUGINDIR, "asklang.so");

	if((handle = dlopen(ptr, RTLD_NOW)) == NULL) {
		fprintf(stderr, "%s", dlerror());
		FREE(ptr);
		return(1);
	}
	free(ptr);

	if ((infop = dlsym(handle, "info")) == NULL) {
		fprintf(stderr, "%s", dlerror());
		return(1);
	}

	plugin_t *pluginlang = infop();
	pluginlang->handle = handle;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Language selection"),
							NULL,
							GTK_DIALOG_MODAL,
							GTK_STOCK_OK,GTK_RESPONSE_OK,
							NULL);
	gtk_widget_set_size_request(pBoite, width, height);
	gtk_window_set_deletable(GTK_WINDOW(pBoite), FALSE );
	gtk_window_set_position(GTK_WINDOW (pBoite), GTK_WIN_POS_CENTER);

	ptr = g_strdup_printf("%s/headlogo.png", IMAGEDIR);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (ptr, NULL);
	free(ptr);

	if(!pixbuf) {
		LOG("warning : can't load image headlogo.png");
	}
	else {
		/* Set it as window icon */
		gtk_window_set_icon(GTK_WINDOW(pBoite),pixbuf);
	}
	g_object_unref(pixbuf);

	GtkWidget *langtab = pluginlang->load_gtk_widget();
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), langtab, TRUE, TRUE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	gtk_dialog_run(GTK_DIALOG(pBoite));
	pluginlang->run(&config);
	gtk_widget_destroy(pBoite);
	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	char *ptr;
	GError *gerror = NULL;
	GdkColor color;
	plugin_t *plugin;
	int width, height;

	gtk_init (&argc, &argv);

	if (getuid() != 0)
	{
		fwife_error(_("Insuffecient privileges. Fwife must be run as root."));
		return 1;
	}

	GdkScreen *screen = gdk_screen_get_default();
	int screenwidth = gdk_screen_get_width(screen);
	int screenheight = gdk_screen_get_height(screen);

	if(screenwidth < 800)
		width = screenwidth - 20;
	else
		width = 800;

	if(screenheight < 600)
		height = screenheight - 20;
	else
		height = 600;

	ask_language(width, height);

	/* Create a new assistant widget with no pages. */
	assistant = gtk_assistant_new();
	gtk_widget_set_size_request(assistant, width, height);
	gtk_window_set_title(GTK_WINDOW (assistant), _("Fwife : Frugalware Installer Front-End"));
	gtk_window_set_position(GTK_WINDOW (assistant), GTK_WIN_POS_CENTER);

	/* Connect signals with functions */
	g_signal_connect(G_OBJECT(assistant), "destroy", G_CALLBACK (cancel_install), NULL);
	g_signal_connect(G_OBJECT(assistant), "cancel", G_CALLBACK (cancel_install), NULL);
	g_signal_connect(G_OBJECT(assistant), "close", G_CALLBACK (close_install), NULL);

	/* Some trick to connect buttons with plugin move functions */
	g_signal_connect(G_OBJECT(((GtkAssistant *) assistant)->forward), "clicked", G_CALLBACK(plugin_next), NULL);
	g_signal_connect(G_OBJECT(((GtkAssistant *) assistant)->back), "clicked", G_CALLBACK(plugin_previous), NULL);

	/* Trick to remove 'last' button (add some GtkWarnings) */
	gtk_assistant_remove_action_widget(GTK_ASSISTANT(assistant), (GtkWidget*)(((GtkAssistant *) assistant)->last));

	/* Add a new help button */
	GtkWidget *help = gtk_button_new_from_stock(GTK_STOCK_HELP);
	gtk_widget_show(help);
	g_signal_connect (G_OBJECT (help), "clicked", G_CALLBACK(show_help), NULL);
	gtk_assistant_add_action_widget(GTK_ASSISTANT(assistant), help);

	/* Load a nice image */
	ptr = g_strdup_printf("%s/headlogo.png", IMAGEDIR);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (ptr, &gerror);
	free(ptr);

	if(!pixbuf) {
		LOG("warning : %s", gerror->message);
	}

	/* Set it as icon window */
	gtk_window_set_icon(GTK_WINDOW (assistant),pixbuf);

	/* Make ugly colors ;) */
	gdk_color_parse ("#94b6db", &color);
	gtk_widget_modify_bg (assistant, GTK_STATE_SELECTED, &color);
	/*gdk_color_parse ("black", &color);
	gtk_widget_modify_fg (assistant, GTK_STATE_SELECTED, &color);*/

	/* Load plugins.... */
	fwife_load_plugins(PLUGINDIR);

	plugin_list = g_list_sort(plugin_list, sort_plugins);

	/* Allocate memory for stocking pages */
	MALLOC(pages, sizeof(PageInfo)*g_list_length(plugin_list));

	/* Load data of all pages... */
	for(i=0; i<g_list_length(plugin_list); i++)
	{
		if((plugin = g_list_nth_data(plugin_list, i)) == NULL) {
			LOG("Error when loading plugins...");
		}
		else {
        	LOG("Loading plugin : %s", plugin->name);
        	pages[i] = (PageInfo) {NULL, -1, plugin->desc(), plugin->type, plugin->complete};

			if((pages[i].widget = plugin->load_gtk_widget()) == NULL)
				LOG("Error when loading plugin's widget @ %s", plugin->name);
		}

		pages[i].index = gtk_assistant_append_page (GTK_ASSISTANT (assistant), pages[i].widget);
		gtk_assistant_set_page_title (GTK_ASSISTANT (assistant), pages[i].widget, pages[i].title);
		gtk_assistant_set_page_type (GTK_ASSISTANT (assistant), pages[i].widget, pages[i].type);
		gtk_assistant_set_page_complete (GTK_ASSISTANT (assistant), pages[i].widget, pages[i].complete);
		gtk_assistant_set_page_header_image(GTK_ASSISTANT (assistant), pages[i].widget,pixbuf);
	}

	/* Begin with first plugin */
	plugin_active = g_list_nth_data(plugin_list, 0);

	gtk_widget_show_all (assistant);
	/* begin event loop */
	gtk_main ();

	return 0;
}

/* Set current page completed : grant access to the next page */
void set_page_completed(void)
{
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),  gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant),
					gtk_assistant_get_current_page(GTK_ASSISTANT (assistant))), TRUE);
}

/* Set current page incompleted : deny access to the next page */
void set_page_incompleted(void)
{
	gtk_assistant_set_page_complete(GTK_ASSISTANT(assistant),  gtk_assistant_get_nth_page(GTK_ASSISTANT(assistant),
					gtk_assistant_get_current_page(GTK_ASSISTANT (assistant))), FALSE);
}

