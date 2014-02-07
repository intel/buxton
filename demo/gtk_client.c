/*
 * This file is part of buxton.
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * buxton is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <glib-unix.h>

#include "buxton.h"
#include "gtk_client.h"

/* BuxtonTest object */
struct _BuxtonTest {
        GtkWindow parent;
        BuxtonClient client;
        gint fd;
        GtkWidget *info_label;
        GtkWidget *info;
        GtkWidget *entry;
        guint tag;
        GHashTable *mapping;
};

/* BuxtonTest class definition */
struct _BuxtonTestClass {
        GtkWindowClass parent_class;
};

G_DEFINE_TYPE(BuxtonTest, buxton_test, GTK_TYPE_WINDOW)

/* Boilerplate GObject code */
static void buxton_test_class_init(BuxtonTestClass *klass);
static void buxton_test_init(BuxtonTest *self);
static void buxton_test_dispose(GObject *object);

static void update_key(GtkWidget *self, gpointer userdata);
static void update_secret_key(GtkWidget *self, gpointer userdata);
static void update_value(gpointer hkey, gpointer hvalue, gpointer userdata);
static void report_error(BuxtonTest *self, gchar *error);
static void buxton_callback(BuxtonResponse response, gpointer userdata);
static gboolean buxton_update(gint fd, GIOCondition cond, gpointer userdata);

/**
 * Initialise Buxton
 */
static gboolean buxton_init(BuxtonTest *self);

/* Initialisation */
static void buxton_test_class_init(BuxtonTestClass *klass)
{
	GObjectClass *g_object_class;

	g_object_class = G_OBJECT_CLASS(klass);
	g_object_class->dispose = &buxton_test_dispose;
}

static void buxton_test_init(BuxtonTest *self)
{
	GtkWidget *header, *info, *layout;
	GtkWidget *label, *container, *box, *box2;
	GtkWidget *entry, *button;
	GtkWidget *check;
	GtkStyleContext *style;

	/* We use the key name as our key in the hashtable, and a GtkWidget
	 * as the value. This means we can lookup the appropriate widget
	 * to set based on the hashmap. In your application you'll most
	 * likely be using GtkBuilder, and can do something similar to
	 * look up a widget by using the key name as a component */
	self->mapping = g_hash_table_new(g_str_hash, g_str_equal);

	/* Window setup */
	g_signal_connect(self, "destroy", gtk_main_quit, NULL);
	gtk_window_set_default_size(GTK_WINDOW(self), 700, 300);
	gtk_window_set_title(GTK_WINDOW(self), "BuxtonTest");

	/* layout */
	layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_halign(layout, GTK_ALIGN_CENTER);
	gtk_container_add(GTK_CONTAINER(self), layout);

	info = gtk_info_bar_new();
	label = gtk_label_new("Connecting");
	self->info_label = label;
	self->info = info;
	container = gtk_info_bar_get_content_area(GTK_INFO_BAR(info));
	gtk_container_add(GTK_CONTAINER(container), label);
	gtk_box_pack_start(GTK_BOX(layout), info, FALSE, FALSE, 0);

	/* Help label */
	label = gtk_label_new("<big>"
		"Using the controls below, you can set a key within the\n"
		"<b>base</b> layer. Open another instance of this client to\n"
		"check notification support.</big>");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(layout), label, FALSE, FALSE, 10);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
	gtk_box_pack_start(GTK_BOX(layout), box, TRUE, TRUE, 0);

	/* Updated to key value */
	label = gtk_label_new("<big>\'test\' value:</big>");
	g_hash_table_insert(self->mapping, PRIMARY_KEY, label);
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 10);

	box2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	style = gtk_widget_get_style_context(box2);
	gtk_style_context_add_class(style, GTK_STYLE_CLASS_LINKED);
	gtk_box_pack_start(GTK_BOX(box), box2, TRUE, TRUE, 0);

	/* Give entry and button a linked effect */
	entry = gtk_entry_new();
	self->entry = entry;
	gtk_entry_set_placeholder_text(GTK_ENTRY(self->entry),
		"Type a new value");
	g_signal_connect(entry, "activate", G_CALLBACK(update_key), self);
	gtk_box_pack_start(GTK_BOX(box2), entry, TRUE, TRUE, 0);

	button = gtk_button_new_with_label("Update");
	g_signal_connect(button, "clicked", G_CALLBACK(update_key), self);
	gtk_box_pack_start(GTK_BOX(box2), button, FALSE, FALSE, 0);

	/* Add sample check button */
	check = gtk_check_button_new_with_label("Super secret setting");
	gtk_box_pack_start(GTK_BOX(layout), check, FALSE, FALSE, 10);
	g_hash_table_insert(self->mapping, SECRET_KEY, check);
	g_signal_connect(check, "clicked", G_CALLBACK(update_secret_key), self);

	/* Integrate with Mutter based desktops */
	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), "BuxtonTest");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(self), header);

	gtk_widget_show_all(GTK_WIDGET(self));
	gtk_widget_grab_focus(button);

	self->fd = -1;

	/* Attempt connection to Buxton */
	if (!buxton_init(self)) {
		gtk_info_bar_set_message_type(GTK_INFO_BAR(info),
			GTK_MESSAGE_ERROR);
		gtk_label_set_markup(GTK_LABEL(self->info_label), "No connection!");
		gtk_widget_show(info);
	} else {
		gtk_widget_hide(info);
		g_hash_table_foreach(self->mapping, update_value, self);
	}
}

