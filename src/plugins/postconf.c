/*
 *  finish.c for Fwife
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
#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>
#include <libfwutil.h>
#include <libfwmouseconfig.h>
#include <pacman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <libintl.h>
#include <unistd.h>

#include "common.h"

static GtkWidget *pHBoxFrameX = NULL;

static char *xlayout = NULL;
static char *xvariant = NULL;

extern GtkWidget *assistant;

plugin_t plugin =
{
	"configuration",
	desc,
	95,
	load_gtk_widget,
	GTK_ASSISTANT_PAGE_CONTENT,
	TRUE,
	NULL,
	prerun,
	run,
	NULL // dlopen handle
};

char *desc(void)
{
	return _("Configure your system");
}

plugin_t *info(void)
{
	return &plugin;
}

//* --------------------------------------------  Mouse Configuration ----------------------------------------*//

GtkWidget *getMouseCombo(void)
{
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

	char *modes[] =
	{
		"ps2", _("PS/2 port mouse (most desktops and laptops)"),
		"imps2", _("Microsoft PS/2 Intellimouse"),
		"bare", _("2 button Microsoft compatible serial mouse"),
		"ms", _("3 button Microsoft compatible serial mouse"),
		"mman", _("Logitech serial MouseMan and similar devices"),
		"msc", _("MouseSystems serial (most 3 button serial mice)"),
		"pnp", _("Plug and Play (serial mice that do not work with ms)"),
		"usb", _("USB connected mouse"),
		"ms3", _("Microsoft serial Intellimouse"),
		"netmouse", _("Genius Netmouse on PS/2 port"),
		"logi", _("Some serial Logitech devices"),
		"logim", _("Make serial Logitech behave like msc"),
		"atibm", _("ATI XL busmouse (mouse card)"),
		"inportbm", _("Microsoft busmouse (mouse card)"),
		"logibm", _("Logitech busmouse (mouse card)"),
		"ncr", _("A pointing pen (NCR3125) on some laptops"),
		"twid", _("Twiddler keyboard, by HandyKey Corp"),
		"genitizer", _("Genitizer tablet (relative mode)"),
		"js", _("Use a joystick as a mouse"),
		"wacom", _("Wacom serial graphics tablet")
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

	for (i = 0; i < 40; i+=2) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, modes[i], 1, modes[i+1], -1);
	}

	return combo;
}

GtkWidget *getPortCombo(void)
{
	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	gint i;

	char *ports[] =
	{
		"/dev/ttyS0", _("(COM1: under DOS)"),
		"/dev/ttyS1", _("(COM2: under DOS)"),
		"/dev/ttyS2", _("(COM3: under DOS)"),
		"/dev/ttyS3", _("(COM4: under DOS)")
	};

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));
	gtk_widget_set_size_request(combo, 300, 40);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);

	for (i = 0; i < 8; i+=2) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, ports[i], 1, ports[i+1], -1);
	}

	return combo;
}

void change_mouse(GtkComboBox *combo, gpointer data)
{
	char* mouse_type;

	GtkTreeIter iter;
	GtkTreeModel *model;
	gtk_combo_box_get_active_iter(combo, &iter);
	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get (model, &iter, 0, &mouse_type, -1);

	if(!strcmp(mouse_type, "bar") || !strcmp(mouse_type, "ms") || !strcmp(mouse_type, "mman") ||
	!strcmp(mouse_type, "msc") || !strcmp(mouse_type, "genitizer") || !strcmp(mouse_type, "pnp") ||
	!strcmp(mouse_type, "ms3") || !strcmp(mouse_type, "logi") || !strcmp(mouse_type, "logim") ||
	!strcmp(mouse_type, "wacom") || !strcmp(mouse_type, "twid")) {
		g_object_set (GTK_WIDGET(data), "sensitive", TRUE, NULL);
	} else {
		g_object_set (GTK_WIDGET(data), "sensitive", FALSE, NULL);
	}
}

