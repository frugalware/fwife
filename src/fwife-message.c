/*
 *  fwife.c for Fwife
 *
 *  Copyright (c) 2009,2010 by Albar Boris <boris.a@cegetel.net>
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
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include "fwife.h"
#include "fwife-message.h"

extern GtkWidget *assistant;

/* Just an error */
void fwife_error(const char* error_str)
{
	GtkWidget *error_dlg = NULL;

	if (!strlen(error_str))
		return;
	error_dlg = gtk_message_dialog_new (GTK_WINDOW(assistant),
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "%s",
                                         error_str);

	gtk_window_set_resizable (GTK_WINDOW(error_dlg), FALSE);
	gtk_window_set_title (GTK_WINDOW(error_dlg), _("Fwife error"));

	gtk_dialog_run (GTK_DIALOG(error_dlg));

	gtk_widget_destroy (error_dlg);

	return;
}

/* Information dialog */
void fwife_info(const char *info_str)
{
	GtkWidget *info_dlg = NULL;

	if (!strlen(info_str))
		return;

	info_dlg = gtk_message_dialog_new(GTK_WINDOW(assistant),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO,
						GTK_BUTTONS_OK,
						"%s",
						info_str);

	gtk_window_set_resizable (GTK_WINDOW(info_dlg), FALSE);
	gtk_window_set_title (GTK_WINDOW(info_dlg), _("Fwife information"));

	gtk_dialog_run (GTK_DIALOG(info_dlg));

	gtk_widget_destroy (info_dlg);

	return;
}

/* Fatal error : quit fwife */
void fwife_fatal_error(const char* error_str)
{
	GtkWidget *error_dlg = NULL;

	if (!strlen(error_str))
		return;

	error_dlg = gtk_message_dialog_new(GTK_WINDOW(assistant),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_OK,
						"%s",
						error_str);

	gtk_window_set_resizable (GTK_WINDOW(error_dlg), FALSE);
	gtk_window_set_title (GTK_WINDOW(error_dlg), _("Fwife error"));

	gtk_dialog_run (GTK_DIALOG(error_dlg));

	gtk_widget_destroy (error_dlg);

	fwife_exit();
	return;
}

/* A yes-no question */
int fwife_question(const char* msg_str)
{
	GtkWidget *question_dlg;

	question_dlg = gtk_message_dialog_new(GTK_WINDOW(assistant),
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						msg_str);

	gtk_window_set_resizable (GTK_WINDOW(question_dlg), FALSE);
	gtk_window_set_title (GTK_WINDOW(question_dlg), _("Fwife question"));

	int rep = gtk_dialog_run(GTK_DIALOG(question_dlg));
	gtk_widget_destroy(question_dlg);
	return(rep);
}

/* A simple entry dialog, return NULL if empty */
char* fwife_entry(const char* title, const char* msg_str, const char* defaul)
{
	char *str;

	GtkWidget *pBoite = gtk_dialog_new_with_buttons(title,
							GTK_WINDOW(assistant),
							GTK_DIALOG_MODAL,
							GTK_STOCK_OK,GTK_RESPONSE_OK,
							NULL);

	GtkWidget *labelinfo = gtk_label_new(msg_str);
	GtkWidget *pEntry = gtk_entry_new();
	if(defaul)
		gtk_entry_set_text(GTK_ENTRY(pEntry), defaul);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), labelinfo, FALSE, FALSE, 5);
   	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pBoite)->vbox), pEntry, TRUE, FALSE, 5);

	gtk_widget_show_all(GTK_DIALOG(pBoite)->vbox);

	/* Show the dialog box */
	switch (gtk_dialog_run(GTK_DIALOG(pBoite)))
	{
		/* return text entry */
		case GTK_RESPONSE_OK:
			str = strdup((char*)gtk_entry_get_text(GTK_ENTRY(pEntry)));
			break;
		/* User cancel so return NULL */
        case GTK_RESPONSE_CANCEL:
        case GTK_RESPONSE_NONE:
        default:
            str = NULL;
            break;
    	}
		gtk_widget_destroy(pBoite);

	return str;
}