static void buxton_test_dispose(GObject *object)
{
	BuxtonTest *self = BUXTON_TEST(object);
	if (self->tag > 0) {
		g_source_remove(self->tag);
		self->tag = 0;
	}
	buxton_client_close(self->client);
	g_hash_table_unref(self->mapping);
        /* Destruct */
        G_OBJECT_CLASS (buxton_test_parent_class)->dispose (object);
}

/* Utility; return a new BuxtonTest */
GtkWidget* buxton_test_new(void)
{
	BuxtonTest *self;

	self = g_object_new(BUXTON_TEST_TYPE, NULL);
	return GTK_WIDGET(self);
}
static gboolean buxton_init(BuxtonTest *self)
{
	gint fd;
	BuxtonKey key;

	/* Bail if initialized */
	if (self->fd > 0)
		return TRUE;
	/* Stop probing Buxton */
	if (self->tag > 0) {
		g_source_remove(self->tag);
		self->tag = 0;
	}

	fd = buxton_client_open(&self->client);
	if (fd <= 0)
		return FALSE;
	self->fd = fd;

	/* Poll Buxton events on idle loop, Buxton will then dispatch them
	 * to appropriate callbacks */
	self->tag = g_unix_fd_add(self->fd, G_IO_IN | G_IO_PRI | G_IO_HUP,
		buxton_update, self->client);

	/* Register primary key */
	key = buxton_make_key(GROUP, PRIMARY_KEY, LAYER, STRING);
	if (!buxton_client_register_notification(self->client, key,
		buxton_callback, self, true))
		report_error(self, "Unable to register for notifications");
	buxton_free_key(key);

	/* Register secret key */
	key = buxton_make_key(GROUP, SECRET_KEY, LAYER, BOOLEAN);
	if (!buxton_client_register_notification(self->client, key,
		buxton_callback, self, true))
		report_error(self, "Unable to register for notifications");
	buxton_free_key(key);

	return TRUE;
}

static void update_key(GtkWidget *widget, gpointer userdata)
{
	BuxtonTest *self = BUXTON_TEST(userdata);
	BuxtonKey key;
	const gchar *value;

	value = gtk_entry_get_text(GTK_ENTRY(self->entry));
	if (strlen(value) == 0 || g_str_equal(value, ""))
		return;

	key = buxton_make_key(GROUP, PRIMARY_KEY, LAYER, STRING);

	if (!buxton_client_set_value(self->client, key, (void*)value,
		buxton_callback, NULL, true))
		report_error(self, "Unable to set value!");
	buxton_free_key(key);
}

