/*
 * test-ui-chmfile.cpp
 *
 *  Created on: Aug 2, 2009
 *      Author: lidb
 */

#include "ui_chmfile.h"
#include "models/chmfile-factory.h"

int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);



	GtkWidget* widget = chmsee_ui_chmfile_new();
	ChmseeIchmfile* model = chmsee_chmfile_new("/home/lidb/bug1.chm");
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget), model);
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget), model);
	g_object_unref(model);

	model = chmsee_chmfile_new("/home/lidb/soft/bug2.chm");
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget), model);
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget), model);
	g_object_unref(model);


	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_add(GTK_CONTAINER(window), widget);

    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, NULL);
	gtk_widget_show(widget);
	gtk_widget_show(window);
	gtk_main();
	return 0;
}
