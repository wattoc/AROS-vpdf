//========================================================================
//
// GlobalParams.cc
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Martin Kretzschmar <martink@gnome.org>
// Copyright (C) 2005, 2006 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2005, 2007-2010, 2012 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2005 Jonathan Blandford <jrb@redhat.com>
// Copyright (C) 2006, 2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2006 Takashi Iwai <tiwai@suse.de>
// Copyright (C) 2006 Ed Catmur <ed@catmur.co.uk>
// Copyright (C) 2007 Krzysztof Kowalczyk <kkowalczyk@gmail.com>
// Copyright (C) 2007, 2009 Jonathan Kew <jonathan_kew@sil.org>
// Copyright (C) 2009 Petr Gajdos <pgajdos@novell.com>
// Copyright (C) 2009, 2011, 2012 William Bader <williambader@hotmail.com>
// Copyright (C) 2009 Kovid Goyal <kovid@kovidgoyal.net>
// Copyright (C) 2010 Hib Eris <hib@hiberis.nl>
// Copyright (C) 2010 Patrick Spendrin <ps_ml@gmx.de>
// Copyright (C) 2010 Jakub Wilk <ubanus@users.sf.net>
// Copyright (C) 2011 Pino Toscano <pino@kde.org>
// Copyright (C) 2011 Koji Otani <sho@bbr.jp>
// Copyright (C) 2012 Yi Yang <ahyangyi@gmail.com>
// Copyright (C) 2012 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2012 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2012 Peter Breitenlohner <peb@mppmu.mpg.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifdef ENABLE_PLUGINS
#  ifndef _WIN32
#    include <dlfcn.h>
#  endif
#endif
#ifdef _WIN32
#  include <shlobj.h>
#  include <mbstring.h>
#endif
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "goo/GooList.h"
#include "goo/GooHash.h"
#include "goo/gfile.h"
#include "Error.h"
#include "NameToCharCode.h"
#include "CharCodeToUnicode.h"
#include "UnicodeMap.h"
#include "CMap.h"
#include "BuiltinFontTables.h"
#include "FontEncodingTables.h"
#ifdef ENABLE_PLUGINS
#  include "XpdfPluginAPI.h"
#endif
#include "GlobalParams.h"
#include "GfxFont.h"

#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>
#include <dos/dosextens.h>
#include <clib/debug_protos.h>
#include "../../fontcache.h"
#endif

#if WITH_FONTCONFIGURATION_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#ifdef _WIN32
#  define strcasecmp stricmp
#else
#  include <strings.h>
#endif

#if MULTITHREADED
#  define lockGlobalParams            gLockMutex(&mutex)
#  define lockUnicodeMapCache         gLockMutex(&unicodeMapCacheMutex)
#  define lockCMapCache               gLockMutex(&cMapCacheMutex)
#  define unlockGlobalParams          gUnlockMutex(&mutex)
#  define unlockUnicodeMapCache       gUnlockMutex(&unicodeMapCacheMutex)
#  define unlockCMapCache             gUnlockMutex(&cMapCacheMutex)
#else
#  define lockGlobalParams
#  define lockUnicodeMapCache
#  define lockCMapCache
#  define unlockGlobalParams
#  define unlockUnicodeMapCache
#  define unlockCMapCache
#endif

#ifndef FC_WEIGHT_BOOK
#define FC_WEIGHT_BOOK 75
#endif

#include "NameToUnicodeTable.h"
#include "UnicodeMapTables.h"
#include "UTF8.h"

#ifdef ENABLE_PLUGINS
#  ifdef _WIN32
extern XpdfPluginVecTable xpdfPluginVecTable;
#  endif
#endif

#ifdef __AROS__
#define DEFAULT_SUBSTITUTE_FONT "Helvetica"

static struct {
    char *name;
    char *t1FileName;
    char *ttFileName;
} displayFontTab[] = {
    {"Courier",               "n022003l.pfb", "cour.ttf"},
    {"Courier-Bold",          "n022004l.pfb", "courbd.ttf"},
    {"Courier-BoldOblique",   "n022024l.pfb", "courbi.ttf"},
    {"Courier-Oblique",       "n022023l.pfb", "couri.ttf"},
    {"Helvetica",             "n019003l.pfb", "arial.ttf"},
    {"Helvetica-Bold",        "n019004l.pfb", "arialbd.ttf"},
    {"Helvetica-BoldOblique", "n019024l.pfb", "arialbi.ttf"},
    {"Helvetica-Oblique",     "n019023l.pfb", "ariali.ttf"},
    // TODO: not sure if "symbol.ttf" is right
    {"Symbol",                "s050000l.pfb", "symbol.ttf"},
    {"Times-Bold",            "n021004l.pfb", "timesbd.ttf"},
    {"Times-BoldItalic",      "n021024l.pfb", "timesbi.ttf"},
    {"Times-Italic",          "n021023l.pfb", "timesi.ttf"},
    {"Times-Roman",           "n021003l.pfb", "times.ttf"},
    // TODO: not sure if "wingding.ttf" is right
    {"ZapfDingbats",          "d050000l.pfb", "wingding.ttf"},

    // those seem to be frequently accessed by PDF files and I kind of guess
    // which font file do the refer to
    {"Palatino", NULL, "pala.ttf"},
    {"Palatino-Roman", NULL, "pala.ttf"},
    {"Palatino-Bold", NULL, "palab.ttf"},
    {"Palatino-Italic", NULL, "palai.ttf"},
    {"Palatino,Italic", NULL, "palai.ttf"},
    {"Palatino-BoldItalic", NULL, "palabi.ttf"},

    {"ArialBlack",        NULL, "arialbd.ttf"},

    {"ArialNarrow", NULL, "arialn.ttf"},
    {"ArialNarrow,Bold", NULL, "arialnb.ttf"},
    {"ArialNarrow,Italic", NULL, "arialni.ttf"},
    {"ArialNarrow,BoldItalic", NULL, "arialnbi.ttf"},
    {"ArialNarrow-Bold", NULL, "arialnb.ttf"},
    {"ArialNarrow-Italic", NULL, "arialni.ttf"},
    {"ArialNarrow-BoldItalic", NULL, "arialnbi.ttf"},

    {"HelveticaNarrow", NULL, "arialn.ttf"},
    {"HelveticaNarrow,Bold", NULL, "arialnb.ttf"},
    {"HelveticaNarrow,Italic", NULL, "arialni.ttf"},
    {"HelveticaNarrow,BoldItalic", NULL, "arialnbi.ttf"},
    {"HelveticaNarrow-Bold", NULL, "arialnb.ttf"},
    {"HelveticaNarrow-Italic", NULL, "arialni.ttf"},
    {"HelveticaNarrow-BoldItalic", NULL, "arialnbi.ttf"},

    {"BookAntiqua", NULL, "bkant.ttf"},
    {"BookAntiqua,Bold", NULL, "bkant.ttf"},
    {"BookAntiqua,Italic", NULL, "bkant.ttf"},
    {"BookAntiqua,BoldItalic", NULL, "bkant.ttf"},
    {"BookAntiqua-Bold", NULL, "bkant.ttf"},
    {"BookAntiqua-Italic", NULL, "bkant.ttf"},
    {"BookAntiqua-BoldItalic", NULL, "bkant.ttf"},

    {"Verdana", NULL, "verdana.ttf"},
    {"Verdana,Bold", NULL, "verdanab.ttf"},
    {"Verdana,Italic", NULL, "verdanai.ttf"},
    {"Verdana,BoldItalic", NULL, "verdanaz.ttf"},
    {"Verdana-Bold", NULL, "verdanab.ttf"},
    {"Verdana-Italic", NULL, "verdanai.ttf"},
    {"Verdana-BoldItalic", NULL, "verdanaz.ttf"},

    {"Tahoma", NULL, "tahoma.ttf"},
    {"Tahoma,Bold", NULL, "tahomabd.ttf"},
    {"Tahoma,Italic", NULL, "tahoma.ttf"},
    {"Tahoma,BoldItalic", NULL, "tahomabd.ttf"},
    {"Tahoma-Bold", NULL, "tahomabd.ttf"},
    {"Tahoma-Italic", NULL, "tahoma.ttf"},
    {"Tahoma-BoldItalic", NULL, "tahomabd.ttf"},

    {"CCRIKH+Verdana", NULL, "verdana.ttf"},
    {"CCRIKH+Verdana,Bold", NULL, "verdanab.ttf"},
    {"CCRIKH+Verdana,Italic", NULL, "verdanai.ttf"},
    {"CCRIKH+Verdana,BoldItalic", NULL, "verdanaz.ttf"},
    {"CCRIKH+Verdana-Bold", NULL, "verdanab.ttf"},
    {"CCRIKH+Verdana-Italic", NULL, "verdanai.ttf"},
    {"CCRIKH+Verdana-BoldItalic", NULL, "verdanaz.ttf"},

    {"Georgia", NULL, "georgia.ttf"},
    {"Georgia,Bold", NULL, "georgiab.ttf"},
    {"Georgia,Italic", NULL, "georgiai.ttf"},
    {"Georgia,BoldItalic", NULL, "georgiaz.ttf"},
    {"Georgia-Bold", NULL, "georgiab.ttf"},
    {"Georgia-Italic", NULL, "georgiai.ttf"},
    {"Georgia-BoldItalic", NULL, "georgiaz.ttf"},

    {NULL}
};

#define FONTS_SUBDIR "FONTS:"	// TODO: make sure to scan all directories belonging to an assign

static bool FileExists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f)
		fclose(f);
    return f != NULL;
    
}

#endif

//------------------------------------------------------------------------

#define cidToUnicodeCacheSize     4
#define unicodeToUnicodeCacheSize 4

