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



	GtkWidget* widget1 = chmsee_ui_chmfile_new();
	GtkWidget* widget2 = chmsee_ui_chmfile_new();
	ChmseeIchmfile* model1 = chmsee_chmfile_new("/home/lidb/bug1.chm");
	ChmseeIchmfile* model2 = chmsee_chmfile_new("/home/lidb/soft/bug2.chm");

	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget1), model2);
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget1), model1);
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget2), model1);
	chmsee_ui_chmfile_set_model(CHMSEE_UI_CHMFILE(widget2), model2);


	g_object_unref(model1);
	g_object_unref(model2);

	GtkWidget* box = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), widget1, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), widget2, TRUE, TRUE, 0);


	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_add(GTK_CONTAINER(window), box);

    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, NULL);
	gtk_widget_show_all(window);
	gtk_main();
	return 0;
}
