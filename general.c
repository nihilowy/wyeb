/*
Copyright 2017-2018 jun7@hush.mail

This file is part of wyeb.

wyeb is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

wyeb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with wyeb.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <regex.h>

#define OLDNAME  "wyebrowser"
#define DIRNAME  "wyeb."
#define APP      "wyeb"

#define DSET "set;"
#define MIMEOPEN "mimeopen -n %s"
#define HINTKEYS "fsedagwrvxqcz"
//bt324"

#if DEBUG
# define D(f, ...) g_print("#"#f"\n", __VA_ARGS__);
# define DNN(f, ...) g_print(#f, __VA_ARGS__);
# define DD(a) g_print("#"#a"\n");
# define DENUM(v, e) if (v == e) D(%s:%3d is %s, #v, v, #e);
#else
# define D(...) ;
# define DNN(...) ;
# define DD(a) ;
# define DENUM(v, e) ;
#endif


#if WEBKIT_MAJOR_VERSION > 2 || WEBKIT_MINOR_VERSION > 16
# define V18 1
#else
# define V18 0
#endif
#if WEBKIT_MAJOR_VERSION > 2 || WEBKIT_MINOR_VERSION > 20
# define V22 1
#else
# define V22 0
#endif

#ifdef MAINC
#if ! v22
#include <JavaScriptCore/JSStringRef.h>
#endif
#endif


#define SIG(o, n, c, u) \
	g_signal_connect(o, n, G_CALLBACK(c), u)
#define SIGA(o, n, c, u) \
	g_signal_connect_after(o, n, G_CALLBACK(c), u)
#define SIGW(o, n, c, u) \
	g_signal_connect_swapped(o, n, G_CALLBACK(c), u)
#define GFA(p, v) {void *__b = p; p = v; g_free(__b);}

static gchar *fullname = "";
static bool shared = true;
static GKeyFile *conf = NULL;
static gchar *confpath = NULL;

typedef struct _WP WP;

typedef enum {
	Cload   = 'L',
	Coverset= 'O',
	Cstart  = 's',
	Con     = 'o',

	//hint
	Ckey    = 'k',
	Cclick  = 'c',
	Clink   = 'l',
	Curi    = 'u',
	Ctext   = 't',
	Cspawn  = 'S',
	Crange  = 'r',
	Crm     = 'R',

	Cmode   = 'm',
	Cfocus  = 'f',
	Cblur   = 'b',
	Cwhite  = 'w',
	Ctlset  = 'T',
	Ctlget  = 'g',
	Cwithref= 'W',
	Cscroll = 'v',
} Coms;


//@conf
typedef struct {
	gchar *group;
	gchar *key;
	gchar *val;
	gchar *desc;
} Conf;
Conf dconf[] = {
	{"all"   , "editor"       , MIMEOPEN,
		"editor=xterm -e nano %s\n"
		"editor=gvim --servername "APP" --remote-silent \"%s\""
	},
	{"all"   , "mdeditor"     , ""},
	{"all"   , "diropener"    , MIMEOPEN},
	{"all"   , "generator"    , "markdown -f -style %s"},

	{"all"   , "hintkeys"     , HINTKEYS},
	{"all"   , "keybindswaps" , "",
		"keybindswaps=Xx;ZZ;zZ ->if typed x: x to X, if Z: Z to Z"},

	{"all"   , "winwidth"     , "1000"},
	{"all"   , "winheight"    , "1000"},
	{"all"   , "zoom"         , "1.000"},

	{"all"   , "keepfavicondb", "false"},
	{"all"   , "dlwinback"    , "true"},
	{"all"   , "dlwinclosemsec","3000"},
	{"all"   , "msgmsec"      , "600"},
	{"all"   , "ignoretlserr" , "false"},
	{"all"   , "histviewsize" , "99"},
	{"all"   , "histimgs"     , "99"},
	{"all"   , "histimgsize"  , "222"},
	{"all"   , "searchstrmax" , "99"},
	//compatibility
	{"all"   , "pointerwarp"  , "false"},

//	{"all"   , "configreload" , "true",
//			"reload last window when whiteblack.conf or reldomain are changed"},

	{"boot"  , "enablefavicon", "true"},
	{"boot"  , "extensionargs", "adblock:true;"},
	{"boot"  , "multiwebprocs", "false"},
	{"boot"  , "ephemeral"    , "false"},
	{"boot"  , "unsetGTK_OVERLAY_SCROLLING", "true", "workaround"},

	{"search", "g"            , "https://www.google.com/search?q=%s"},
	{"search", "f"            , "https://www.google.com/search?q=%s&btnI=I"},
	{"search", "u"            , "https://www.urbandictionary.com/define.php?term=%s"},

	{"template", "na"         , "%s"},
	{"template", "h"          , "http://%s"},

	{"set:v"     , "enable-caret-browsing", "true"},
	{"set:script", "enable-javascript"    , "true"},
	{"set:image" , "auto-load-images"     , "true"},
	{"set:image" , "linkformat"   , "[![]("APP":F) %.40s](%s)"},
	{"set:image" , "linkdata"     , "tu"},
	{"set:image" , "hintstyle"    ,
		"font-size:medium !important;-webkit-transform:rotate(-9deg)"},

	{DSET    , "search"           , "https://www.google.com/search?q=%s", "search=g"},
	{DSET    , "usercss"          , "user.css", "usercss=user.css;user2.css"},
	{DSET    , "addressbar"       , "false"},
	{DSET    , "msgcolor"         , "#c07"},

	//loading
	{DSET    , "adblock"          , "true",
		"This has a point only while "APP"adblock is working."
	},
	{DSET    , "reldomaindataonly", "false"},
	{DSET    , "reldomaincutheads", "www.;wiki.;bbs.;developer."},
	{DSET    , "showblocked"      , "false"},
	{DSET    , "stdoutheaders"    , "false"},
	{DSET    , "removeheaders"    , "",
		"removeheaders=Upgrade-Insecure-Requests;Referer;"},
	{DSET    , "rmnoscripttag"    , "false"},

	//event
	{DSET    , "multiplescroll"   , "2"},
	{DSET    , "newwinhandle"     , "normal",
		"newwinhandle=notnew | ignore | back | normal"},
	{DSET    , "scriptdialog"     , "true"},
	{DSET    , "hjkl2arrowkeys"   , "false",
		"hjkl's default are scrolls, not arrow keys"},

	//link
	{DSET    , "linkformat"       , "[%.40s](%s)"},
	{DSET    , "linkdata"         , "tu", "t: title, u: uri, f: favicon"},

	//hint
	{DSET    , "hintstyle"        , ""},
	{DSET    , "hackedhint4js"    , "true"},
	{DSET    , "hintrangemax"     , "9"},
	{DSET    , "rangeloopusec"    , "90000"},

	//dl
	{DSET    , "dlwithheaders"    , "false"},
	{DSET    , "dlmimetypes"      , "",
		"dlmimetypes=text/plain;video/;audio/;application/\n"
		"dlmimetypes=*"},
	{DSET    , "dlsubdir"         , ""},

	//script
	{DSET    , "spawnmsg"         , "false"},

	{DSET    , "onstartmenu"      , "",
		"onstartmenu spawns a shell in the menu dir when load started before redirect"},
	{DSET    , "onloadmenu"       , "", "when load commited"},
	{DSET    , "onloadedmenu"     , "", "when load finished"},

	{DSET    , "mdlbtnlinkaction" , "openback"},
	{DSET    , "mdlbtnleft"       , "prevwin", "mdlbtnleft=winlist"},
	{DSET    , "mdlbtnright"      , "nextwin"},
	{DSET    , "mdlbtnup"         , "top"},
	{DSET    , "mdlbtndown"       , "bottom"},
	{DSET    , "pressscrollup"    , "top"},
	{DSET    , "pressscrolldown"  , "winlist"},
	{DSET    , "rockerleft"       , "back"},
	{DSET    , "rockerright"      , "forward"},
	{DSET    , "rockerup"         , "quitprev"},
	{DSET    , "rockerdown"       , "quitnext"},

	//changes
	//{DSET      , "auto-load-images" , "false"},
	//{DSET      , "enable-plugins"   , "false"},
	//{DSET      , "enable-java"      , "false"},
	//{DSET      , "enable-fullscreen", "false"},
};
#ifdef MAINC
static bool confbool(gchar *key)
{ return g_key_file_get_boolean(conf, "all", key, NULL); }
static gint confint(gchar *key)
{ return g_key_file_get_integer(conf, "all", key, NULL); }
static gdouble confdouble(gchar *key)
{ return g_key_file_get_double(conf, "all", key, NULL); }
#endif
static gchar *confcstr(gchar *key)
{//return is static string
	static gchar *str = NULL;
	GFA(str, g_key_file_get_string(conf, "all", key, NULL))
	return str;
}
static gchar *getset(WP *wp, gchar *key)
{//return is static string
	static gchar *ret = NULL;
	if (!wp)
	{
		GFA(ret, g_key_file_get_string(conf, DSET, key, NULL))
		return ret;
	}
	return g_object_get_data(wp->seto, key);
}
static bool getsetbool(WP *wp, gchar *key)
{ return !g_strcmp0(getset(wp, key), "true"); }
static int getsetint(WP *wp, gchar *key)
{ return atoi(getset(wp, key) ?: "0"); }


static gchar *path2conf(const gchar *name)
{
	return g_build_filename(
			g_get_user_config_dir(), fullname, name, NULL);
}

static bool setprop(WP *wp, GKeyFile *kf, gchar *group, gchar *key)
{
	if (!g_key_file_has_key(kf, group, key, NULL)) return false;
	gchar *val = g_key_file_get_string(kf, group, key, NULL);
	g_object_set_data_full(wp->seto, key, *val ? val : NULL, g_free);
	return true;
}
static void setprops(WP *wp, GKeyFile *kf, gchar *group)
{
	//sets
	static int deps = 0;
	if (deps > 99) return;
	gchar **sets = g_key_file_get_string_list(kf, group, "sets", NULL, NULL);
	for (gchar **set = sets; set && *set; set++) {
		gchar *setstr = g_strdup_printf("set:%s", *set);
		deps++;
		setprops(wp, kf, setstr);
		deps--;
		g_free(setstr);
	}
	g_strfreev(sets);

	//D(set props group: %s, group)
#ifdef MAINC
	_kitprops(true, wp->seto, kf, group);
#else
	setprop(wp, kf, group, "javascript-can-open-windows-automatically");
	if (setprop(wp, kf, group, "user-agent") && strcmp(group, DSET))
		wp->setagent = true;
#endif

	//non webkit settings
	int len = sizeof(dconf) / sizeof(*dconf);
	for (int i = 0; i < len; i++)
		if (!strcmp(dconf[i].group, DSET))
			setprop(wp, kf, group, dconf[i].key);
}

static GSList *regs = NULL;
static GSList *regsrev = NULL;
static void makeuriregs() {
	for (GSList *next = regs; next; next = next->next)
	{
		regfree(((void **)next->data)[0]);
		g_free( ((void **)next->data)[1]);
	}
	g_slist_free_full(regs, g_free);
	regs = NULL;
	g_slist_free(regsrev);
	regsrev = NULL;

	gchar **groups = g_key_file_get_groups(conf, NULL);
	for (gchar **next = groups; *next; next++)
	{
		gchar *gl = *next;
		if (!g_str_has_prefix(gl, "uri:")) continue;

		gchar *g = gl;
		gchar *tofree = NULL;
		if (g_key_file_has_key(conf, g, "reg", NULL))
		{
			g = tofree = g_key_file_get_string(conf, g, "reg", NULL);
		} else {
			g += 4;
		}

		void **reg = g_new(void*, 2);
		*reg = g_new(regex_t, 1);
//		if (regcomp(*reg, g, REG_EXTENDED | REG_NOSUB))
		if (regcomp(*reg, g, REG_EXTENDED))
		{ //failed
			g_free(*reg);
			g_free(reg);
		} else {
			reg[1] = g_strdup(gl);
			regsrev = g_slist_prepend(regsrev, reg);
		}
		g_free(tofree);
	}
	g_strfreev(groups);

	for (GSList *next = regsrev; next; next = next->next)
		regs = g_slist_prepend(regs, next->data);
}
static bool eachuriconf(WP *wp, const gchar* uri, bool lastone,
		bool (*func)(WP *, const gchar *uri, gchar *group))
{
	bool ret = false;
	regmatch_t match[2];
	for (GSList *reg = lastone ? regsrev : regs; reg; reg = reg->next)
		if (regexec(*(void **)reg->data, uri, 2, match, 0) == 0)
		{
			gchar *m = match[1].rm_so == -1 ? NULL : g_strndup(
					uri + match[1].rm_so, match[1].rm_eo - match[1].rm_so);

			ret = func(wp, m ?: uri, ((void **)reg->data)[1]) || ret;
			g_free(m);
			if (lastone) break;
		}

	return ret;
}
static bool seturiprops(WP *wp, const gchar *uri, gchar *group)
{
	setprops(wp, conf, group);
	return true;
}

static void _resetconf(WP *wp, const gchar *uri, bool force)
{
	if (wp->lasturiconf || force)
	{
		GFA(wp->lasturiconf, NULL)
		//clearing. don't worry about reg and handler they are not set
		setprops(wp, conf, DSET);
	}

	GFA(wp->lastreset, g_strdup(uri))
	if (uri && eachuriconf(wp, uri, false, seturiprops))
		GFA(wp->lasturiconf, g_strdup(uri))
	if (wp->overset) {
		gchar **sets = g_strsplit(wp->overset, "/", -1);
		for (gchar **set = sets; *set; set++)
		{
			gchar *setstr = g_strdup_printf("set:%s", *set);
			setprops(wp, conf, setstr);
			g_free(setstr);
		}
		g_strfreev(sets);
	}
}
static void initconf(GKeyFile *kf)
{
	if (conf) g_key_file_free(conf);
	conf = kf ?: g_key_file_new();
	makeuriregs();

	gint len = sizeof(dconf) / sizeof(*dconf);
	for (int i = 0; i < len; i++)
	{
		Conf c = dconf[i];

		if (g_key_file_has_key(conf, c.group, c.key, NULL)) continue;
		if (kf)
		{
			if (!strcmp(c.group, "search")) continue;
			if (!strcmp(c.group, "template")) continue;
			if (g_str_has_prefix(c.group, "set:")) continue;
		}

		g_key_file_set_value(conf, c.group, c.key, c.val);
		if (c.desc)
			g_key_file_set_comment(conf, c.group, c.key, c.desc, NULL);
	}

#ifdef MAINC
	if (!kf)
	{
		g_key_file_set_value(  conf, DSET, "dummy", "endof"APP);
		g_key_file_set_comment(conf, DSET, "dummy", "\nWebkit's settings", NULL);
		g_key_file_remove_key( conf, DSET, "dummy", NULL);
	}

	//fill vals not set
	if (LASTWIN)
		_kitprops(false, LASTWIN->seto, conf, DSET);
	else {
		WebKitSettings *set = webkit_settings_new();
		_kitprops(false, (GObject *)set, conf, DSET);
		g_object_unref(set);
	}

	if (kf) return;

	//sample and comment
	g_key_file_set_comment(conf, "all", NULL,
			"Basically "APP" doesn't cut spaces."
			" Also true is only 'true' not 'True'\n", NULL);
	g_key_file_set_comment(conf, "template", NULL,
			"Unlike the search group, the arg is not escaped\n"
			"but can be called the same as the search", NULL);
	g_key_file_set_comment(conf, "set:v", NULL,
			"Settings set. You can add set:*\n"
			"It is enabled by actions(set/set2/setstack) or included by others"
			, NULL);
	g_key_file_set_comment(conf, DSET, NULL,
			"Default of 'set's\n"
			"You can use set;'s keys in set:* and uri:*", NULL);
	g_key_file_set_comment(conf, DSET, "hardware-acceleration-policy",
			"ON_DEMAND | ALWAYS | NEVER", NULL);

	const gchar *sample = "uri:^https?://(www\\.)?foo\\.bar/.*";

	g_key_file_set_boolean(conf, sample, "enable-javascript", true);
	g_key_file_set_comment(conf, sample, NULL,
			"After 'uri:' is regular expressions for the setting set.\n"
			"preferential order of groups: lower > upper > '"DSET"'"
			, NULL);

	sample = "uri:^foo|a-zA-Z0-9|*";

	g_key_file_set_string(conf, sample, "reg", "^foo[^a-zA-Z0-9]*$");
	g_key_file_set_comment(conf, sample, "reg",
			"Use reg if the regular expression has []"
			, NULL);
	g_key_file_set_string(conf, sample, "handler", "chromium %s");
	g_key_file_set_comment(conf, sample, "handler",
			"handler cancels request before sent and\n"
			"spawns the command with a URI matched the 'uri:'"
			, NULL);
	g_key_file_set_string(conf, sample, "handlerunesc", "false");
	g_key_file_set_string(conf, sample, "handlerescchrs", "");

	g_key_file_set_string(conf, sample, "sets", "image;script");
	g_key_file_set_comment(conf, sample, "sets",
			"include sets" , NULL);
#endif
}


//@misc
static void _mkdirif(gchar *path, bool isfile)
{
	gchar *dir;
	if (isfile)
		dir = g_path_get_dirname(path);
	else
		dir = path;

	if (!g_file_test(dir, G_FILE_TEST_EXISTS))
		g_mkdir_with_parents(dir, 0700);

	g_assert(g_file_test(dir, G_FILE_TEST_IS_DIR));

	if (isfile)
		g_free(dir);
}
static void mkdirif(gchar *path)
{
	_mkdirif(path, true);
}

static gchar *_escape(const gchar *str, gchar *esc)
{
	gulong len = 0;
	for (const gchar *c = str; *c; c++)
	{
		len++;
		for (gchar *e = esc; *e; e++)
			if (*e == *c)
			{
				len++;
				break;
			}
	}
	gchar ret[len + 1];
	ret[len] = '\0';

	gulong i = 0;
	for (const gchar *c = str; *c; c++)
	{
		for (gchar *e = esc; *e; e++)
			if (*e == *c)
			{
				ret[i++] = '\\';
				break;
			}

		ret[i++] = *c;
	}

	return g_strdup(ret);
}
static gchar *regesc(const gchar *str)
{
	return _escape(str, ".?+");
}


//@ipc
static gchar *ipcpath(gchar *name)
{
	static gchar *path = NULL;
	GFA(path, g_build_filename(g_get_user_runtime_dir(), fullname, name, NULL));
	mkdirif(path);
	return path;
}

static void ipccb(const gchar *line);
static gboolean ipcgencb(GIOChannel *ch, GIOCondition c, gpointer p)
{
	gchar *line;
//	GError *err = NULL;
	g_io_channel_read_line(ch, &line, NULL, NULL, NULL);
//	if (err)
//	{
//		D("ioerr: ", err->message);
//		g_error_free(err);
//	}
	if (!line) return true;
	g_strchomp(line);

	//D(receive %s, line)
	gchar *unesc = g_strcompress(line);
	ipccb(unesc);
	g_free(unesc);
	g_free(line);
	return true;
}

static bool ipcsend(gchar *name, gchar *str) {
	gchar *path = ipcpath(name);
	bool ret = false;
	int cpipe = 0;
	if (
		(g_file_test(path, G_FILE_TEST_EXISTS)) &&
		(cpipe = open(path, O_WRONLY | O_NONBLOCK)))
	{
		//D(send start %s %s, name, str)
		char *esc = g_strescape(str, "");
		gchar *send = g_strconcat(esc, "\n", NULL);
		int len = strlen(send);
		if (len > PIPE_BUF)
			fcntl(cpipe, F_SETFL, 0);
		ret = write(cpipe, send, len) == len;
		g_free(send);
		g_free(esc);
		close(cpipe);

		//D(send -end- %s %s, name, str)
	}
	return ret;
}
static GSource *_ipcwatch(gchar *name, GMainContext *ctx) {
	gchar *path = ipcpath(name);

	if (!g_file_test(path, G_FILE_TEST_EXISTS))
		mkfifo(path, 0600);

	GIOChannel *io = g_io_channel_new_file(path, "r+", NULL);
	GSource *watch = g_io_create_watch(io, G_IO_IN);
	g_io_channel_unref(io);
	g_source_set_callback(watch, (GSourceFunc)ipcgencb, NULL, NULL);
	g_source_attach(watch, ctx);

	return watch;
}
static void ipcwatch(gchar *name)
{
	_ipcwatch(name, g_main_context_default());
}