//------------------------------------------------------------------------

GlobalParams *globalParams = NULL;

//------------------------------------------------------------------------
// PSFontParam16
//------------------------------------------------------------------------

PSFontParam16::PSFontParam16(GooString *nameA, int wModeA,
			     GooString *psFontNameA, GooString *encodingA) {
  name = nameA;
  wMode = wModeA;
  psFontName = psFontNameA;
  encoding = encodingA;
}

PSFontParam16::~PSFontParam16() {
  delete name;
  delete psFontName;
  delete encoding;
}

#if ENABLE_RELOCATABLE && defined(_WIN32)

/* search for data relative to where we are installed */

static HMODULE hmodule;

extern "C" {
BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
	 DWORD     fdwReason,
	 LPVOID    lpvReserved)
{
  switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      hmodule = hinstDLL;
      break;
    }

  return TRUE;
}
}

static const char *
get_poppler_datadir (void)
{
  static char retval[MAX_PATH];
  static int beenhere = 0;

  unsigned char *p;

  if (beenhere)
    return retval;

  if (!GetModuleFileName (hmodule, (CHAR *) retval, sizeof(retval) - 20))
    return POPPLER_DATADIR;

  p = _mbsrchr ((unsigned char *) retval, '\\');
  *p = '\0';
  p = _mbsrchr ((unsigned char *) retval, '\\');
  if (p) {
    if (stricmp ((const char *) (p+1), "bin") == 0)
      *p = '\0';
  }
  strcat (retval, "\\share\\poppler");

  beenhere = 1;

  return retval;
}

#undef POPPLER_DATADIR
#define POPPLER_DATADIR get_poppler_datadir ()

#endif

//------------------------------------------------------------------------
// SysFontInfo
//------------------------------------------------------------------------

class SysFontInfo {
public:

  GooString *name;
  GBool bold;
  GBool italic;
  GBool oblique;
  GBool fixedWidth;
  GooString *path;
  SysFontType type;
  int fontNum;			// for TrueType collections
  GooString *substituteName;

  SysFontInfo(GooString *nameA, GBool boldA, GBool italicA, GBool obliqueA, GBool fixedWidthA,
	      GooString *pathA, SysFontType typeA, int fontNumA, GooString *substituteNameA);
  ~SysFontInfo();
  GBool match(SysFontInfo *fi);
  GBool match(GooString *nameA, GBool boldA, GBool italicA, GBool obliqueA, GBool fixedWidthA);
  GBool match(GooString *nameA, GBool boldA, GBool italicA);
};

SysFontInfo::SysFontInfo(GooString *nameA, GBool boldA, GBool italicA, GBool obliqueA, GBool fixedWidthA,
			 GooString *pathA, SysFontType typeA, int fontNumA, GooString *substituteNameA) {
  name = nameA;
  bold = boldA;
  italic = italicA;
  oblique = obliqueA;
  fixedWidth = fixedWidthA;
  path = pathA;
  type = typeA;
  fontNum = fontNumA;
  substituteName = substituteNameA;
}

SysFontInfo::~SysFontInfo() {
  delete name;
  delete path;
  delete substituteName;
}

GBool SysFontInfo::match(SysFontInfo *fi) {
  return !strcasecmp(name->getCString(), fi->name->getCString()) &&
         bold == fi->bold && italic == fi->italic && oblique == fi->oblique && fixedWidth == fi->fixedWidth;
}

GBool SysFontInfo::match(GooString *nameA, GBool boldA, GBool italicA, GBool obliqueA, GBool fixedWidthA) {
  return !strcasecmp(name->getCString(), nameA->getCString()) &&
         bold == boldA && italic == italicA && oblique == obliqueA && fixedWidth == fixedWidthA;
}

GBool SysFontInfo::match(GooString *nameA, GBool boldA, GBool italicA) {
  return !strcasecmp(name->getCString(), nameA->getCString()) &&
         bold == boldA && italic == italicA;
}

//------------------------------------------------------------------------
// SysFontList
//------------------------------------------------------------------------

class SysFontList {
public:

  SysFontList();
  ~SysFontList();
  SysFontInfo *find(GooString *name, GBool isFixedWidth, GBool exact);

#ifdef WIN32
  void scanWindowsFonts(GooString *winFontDir);
#endif
#if defined(WITH_FONTCONFIGURATION_FONTCONFIG) || defined(__MORPHOS__) || defined(__AROS__)
  void addFcFont(SysFontInfo *si) {fonts->append(si);}
#endif
private:

#ifdef WIN32
  SysFontInfo *makeWindowsFont(char *name, int fontNum,
			       char *path);
#endif

  GooList *fonts;			// [SysFontInfo]
};

SysFontList::SysFontList() {
  fonts = new GooList();
}

SysFontList::~SysFontList() {
  deleteGooList(fonts, SysFontInfo);
}

SysFontInfo *SysFontList::find(GooString *name, GBool fixedWidth, GBool exact) {
  GooString *name2;
  GBool bold, italic, oblique;
  SysFontInfo *fi;
  char c;
  int n, i;

  name2 = name->copy();

  // remove space, comma, dash chars
  i = 0;
  while (i < name2->getLength()) {
    c = name2->getChar(i);
    if (c == ' ' || c == ',' || c == '-') {
      name2->del(i);
    } else {
      ++i;
    }
  }
  n = name2->getLength();

  // remove trailing "MT" (Foo-MT, Foo-BoldMT, etc.)
  if (n > 2 && !strcmp(name2->getCString() + n - 2, "MT")) {
    name2->del(n - 2, 2);
    n -= 2;
  }

  // look for "Regular"
  if (n > 7 && !strcmp(name2->getCString() + n - 7, "Regular")) {
    name2->del(n - 7, 7);
    n -= 7;
  }

  // look for "Italic"
  if (n > 6 && !strcmp(name2->getCString() + n - 6, "Italic")) {
    name2->del(n - 6, 6);
    italic = gTrue;
    n -= 6;
  } else {
    italic = gFalse;
  }

  // look for "Oblique"
  if (n > 6 && !strcmp(name2->getCString() + n - 7, "Oblique")) {
    name2->del(n - 7, 7);
    oblique = gTrue;
    n -= 6;
  } else {
    oblique = gFalse;
  }

  // look for "Bold"
  if (n > 4 && !strcmp(name2->getCString() + n - 4, "Bold")) {
    name2->del(n - 4, 4);
    bold = gTrue;
    n -= 4;
  } else {
    bold = gFalse;
  }

  // remove trailing "MT" (FooMT-Bold, etc.)
  if (n > 2 && !strcmp(name2->getCString() + n - 2, "MT")) {
    name2->del(n - 2, 2);
    n -= 2;
  }

  // remove trailing "PS"
  if (n > 2 && !strcmp(name2->getCString() + n - 2, "PS")) {
    name2->del(n - 2, 2);
    n -= 2;
  }

  // remove trailing "IdentityH"
  if (n > 9 && !strcmp(name2->getCString() + n - 9, "IdentityH")) {
    name2->del(n - 9, 9);
    n -= 9;
  }

  // search for the font
  fi = NULL;
  for (i = 0; i < fonts->getLength(); ++i) {
    fi = (SysFontInfo *)fonts->get(i);
    if (fi->match(name2, bold, italic, oblique, fixedWidth)) {
      break;
    }
    fi = NULL;
  }
  if (!fi && !exact && bold) {
    // try ignoring the bold flag
    for (i = 0; i < fonts->getLength(); ++i) {
      fi = (SysFontInfo *)fonts->get(i);
      if (fi->match(name2, gFalse, italic)) {
	break;
      }
      fi = NULL;
    }
  }
  if (!fi && !exact && (bold || italic)) {
    // try ignoring the bold and italic flags
    for (i = 0; i < fonts->getLength(); ++i) {
      fi = (SysFontInfo *)fonts->get(i);
      if (fi->match(name2, gFalse, gFalse)) {
	break;
      }
      fi = NULL;
    }
  }

  delete name2;
  return fi;
}


#ifdef ENABLE_PLUGINS
//------------------------------------------------------------------------
// Plugin
//------------------------------------------------------------------------

class Plugin {
public:

  static Plugin *load(char *type, char *name);
  ~Plugin();

private:

#ifdef _WIN32
  Plugin(HMODULE libA);
  HMODULE lib;
#else
  Plugin(void *dlA);
  void *dl;
#endif
};