void mouse_config(GtkWidget *button, gpointer data)
{
	char *mouse_type=NULL, *mtype=NULL, *link=NULL, *desc=NULL;
	int ret;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *image;

	if(fwmouse_detect_usb()) {
		mtype=strdup("imps2");
		link=strdup("input/mice");
	} else {
		GtkWidget *pBoite = gtk_dialog_new_with_buttons(_("Mouse configuration"),
        				GTK_WINDOW(assistant),
        				GTK_DIALOG_MODAL,
        				GTK_STOCK_OK,GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
        				NULL);

		image = gtk_image_new_from_file(g_strdup_printf("%s/mouse48.png", IMAGEDIR));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), image, FALSE, FALSE, 0);

		GtkWidget *label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Select your mouse : </b>"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), label, FALSE, FALSE, 5);
		GtkWidget *combomouse = getMouseCombo();
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), combomouse, TRUE, FALSE, 10);
		GtkWidget *label2 = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label2), _("<b>Select mouse's port :</b>"));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), label2, FALSE, FALSE, 5);
		GtkWidget *comboport = getPortCombo();
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), comboport, TRUE, FALSE, 10);
		g_signal_connect(G_OBJECT(combomouse), "changed", G_CALLBACK(change_mouse), comboport);
		gtk_combo_box_set_active (GTK_COMBO_BOX (combomouse), 0);
		gtk_combo_box_set_active (GTK_COMBO_BOX (comboport), 0);

		/* show dialog box */
		gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

		switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
		{
		case GTK_RESPONSE_OK:
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combomouse), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(combomouse));
			gtk_tree_model_get (model, &iter, 0, &mouse_type, 1, &desc, -1);

			if(!strcmp(mouse_type, "bar") || !strcmp(mouse_type, "ms") || !strcmp(mouse_type, "mman") ||
			!strcmp(mouse_type, "msc") || !strcmp(mouse_type, "genitizer") || !strcmp(mouse_type, "pnp") ||
			!strcmp(mouse_type, "ms3") || !strcmp(mouse_type, "logi") || !strcmp(mouse_type, "logim") ||
			!strcmp(mouse_type, "wacom") || !strcmp(mouse_type, "twid")) {
				gtk_combo_box_get_active_iter(GTK_COMBO_BOX(comboport), &iter);
				model = gtk_combo_box_get_model(GTK_COMBO_BOX(comboport));
				gtk_tree_model_get (model, &iter, 0, &link, -1);
				mtype=mouse_type;
				mouse_type=NULL;
			} else if(!strcmp(mouse_type, "ps2")) {
				link = strdup("psaux");
				mtype = strdup("ps2");
			} else if(!strcmp(mouse_type, "ncr")) {
				link = strdup("psaux");
				mtype = strdup("ncr");
			} else if(!strcmp(mouse_type, "imps2")) {
				link = strdup("psaux");
				mtype = strdup("imps2");
			} else if(!strcmp(mouse_type, "logibm")) {
				link = strdup("logibm");
				mtype = strdup("ps2");
			} else if(!strcmp(mouse_type, "atibm")) {
				link = strdup("atibm");
				mtype = strdup("ps2");
			} else if(!strcmp(mouse_type, "inportbm")) {
				link = strdup("inportbm");
				mtype = strdup("bm");
			} else if(!strcmp(mouse_type, "js")) {
				link = strdup("js0");
				mtype = strdup("js");
			} else {
				// usb
				link = strdup("input/mice");
				mtype = strdup("imps2");
			}

			gtk_label_set_text(GTK_LABEL(data), desc);
			break;

		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_NONE:
		default:
			break;
		}
		gtk_widget_destroy(pBoite);
	}

	pid_t pid = fork();

	if(pid == -1) {
		LOG("Error when forking process in mouse config.");
	} else if(pid == 0) {
		chroot(TARGETDIR);

		if(link && mtype)
			fwmouse_writeconfig(link, mtype);
		exit(0);
	} else {
		wait(&ret);
	}

	free(link);
	free(mtype);
}

//*----------------------------------------------------- X Configuration -----------------------------------------------------*//

