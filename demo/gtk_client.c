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
#include "buxton-array.h"
#include "gtk_client.h"

/* BuxtonTest object */
struct _BuxtonTest {
        GtkWindow parent;
        BuxtonClient client;
        GtkWidget *info_label;
        GtkWidget *info;
        GtkWidget *value_label;
        GtkWidget *entry;
        gboolean registered;
        guint tag;
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

/**
 * Update the key using data from the GtkEntry
 */
static void update_key(GtkWidget *self, gpointer userdata);

/**
 * Update our known value at startup from Buxton
 */
static void update_value(BuxtonTest *self);

/**
 * Report an error in the UI
 * @param error Error details
 */
static void report_error(BuxtonTest *self, gchar *error);

/**
 * Executed by both GET and SET methods asynchronously
 * @param list Array of BuxtonData's as call result
 * @param userdata Userdata (BuxtonTest)
 */
static void buxton_callback(BuxtonArray *list, void *userdata);

/**
 * Executed whenever we recieve notification of key change
 * @param list Array of BuxtonData's as notification change
 * @param userdata Userdata (BuxtonTest)
 */
static void notify_callback(BuxtonArray *list, void *userdata);

/**
 * Executed when G_IO_PRI|G_IO_IN|G_IO_HUP conditions are met on client socket
 *
 * When G_IO_IN or G_IO_PRI conditions are satisfied, we dispatch to libbuxton
 * to handle all messages, which in turn will execute our callbacks, enabling
 * asynchronous execution.
 *
 * @param fd Buxton client socket file descriptor
 * @param cond Conditions that caused execution of callback
 * @param userdata Userdata (BuxtonClient)
 */
static gboolean buxton_update(gint fd, GIOCondition cond, gpointer userdata);

/**
 * Initialize, or even re-initalize Buxton
 * @return a boolean value, indicating success of the operation
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
	GtkStyleContext *style;

	/* Window setup */
	g_signal_connect(self, "destroy", gtk_main_quit, NULL);
	gtk_window_set_default_size(GTK_WINDOW(self), 700, 300);
	gtk_window_set_title(GTK_WINDOW(self), "BuxtonTest");

	/* layout */
	layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
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
	self->value_label = label;
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

	/* Integrate with Mutter based desktops */
	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), "BuxtonTest");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(self), header);

	gtk_widget_show_all(GTK_WIDGET(self));
	gtk_widget_grab_focus(button);

	self->client.fd = -1;
	self->tag = 0;

	/* Attempt connection to Buxton */
	if (!buxton_init(self)) {
		gtk_info_bar_set_message_type(GTK_INFO_BAR(info),
			GTK_MESSAGE_ERROR);
		gtk_label_set_markup(GTK_LABEL(self->info_label), "No connection!");
		gtk_widget_show(info);
	} else {
		gtk_widget_hide(info);
		update_value(self);
	}
}

static gboolean buxton_init(BuxtonTest *self)
{
	BuxtonString *key;

	if (self->client.fd != -1)
		return TRUE;
	/* Cleanup old poll */
	if (self->tag > 0) {
		g_source_remove(self->tag);
		self->tag = 0;
	}

	self->client.fd = -1;
	self->registered = FALSE;

	if (!buxton_client_open(&self->client))
		return FALSE;

	/* (re)register notifications */
	key = buxton_make_key("test", "test");
	if (!self->registered) {
		if (!buxton_client_register_notification(&self->client, key,
			notify_callback, self, false))
			report_error(self, "Unable to register for notifications");
		else
			self->registered = TRUE;
	}

	/* Ensure we have a poll */
	self->tag = g_unix_fd_add(self->client.fd, G_IO_IN | G_IO_PRI | G_IO_HUP,
		buxton_update, &self->client);
	free(key);

	return TRUE;
}

static void buxton_test_dispose(GObject *object)
{
	BuxtonTest *self = BUXTON_TEST(object);

	/* Stop polling buxton */
	if (self->tag > 0) {
		g_source_remove(self->tag);
		self->tag = 0;
	}

	buxton_client_close(&self->client);
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

static void update_key(GtkWidget *widget, gpointer userdata)
{
	BuxtonTest *self = BUXTON_TEST(userdata);
	BuxtonData data;
	BuxtonString *key;
	BuxtonString layer;
	BuxtonString value;
	char *layer_name = "base";
	const gchar *t_value;

	/* Ensure everything is connected and registered */
	buxton_init(self);

	t_value = gtk_entry_get_text(GTK_ENTRY(self->entry));
	if (strlen(t_value) == 0 || g_str_equal(t_value, ""))
		return;

	key = buxton_make_key("test", "test");
	layer.value = layer_name;
	layer.length = (uint32_t)strlen(layer_name)+1;

	/* Set data */
	value.value = (char*)t_value;
	value.length = (uint32_t)strlen(t_value)+1;
	buxton_string_to_data(&value, &data);

	/* Clients should **not** be setting their own smack label */
	data.label.value = "_";
	data.label.length = 2;

	if (!buxton_client_set_value(&self->client, &layer, key, &data,
		buxton_callback, NULL, false))
		report_error(self, "Unable to set value!");
	free(key);
}

static void update_value(BuxtonTest *self)
{
	BuxtonString *key;
	BuxtonString layer;
	layer.value = "base";
	layer.length = (uint32_t)strlen("base")+1;

	/* Ensure everything is connected and registered */
	buxton_init(self);

	key = buxton_make_key("test", "test");

	if (!buxton_client_get_value_for_layer(&self->client, &layer, key,
		buxton_callback, self, false)) {
		report_error(self, "Cannot retrieve value");
		goto end;
	}
end:
	free(key);
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
	BuxtonClient *client = (BuxtonClient*)userdata;
	ssize_t handled = 0;
	/* Make sure we terminate properly when the connection is killed */
	if (!(cond & G_IO_HUP)) {
		handled = buxton_client_handle_response(client);
	} else {
		buxton_client_close(client);
		client->fd = -1;
	}
	return (handled >= 0);
}

static void buxton_callback(BuxtonArray *list, void *userdata)
{
	BuxtonData *data, *key;
	BuxtonTest *self;
	gchar *lab;

	if (!userdata)
		return;

	self = BUXTON_TEST(userdata);
	data = buxton_array_get(list, 2);
	key = buxton_array_get(list, 1);

	/* Include key in the callback update */
	lab = g_strdup_printf("<big>\'%s\' value: %s</big>",
		key->store.d_string.value,
		data->store.d_string.value);
	gtk_label_set_markup(GTK_LABEL(self->value_label), lab);
	free(lab);
}

static void notify_callback(BuxtonArray *list, void *userdata)
{
	BuxtonData *data, *key;
	BuxtonTest *self = NULL;
	gchar *lab;

	if (!userdata)
		return;

	self = (BuxtonTest*)userdata;

	/* Note: real world programs should actually test this, as this
	 * is the key that was updated. This enables you to determine
	 * which key to react to */
	key = buxton_array_get(list, 0);

	/* If the list length is 1, key was unset. If it's 2, the value
	 * of the key was changed */
	if (list->len < 2) {
		// Not much of an error, but nice to report it anyway
		report_error(self, "Key has been unset!\n");
		return;
	}

	data = buxton_array_get(list, 1);

	/* Include key in the callback update */
	lab = g_strdup_printf("<big>\'%s\' value: %s</big>",
		key->store.d_string.value,
		data->store.d_string.value);
	gtk_label_set_markup(GTK_LABEL(self->value_label), lab);
	free(lab);
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
