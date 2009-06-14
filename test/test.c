#include <glib.h>
#include "utils/utils.h"
#include "models/link.h"
#include "models/chmindex.h"

void test_issue_19() {
	g_test_bug("19");
	g_assert_cmpstr("WINDOWS-1256", ==, get_encoding_by_lcid(0xc01));
}

void test_chmindex() {
	ChmIndex* chmIndex = chmindex_new("test/chmindex.hhk", "UTF-8");
	GList* data = chmindex_get_data(chmIndex);
	g_assert_cmpint(3, ==, g_list_length(data));
	g_assert_cmpstr("a", ==, ((Link*)data->data)->name);
	g_assert_cmpstr("a1", ==, ((Link*)data->data)->uri);

	data = data->next;
	g_assert_cmpstr("a", ==, ((Link*)data->data)->name);
	g_assert_cmpstr("a2", ==, ((Link*)data->data)->uri);

	data = data->next;
	g_assert_cmpstr("b", ==, ((Link*)data->data)->name);
	g_assert_cmpstr("a1", ==, ((Link*)data->data)->uri);
}


int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);
	g_test_init(&argc, &argv, NULL);
	g_test_bug_base("http://code.google.com/p/chmsee/issues/detail?id=");
	g_test_add_func("/chmsee/19", test_issue_19);
	g_test_add_func("/chmsee/chmindex", test_chmindex);
	g_test_run();
	return 0;
}