int write_dms(char *dms)
{
	FILE *fd = fopen("/etc/sysconfig/desktop", "w");

	if(!fd)
		return -1;

	fprintf(fd, "# /etc/sysconfig/desktop\n\n");
	fprintf(fd, "# Which session manager try to use.\n\n");

	if(!strcmp(dms, "KDM"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "desktop=\"/usr/bin/kdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/lxdm\"\n");
	}
	else if(!strcmp(dms, "GDM"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/lxdm\"\n");
	}
	else if(!strcmp(dms, "Slim"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/lxdm\"\n");
	}
	else if(!strcmp(dms, "Lxdm"))
	{
		fprintf(fd, "#desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
		fprintf(fd, "desktop=\"/usr/sbin/lxdm\"\n");
	}
	else // default : XDM
	{
		fprintf(fd, "desktop=\"/usr/bin/xdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/slim\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/gdm --nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/bin/kdm -nodaemon\"\n");
		fprintf(fd, "#desktop=\"/usr/sbin/lxdm\"\n");
	}

	fflush(fd);
	fclose(fd);

	return 0;
}

void checkdms(GtkListStore *store)
{
	PM_DB *db;
	GtkTreeIter iter;

	if(pacman_initialize(TARGETDIR)==-1)
		return;
	if(!(db = pacman_db_register("local")))
	{
		pacman_release();
		return;
	}

	if(pacman_db_readpkg(db, "kdm"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "KDM", 1, _("  KDE Display Manager"), -1);
	}
	if(pacman_db_readpkg(db, "gdm"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "GDM", 1, _("  Gnome Display Manager"), -1);
	}
	if(pacman_db_readpkg(db, "slim"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "Slim", 1, _("  Simple Login Manager"), -1);
	}
	if(pacman_db_readpkg(db, "lxdm"))
	{
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, "Lxdm", 1, _("  LXDE Display Manager"), -1);
	}
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, "XDM", 1, _("  X Window Display Manager"), -1);

	pacman_db_unregister(db);
	pacman_release();
	return;
}

void write_evdev_conf(char *layout, char *variant)
{
	char *path, *ptr;
	path = g_strdup_printf("%s/etc/X11/xorg.conf.d/10-evdev.conf", TARGETDIR);

	FILE *file = fopen(path, "w");
	if(file == NULL)
		return;

	fprintf(file, "Section \"InputClass\"\n");
	fprintf(file, "        Identifier \"evdev keyboard catchall\"\n");
	fprintf(file, "        MatchIsKeyboard \"on\"\n");
        fprintf(file, "        MatchDevicePath \"/dev/input/event*\"\n");
	fprintf(file, "        Driver \"evdev\"\n");
	ptr = g_strdup_printf("        Option \"XkbLayout\" \"%s\"\n", layout);
        fprintf(file, ptr);
	free(ptr);
	ptr = g_strdup_printf("        Option \"XkbVariant\" \"%s\"\n", variant);
	fprintf(file, ptr);
	free(ptr);
	fprintf(file, "EndSection\n");
	fclose(file);
	free(path);
}

void x_config(GtkWidget *button, gpointer data)
{
	GtkWidget* pBoite;
  	char *sDms;

	GtkWidget *pVBox;
	GtkWidget *pFrame;
	GtkWidget *pVBoxFrame;
	GtkWidget *image;

	GtkWidget *combo;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeModel *model;

	pBoite = gtk_dialog_new_with_buttons(_("Configuring X11"),
						NULL,
						GTK_DIALOG_MODAL,
						GTK_STOCK_OK,GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
						NULL);
	gtk_window_set_transient_for(GTK_WINDOW(pBoite), GTK_WINDOW(assistant));
	gtk_window_set_position(GTK_WINDOW(pBoite), GTK_WIN_POS_CENTER_ON_PARENT);

	pVBox = gtk_vbox_new(TRUE, 0);
   	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pVBox, TRUE, FALSE, 5);

	image = gtk_image_new_from_file(g_strdup_printf("%s/xorg48.png", IMAGEDIR));
	gtk_box_pack_start(GTK_BOX(pVBox), image, FALSE, FALSE, 0);

	pFrame = gtk_frame_new(_("Select your default display manager : "));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, TRUE, FALSE, 0);

	pVBoxFrame = gtk_vbox_new(TRUE, 0);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
	g_object_unref (GTK_TREE_MODEL (store));

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 0, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,"text", 1, NULL);

	//* Find Desktop managers installed *//
	checkdms(store);
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

	gtk_box_pack_start(GTK_BOX(pVBoxFrame), combo, TRUE, FALSE, 10);

	gtk_widget_show_all(pBoite);

	//* Run dialog box *//
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		case GTK_RESPONSE_OK:
			gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter);
			model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
			gtk_tree_model_get (model, &iter, 0, &sDms, -1);

			pid_t pid = fork();

			if(pid == -1) {
				LOG("Error when forking process (X11 config)");
			} else if (pid == 0) {
				chroot(TARGETDIR);

				//* create /sysconfig/desktop file *//
				write_dms(sDms);
				exit(0);
			} else {
				wait(NULL);

				//* change keyboard localisation *//
				write_evdev_conf(xlayout, xvariant);

				gtk_widget_destroy(pBoite);
				gtk_label_set_text(GTK_LABEL(data), "Done");
			}
           		break;
        	case GTK_RESPONSE_CANCEL:
        	case GTK_RESPONSE_NONE:
        	default:
			gtk_widget_destroy(pBoite);
			break;
    	}
	return;
}