static void update_secret_key(GtkWidget *widget, gpointer userdata)
{
	BuxtonTest *self = BUXTON_TEST(userdata);
	BuxtonKey key;
	gboolean active;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	key = buxton_make_key(GROUP, SECRET_KEY, LAYER, BOOLEAN);

	if (!buxton_client_set_value(self->client, key, &active,
		buxton_callback, NULL, false))
		report_error(self, "Unable to set value!");
	buxton_free_key(key);
}

static void update_value(gpointer hkey, gpointer hvalue, gpointer userdata)
{
	BuxtonTest *self = BUXTON_TEST(userdata);
	BuxtonDataType type;
	BuxtonKey key;
	GtkWidget *widget = GTK_WIDGET(hvalue);

	if (!hkey || !hvalue)
		return;

	/* We deal with a limited set right now in this example */
	if (GTK_IS_LABEL(widget))
		type = STRING;
	else if (GTK_IS_CHECK_BUTTON(widget))
		type = BOOLEAN;
	else
		return;

	key = buxton_make_key(GROUP, (gchar*)hkey, LAYER, type);

	if (!buxton_client_get_value(self->client, key,
		buxton_callback, self, true)) {
		/* Buxton disconnects us when this happens. ##FIXME##
		 * We force a reconnect */
		report_error(self, "Cannot retrieve value");
		buxton_client_close(self->client);
		self->fd = -1;
		/* Just try reconnecting */
		if (!buxton_init(self))
			report_error(self, "Unable to connect");
	}


	buxton_free_key(key);
}

static void report_error(BuxtonTest *self, gchar *error)
{
	if (error != NULL) {
		printf("Error! %s\n", error);
		gtk_label_set_markup(GTK_LABEL(self->info_label), error);
		gtk_widget_show_all(GTK_WIDGET(self->info));
		gtk_info_bar_set_message_type(GTK_INFO_BAR(self->info),
			GTK_MESSAGE_ERROR);
	} else {
		gtk_widget_hide(GTK_WIDGET(self->info));
	}
}

static gboolean buxton_update(gint fd, GIOCondition cond, gpointer userdata)
{
	BuxtonClient client = (BuxtonClient)userdata;
	ssize_t handled = buxton_client_handle_response(client);
	return (handled >= 0);
}

static void buxton_callback(BuxtonResponse response, gpointer userdata)
{
	BuxtonKey key;
	BuxtonTest *self;
	void *value;
	gchar *key_name = NULL;
	GtkWidget *widget;

	if (!userdata)
		return;

	self = BUXTON_TEST(userdata);
	key = response_key(response);
	key_name = buxton_get_name(key);
	value = response_value(response);

	widget = g_hash_table_lookup(self->mapping, key_name);
	if (!widget) {
		/* We have no widgets hooked up to this key */
		goto end;
	}

	/* Handle PRIMARY_KEY (string) */
	if (g_str_equal(key_name, PRIMARY_KEY) && buxton_get_type(key) == STRING) {
		gchar *lab;
		/* Key unset */
		if (!value)
			lab = g_strdup_printf("<big>\'%s\' unset</big>", key_name);
		else
			lab = g_strdup_printf("<big>\'%s\' value: %s</big>",
				key_name, (gchar*)value);
		/* Update UI */
		gtk_label_set_markup(GTK_LABEL(widget), lab);
		g_free(lab);
	} else if (g_str_equal(key_name, SECRET_KEY) && buxton_get_type(key) == BOOLEAN) {
		/* Handle SECRET_KEY (boolean) */
		if (!value)
			goto end;
		gboolean b = *(gboolean*)value;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), b);
	}

end:
	free(key_name);
	buxton_free_key(key);
}

/** Main entry */
int main(int argc, char **argv)
{
	__attribute__ ((unused)) GtkWidget *window = NULL;

	gtk_init(&argc, &argv);
	window = buxton_test_new();
	gtk_main();

	return EXIT_SUCCESS;
}
