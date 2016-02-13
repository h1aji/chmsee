// mouse wheel: using mozilla's ctrl+wheel zooming
pref("mousewheel.withcontrolkey.action", 3);
pref("mousewheel.withcontrolkey.numlines", 1);
pref("mousewheel.withcontrolkey.sysnumlines", false);

// mouse wheel: using mozilla's shift+wheel smooth scrolling
pref("mousewheel.withshiftkey.action", 0);
pref("mousewheel.withshiftkey.numlines", 1);
pref("mousewheel.withshiftkey.sysnumlines", false);

// horizontal scroll with 2nd wheel
pref("mousewheel.horizscroll.withnokey.action", 0);
pref("mousewheel.horizscroll.withnokey.sysnumlines", true);

// enable line wrapping in View Source
pref("view_source.wrap_long_lines", true);

// disable sidebar What's Related, we don't use it
pref("browser.related.enabled", false);

// Work around for mozilla focus bugs
pref("mozilla.widget.raise-on-setfocus", false);

// disable sucky XUL ftp view, have nice ns4-like html page instead
pref("network.dir.generate_html", true);

// deactivate mailcap support, it breaks Gnome-based helper apps
pref("helpers.global_mailcap_file", "");
pref("helpers.private_mailcap_file", "");

// use the mozilla defaults for mime.types files to let mozilla guess proper
// Content-Type for file uploads instead of always falling back to
// application/octet-stream
pref("helpers.global_mime_types_file", "");
pref("helpers.private_mime_types_file", "");

// enable keyword search
pref("keyword.enabled", false);

// disable usless security warnings
pref("security.warn_entering_secure", false);
pref("security.warn_entering_secure.show_once", true);
pref("security.warn_leaving_secure", false);
pref("security.warn_leaving_secure.show_once", false);
pref("security.warn_submit_insecure", false);
pref("security.warn_submit_insecure.show_once", false);
pref("security.warn_viewing_mixed", true);
pref("security.warn_viewing_mixed.show_once", false);

// fonts
pref("browser.display.use_document_fonts", 0);
pref("font.size.unit", "pt");

// protocols
pref("network.protocol-handler.external-default", false);
pref("network.protocol-handler.warn-external-default", true);
pref("network.protocol-handler.external.ftp", true);
pref("network.protocol-handler.external.http", true);
pref("network.protocol-handler.external.https", true);
pref("network.protocol-handler.external.news", true);
pref("network.protocol-handler.external.mailto", true);
pref("network.protocol-handler.external.irc", true);
pref("network.protocol-handler.external.webcal", true);

// disable xpinstall
pref("xpinstall.enabled", false);

// enable typeahead find
pref("accessibility.typeaheadfind", false);

// disable pings
pref("browser.send_pings", false);

pref("browser.display.use_document_colors", true);
//pref("browser.display.use_system_colors", true);