GtkWidget *load_gtk_widget()
{
	GtkWidget *pVBox;
	GtkWidget *pFrame;
	GtkWidget *pHBoxFrame, *pVBoxFrame;
	GtkWidget *pLabelMouse, *pLabelMouseStatus, *pButtonMouse;
	GtkWidget *pLabelX, *pLabelXStatus, *pButtonX;
	GtkWidget *info;

	pVBox = gtk_vbox_new(FALSE, 0);

	info = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(info), _("<b>You can configure some parts of your system</b>"));
	gtk_box_pack_start(GTK_BOX(pVBox), info, FALSE, FALSE, 6);

	pFrame = gtk_frame_new(_("Hardware"));
	gtk_box_pack_start(GTK_BOX(pVBox), pFrame, FALSE, FALSE, 5);

	pVBoxFrame = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(pFrame), pVBoxFrame);

	//* Mouse configuration button *//
	pHBoxFrame = gtk_hbox_new(FALSE, 5);
	pLabelMouse = gtk_label_new(_("Mouse - "));
	pLabelMouseStatus = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(pLabelMouseStatus), _("<span foreground=\"red\"> Not Configured </span>"));

	gtk_box_pack_start(GTK_BOX(pHBoxFrame), pLabelMouse, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pHBoxFrame), pLabelMouseStatus, FALSE, TRUE, 0);

	pButtonMouse = gtk_button_new_with_label(_("Configure"));
	g_signal_connect(G_OBJECT(pButtonMouse), "clicked", G_CALLBACK(mouse_config), pLabelMouseStatus);
	gtk_box_pack_start(GTK_BOX(pHBoxFrame), pButtonMouse, TRUE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pHBoxFrame, TRUE, FALSE, 5);

	//* X configuration *//
	pHBoxFrameX = gtk_hbox_new(FALSE, 5);
	pLabelX = gtk_label_new(_("X server - "));
	pLabelXStatus = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(pLabelXStatus), _("<span foreground=\"red\"> Not Configured </span>"));

	gtk_box_pack_start(GTK_BOX(pHBoxFrameX), pLabelX, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pHBoxFrameX), pLabelXStatus, FALSE, TRUE, 0);

	pButtonX = gtk_button_new_with_label(_("Configure"));
	g_signal_connect(G_OBJECT(pButtonX), "clicked", G_CALLBACK(x_config), pLabelXStatus);
	gtk_box_pack_start(GTK_BOX(pHBoxFrameX), pButtonX, TRUE, FALSE, 0);
	gtk_widget_set_sensitive(pHBoxFrameX, FALSE);

	gtk_box_pack_start(GTK_BOX(pVBoxFrame), pHBoxFrameX, TRUE, FALSE, 5);

	return pVBox;
}

int prerun(GList **config)
{
	struct stat buf;
	char *ptr;

	//* disable x configuration if no x server detected *//
	ptr = g_strdup_printf("%s/usr/bin/Xorg", TARGETDIR);
	if(!stat(ptr, &buf)) {
		gtk_widget_set_sensitive(pHBoxFrameX, TRUE);
		gtk_widget_queue_draw(pHBoxFrameX);
		xlayout = (char*)data_get(*config, "xlayout");
		xvariant = (char*)data_get(*config, "xvariant");
	}

	// configure kernel modules
	ptr = g_strdup_printf("chroot %s /sbin/depmod -a", TARGETDIR);
	fw_system(ptr);
	free(ptr);

	return 0;
}

int run(GList **config)
{
	return 0;
}