Plugin *Plugin::load(char *type, char *name) {
  GooString *path;
  Plugin *plugin;
  XpdfPluginVecTable *vt;
  XpdfBool (*xpdfInitPlugin)(void);
#ifdef _WIN32
  HMODULE libA;
#else
  void *dlA;
#endif

  path = new GooString(POPPLER_DATADIR);
  appendToPath(path, "plugins");
  appendToPath(path, type);
  appendToPath(path, name);

#ifdef _WIN32
  path->append(".dll");
  if (!(libA = LoadLibrary(path->getCString()))) {
    error(errIO, -1, "Failed to load plugin '{0:t}'", path);
    goto err1;
  }
  if (!(vt = (XpdfPluginVecTable *)
	         GetProcAddress(libA, "xpdfPluginVecTable"))) {
    error(errIO, -1, "Failed to find xpdfPluginVecTable in plugin '{0:t}'",
	  path);
    goto err2;
  }
#else
  //~ need to deal with other extensions here
  path->append(".so");
  if (!(dlA = dlopen(path->getCString(), RTLD_NOW))) {
    error(errIO, -1, "Failed to load plugin '{0:t}': {1:s}",
	  path, dlerror());
    goto err1;
  }
  if (!(vt = (XpdfPluginVecTable *)dlsym(dlA, "xpdfPluginVecTable"))) {
    error(errIO, -1, "Failed to find xpdfPluginVecTable in plugin '{0:t}'",
	  path);
    goto err2;
  }
#endif

  if (vt->version != xpdfPluginVecTable.version) {
    error(errIO, -1, "Plugin '{0:t}' is wrong version", path);
    goto err2;
  }
  memcpy(vt, &xpdfPluginVecTable, sizeof(xpdfPluginVecTable));

#ifdef _WIN32
  if (!(xpdfInitPlugin = (XpdfBool (*)(void))
	                     GetProcAddress(libA, "xpdfInitPlugin"))) {
    error(errIO, -1, "Failed to find xpdfInitPlugin in plugin '{0:t}'",
	  path);
    goto err2;
  }
#else
  if (!(xpdfInitPlugin = (XpdfBool (*)(void))dlsym(dlA, "xpdfInitPlugin"))) {
    error(errIO, -1, "Failed to find xpdfInitPlugin in plugin '{0:t}'",
	  path);
    goto err2;
  }
#endif

  if (!(*xpdfInitPlugin)()) {
    error(errIO, -1, "Initialization of plugin '{0:t}' failed", path);
    goto err2;
  }

#ifdef _WIN32
  plugin = new Plugin(libA);
#else
  plugin = new Plugin(dlA);
#endif

  delete path;
  return plugin;

 err2:
#ifdef _WIN32
  FreeLibrary(libA);
#else
  dlclose(dlA);
#endif
 err1:
  delete path;
  return NULL;
}

#ifdef _WIN32
Plugin::Plugin(HMODULE libA) {
  lib = libA;
}
#else
Plugin::Plugin(void *dlA) {
  dl = dlA;
}
#endif

Plugin::~Plugin() {
  void (*xpdfFreePlugin)(void);

#ifdef _WIN32
  if ((xpdfFreePlugin = (void (*)(void))
                            GetProcAddress(lib, "xpdfFreePlugin"))) {
    (*xpdfFreePlugin)();
  }
  FreeLibrary(lib);
#else
  if ((xpdfFreePlugin = (void (*)(void))dlsym(dl, "xpdfFreePlugin"))) {
    (*xpdfFreePlugin)();
  }
  dlclose(dl);
#endif
}

#endif // ENABLE_PLUGINS

//------------------------------------------------------------------------
// parsing
//------------------------------------------------------------------------

