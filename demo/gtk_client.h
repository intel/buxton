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

#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

typedef struct _BuxtonTest BuxtonTest;
typedef struct _BuxtonTestClass   BuxtonTestClass;

#define BUXTON_TEST_TYPE (buxton_test_get_type())
#define BUXTON_TEST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUXTON_TEST_TYPE, BuxtonTest))
#define IS_BUXTON_TEST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUXTON_TEST_TYPE))
#define BUXTON_TEST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BUXTON_TEST_TYPE, BuxtonTestClass))
#define IS_BUXTON_TEST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BUXTON_TEST_TYPE))
#define BUXTON_TEST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BUXTON_TEST_TYPE, BuxtonTestClass))

GType buxton_test_get_type(void);

/* BuxtonTest methods */
GtkWidget* buxton_test_new(void);
