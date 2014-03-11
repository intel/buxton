/*
 * This file is part of buxton.
 *
 * Copyright (C) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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

#define PRIMARY_KEY "test"
#define GROUP "test"
#define LAYER "user"

GType buxton_test_get_type(void);

/* BuxtonTest methods */
GtkWidget* buxton_test_new(void);