GlobalParams::GlobalParams(const char *customPopplerDataDir)
  : popplerDataDir(customPopplerDataDir)
{
  UnicodeMap *map;
  int i;

#if MULTITHREADED
  gInitMutex(&mutex);
  gInitMutex(&unicodeMapCacheMutex);
  gInitMutex(&cMapCacheMutex);
#endif

  initBuiltinFontTables();

  // scan the encoding in reverse because we want the lowest-numbered
  // index for each char name ('space' is encoded twice)
  macRomanReverseMap = new NameToCharCode();
  for (i = 255; i >= 0; --i) {
    if (macRomanEncoding[i]) {
      macRomanReverseMap->add(macRomanEncoding[i], (CharCode)i);
    }
  }

#ifdef _WIN32
  substFiles = new GooHash(gTrue);
#endif
  nameToUnicode = new NameToCharCode();
  cidToUnicodes = new GooHash(gTrue);
  unicodeToUnicodes = new GooHash(gTrue);
  residentUnicodeMaps = new GooHash();
  unicodeMaps = new GooHash(gTrue);
  cMapDirs = new GooHash(gTrue);
  toUnicodeDirs = new GooList();
  fontFiles = new GooHash(gTrue);
  fontDirs = new GooList();
  ccFontFiles = new GooHash(gTrue);
  sysFonts = new SysFontList();
  psExpandSmaller = gFalse;
  psShrinkLarger = gTrue;
  psCenter = gTrue;
  psLevel = psLevel2;
  psFile = NULL;
  psResidentFonts = new GooHash(gTrue);
  psResidentFonts16 = new GooList();
  psResidentFontsCC = new GooList();
  psEmbedType1 = gTrue;
  psEmbedTrueType = gTrue;
  psEmbedCIDPostScript = gTrue;
  psEmbedCIDTrueType = gTrue;
  psFontPassthrough = gFalse;
  psPreload = gFalse;
  psOPI = gFalse;
  psASCIIHex = gFalse;
  psBinary = gFalse;
  psUncompressPreloadedImages = gFalse;
  psRasterResolution = 300;
  psRasterMono = gFalse;
  textEncoding = new GooString("UTF-8");
#if defined(_WIN32)
  textEOL = eolDOS;
#elif defined(MACOS)
  textEOL = eolMac;
#else
  textEOL = eolUnix;
#endif
  textPageBreaks = gTrue;
  textKeepTinyChars = gFalse;
  enableFreeType = gTrue;
  antialias = gTrue;
  vectorAntialias = gTrue;
  antialiasPrinting = gFalse;
  strokeAdjust = gTrue;
  screenType = screenUnset;
  screenSize = -1;
  screenDotRadius = -1;
  screenGamma = 1.0;
  screenBlackThreshold = 0.0;
  screenWhiteThreshold = 1.0;
  minLineWidth = 0.0;
  overprintPreview = gFalse;
  mapNumericCharNames = gTrue;
  mapUnknownCharNames = gFalse;
  printCommands = gFalse;
  profileCommands = gFalse;
  errQuiet = gFalse;
  fontcache = NULL;

  cidToUnicodeCache = new CharCodeToUnicodeCache(cidToUnicodeCacheSize);
  unicodeToUnicodeCache =
      new CharCodeToUnicodeCache(unicodeToUnicodeCacheSize);
  unicodeMapCache = new UnicodeMapCache();
  cMapCache = new CMapCache();

  baseFontsInitialized = gFalse;
#ifdef ENABLE_PLUGINS
  plugins = new GooList();
  securityHandlers = new GooList();
#endif

  // set up the initial nameToUnicode table
  for (i = 0; nameToUnicodeTab[i].name; ++i) {
    nameToUnicode->add(nameToUnicodeTab[i].name, nameToUnicodeTab[i].u);
  }

  // set up the residentUnicodeMaps table
  map = new UnicodeMap("Latin1", gFalse,
		       latin1UnicodeMapRanges, latin1UnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("ASCII7", gFalse,
		       ascii7UnicodeMapRanges, ascii7UnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("Symbol", gFalse,
		       symbolUnicodeMapRanges, symbolUnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("ZapfDingbats", gFalse, zapfDingbatsUnicodeMapRanges,
		       zapfDingbatsUnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("UTF-8", gTrue, &mapUTF8);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("UCS-2", gTrue, &mapUCS2);
  residentUnicodeMaps->add(map->getEncodingName(), map);

  scanEncodingDirs();
}


void GlobalParams::scanEncodingDirs() {
  GDir *dir;
  GDirEntry *entry;
  const char *dataRoot = popplerDataDir ? popplerDataDir : POPPLER_DATADIR;

  // allocate buffer large enough to append "/nameToUnicode"
  size_t bufSize = strlen(dataRoot) + strlen("/nameToUnicode") + 1;
  char *dataPathBuffer = new char[bufSize];
  
  snprintf(dataPathBuffer, bufSize, "%s/nameToUnicode", dataRoot);

  dir = new GDir(dataPathBuffer, gTrue);

  while (entry = dir->getNextEntry(), entry != NULL) {
    if (!entry->isDir()) {
		/*kprintf("parsding:%s\n", entry->getFullPath());*/
      parseNameToUnicode(entry->getFullPath());
    }
    delete entry;
  }
  delete dir;

  snprintf(dataPathBuffer, bufSize, "%s/cidToUnicode", dataRoot);
  dir = new GDir(dataPathBuffer, gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    addCIDToUnicode(entry->getName(), entry->getFullPath());
    delete entry;
  }
  delete dir;

  snprintf(dataPathBuffer, bufSize, "%s/unicodeMap", dataRoot);
  dir = new GDir(dataPathBuffer, gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    addUnicodeMap(entry->getName(), entry->getFullPath());
    delete entry;
  }
  delete dir;

  snprintf(dataPathBuffer, bufSize, "%s/cMap", dataRoot);
  dir = new GDir(dataPathBuffer, gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    addCMapDir(entry->getName(), entry->getFullPath());
    toUnicodeDirs->append(entry->getFullPath()->copy());
    delete entry;
  }
  delete dir;

  delete[] dataPathBuffer;
}

void GlobalParams::parseNameToUnicode(GooString *name) {
  char *tok1, *tok2;
  FILE *f;
  char buf[256];
  int line;
  Unicode u;
  char *tokptr;

  if (!(f = openFile(name->getCString(), "r"))) {
    error(errIO, -1, "Couldn't open 'nameToUnicode' file '{0:t}'",
	  name);
    return;
  }
  line = 1;
  while (getLine(buf, sizeof(buf), f)) {
    tok1 = strtok_r(buf, " \t\r\n", &tokptr);
    tok2 = strtok_r(NULL, " \t\r\n", &tokptr);
    if (tok1 && tok2) {
      sscanf(tok1, "%x", &u);
      nameToUnicode->add(tok2, u);
    } else {
      error(errConfig, -1, "Bad line in 'nameToUnicode' file ({0:t}:{1:d})",
	    name, line);
    }
    ++line;
  }
  fclose(f);
}

void GlobalParams::addCIDToUnicode(GooString *collection,
				   GooString *fileName) {
  GooString *old;

  if ((old = (GooString *)cidToUnicodes->remove(collection))) {
    delete old;
  }
  cidToUnicodes->add(collection->copy(), fileName->copy());
}

void GlobalParams::addUnicodeMap(GooString *encodingName, GooString *fileName)
{
  GooString *old;

  if ((old = (GooString *)unicodeMaps->remove(encodingName))) {
    delete old;
  }
  unicodeMaps->add(encodingName->copy(), fileName->copy());
}

void GlobalParams::addCMapDir(GooString *collection, GooString *dir) {
  GooList *list;

  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    list = new GooList();
    cMapDirs->add(collection->copy(), list);
  }
  list->append(dir->copy());
}

GBool GlobalParams::parseYesNo2(const char *token, GBool *flag) {
  if (!strcmp(token, "yes")) {
    *flag = gTrue;
  } else if (!strcmp(token, "no")) {
    *flag = gFalse;
  } else {
    return gFalse;
  }
  return gTrue;
}

GlobalParams::~GlobalParams() {
  freeBuiltinFontTables();

  delete macRomanReverseMap;

  delete nameToUnicode;
  deleteGooHash(cidToUnicodes, GooString);
  deleteGooHash(unicodeToUnicodes, GooString);
  deleteGooHash(residentUnicodeMaps, UnicodeMap);
  deleteGooHash(unicodeMaps, GooString);
  deleteGooList(toUnicodeDirs, GooString);
  deleteGooHash(fontFiles, GooString);
  deleteGooList(fontDirs, GooString);
  deleteGooHash(ccFontFiles, GooString);
#ifdef _WIN32
  deleteGooHash(substFiles, GooString);
#endif
  delete sysFonts;
  if (psFile) {
    delete psFile;
  }
  deleteGooHash(psResidentFonts, GooString);
  deleteGooList(psResidentFonts16, PSFontParam16);
  deleteGooList(psResidentFontsCC, PSFontParam16);
  delete textEncoding;

  GooHashIter *iter;
  GooString *key;
  cMapDirs->startIter(&iter);
  void *val;
  while (cMapDirs->getNext(&iter, &key, &val)) {
    GooList* list = (GooList*)val;
    deleteGooList(list, GooString);
  }
  delete cMapDirs;

  delete cidToUnicodeCache;
  delete unicodeToUnicodeCache;
  delete unicodeMapCache;
  delete cMapCache;

#ifdef ENABLE_PLUGINS
  delete securityHandlers;
  deleteGooList(plugins, Plugin);
#endif

#if MULTITHREADED
  gDestroyMutex(&mutex);
  gDestroyMutex(&unicodeMapCacheMutex);
  gDestroyMutex(&cMapCacheMutex);
#endif
}


//------------------------------------------------------------------------
// accessors
//------------------------------------------------------------------------

CharCode GlobalParams::getMacRomanCharCode(char *charName) {
  // no need to lock - macRomanReverseMap is constant
  return macRomanReverseMap->lookup(charName);
}

Unicode GlobalParams::mapNameToUnicode(const char *charName) {
  // no need to lock - nameToUnicode is constant
  return nameToUnicode->lookup(charName);
}

UnicodeMap *GlobalParams::getResidentUnicodeMap(GooString *encodingName) {
  UnicodeMap *map;

  lockGlobalParams;
  map = (UnicodeMap *)residentUnicodeMaps->lookup(encodingName);
  unlockGlobalParams;
  if (map) {
    map->incRefCnt();
  }
  return map;
}

FILE *GlobalParams::getUnicodeMapFile(GooString *encodingName) {
  GooString *fileName;
  FILE *f;

  lockGlobalParams;
  if ((fileName = (GooString *)unicodeMaps->lookup(encodingName))) {
    f = openFile(fileName->getCString(), "r");
  } else {
    f = NULL;
  }
  unlockGlobalParams;
  return f;
}

FILE *GlobalParams::findCMapFile(GooString *collection, GooString *cMapName) {
  GooList *list;
  GooString *dir;
  GooString *fileName;
  FILE *f;
  int i;

  lockGlobalParams;
  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    unlockGlobalParams;
    return NULL;
  }
  for (i = 0; i < list->getLength(); ++i) {
    dir = (GooString *)list->get(i);
    fileName = appendToPath(dir->copy(), cMapName->getCString());
    f = openFile(fileName->getCString(), "r");
    delete fileName;
    if (f) {
      unlockGlobalParams;
      return f;
    }
  }
  unlockGlobalParams;
  return NULL;
}

FILE *GlobalParams::findToUnicodeFile(GooString *name) {
  GooString *dir, *fileName;
  FILE *f;
  int i;

  lockGlobalParams;
  for (i = 0; i < toUnicodeDirs->getLength(); ++i) {
    dir = (GooString *)toUnicodeDirs->get(i);
    fileName = appendToPath(dir->copy(), name->getCString());
    f = openFile(fileName->getCString(), "r");
    delete fileName;
    if (f) {
      unlockGlobalParams;
      return f;
    }
  }
  unlockGlobalParams;
  return NULL;
}

#if WITH_FONTCONFIGURATION_FONTCONFIG
static GBool findModifier(const char *name, const char *modifier, const char **start)
{
  const char *match;

  if (name == NULL)
    return gFalse;

  match = strstr(name, modifier);
  if (match) {
    if (*start == NULL || match < *start)
      *start = match;
    return gTrue;
  }
  else {
    return gFalse;
  }
}

static const char *getFontLang(GfxFont *font)
{
  const char *lang;

  // find the language we want the font to support
  if (font->isCIDFont())
  {
    GooString *collection = ((GfxCIDFont *)font)->getCollection();
    if (collection)
    {
      if (strcmp(collection->getCString(), "Adobe-GB1") == 0)
        lang = "zh-cn"; // Simplified Chinese
      else if (strcmp(collection->getCString(), "Adobe-CNS1") == 0)
        lang = "zh-tw"; // Traditional Chinese
      else if (strcmp(collection->getCString(), "Adobe-Japan1") == 0)
        lang = "ja"; // Japanese
      else if (strcmp(collection->getCString(), "Adobe-Japan2") == 0)
        lang = "ja"; // Japanese
      else if (strcmp(collection->getCString(), "Adobe-Korea1") == 0)
        lang = "ko"; // Korean
      else if (strcmp(collection->getCString(), "Adobe-UCS") == 0)
        lang = "xx";
      else if (strcmp(collection->getCString(), "Adobe-Identity") == 0)
        lang = "xx";
      else
      {
        error(errUnimplemented, -1, "Unknown CID font collection, please report to poppler bugzilla.");
        lang = "xx";
      }
    }
    else lang = "xx";
  }
  else lang = "xx";
  return lang;
}

static FcPattern *buildFcPattern(GfxFont *font, GooString *base14Name)
{
  int weight = -1,
      slant = -1,
      width = -1,
      spacing = -1;
  bool deleteFamily = false;
  char *family, *name, *modifiers;
  const char *start;
  FcPattern *p;

  // this is all heuristics will be overwritten if font had proper info
  name = (base14Name == NULL) ? font->getName()->getCString() : base14Name->getCString();
  
  modifiers = strchr (name, ',');
  if (modifiers == NULL)
    modifiers = strchr (name, '-');
  
  // remove the - from the names, for some reason, Fontconfig does not
  // understand "MS-Mincho" but does with "MS Mincho"
  int len = strlen(name);
  for (int i = 0; i < len; i++)
    name[i] = (name[i] == '-' ? ' ' : name[i]);

  start = NULL;
  findModifier(modifiers, "Regular", &start);
  findModifier(modifiers, "Roman", &start);
  
  if (findModifier(modifiers, "Oblique", &start))
    slant = FC_SLANT_OBLIQUE;
  if (findModifier(modifiers, "Italic", &start))
    slant = FC_SLANT_ITALIC;
  if (findModifier(modifiers, "Bold", &start))
    weight = FC_WEIGHT_BOLD;
  if (findModifier(modifiers, "Light", &start))
    weight = FC_WEIGHT_LIGHT;
  if (findModifier(modifiers, "Medium", &start))
    weight = FC_WEIGHT_MEDIUM;
  if (findModifier(modifiers, "Condensed", &start))
    width = FC_WIDTH_CONDENSED;
  
  if (start) {
    // There have been "modifiers" in the name, crop them to obtain
    // the family name
    family = new char[len+1];
    strcpy(family, name);
    int pos = (modifiers - name);
    family[pos] = '\0';
    deleteFamily = true;
  }
  else {
    family = name;
  }
  
  // use font flags
  if (font->isFixedWidth())
    spacing = FC_MONO;
  if (font->isBold())
    weight = FC_WEIGHT_BOLD;
  if (font->isItalic())
    slant = FC_SLANT_ITALIC;
  
  // if the FontDescriptor specified a family name use it
  if (font->getFamily()) {
    if (deleteFamily) {
      delete[] family;
      deleteFamily = false;
    }
    family = font->getFamily()->getCString();
  }
  
  // if the FontDescriptor specified a weight use it
  switch (font -> getWeight())
  {
    case GfxFont::W100: weight = FC_WEIGHT_EXTRALIGHT; break; 
    case GfxFont::W200: weight = FC_WEIGHT_LIGHT; break; 
    case GfxFont::W300: weight = FC_WEIGHT_BOOK; break; 
    case GfxFont::W400: weight = FC_WEIGHT_NORMAL; break; 
    case GfxFont::W500: weight = FC_WEIGHT_MEDIUM; break; 
    case GfxFont::W600: weight = FC_WEIGHT_DEMIBOLD; break; 
    case GfxFont::W700: weight = FC_WEIGHT_BOLD; break; 
    case GfxFont::W800: weight = FC_WEIGHT_EXTRABOLD; break; 
    case GfxFont::W900: weight = FC_WEIGHT_BLACK; break; 
    default: break; 
  }
  
  // if the FontDescriptor specified a width use it
  switch (font -> getStretch())
  {
    case GfxFont::UltraCondensed: width = FC_WIDTH_ULTRACONDENSED; break; 
    case GfxFont::ExtraCondensed: width = FC_WIDTH_EXTRACONDENSED; break; 
    case GfxFont::Condensed: width = FC_WIDTH_CONDENSED; break; 
    case GfxFont::SemiCondensed: width = FC_WIDTH_SEMICONDENSED; break; 
    case GfxFont::Normal: width = FC_WIDTH_NORMAL; break; 
    case GfxFont::SemiExpanded: width = FC_WIDTH_SEMIEXPANDED; break; 
    case GfxFont::Expanded: width = FC_WIDTH_EXPANDED; break; 
    case GfxFont::ExtraExpanded: width = FC_WIDTH_EXTRAEXPANDED; break; 
    case GfxFont::UltraExpanded: width = FC_WIDTH_ULTRAEXPANDED; break; 
    default: break; 
  }
  
  const char *lang = getFontLang(font);
  
  p = FcPatternBuild(NULL,
                    EFC_FAMILY, FcTypeString, family,
                    EFC_LANG, FcTypeString, lang,
                    NULL);
  if (slant != -1) FcPatternAddInteger(p, FC_SLANT, slant);
  if (weight != -1) FcPatternAddInteger(p, FC_WEIGHT, weight);
  if (width != -1) FcPatternAddInteger(p, FC_WIDTH, width);
  if (spacing != -1) FcPatternAddInteger(p, FC_SPACING, spacing);

  if (deleteFamily)
    delete[] family;
  return p;
}
#endif

GooString *GlobalParams::findFontFile(GooString *fontName) {
  static const char *exts[] = { ".pfa", ".pfb", ".ttf", ".ttc", ".otf" };
  GooString *path, *dir;
#ifdef WIN32
  GooString *fontNameU;
#endif
  const char *ext;
  FILE *f;
  int i, j;

  setupBaseFonts(NULL);
  lockGlobalParams;
  if ((path = (GooString *)fontFiles->lookup(fontName))) {
    path = path->copy();
    unlockGlobalParams;
    return path;
  }
  for (i = 0; i < fontDirs->getLength(); ++i) {
    dir = (GooString *)fontDirs->get(i);
    for (j = 0; j < (int)(sizeof(exts) / sizeof(exts[0])); ++j) {
      ext = exts[j];
#ifdef WIN32
      fontNameU = fileNameToUTF8(fontName->getCString());
      path = appendToPath(dir->copy(), fontNameU->getCString());
      delete fontNameU;
#else
      path = appendToPath(dir->copy(), fontName->getCString());
#endif
      path->append(ext);
      if ((f = openFile(path->getCString(), "rb"))) {
	fclose(f);
	unlockGlobalParams;
	return path;
      }
      delete path;
    }
  }
  unlockGlobalParams;
  return NULL;
}

static struct fontcache *setupFontCache()
{
	char *fonts[] = {"sys:fonts", "mossys:fonts", "PROGDIR:fonts", NULL};
	struct fontcache *fontcache;
	int isnew = FALSE;

	struct Process *process = (struct Process*)FindTask(NULL);
	struct Window *oldWindowPtr = (struct Window*)process->pr_WindowPtr;
	process->pr_WindowPtr = (struct Window*)-1;
		
	fontcache = fcLoad("PROGDIR:vpdf.fontcache");
	D(kprintf("checking progdir. %p\n", fontcache));
	if (fontcache == NULL)
	{
		/* retry from T: in case we had PROGDIR write protected */
		fontcache = fcLoad("T:vpdf.fontcache");
		D(kprintf("checking t. %p\n", fontcache));
		if (fontcache == NULL)
		{
			isnew = TRUE;
			fontcache = fcCreate(fonts);
		}
    }
    
    /* add builtin substitutions to the cache */
    
    if (fontcache != NULL)	
    {
    	int i = 0;
		while(displayFontTab[i].name != NULL)
		{
			struct fontpattern *pat = fcPatternAlloc();
			if (pat != NULL)
			{
				char *name = displayFontTab[i].name;
				char family[128];
				char path[512];
				int j;
				int exists = FALSE;
				
				strcpy(family, name);
				if (strstr(family, ","))
					*strstr(family, ",") = '\0';
				if (strstr(family, "-"))
					*strstr(family, "-") = '\0';
			
				fcPatternAddString(pat, EFC_FAMILY, family);
				fcPatternAddString(pat, EFC_FULLNAME, name);
				
				j=0; 
				while(fonts[j] != NULL)
				{
					if (displayFontTab[i].t1FileName != NULL)
					{
						snprintf(path, sizeof(path), "%s/%s", fonts[j], displayFontTab[i].t1FileName);
						if (FileExists(path))
						{
							fcPatternAddString(pat, EFC_FILE, path);
							exists = TRUE;
						}
					}
					if (displayFontTab[i].ttFileName != NULL)
					{
						snprintf(path, sizeof(path), "%s/%s", fonts[j], displayFontTab[i].ttFileName);
						if (FileExists(path))
						{
							fcPatternAddString(pat, EFC_FILE, path);
							exists = TRUE;
						}
					}
					j++;
				}
					
				if (strstr(name, "Italic"))
					fcPatternAddInteger(pat, EFC_SLANT, FC_SLANT_ITALIC);
				if (strstr(name, "Roman"))
					fcPatternAddInteger(pat, EFC_SLANT, FC_SLANT_ROMAN);
				if (strstr(name, "Bold"))
					fcPatternAddInteger(pat, EFC_WEIGHT, FC_WEIGHT_BOLD);
				else
					fcPatternAddInteger(pat, EFC_WEIGHT, FC_WEIGHT_REGULAR);
					
				if (exists)
					fcAddPattern(fontcache, pat);
				else
					fcPatternDestroy(pat);
			}
			
			i++;
		}
		
		if (isnew)
		{
			/* save cache. main dir is PROGDIR: but when it fails (write protected?) try T: */
			if (fcSave(fontcache, "PROGDIR:vpdf.fontcache") == FALSE)
			{
				fcSave(fontcache, "T:vpdf.fontcache");
			}
		}
    }
    
    D(kprintf("fontcache loaded to %p\n", fontcache));
    
    process->pr_WindowPtr = oldWindowPtr;
    
    /* TODO: handle some substitution rules here? */
    
    return fontcache;
}

static struct fontpattern *buildFcPattern(struct fontpattern *p, GfxFont *font, GooString *base14Name)
{
	int weight = -1,
		slant = -1,
		width = -1;
		
	bool deleteFamily = false;
	char *family, *name, *lang, *modifiers;
	const char *start;
	
	// this is all heuristics will be overwritten if font had proper info
	name = base14Name != NULL ? base14Name->getCString() : font->getName()->getCString();
	//printf("  building font substitution for font:%s, family:%s, serif:%d, symbolic:%d\n", name, font->getFamily()->getCString(), font->isSerif(), font->isSymbolic());

	modifiers = strchr (name, ',');
	if (modifiers == NULL)
		modifiers = strchr (name, '-');

	// remove the - from the names, for some reason, Fontconfig does not understand "MS-Mincho" but does with "MS Mincho"
	int len = strlen(name);
	for (int i = 0; i < len; i++)
		name[i] = (name[i] == '-' ? ' ' : name[i]);

	start = NULL;
	findModifier(modifiers, "Regular", &start);
	findModifier(modifiers, "Roman", &start);

	if (findModifier(modifiers, "Oblique", &start))
		slant = FC_SLANT_OBLIQUE;
	if (findModifier(modifiers, "Italic", &start))
		slant = FC_SLANT_ITALIC;
	if (findModifier(modifiers, "Bold", &start))
		weight = FC_WEIGHT_BOLD;
	if (findModifier(modifiers, "Light", &start))
		weight = FC_WEIGHT_LIGHT;
	if (findModifier(modifiers, "Condensed", &start))
		width = FC_WIDTH_CONDENSED;

	if (start)
	{
		// There have been "modifiers" in the name, crop them to obtain the family name
		family = new char[len+1];
		strcpy(family, name);
		int pos = (modifiers - name);
		family[pos] = '\0';
		deleteFamily = true;
	}
	else
	{
		family = name;
	}

	//printf("   family: %s\n", family != NULL ? family : "no family");

	// use font flags
	if (font->isBold())
		weight = FC_WEIGHT_BOLD;
	if (font->isItalic())
		slant = FC_SLANT_ITALIC;

	// if the FontDescriptor specified a family name use it
	if (font->getFamily()) {
		if (deleteFamily)
		{
			delete[] family;
			deleteFamily = false;
		}
		family = font->getFamily()->getCString();
	}

	// if the FontDescriptor specified a weight use it
	switch (font -> getWeight())
	{
		case GfxFont::W100: weight = FC_WEIGHT_EXTRALIGHT; break; 
		case GfxFont::W200: weight = FC_WEIGHT_LIGHT; break; 
		case GfxFont::W300: weight = FC_WEIGHT_BOOK; break; 
		case GfxFont::W400: weight = FC_WEIGHT_NORMAL; break; 
		case GfxFont::W500: weight = FC_WEIGHT_MEDIUM; break; 
		case GfxFont::W600: weight = FC_WEIGHT_DEMIBOLD; break; 
		case GfxFont::W700: weight = FC_WEIGHT_BOLD; break; 
		case GfxFont::W800: weight = FC_WEIGHT_EXTRABOLD; break; 
		case GfxFont::W900: weight = FC_WEIGHT_BLACK; break; 
		default: break; 
	}

	// if the FontDescriptor specified a width use it
	switch (font -> getStretch())
	{
		case GfxFont::UltraCondensed: width = FC_WIDTH_ULTRACONDENSED; break; 
		case GfxFont::ExtraCondensed: width = FC_WIDTH_EXTRACONDENSED; break; 
		case GfxFont::Condensed: width = FC_WIDTH_CONDENSED; break; 
		case GfxFont::SemiCondensed: width = FC_WIDTH_SEMICONDENSED; break; 
		case GfxFont::Normal: width = FC_WIDTH_NORMAL; break; 
		case GfxFont::SemiExpanded: width = FC_WIDTH_SEMIEXPANDED; break; 
		case GfxFont::Expanded: width = FC_WIDTH_EXPANDED; break; 
		case GfxFont::ExtraExpanded: width = FC_WIDTH_EXTRAEXPANDED; break; 
		case GfxFont::UltraExpanded: width = FC_WIDTH_ULTRAEXPANDED; break; 
		default: break; 
	}

	// find the language we want the font to support
	if (font->isCIDFont())
	{
		GooString *collection = ((GfxCIDFont *)font)->getCollection();
		if (collection)
		{
			if (strcmp(collection->getCString(), "Adobe-GB1") == 0)
				lang = "zh-cn"; // Simplified Chinese
			else if (strcmp(collection->getCString(), "Adobe-CNS1") == 0)
				lang = "zh-tw"; // Traditional Chinese
			else if (strcmp(collection->getCString(), "Adobe-Japan1") == 0)
				lang = "ja"; // Japanese
			else if (strcmp(collection->getCString(), "Adobe-Japan2") == 0)
				lang = "ja"; // Japanese
			else if (strcmp(collection->getCString(), "Adobe-Korea1") == 0)
				lang = "ko"; // Korean
			else if (strcmp(collection->getCString(), "Adobe-UCS") == 0)
				lang = "xx";
			else if (strcmp(collection->getCString(), "Adobe-Identity") == 0)
				lang = "xx";
			else
			{
				error(errInternal, -1, "Unknown CID font collection, please report to poppler bugzilla.");
				lang = "xx";
			}
		}
		else
			lang = "xx";
	}
	else
		lang = "xx";

	fcPatternAddString(p, EFC_FAMILY, family);
	fcPatternAddString(p, EFC_LANG, lang);

	if (weight != -1)
		fcPatternAddInteger(p, EFC_WEIGHT, weight);
	else if (!font->isBold())
		fcPatternAddInteger(p, EFC_WEIGHT, FC_WEIGHT_NORMAL);
		
	if (width != -1) fcPatternAddInteger(p, EFC_WIDTH, width);
	
	if (slant != -1)
		fcPatternAddInteger(p, EFC_SLANT, slant);
	else if (!font->isItalic())
		fcPatternAddExcludingInteger(p, EFC_SLANT, FC_SLANT_ITALIC);
	
	if (font->isFixedWidth())
		fcPatternAddInteger(p, EFC_SPACING, FC_MONO);
	else 
		fcPatternAddExcludingInteger(p, EFC_SPACING, FC_MONO);
		
	if (font->isSerif())
		fcPatternAddInteger(p, EFC_SERIF, TRUE);
	else
		fcPatternAddExcludingInteger(p, EFC_SERIF, TRUE);
	
	if (deleteFamily)
		delete[] family;
		
	return p;
}


void GlobalParams::setupBaseFonts(char * dir)
{
}

GooString *GlobalParams::findSystemFontFile(GfxFont *font,
					  SysFontType *type,
					  int *fontNum, GooString *substituteFontName, GooString *base14Name)
{
	SysFontInfo *fi = NULL;
	fontpattern *p = NULL;
	GooString *path = NULL;
	GooString substituteName;
	GooString *fontName = font->getName();
	GooString *lookupName = base14Name != NULL ? base14Name : font->getName();
	if (!fontName) return NULL;
		
	lockGlobalParams;
	
	if (fontcache == NULL)
	{
		D(kprintf("setup font cache...\n"));
		fontcache = setupFontCache();
	}

	kprintf("font lookup :%s:%s:%s\n", fontName->getCString(), substituteFontName != NULL ? substituteFontName->getCString() : "nosubst", base14Name != NULL ? base14Name->getCString() : "nobase14");

	if ((fi = sysFonts->find(lookupName, font->isFixedWidth(), gTrue)))
	{
	kprintf("Found if sysFonts\n");
		path = fi->path->copy();
		*type = fi->type;
		*fontNum = fi->fontNum;
		substituteName.Set(fi->substituteName->getCString());
	}
	else
	{
		kprintf("Not Found if sysFonts\n");
		char *s, *s2;
		char *ext;
		struct fontpattern *match;
		int matchingcriteria[EFC_LAST_CRITERIA];
		
		p = fcPatternAlloc();
		buildFcPattern(p, font, base14Name);
		
		if (p == NULL)
			goto fin;

		kprintf("About to call fcMatch\n");

		match = fcMatch((struct fontcache*)fontcache, p, matchingcriteria);
		// if we have a match but it is a shitty one (no filename, no family)
		// then pick one of the builtins
		kprintf("Called fcMatch\n");

		
		if (matchingcriteria[EFC_FAMILY] == FALSE && //FC_FAMILY - messed up enum vs const strings?? wtf
			matchingcriteria[EFC_FILE] == FALSE && //FC_FILE
			matchingcriteria[EFC_FULLNAME] == FALSE) //FC_FULLNAME
		{
		
			//s = fcPatternGetString(match, FC_FILE, 0);
			//s2 = fcPatternGetString(match, FC_FULLNAME, 0);
			//kprintf("shitty mached one:%s:%s, retry\n", s, s2);
			
			if (font->isFixedWidth())
				fcPatternAddString(p, EFC_FAMILY, "Courier");
			else
				fcPatternAddString(p, EFC_FAMILY, "Times");
			kprintf("About to call fcMatch after adding criteria\n");

			match = fcMatch((struct fontcache*)fontcache, p, matchingcriteria);
			kprintf("Called fcMatch (Again)\n");
		}
		
		if (match != NULL) /* TODO: think what to do in this case */
		{
			
			s = fcPatternGetString(match, EFC_FILE, 0);
			s2 = fcPatternGetString(match, EFC_FULLNAME, 0);
			
			kprintf("mached one:%s:%s\n", s, s2);
			
			ext = strrchr(s, '.');
			
			if (s2 != NULL)
				substituteName.Set((char*)s2);
			else
			{
				// fontconfig does not extract fullname for some fonts
				// create the fullname from family and style
				s2 = fcPatternGetString(match, EFC_FAMILY, 0);
				if (s2 != NULL)
				{
					substituteName.Set((char*)s2);
					s2 = fcPatternGetString(match, EFC_STYLE, 0);
					if (s2 != NULL)
					{
						GooString *style = new GooString((char*)s2);
						if (style->cmp("Regular") != 0)
						{
							substituteName.append(" ");
							substituteName.append(style);
						}
						delete style;
					}
				}
			}

			if (ext == NULL)
				goto fin; /* TODO: ok, this is silly and shouldn't happen i guess */
				
			if (!strncasecmp(ext,".ttf",4) || !strncasecmp(ext, ".ttc", 4))
			{
				int weight, slant;
				GBool bold = font->isBold();
				GBool italic = font->isItalic();
				GBool oblique = gFalse;

				weight = fcPatternGetInteger(match, EFC_WEIGHT, 0);
				slant = fcPatternGetInteger(match, EFC_SLANT, 0);

				if (weight == FC_WEIGHT_DEMIBOLD || weight == FC_WEIGHT_BOLD
				    || weight == FC_WEIGHT_EXTRABOLD || weight == FC_WEIGHT_BLACK)
				{
					bold = gTrue;
				}
				if (slant == FC_SLANT_ITALIC)
					italic = gTrue;
				if (slant == FC_SLANT_OBLIQUE)
					oblique = gTrue;

				*fontNum = 0;
				*type = (!strncasecmp(ext,".ttc",4)) ? sysFontTTC : sysFontTTF;
				*fontNum = fcPatternGetInteger(match, EFC_INDEX, 0);

				fi = new SysFontInfo(fontName->copy(), bold, italic, oblique, font->isFixedWidth(), new GooString((char*)s), *type, *fontNum, substituteName.copy());
				sysFonts->addFcFont(fi);
				path = new GooString((char*)s);
			}
			else if (!strncasecmp(ext,".pfa",4) || !strncasecmp(ext,".pfb",4)) 
			{
				int weight, slant;
				GBool bold = font->isBold();
				GBool italic = font->isItalic();
				GBool oblique = gFalse;

				weight = fcPatternGetInteger(match, EFC_WEIGHT, 0);
				slant = fcPatternGetInteger(match, EFC_SLANT, 0);

				if (weight == FC_WEIGHT_DEMIBOLD || weight == FC_WEIGHT_BOLD
				    || weight == FC_WEIGHT_EXTRABOLD || weight == FC_WEIGHT_BLACK)
				{
				  bold = gTrue;
				}
				if (slant == FC_SLANT_ITALIC)
					italic = gTrue;
				if (slant == FC_SLANT_OBLIQUE)
					oblique = gTrue;

				*type = (!strncasecmp(ext,".pfa",4)) ? sysFontPFA : sysFontPFB;
				*fontNum = fcPatternGetInteger(match, EFC_INDEX, 0);
				fi = new SysFontInfo(fontName->copy(), bold, italic, oblique, font->isFixedWidth(), new GooString((char*)s), *type, *fontNum, substituteName.copy());

				sysFonts->addFcFont(fi);
				path = new GooString((char*)s);
			}
			else /* can there be anything else? */
			{
				D(printf("******Fontcache:unknown font name extension:%s\n", s));
			}
		}
	}

	if (path == NULL && (fi = sysFonts->find(fontName, font->isFixedWidth(), gFalse)))
	{
		path = fi->path->copy();
		*type = fi->type;
		*fontNum = fi->fontNum;
	}

	if (substituteFontName)
	{
		substituteFontName->Set(substituteName.getCString());
	}

	fin:

	if (p != NULL)
		fcPatternDestroy(p);

	unlockGlobalParams;
	return path;
    
}


GooString *GlobalParams::findBase14FontFile(GooString *base14Name, GfxFont *font) {
  SysFontType type;
  int fontNum;
  
  return findSystemFontFile(font, &type, &fontNum, NULL, base14Name);
}

GooString *GlobalParams::findCCFontFile(GooString *collection) {
  GooString *path;

  lockGlobalParams;
  if ((path = (GooString *)ccFontFiles->lookup(collection))) {
    path = path->copy();
  }
  unlockGlobalParams;
  return path;
}


GBool GlobalParams::getPSExpandSmaller() {
  GBool f;

  lockGlobalParams;
  f = psExpandSmaller;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getPSShrinkLarger() {
  GBool f;

  lockGlobalParams;
  f = psShrinkLarger;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getPSCenter() {
  GBool f;

  lockGlobalParams;
  f = psCenter;
  unlockGlobalParams;
  return f;
}

PSLevel GlobalParams::getPSLevel() {
  PSLevel level;

  lockGlobalParams;
  level = psLevel;
  unlockGlobalParams;
  return level;
}

GooString *GlobalParams::getPSResidentFont(GooString *fontName) {
  GooString *psName;

  lockGlobalParams;
  psName = (GooString *)psResidentFonts->lookup(fontName);
  unlockGlobalParams;
  return psName;
}

GooList *GlobalParams::getPSResidentFonts() {
  GooList *names;
  GooHashIter *iter;
  GooString *name;
  GooString *psName;

  names = new GooList();
  lockGlobalParams;
  psResidentFonts->startIter(&iter);
  while (psResidentFonts->getNext(&iter, &name, (void **)&psName)) {
    names->append(psName->copy());
  }
  unlockGlobalParams;
  return names;
}

PSFontParam16 *GlobalParams::getPSResidentFont16(GooString *fontName,
						 int wMode) {
  PSFontParam16 *p;
  int i;

  lockGlobalParams;
  p = NULL;
  for (i = 0; i < psResidentFonts16->getLength(); ++i) {
    p = (PSFontParam16 *)psResidentFonts16->get(i);
    if (!(p->name->cmp(fontName)) && p->wMode == wMode) {
      break;
    }
    p = NULL;
  }
  unlockGlobalParams;
  return p;
}

PSFontParam16 *GlobalParams::getPSResidentFontCC(GooString *collection,
						 int wMode) {
  PSFontParam16 *p;
  int i;

  lockGlobalParams;
  p = NULL;
  for (i = 0; i < psResidentFontsCC->getLength(); ++i) {
    p = (PSFontParam16 *)psResidentFontsCC->get(i);
    if (!(p->name->cmp(collection)) && p->wMode == wMode) {
      break;
    }
    p = NULL;
  }
  unlockGlobalParams;
  return p;
}

GBool GlobalParams::getPSEmbedType1() {
  GBool e;

  lockGlobalParams;
  e = psEmbedType1;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSEmbedTrueType() {
  GBool e;

  lockGlobalParams;
  e = psEmbedTrueType;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSEmbedCIDPostScript() {
  GBool e;

  lockGlobalParams;
  e = psEmbedCIDPostScript;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSEmbedCIDTrueType() {
  GBool e;

  lockGlobalParams;
  e = psEmbedCIDTrueType;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSFontPassthrough() {
  GBool e;

  lockGlobalParams;
  e = psFontPassthrough;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSPreload() {
  GBool preload;

  lockGlobalParams;
  preload = psPreload;
  unlockGlobalParams;
  return preload;
}

GBool GlobalParams::getPSOPI() {
  GBool opi;

  lockGlobalParams;
  opi = psOPI;
  unlockGlobalParams;
  return opi;
}

GBool GlobalParams::getPSASCIIHex() {
  GBool ah;

  lockGlobalParams;
  ah = psASCIIHex;
  unlockGlobalParams;
  return ah;
}

GBool GlobalParams::getPSBinary() {
  GBool binary;

  lockGlobalParams;
  binary = psBinary;
  unlockGlobalParams;
  return binary;
}

GBool GlobalParams::getPSUncompressPreloadedImages() {
  GBool ah;

  lockGlobalParams;
  ah = psUncompressPreloadedImages;
  unlockGlobalParams;
  return ah;
}

double GlobalParams::getPSRasterResolution() {
  double res;

  lockGlobalParams;
  res = psRasterResolution;
  unlockGlobalParams;
  return res;
}

GBool GlobalParams::getPSRasterMono() {
  GBool mono;

  lockGlobalParams;
  mono = psRasterMono;
  unlockGlobalParams;
  return mono;
}

GooString *GlobalParams::getTextEncodingName() {
  GooString *s;

  lockGlobalParams;
  s = textEncoding->copy();
  unlockGlobalParams;
  return s;
}

EndOfLineKind GlobalParams::getTextEOL() {
  EndOfLineKind eol;

  lockGlobalParams;
  eol = textEOL;
  unlockGlobalParams;
  return eol;
}

GBool GlobalParams::getTextPageBreaks() {
  GBool pageBreaks;

  lockGlobalParams;
  pageBreaks = textPageBreaks;
  unlockGlobalParams;
  return pageBreaks;
}

GBool GlobalParams::getTextKeepTinyChars() {
  GBool tiny;

  lockGlobalParams;
  tiny = textKeepTinyChars;
  unlockGlobalParams;
  return tiny;
}

GBool GlobalParams::getEnableFreeType() {
  GBool f;

  lockGlobalParams;
  f = enableFreeType;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getAntialias() {
  GBool f;

  lockGlobalParams;
  f = antialias;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getVectorAntialias() {
  GBool f;

  lockGlobalParams;
  f = vectorAntialias;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getAntialiasPrinting() {
  GBool f;

  lockGlobalParams;
  f = antialiasPrinting;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getStrokeAdjust() {
  GBool f;

  lockGlobalParams;
  f = strokeAdjust;
  unlockGlobalParams;
  return f;
}

ScreenType GlobalParams::getScreenType() {
  ScreenType t;

  lockGlobalParams;
  t = screenType;
  unlockGlobalParams;
  return t;
}

int GlobalParams::getScreenSize() {
  int size;

  lockGlobalParams;
  size = screenSize;
  unlockGlobalParams;
  return size;
}

int GlobalParams::getScreenDotRadius() {
  int r;

  lockGlobalParams;
  r = screenDotRadius;
  unlockGlobalParams;
  return r;
}

double GlobalParams::getScreenGamma() {
  double gamma;

  lockGlobalParams;
  gamma = screenGamma;
  unlockGlobalParams;
  return gamma;
}

double GlobalParams::getScreenBlackThreshold() {
  double thresh;

  lockGlobalParams;
  thresh = screenBlackThreshold;
  unlockGlobalParams;
  return thresh;
}

double GlobalParams::getScreenWhiteThreshold() {
  double thresh;

  lockGlobalParams;
  thresh = screenWhiteThreshold;
  unlockGlobalParams;
  return thresh;
}

double GlobalParams::getMinLineWidth() {
  double minLineWidthA;

  lockGlobalParams;
  minLineWidthA = minLineWidth;
  unlockGlobalParams;
  return minLineWidthA;
}

GBool GlobalParams::getMapNumericCharNames() {
  GBool map;

  lockGlobalParams;
  map = mapNumericCharNames;
  unlockGlobalParams;
  return map;
}

GBool GlobalParams::getMapUnknownCharNames() {
  GBool map;

  lockGlobalParams;
  map = mapUnknownCharNames;
  unlockGlobalParams;
  return map;
}

GBool GlobalParams::getPrintCommands() {
  GBool p;

  lockGlobalParams;
  p = printCommands;
  unlockGlobalParams;
  return p;
}

GBool GlobalParams::getProfileCommands() {
  GBool p;

  lockGlobalParams;
  p = profileCommands;
  unlockGlobalParams;
  return p;
}

GBool GlobalParams::getErrQuiet() {
  // no locking -- this function may get called from inside a locked
  // section
  return errQuiet;
}

CharCodeToUnicode *GlobalParams::getCIDToUnicode(GooString *collection) {
  GooString *fileName;
  CharCodeToUnicode *ctu;

  lockGlobalParams;
  if (!(ctu = cidToUnicodeCache->getCharCodeToUnicode(collection))) {
    if ((fileName = (GooString *)cidToUnicodes->lookup(collection)) &&
	(ctu = CharCodeToUnicode::parseCIDToUnicode(fileName, collection))) {
      cidToUnicodeCache->add(ctu);
    }
  }
  unlockGlobalParams;
  return ctu;
}

CharCodeToUnicode *GlobalParams::getUnicodeToUnicode(GooString *fontName) {
  lockGlobalParams;
  GooHashIter *iter;
  unicodeToUnicodes->startIter(&iter);
  GooString *fileName = NULL;
  GooString *fontPattern;
  void *val;
  while (!fileName && unicodeToUnicodes->getNext(&iter, &fontPattern, &val)) {
    if (strstr(fontName->getCString(), fontPattern->getCString())) {
      unicodeToUnicodes->killIter(&iter);
      fileName = (GooString*)val;
    }
  }
  CharCodeToUnicode *ctu = NULL;
  if (fileName) {
    ctu = unicodeToUnicodeCache->getCharCodeToUnicode(fileName);
    if (!ctu) {
      ctu = CharCodeToUnicode::parseUnicodeToUnicode(fileName);
      if (ctu)
         unicodeToUnicodeCache->add(ctu);
    }
  }
  unlockGlobalParams;
  return ctu;
}

UnicodeMap *GlobalParams::getUnicodeMap(GooString *encodingName) {
  return getUnicodeMap2(encodingName);
}

UnicodeMap *GlobalParams::getUnicodeMap2(GooString *encodingName) {
  UnicodeMap *map;

  if (!(map = getResidentUnicodeMap(encodingName))) {
    lockUnicodeMapCache;
    map = unicodeMapCache->getUnicodeMap(encodingName);
    unlockUnicodeMapCache;
  }
  return map;
}

CMap *GlobalParams::getCMap(GooString *collection, GooString *cMapName, Stream *stream) {
  CMap *cMap;

  lockCMapCache;
  cMap = cMapCache->getCMap(collection, cMapName, stream);
  unlockCMapCache;
  return cMap;
}

UnicodeMap *GlobalParams::getTextEncoding() {
  return getUnicodeMap2(textEncoding);
}

GooList *GlobalParams::getEncodingNames()
{
  GooList *result = new GooList;
  GooHashIter *iter;
  GooString *key;
  void *val;
  residentUnicodeMaps->startIter(&iter);
  while (residentUnicodeMaps->getNext(&iter, &key, &val)) {
    result->append(key);
  }
  residentUnicodeMaps->killIter(&iter);
  unicodeMaps->startIter(&iter);
  while (unicodeMaps->getNext(&iter, &key, &val)) {
    result->append(key);
  }
  unicodeMaps->killIter(&iter);
  return result;
}

//------------------------------------------------------------------------
// functions to set parameters
//------------------------------------------------------------------------

void GlobalParams::addFontFile(GooString *fontName, GooString *path) {
  lockGlobalParams;
  fontFiles->add(fontName, path);
  unlockGlobalParams;
}

void GlobalParams::setPSFile(char *file) {
  lockGlobalParams;
  if (psFile) {
    delete psFile;
  }
  psFile = new GooString(file);
  unlockGlobalParams;
}

void GlobalParams::setPSExpandSmaller(GBool expand) {
  lockGlobalParams;
  psExpandSmaller = expand;
  unlockGlobalParams;
}

void GlobalParams::setPSShrinkLarger(GBool shrink) {
  lockGlobalParams;
  psShrinkLarger = shrink;
  unlockGlobalParams;
}

void GlobalParams::setPSCenter(GBool center) {
  lockGlobalParams;
  psCenter = center;
  unlockGlobalParams;
}

void GlobalParams::setPSLevel(PSLevel level) {
  lockGlobalParams;
  psLevel = level;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedType1(GBool embed) {
  lockGlobalParams;
  psEmbedType1 = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedTrueType(GBool embed) {
  lockGlobalParams;
  psEmbedTrueType = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedCIDPostScript(GBool embed) {
  lockGlobalParams;
  psEmbedCIDPostScript = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedCIDTrueType(GBool embed) {
  lockGlobalParams;
  psEmbedCIDTrueType = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSFontPassthrough(GBool passthrough) {
  lockGlobalParams;
  psFontPassthrough = passthrough;
  unlockGlobalParams;
}

void GlobalParams::setPSPreload(GBool preload) {
  lockGlobalParams;
  psPreload = preload;
  unlockGlobalParams;
}

void GlobalParams::setPSOPI(GBool opi) {
  lockGlobalParams;
  psOPI = opi;
  unlockGlobalParams;
}

void GlobalParams::setPSASCIIHex(GBool hex) {
  lockGlobalParams;
  psASCIIHex = hex;
  unlockGlobalParams;
}

void GlobalParams::setPSBinary(GBool binary) {
  lockGlobalParams;
  psBinary = binary;
  unlockGlobalParams;
}

void GlobalParams::setPSUncompressPreloadedImages(GBool uncomp) {
  lockGlobalParams;
  psUncompressPreloadedImages = uncomp;
  unlockGlobalParams;
}

void GlobalParams::setPSRasterResolution(double res) {
  lockGlobalParams;
  psRasterResolution = res;
  unlockGlobalParams;
}

void GlobalParams::setPSRasterMono(GBool mono) {
  lockGlobalParams;
  psRasterMono = mono;
  unlockGlobalParams;
}

void GlobalParams::setTextEncoding(char *encodingName) {
  lockGlobalParams;
  delete textEncoding;
  textEncoding = new GooString(encodingName);
  unlockGlobalParams;
}

GBool GlobalParams::setTextEOL(char *s) {
  lockGlobalParams;
  if (!strcmp(s, "unix")) {
    textEOL = eolUnix;
  } else if (!strcmp(s, "dos")) {
    textEOL = eolDOS;
  } else if (!strcmp(s, "mac")) {
    textEOL = eolMac;
  } else {
    unlockGlobalParams;
    return gFalse;
  }
  unlockGlobalParams;
  return gTrue;
}

void GlobalParams::setTextPageBreaks(GBool pageBreaks) {
  lockGlobalParams;
  textPageBreaks = pageBreaks;
  unlockGlobalParams;
}

void GlobalParams::setTextKeepTinyChars(GBool keep) {
  lockGlobalParams;
  textKeepTinyChars = keep;
  unlockGlobalParams;
}

GBool GlobalParams::setEnableFreeType(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &enableFreeType);
  unlockGlobalParams;
  return ok;
}

GBool GlobalParams::setDisableFreeTypeHinting(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &disableFreeTypeHinting);
  unlockGlobalParams;
  return ok;
}

GBool GlobalParams::setAntialias(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &antialias);
  unlockGlobalParams;
  return ok;
}

GBool GlobalParams::setVectorAntialias(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &vectorAntialias);
  unlockGlobalParams;
  return ok;
}

void GlobalParams::setAntialiasPrinting(GBool anti) {
  lockGlobalParams;
  antialiasPrinting = anti;
  unlockGlobalParams;
}

void GlobalParams::setStrokeAdjust(GBool adjust)
{
  lockGlobalParams;
  strokeAdjust = adjust;
  unlockGlobalParams;
}

void GlobalParams::setScreenType(ScreenType st)
{
  lockGlobalParams;
  screenType = st;
  unlockGlobalParams;
}

void GlobalParams::setScreenSize(int size)
{
  lockGlobalParams;
  screenSize = size;
  unlockGlobalParams;
}

void GlobalParams::setScreenDotRadius(int radius)
{
  lockGlobalParams;
  screenDotRadius = radius;
  unlockGlobalParams;
}

void GlobalParams::setScreenGamma(double gamma)
{
  lockGlobalParams;
  screenGamma = gamma;
  unlockGlobalParams;
}

void GlobalParams::setScreenBlackThreshold(double blackThreshold)
{
  lockGlobalParams;
  screenBlackThreshold = blackThreshold;
  unlockGlobalParams;
}

void GlobalParams::setScreenWhiteThreshold(double whiteThreshold)
{
  lockGlobalParams;
  screenWhiteThreshold = whiteThreshold;
  unlockGlobalParams;
}

void GlobalParams::setMinLineWidth(double minLineWidthA)
{
  lockGlobalParams;
  minLineWidth = minLineWidthA;
  unlockGlobalParams;
}

void GlobalParams::setOverprintPreview(GBool overprintPreviewA) {
  lockGlobalParams;
  overprintPreview = overprintPreviewA;
  unlockGlobalParams;
}

void GlobalParams::setMapNumericCharNames(GBool map) {
  lockGlobalParams;
  mapNumericCharNames = map;
  unlockGlobalParams;
}

void GlobalParams::setMapUnknownCharNames(GBool map) {
  lockGlobalParams;
  mapUnknownCharNames = map;
  unlockGlobalParams;
}

void GlobalParams::setPrintCommands(GBool printCommandsA) {
  lockGlobalParams;
  printCommands = printCommandsA;
  unlockGlobalParams;
}

void GlobalParams::setProfileCommands(GBool profileCommandsA) {
  lockGlobalParams;
  profileCommands = profileCommandsA;
  unlockGlobalParams;
}

void GlobalParams::setErrQuiet(GBool errQuietA) {
  lockGlobalParams;
  errQuiet = errQuietA;
  unlockGlobalParams;
}

void GlobalParams::addSecurityHandler(XpdfSecurityHandler *handler) {
#ifdef ENABLE_PLUGINS
  lockGlobalParams;
  securityHandlers->append(handler);
  unlockGlobalParams;
#endif
}

XpdfSecurityHandler *GlobalParams::getSecurityHandler(char *name) {
#ifdef ENABLE_PLUGINS
  XpdfSecurityHandler *hdlr;
  int i;

  lockGlobalParams;
  for (i = 0; i < securityHandlers->getLength(); ++i) {
    hdlr = (XpdfSecurityHandler *)securityHandlers->get(i);
    if (!strcasecmp(hdlr->name, name)) {
      unlockGlobalParams;
      return hdlr;
    }
  }
  unlockGlobalParams;

  if (!loadPlugin("security", name)) {
    return NULL;
  }

  lockGlobalParams;
  for (i = 0; i < securityHandlers->getLength(); ++i) {
    hdlr = (XpdfSecurityHandler *)securityHandlers->get(i);
    if (!strcmp(hdlr->name, name)) {
      unlockGlobalParams;
      return hdlr;
    }
  }
  unlockGlobalParams;
#else
  (void)name;
#endif

  return NULL;
}

#ifdef ENABLE_PLUGINS
//------------------------------------------------------------------------
// plugins
//------------------------------------------------------------------------

GBool GlobalParams::loadPlugin(char *type, char *name) {
  Plugin *plugin;

  if (!(plugin = Plugin::load(type, name))) {
    return gFalse;
  }
  lockGlobalParams;
  plugins->append(plugin);
  unlockGlobalParams;
  return gTrue;
}

#endif // ENABLE_PLUGINS
