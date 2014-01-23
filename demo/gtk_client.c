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

#include "bt-daemon.h"
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
static void update_value(BuxtonTest *self);
static void report_error(BuxtonTest *self, gchar *error);
static gboolean update_buxton(BuxtonTest *self);

static void notify_cb(BuxtonArray *list, void *userdata)
{
	BuxtonTest *self = (BuxtonTest*)userdata;
	gchar *lab;
	BuxtonData *key, *value;

	printf("Length: %d\n", list->len);
	if (list->len != 2) {
		report_error(self, "Key has been unset\n");
		return;
	}

	key = buxton_array_get(list, 0);
	value = buxton_array_get(list, 1);
	lab = g_strdup_printf("<big>\'%s\' value: %s</big>", key->store.d_string.value, value->store.d_string.value);
	gtk_label_set_markup(GTK_LABEL(self->value_label), lab);
	free(lab);
}

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
		"<b>user</b> layer. Open another instance of this client to\n"
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

	/* Attempt connection to Buxton */
	if (!buxton_client_open(&self->client)) {
		gtk_info_bar_set_message_type(GTK_INFO_BAR(info),
			GTK_MESSAGE_ERROR);
		gtk_label_set_markup(GTK_LABEL(self->info_label), "No connection!");
		gtk_widget_show(info);
	} else {
		gtk_widget_hide(info);
	}

	self->registered = FALSE;
	update_value(self);
	/* Grab buxton notifications on idle loop */
	g_idle_add((GSourceFunc)update_buxton, self);
}

static void buxton_test_dispose(GObject *object)
{
	BuxtonTest *self = BUXTON_TEST(object);
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

/**
 * Callback for when we ask buxton for the key value
 */
static void update_ui(BuxtonArray *array, void *p)
{
	BuxtonTest *self = (BuxtonTest*)p;
	gchar *lab;
	BuxtonData *data;
	BuxtonData *key;

	key = (BuxtonData*)buxton_array_get(array, 1);
	data = (BuxtonData*)buxton_array_get(array, 2);

	lab = g_strdup_printf("<big>\'%s\' value: %s</big>", key->store.d_string.value, data->store.d_string.value);
	gtk_label_set_markup(GTK_LABEL(self->value_label), lab);
	free(lab);
}

/**
 * Called from set_value
 *
 * In the future may use this to check the status of the operation
 */
static void callback(BuxtonArray *array, void *p)
{
}

static void update_key(GtkWidget *widget, gpointer userdata)
{
	BuxtonTest *self = BUXTON_TEST(userdata);
	BuxtonData data;
	BuxtonString *key;
	BuxtonString layer;
	BuxtonString value;
	char *layer_name = "user";
	const gchar *t_value;


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

	if (!buxton_client_set_value(&self->client, &layer, key, &data, callback, NULL, true))
		report_error(self, "Unable to set value!");
	free(key);
}

static void update_value(BuxtonTest *self)
{
	BuxtonString *key;

	key = buxton_make_key("test", "test");

	if (!self->registered) {
		if (!buxton_client_register_notification(&self->client,
			key, notify_cb, self, true))
			report_error(self, "Unable to register for notifications");
		else
			self->registered = TRUE;
	}

	if (!buxton_client_get_value(&self->client, key, update_ui, self, true)) {
		/* Buxton disconnects us when this happens. ##FIXME##
		 * We force a reconnect */
		report_error(self, "Cannot retrieve value");
		buxton_client_close(&self->client);
		if (!buxton_client_open(&self->client))
			report_error(self, "Unable to connect to Buxton!");
	}

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

static gboolean update_buxton(BuxtonTest *self)
{
	buxton_client_poll(&self->client);
	return TRUE;
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
