/* Copyright (c) 1997-2001 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* this file defines the "glist" class, also known as "canvas" (the two used
to be different but are now unified except for some fossilized names.) */

/* changes by Thomas Musil IEM KUG Graz Austria 2001 */

/* bug-fix: canvas_menuclose(): by Krzysztof Czaja */
/* bug-fix: table_new(): I reversed the y-bounds  */

/* IOhannes :
 * changed the canvas_restore, so that it might accept $args as well
 * (like "pd $0_test")
 * so you can make multiple & distinguishable templates
 * 1511:forum::f�r::uml�ute:2001
 * changes marked with    IOhannes
 */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#else /* ROCKBOX */
#include <stdlib.h>
#include <stdio.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include <string.h>
#include "g_all_guis.h"
#endif /* ROCKBOX */

struct _canvasenvironment
{
    t_symbol *ce_dir;	/* directory patch lives in */
    int ce_argc;    	/* number of "$" arguments */
    t_atom *ce_argv;	/* array of "$" arguments */
    int ce_dollarzero;	/* value of "$0" */
};

#define GLIST_DEFCANVASWIDTH 240
#define GLIST_DEFCANVASHEIGHT 300

#ifdef MACOSX
#define GLIST_DEFCANVASYLOC 22
#else
#define GLIST_DEFCANVASYLOC 0
#endif

/* ---------------------- variables --------------------------- */

extern t_pd *newest;
t_class *canvas_class;
static int canvas_dspstate; 	    /* whether DSP is on or off */  
t_canvas *canvas_editing;   	    /* last canvas to start text edting */ 
t_canvas *canvas_whichfind; 	    /* last canvas we did a find in */ 
t_canvas *canvas_list;	    	    /* list of all root canvases */

/* ------------------ forward function declarations --------------- */
static void canvas_start_dsp(void);
static void canvas_stop_dsp(void);
static void canvas_drawlines(t_canvas *x);
static void canvas_setbounds(t_canvas *x, int x1, int y1, int x2, int y2);
static void canvas_reflecttitle(t_canvas *x);
static void canvas_addtolist(t_canvas *x);
static void canvas_takeofflist(t_canvas *x);
static void canvas_pop(t_canvas *x, t_floatarg fvis);
void canvas_create_editor(t_glist *x, int createit);

/* --------- functions to handle the canvas environment ----------- */

static t_symbol *canvas_newfilename = &s_;
static t_symbol *canvas_newdirectory = &s_;
static int canvas_newargc;
static t_atom *canvas_newargv;

static void glist_doupdatewindowlist(t_glist *gl, char *sbuf)
{
    t_gobj *g;
    if (!gl->gl_owner)
    {
    	/* this is a canvas; if we have a window, put on "windows" list */
	t_canvas *canvas = (t_canvas *)gl;
	if (canvas->gl_havewindow)
	{
	    if (strlen(sbuf) + strlen(gl->gl_name->s_name) + 100 <= 1024)
	    {
	    	char tbuf[1024];
#ifdef ROCKBOX
                snprintf(tbuf, sizeof(tbuf),
                    "{%s .x%lx} ", gl->gl_name->s_name,
                                  (unsigned long) (t_int) canvas);
#else /* ROCKBOX */
		sprintf(tbuf, "{%s .x%x} ", gl->gl_name->s_name, (t_int)canvas);
#endif /* ROCKBOX */
	    	strcat(sbuf, tbuf);
	    }
	}
    }
    for (g = gl->gl_list; g; g = g->g_next)
    {
    	if (pd_class(&g->g_pd) == canvas_class)
	    glist_doupdatewindowlist((t_glist *)g, sbuf);
    }
    return;
}

    /* maintain the list of visible toplevels for the GUI's "windows" menu */
void canvas_updatewindowlist( void)
{
    t_canvas *x;
    char sbuf[1024];
    strcpy(sbuf, "set menu_windowlist {");
    	/* find all root canvases */
    for (x = canvas_list; x; x = x->gl_next)
    	glist_doupdatewindowlist(x, sbuf);
    /* next line updates the window menu state before -postcommand tries it */
    strcat(sbuf, "}\npdtk_fixwindowmenu\n");
#ifndef ROCKBOX
    sys_gui(sbuf);
#endif
}

    /* add a glist the list of "root" canvases (toplevels without parents.) */
static void canvas_addtolist(t_canvas *x)
{
    x->gl_next = canvas_list;
    canvas_list = x;
}

static void canvas_takeofflist(t_canvas *x)
{
	/* take it off the window list */
    if (x == canvas_list) canvas_list = x->gl_next;
    else
    {
	t_canvas *z;
	for (z = canvas_list; z->gl_next != x; z = z->gl_next)
	    ;
	z->gl_next = x->gl_next;
    }
}


void canvas_setargs(int argc, t_atom *argv)
{
    	/* if there's an old one lying around free it here.  This
	happens if an abstraction is loaded but never gets as far
	as calling canvas_new(). */
    if (canvas_newargv)
    	freebytes(canvas_newargv, canvas_newargc * sizeof(t_atom));
    canvas_newargc = argc;
    canvas_newargv = copybytes(argv, argc * sizeof(t_atom));
}

void glob_setfilename(void *dummy, t_symbol *filesym, t_symbol *dirsym)
{
#ifdef ROCKBOX
    (void) dummy;
#endif
    canvas_newfilename = filesym;
    canvas_newdirectory = dirsym;
}

t_canvas *canvas_getcurrent(void)
{
    return ((t_canvas *)pd_findbyclass(&s__X, canvas_class));
}

void canvas_setcurrent(t_canvas *x)
{
    pd_pushsym(&x->gl_pd);
}

void canvas_unsetcurrent(t_canvas *x)
{
    pd_popsym(&x->gl_pd);
}

t_canvasenvironment *canvas_getenv(t_canvas *x)
{
    if (!x) bug("canvas_getenv");
    while (!x->gl_env)
    	if (!(x = x->gl_owner))
    	    bug("t_canvasenvironment", x);
    return (x->gl_env);
}

int canvas_getdollarzero( void)
{
    t_canvas *x = canvas_getcurrent();
    t_canvasenvironment *env = (x ? canvas_getenv(x) : 0);
    if (env)
    	return (env->ce_dollarzero);
    else return (0);
}

void canvas_getargs(int *argcp, t_atom **argvp)
{
    t_canvasenvironment *e = canvas_getenv(canvas_getcurrent());
    *argcp = e->ce_argc;
    *argvp = e->ce_argv;
}

t_symbol *canvas_realizedollar(t_canvas *x, t_symbol *s)
{
    t_symbol *ret;
    char *name = s->s_name;
    if (*name == '$' && name[1] >= '0' && name[1] <= '9')
    {
    	t_canvasenvironment *env = canvas_getenv(x);
    	canvas_setcurrent(x);
    	ret = binbuf_realizedollsym(gensym(name+1),
	    env->ce_argc, env->ce_argv, 1);
    	canvas_unsetcurrent(x);
    }
    else ret = s;
    return (ret);
}

t_symbol *canvas_getcurrentdir(void)
{
    t_canvasenvironment *e = canvas_getenv(canvas_getcurrent());
    return (e->ce_dir);
}

t_symbol *canvas_getdir(t_canvas *x)
{
    t_canvasenvironment *e = canvas_getenv(x);
    return (e->ce_dir);
}

void canvas_makefilename(t_canvas *x, char *file, char *result, int resultsize)
{
    char *dir = canvas_getenv(x)->ce_dir->s_name;
    if (file[0] == '/' || (file[0] && file[1] == ':') || !*dir)
    {
    	strncpy(result, file, resultsize);
    	result[resultsize-1] = 0;
    }
    else
    {
    	int nleft;
    	strncpy(result, dir, resultsize);
    	result[resultsize-1] = 0;
    	nleft = resultsize - strlen(result) - 1;
    	if (nleft <= 0) return;
    	strcat(result, "/");
    	strncat(result, file, nleft);
    	result[resultsize-1] = 0;
    }    	
}

void canvas_rename(t_canvas *x, t_symbol *s, t_symbol *dir)
{
    if (strcmp(x->gl_name->s_name, "Pd"))
    	pd_unbind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    x->gl_name = s;
    if (strcmp(x->gl_name->s_name, "Pd"))
    	pd_bind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    if (glist_isvisible(x))
    	canvas_reflecttitle(x);
    if (dir && dir != &s_)
    {
    	t_canvasenvironment *e = canvas_getenv(x);
	e->ce_dir = dir;
    }
}

/* --------------- traversing the set of lines in a canvas ----------- */

int canvas_getindex(t_canvas *x, t_gobj *y)
{
    t_gobj *y2;
    int indexno;
    for (indexno = 0, y2 = x->gl_list; y2 && y2 != y; y2 = y2->g_next)
    	indexno++;
    return (indexno);
}

void linetraverser_start(t_linetraverser *t, t_canvas *x)
{
    t->tr_ob = 0;
    t->tr_x = x;
    t->tr_nextoc = 0;
    t->tr_nextoutno = t->tr_nout = 0;
}

t_outconnect *linetraverser_next(t_linetraverser *t)
{
    t_outconnect *rval = t->tr_nextoc;
    int outno;
    while (!rval)
    {
    	outno = t->tr_nextoutno;
	while (outno == t->tr_nout)
	{
    	    t_gobj *y;
    	    t_object *ob = 0;
    	    if (!t->tr_ob) y = t->tr_x->gl_list;
    	    else y = t->tr_ob->ob_g.g_next;
    	    for (; y; y = y->g_next)
    		if((ob = pd_checkobject(&y->g_pd))) break;
    	    if (!ob) return (0);
    	    t->tr_ob = ob;
    	    t->tr_nout = obj_noutlets(ob);
    	    outno = 0;
   	    if (glist_isvisible(t->tr_x))
   	    	gobj_getrect(y, t->tr_x,
   	    	    &t->tr_x11, &t->tr_y11, &t->tr_x12, &t->tr_y12);
   	    else t->tr_x11 = t->tr_y11 = t->tr_x12 = t->tr_y12 = 0;
	}
	t->tr_nextoutno = outno + 1;
	rval = obj_starttraverseoutlet(t->tr_ob, &t->tr_outlet, outno);
    	t->tr_outno = outno;
    }
    t->tr_nextoc = obj_nexttraverseoutlet(rval, &t->tr_ob2,
    	&t->tr_inlet, &t->tr_inno);
    t->tr_nin = obj_ninlets(t->tr_ob2);
    if (!t->tr_nin) bug("drawline");
    if (glist_isvisible(t->tr_x))
    {
    	int inplus = (t->tr_nin == 1 ? 1 : t->tr_nin - 1);
    	int outplus = (t->tr_nout == 1 ? 1 : t->tr_nout - 1);
    	gobj_getrect(&t->tr_ob2->ob_g, t->tr_x,
   	    &t->tr_x21, &t->tr_y21, &t->tr_x22, &t->tr_y22);
    	t->tr_lx1 = t->tr_x11 +
   	    ((t->tr_x12 - t->tr_x11 - IOWIDTH) * t->tr_outno) /
	    	outplus + IOMIDDLE;
    	t->tr_ly1 = t->tr_y12;
    	t->tr_lx2 = t->tr_x21 +
   	    ((t->tr_x22 - t->tr_x21 - IOWIDTH) * t->tr_inno)/inplus +
   	    	IOMIDDLE;
    	t->tr_ly2 = t->tr_y21;
    }
    else
    {
    	t->tr_x21 = t->tr_y21 = t->tr_x22 = t->tr_y22 = 0;
    	t->tr_lx1 = t->tr_ly1 = t->tr_lx2 = t->tr_ly2 = 0;
    }
    
    return (rval);
}

void linetraverser_skipobject(t_linetraverser *t)
{
    t->tr_nextoc = 0;
    t->tr_nextoutno = t->tr_nout;
}

/* -------------------- the canvas object -------------------------- */
int glist_valid = 10000;

void glist_init(t_glist *x)
{
    	/* zero out everyone except "pd" field */
    memset(((char *)x) + sizeof(x->gl_pd), 0, sizeof(*x) - sizeof(x->gl_pd));
    x->gl_stub = gstub_new(x, 0);
    x->gl_valid = ++glist_valid;
    x->gl_xlabel = (t_symbol **)t_getbytes(0);
    x->gl_ylabel = (t_symbol **)t_getbytes(0);
}

    /* make a new glist.  It will either be a "root" canvas or else
    its parent will be a "text" object in another window... we don't
    know which yet. */
t_canvas *canvas_new(void *dummy, t_symbol *sel, int argc, t_atom *argv)
{
    t_canvas *x = (t_canvas *)pd_new(canvas_class);
    t_canvas *owner = canvas_getcurrent();
    t_symbol *s = &s_;
    int vis = 0, width = GLIST_DEFCANVASWIDTH, height = GLIST_DEFCANVASHEIGHT;
    int xloc = 0, yloc = GLIST_DEFCANVASYLOC;
#ifdef ROCKBOX
    (void) dummy;
    (void) sel;
    int font = 10;
#else /* ROCKBOX */
    int font = (owner ? owner->gl_font : sys_defaultfont);
#endif /* ROCKBOX */
    glist_init(x);
    x->gl_obj.te_type = T_OBJECT;
    if (!owner)
    	canvas_addtolist(x);
    /* post("canvas %x, owner %x", x, owner); */

    if (argc == 5)  /* toplevel: x, y, w, h, font */
    {
    	xloc = atom_getintarg(0, argc, argv);
    	yloc = atom_getintarg(1, argc, argv);
    	width = atom_getintarg(2, argc, argv);
    	height = atom_getintarg(3, argc, argv);
    	font = atom_getintarg(4, argc, argv);
    }
    else if (argc == 6)  /* subwindow: x, y, w, h, name, vis */
    {
    	xloc = atom_getintarg(0, argc, argv);
    	yloc = atom_getintarg(1, argc, argv);
    	width = atom_getintarg(2, argc, argv);
    	height = atom_getintarg(3, argc, argv);
    	s = atom_getsymbolarg(4, argc, argv);
    	vis = atom_getintarg(5, argc, argv);
    }
    	/* (otherwise assume we're being created from the menu.) */

    if (canvas_newdirectory->s_name[0])
    {
    	static int dollarzero = 1000;
    	t_canvasenvironment *env = x->gl_env =
    	    (t_canvasenvironment *)getbytes(sizeof(*x->gl_env));
    	env->ce_dir = canvas_newdirectory;
    	env->ce_argc = canvas_newargc;
    	env->ce_argv = canvas_newargv;
	env->ce_dollarzero = dollarzero++;
    	canvas_newdirectory = &s_;
    	canvas_newargc = 0;
    	canvas_newargv = 0;
    }
    else x->gl_env = 0;

    if (yloc < GLIST_DEFCANVASYLOC)
        yloc = GLIST_DEFCANVASYLOC;
    if (xloc < 0)
        xloc = 0;
    x->gl_x1 = 0;
    x->gl_y1 = 0;
    x->gl_x2 = 1;
    x->gl_y2 = 1;
    canvas_setbounds(x, xloc, yloc, xloc + width, yloc + height);
    x->gl_owner = owner;
    x->gl_name = (*s->s_name ? s : 
    	(canvas_newfilename ? canvas_newfilename : gensym("Pd")));
    if (strcmp(x->gl_name->s_name, "Pd"))
    	pd_bind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    x->gl_loading = 1;
    x->gl_willvis = vis;
    x->gl_edit = !strncmp(x->gl_name->s_name, "Untitled", 8);
#ifdef ROCKBOX
    x->gl_font = 10;
#else /* ROCKBOX */
    x->gl_font = sys_nearestfontsize(font);
#endif /* ROCKBOX */
    pd_pushsym(&x->gl_pd);
    return(x);
}

void canvas_setgraph(t_glist *x, int flag);

static void canvas_coords(t_glist *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    x->gl_x1 = atom_getfloatarg(0, argc, argv);
    x->gl_y1 = atom_getfloatarg(1, argc, argv);
    x->gl_x2 = atom_getfloatarg(2, argc, argv);
    x->gl_y2 = atom_getfloatarg(3, argc, argv);
    x->gl_pixwidth = atom_getintarg(4, argc, argv);
    x->gl_pixheight = atom_getintarg(5, argc, argv);
    canvas_setgraph(x, atom_getintarg(6, argc, argv));
}

    /* make a new glist and add it to this glist.  It will appear as
    a "graph", not a text object.  */
t_glist *glist_addglist(t_glist *g, t_symbol *sym,
    float x1, float y1, float x2, float y2,
    float px1, float py1, float px2, float py2)
{
    static int gcount = 0;
    int zz;
    int menu = 0;
    char *str;
    t_glist *x = (t_glist *)pd_new(canvas_class);
    glist_init(x);
    x->gl_obj.te_type = T_OBJECT;
    if (!*sym->s_name)
    {
    	char buf[40];
#ifdef ROCKBOX
        snprintf(buf, sizeof(buf), "graph%d", ++gcount);
#else /* ROCKBOX */
    	sprintf(buf, "graph%d", ++gcount);
#endif /* ROCKBOX */
    	sym = gensym(buf);
    	menu = 1;
    }
    else if (!strncmp((str = sym->s_name), "graph", 5)
    	&& (zz = atoi(str + 5)) > gcount)
	    gcount = zz;
    	/* in 0.34 and earlier, the pixel rectangle and the y bounds were
	reversed; this would behave the same, except that the dialog window
	would be confusing.  The "correct" way is to have "py1" be the value
	that is higher on the screen. */
    if (py2 < py1)
    {
    	float zz;
	zz = y2;
	y2 = y1;
	y1 = zz;
	zz = py2;
	py2 = py1;
	py1 = zz;
    }
    if (x1 == x2 || y1 == y2)
    	x1 = 0, x2 = 100, y1 = 1, y2 = -1;
    if (px1 >= px2 || py1 >= py2)
    	px1 = 100, py1 = 20, px2 = 100 + GLIST_DEFGRAPHWIDTH,
	    py2 = 20 + GLIST_DEFGRAPHHEIGHT;
    x->gl_name = sym;
    x->gl_x1 = x1;
    x->gl_x2 = x2;
    x->gl_y1 = y1;
    x->gl_y2 = y2;
    x->gl_obj.te_xpix = px1;
    x->gl_obj.te_ypix = py1;
    x->gl_pixwidth = px2 - px1;
    x->gl_pixheight = py2 - py1;
#ifdef ROCKBOX
    x->gl_font = 10;
#else /* ROCKBOX */
    x->gl_font =  (canvas_getcurrent() ?
    	canvas_getcurrent()->gl_font : sys_defaultfont);
#endif /* ROCKBOX */
    x->gl_screenx1 = x->gl_screeny1 = 0;
    x->gl_screenx2 = 240;
    x->gl_screeny2 = 300;
    if (strcmp(x->gl_name->s_name, "Pd"))
    	pd_bind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    x->gl_owner = g;
    x->gl_stretch = 1;
    x->gl_isgraph = 1;
    x->gl_obj.te_binbuf = binbuf_new();
    binbuf_addv(x->gl_obj.te_binbuf, "s", gensym("graph"));
    if (!menu)
    	pd_pushsym(&x->gl_pd);
    glist_add(g, &x->gl_gobj);
    if (glist_isvisible(g))
    	canvas_create_editor(x, 1);
    return (x);
}

    /* call glist_addglist from a Pd message */
void glist_glist(t_glist *g, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_symbol *sym = atom_getsymbolarg(0, argc, argv);   
    float x1 = atom_getfloatarg(1, argc, argv);  
    float y1 = atom_getfloatarg(2, argc, argv);  
    float x2 = atom_getfloatarg(3, argc, argv);  
    float y2 = atom_getfloatarg(4, argc, argv);  
    float px1 = atom_getfloatarg(5, argc, argv);  
    float py1 = atom_getfloatarg(6, argc, argv);  
    float px2 = atom_getfloatarg(7, argc, argv);  
    float py2 = atom_getfloatarg(8, argc, argv);
    glist_addglist(g, sym, x1, y1, x2, y2, px1, py1, px2, py2);
}

    /* return true if the glist should appear as a graph on parent;
    otherwise it appears as a text box. */
int glist_isgraph(t_glist *x)
{
    return (x->gl_isgraph);
}

    /* This is sent from the GUI to inform a toplevel that its window has been
    moved or resized. */
static void canvas_setbounds(t_canvas *x, int x1, int y1, int x2, int y2)
{
    int heightwas = y2 - y1;
    int heightchange = y2 - y1 - (x->gl_screeny2 - x->gl_screeny1);
    x->gl_screenx1 = x1;
    x->gl_screeny1 = y1;
    x->gl_screenx2 = x2;
    x->gl_screeny2 = y2;
    /* post("set bounds %d %d %d %d", x1, y1, x2, y2); */
    if (!glist_isgraph(x) && (x->gl_y2 < x->gl_y1)) 
    {
    	    /* if it's flipped so that y grows upward,
	    fix so that zero is bottom edge and redraw.  This is
	    only appropriate if we're a regular "text" object on the
	    parent. */
	float diff = x->gl_y1 - x->gl_y2;
	t_gobj *y;
    	x->gl_y1 = heightwas * diff;
    	x->gl_y2 = x->gl_y1 - diff;
	    /* and move text objects accordingly; they should stick
	    to the bottom, not the top. */
	for (y = x->gl_list; y; y = y->g_next)
	    if (pd_checkobject(&y->g_pd))
	    	gobj_displace(y, x, 0, heightchange);
	canvas_redraw(x);
    }
}

t_symbol *canvas_makebindsym(t_symbol *s)
{
    char buf[MAXPDSTRING];
    strcpy(buf, "pd-");
    strcat(buf, s->s_name);
    return (gensym(buf));
}

void canvas_reflecttitle(t_canvas *x)
{
    char namebuf[MAXPDSTRING];
    t_canvasenvironment *env = canvas_getenv(x);
    if (env->ce_argc)
    {
    	int i;
	strcpy(namebuf, " (");
	for (i = 0; i < env->ce_argc; i++)
	{
	    if (strlen(namebuf) > MAXPDSTRING/2 - 5)
	    	break;
	    if (i != 0)
	    	strcat(namebuf, " ");
	    atom_string(&env->ce_argv[i], namebuf + strlen(namebuf), 
	    	MAXPDSTRING/2);
    	}
	strcat(namebuf, ")");
    }
    else namebuf[0] = 0;
#ifndef ROCKBOX
    sys_vgui("wm title .x%x {%s%c%s - %s}\n", 
    	x, x->gl_name->s_name, (x->gl_dirty? '*' : ' '), namebuf,
    	    canvas_getdir(x)->s_name);
#endif
}

void canvas_dirty(t_canvas *x, t_int n)
{
    t_canvas *x2 = canvas_getrootfor(x);
    if ((unsigned)n != x2->gl_dirty)
    {
    	x2->gl_dirty = n;
    	canvas_reflecttitle(x2);
    }
}

    /* the window becomes "mapped" (visible and not miniaturized) or
    "unmapped" (either miniaturized or just plain gone.)  This should be
    called from the GUI after the fact to "notify" us that we're mapped. */
void canvas_map(t_canvas *x, t_floatarg f)
{
    int flag = (f != 0);
    t_gobj *y;
    if (flag)
    {
    	if (!glist_isvisible(x))
	{
	    t_selection *sel;
    	    if (!x->gl_havewindow)
	    {
		bug("canvas_map");
		canvas_vis(x, 1);
	    }
    	    for (y = x->gl_list; y; y = y->g_next)
	    	gobj_vis(y, x, 1);
    	    for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
	    	gobj_select(sel->sel_what, x, 1);
    	    x->gl_mapped = 1;
    	    canvas_drawlines(x);
	    	/* simulate a mouse up so u_main will calculate scrollbars...
		    ugly! */
#ifndef ROCKBOX
    	    sys_vgui("pdtk_canvas_mouseup .x%x.c 0 0 0\n", x);
#endif
	}
    }
    else
    {
	if (glist_isvisible(x))
	{
	    	/* just clear out the whole canvas... */
#ifndef ROCKBOX
	    sys_vgui(".x%x.c delete all\n", x);
#endif
	    	/* alternatively, we could have erased them one by one...
    	    for (y = x->gl_list; y; y = y->g_next)
	    	gobj_vis(y, x, 0);
	    	    ... but we should go through and erase the lines as well
		    if we do it that way. */
    	    x->gl_mapped = 0;
	}
    }
}

void canvas_redraw(t_canvas *x)
{
    if (glist_isvisible(x))
    {
    	canvas_map(x, 0);
	canvas_map(x, 1);
    }
}

/* ----  editors -- perhaps this and "vis" should go to g_editor.c ------- */

static t_editor *editor_new(t_glist *owner)
{
    char buf[40];
    t_editor *x = (t_editor *)getbytes(sizeof(*x));
    x->e_connectbuf = binbuf_new();
    x->e_deleted = binbuf_new();
    x->e_glist = owner;
#ifdef ROCKBOX
    snprintf(buf, sizeof(buf), ".x%lx", (unsigned long) (t_int) owner);
#else /* ROCKBOX */
    sprintf(buf, ".x%x", (t_int)owner);
#endif /* ROCKBOX */
    x->e_guiconnect = guiconnect_new(&owner->gl_pd, gensym(buf));
    return (x);
}

static void editor_free(t_editor *x, t_glist *y)
{
    glist_noselect(y);
    guiconnect_notarget(x->e_guiconnect, 1000);
    binbuf_free(x->e_connectbuf);
    binbuf_free(x->e_deleted);
    freebytes((void *)x, sizeof(*x));
}

    /* recursively create or destroy all editors of a glist and its 
    sub-glists, as long as they aren't toplevels. */
void canvas_create_editor(t_glist *x, int createit)
{
    t_gobj *y;
    t_object *ob;
    if (createit)
    {
    	if (x->gl_editor)
	    bug("canvas_create_editor");
    	else
	{
	    x->gl_editor = editor_new(x);
	    for (y = x->gl_list; y; y = y->g_next)
		if((ob = pd_checkobject(&y->g_pd)))
	    	    rtext_new(x, ob);
	}
    }
    else
    {
    	if (!x->gl_editor)
	    bug("canvas_create_editor");
    	else
	{
	    for (y = x->gl_list; y; y = y->g_next)
		if((ob = pd_checkobject(&y->g_pd)))
	    	    rtext_free(glist_findrtext(x, ob));
	    editor_free(x->gl_editor, x);
	    x->gl_editor = 0;
	}
    }
    for (y = x->gl_list; y; y = y->g_next)
    	if (pd_class(&y->g_pd) == canvas_class &&
	    ((t_canvas *)y)->gl_isgraph)
    	    	canvas_create_editor((t_canvas *)y, createit);
}

    /* we call this when we want the window to become visible, mapped, and
    in front of all windows; or with "f" zero, when we want to get rid of
    the window. */
void canvas_vis(t_canvas *x, t_floatarg f)
{
#ifndef ROCKBOX
    char buf[30];
#endif
    int flag = (f != 0);
    if (flag)
    {
    	    /* test if we're already visible and toplevel */
    	if (glist_isvisible(x) && !x->gl_isgraph)
	{	    /* just put us in front */
#ifdef MSW
    	    canvas_vis(x, 0);
	    canvas_vis(x, 1);
#else
#ifndef ROCKBOX
	    sys_vgui("raise .x%x\n", x);
	    sys_vgui("focus .x%x.c\n", x);
	    sys_vgui("wm deiconify .x%x\n", x);  
#endif /* ROCKBOX */
#endif
	}
	else
	{
    	    canvas_create_editor(x, 1);
#ifndef ROCKBOX
    	    sys_vgui("pdtk_canvas_new .x%x %d %d +%d+%d %d\n", x,
    		(int)(x->gl_screenx2 - x->gl_screenx1),
    		(int)(x->gl_screeny2 - x->gl_screeny1),
	    	(int)(x->gl_screenx1), (int)(x->gl_screeny1),
    		x->gl_edit);
#endif /* ROCKBOX */
    	    canvas_reflecttitle(x);
	    x->gl_havewindow = 1;
	    canvas_updatewindowlist();
    	}
    }
    else    /* make invisible */
    {
    	int i;
    	t_canvas *x2;
    	if (!x->gl_havewindow)
	{
	    	/* bug workaround -- a graph in a visible patch gets "invised"
		when the patch is closed, and must lose the editor here.  It's
		probably not the natural place to do this.  Other cases like
		subpatches fall here too but don'd need the editor freed, so
		we check if it exists. */
	    if (x->gl_editor)
    	    	canvas_create_editor(x, 0);
	    return;
	}
	glist_noselect(x);
    	if (glist_isvisible(x))
	    canvas_map(x, 0);
	canvas_create_editor(x, 0);
#ifndef ROCKBOX
   	sys_vgui("destroy .x%x\n", x);
#endif
    	for (i = 1, x2 = x; x2; x2 = x2->gl_next, i++)
	    ;
#ifndef ROCKBOX
    	sys_vgui(".mbar.find delete %d\n", i);
#endif
	    /* if we're a graph on our parent, and if the parent exists
	       and is visible, show ourselves on parent. */
	if (glist_isgraph(x) && x->gl_owner)
	{
	    t_glist *gl2 = x->gl_owner;
	    canvas_create_editor(x, 1);
	    if (glist_isvisible(gl2))
    	    	gobj_vis(&x->gl_gobj, gl2, 0);
	    x->gl_havewindow = 0;
	    if (glist_isvisible(gl2))
    	    	gobj_vis(&x->gl_gobj, gl2, 1);
    	}
	else x->gl_havewindow = 0;
	canvas_updatewindowlist();
    }
}

    /* we call this on a non-toplevel glist to "open" it into its
    own window. */
void glist_menu_open(t_glist *x)
{
    if (glist_isvisible(x) && !glist_istoplevel(x))
    {
	t_glist *gl2 = x->gl_owner;
	if (!gl2) 
	    bug("canvas_vis");	/* shouldn't happen but don't get too upset. */
	else
	{
		/* erase ourself in parent window */
    	    gobj_vis(&x->gl_gobj, gl2, 0);
	    	    /* get rid of our editor (and subeditors) */
    	    canvas_create_editor(x, 0);
	    x->gl_havewindow = 1;
		    /* redraw ourself in parent window (blanked out this time) */
    	    gobj_vis(&x->gl_gobj, gl2, 1);
    	}
    }
    canvas_vis(x, 1);
}

int glist_isvisible(t_glist *x)
{
    return ((!x->gl_loading) && glist_getcanvas(x)->gl_mapped);
}

int glist_istoplevel(t_glist *x)
{
    	/* we consider a graph "toplevel" if it has its own window
	or if it appears as a box in its parent window so that we
	don't draw the actual contents there. */
    return (x->gl_havewindow || !x->gl_isgraph);
}

int glist_getfont(t_glist *x)
{
    return (glist_getcanvas(x)->gl_font);
}

void canvas_free(t_canvas *x)
{
    t_gobj *y;
    int dspstate = canvas_suspend_dsp();
    canvas_noundo(x);
    if (canvas_editing == x)
    	canvas_editing = 0;
    if (canvas_whichfind == x)
    	canvas_whichfind = 0;
    glist_noselect(x);
    while((y = x->gl_list))
    	glist_delete(x, y);
    canvas_vis(x, 0);

    if (strcmp(x->gl_name->s_name, "Pd"))
    	pd_unbind(&x->gl_pd, canvas_makebindsym(x->gl_name));
    if (x->gl_env)
    {
    	freebytes(x->gl_env->ce_argv, x->gl_env->ce_argc * sizeof(t_atom));
    	freebytes(x->gl_env, sizeof(*x->gl_env));
    }
    canvas_resume_dsp(dspstate);
    glist_cleanup(x);
#ifndef ROCKBOX
    gfxstub_deleteforkey(x);	    /* probably unnecessary */
#endif
    if (!x->gl_owner)
    	canvas_takeofflist(x);
}

/* ----------------- lines ---------- */

static void canvas_drawlines(t_canvas *x)
{
    t_linetraverser t;
    t_outconnect *oc;
    {
    	linetraverser_start(&t, x);
    	while((oc = linetraverser_next(&t)))
#ifdef ROCKBOX
            ;
#else /* ROCKBOX */
    	    sys_vgui(".x%x.c create line %d %d %d %d -width %d -tags l%x\n",
		    glist_getcanvas(x),
		    	t.tr_lx1, t.tr_ly1, t.tr_lx2, t.tr_ly2, 
			    (outlet_getsymbol(t.tr_outlet) == &s_signal ? 2:1),
			    	oc);
#endif /* ROCKBOX */
    }
}

void canvas_fixlinesfor(t_canvas *x, t_text *text)
{
    t_linetraverser t;
    t_outconnect *oc;

    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
    	if (t.tr_ob == text || t.tr_ob2 == text)
    	{
#ifndef ROCKBOX
    	    sys_vgui(".x%x.c coords l%x %d %d %d %d\n",
		glist_getcanvas(x), oc,
		    t.tr_lx1, t.tr_ly1, t.tr_lx2, t.tr_ly2);
#endif
    	}
    }
}

    /* kill all lines for the object */
void canvas_deletelinesfor(t_canvas *x, t_text *text)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
    	if (t.tr_ob == text || t.tr_ob2 == text)
    	{
    	    if (x->gl_editor)
    	    {
#ifndef ROCKBOX
    	    	sys_vgui(".x%x.c delete l%x\n",
    	    	    glist_getcanvas(x), oc);
#endif
    	    }
    	    obj_disconnect(t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);
    	}
    }
}

    /* kill all lines for one inlet or outlet */
void canvas_deletelinesforio(t_canvas *x, t_text *text,
    t_inlet *inp, t_outlet *outp)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    {
    	if ((t.tr_ob == text && t.tr_outlet == outp) ||
    	    (t.tr_ob2 == text && t.tr_inlet == inp))
    	{
	    if (x->gl_editor)
	    {
#ifndef ROCKBOX
    	    	sys_vgui(".x%x.c delete l%x\n",
    	    	    glist_getcanvas(x), oc);
#endif
    	    }
    	    obj_disconnect(t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);
    	}
    }
}

static void canvas_pop(t_canvas *x, t_floatarg fvis)
{
    if (fvis != 0)
    	canvas_vis(x, 1);
    pd_popsym(&x->gl_pd);
    canvas_resortinlets(x);
    canvas_resortoutlets(x);
    x->gl_loading = 0;
}

void canvas_objfor(t_glist *gl, t_text *x, int argc, t_atom *argv);


void canvas_restore(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{ /* IOhannes */
    t_pd *z;
#ifdef ROCKBOX
    (void) s;
#endif
    	/* this should be unnecessary, but sometimes the canvas's name gets
	out of sync with the owning box's argument; this fixes that */
    if (argc > 3)
    {
	t_atom *ap=argv+3;
	if (ap->a_type == A_SYMBOL)
	{
	    char *buf=ap->a_w.w_symbol->s_name, *bufp;
	    if (*buf == '$' && buf[1] >= '0' && buf[1] <= '9')
	    {
		for (bufp = buf+2; *bufp; bufp++)
		    if (*bufp < '0' || *bufp > '9')
		{
		    SETDOLLSYM(ap, gensym(buf+1));
		    goto didit;
		}
		SETDOLLAR(ap, atoi(buf+1));
	    didit: ;
	    }
	}

	if (ap->a_type == A_DOLLSYM)
	{
	    t_canvasenvironment *e = canvas_getenv(canvas_getcurrent());
    	    canvas_rename(x, binbuf_realizedollsym(ap->a_w.w_symbol,
	    	e->ce_argc, e->ce_argv, 1), 0);	
	}
	else if (ap->a_type == A_SYMBOL)
    	  canvas_rename(x, argv[3].a_w.w_symbol, 0);
    }
    canvas_pop(x, x->gl_willvis);

    if (!(z = gensym("#X")->s_thing)) error("canvas_restore: out of context");
    else if (*z != canvas_class) error("canvas_restore: wasn't a canvas");
    else
    {
    	t_canvas *x2 = (t_canvas *)z;
    	x->gl_owner = x2;
    	canvas_objfor(x2, &x->gl_obj, argc, argv);
    }
}

static void canvas_loadbangabstractions(t_canvas *x)
{
    t_gobj *y;
#ifdef ROCKBOX
    gensym("loadbang");
#else
    t_symbol *s = gensym("loadbang");
#endif
    for (y = x->gl_list; y; y = y->g_next)
    	if (pd_class(&y->g_pd) == canvas_class)
    {
    	if (canvas_isabstraction((t_canvas *)y))
	    canvas_loadbang((t_canvas *)y);
	else
	    canvas_loadbangabstractions((t_canvas *)y);
    }
}

void canvas_loadbangsubpatches(t_canvas *x)
{
    t_gobj *y;
    t_symbol *s = gensym("loadbang");
    for (y = x->gl_list; y; y = y->g_next)
    	if (pd_class(&y->g_pd) == canvas_class)
    {
    	if (!canvas_isabstraction((t_canvas *)y))
    	    canvas_loadbangsubpatches((t_canvas *)y);
    }
    for (y = x->gl_list; y; y = y->g_next)
    	if ((pd_class(&y->g_pd) != canvas_class) &&
    	    zgetfn(&y->g_pd, s))
    	    	pd_vmess(&y->g_pd, s, "");
}

void canvas_loadbang(t_canvas *x)
{
#ifndef ROCKBOX
    t_gobj *y;
#endif
    canvas_loadbangabstractions(x);
    canvas_loadbangsubpatches(x);
}

    /* When you ask a canvas its size the result is 2 pixels more than what
    you gave it to open it; perhaps there's a 1-pixel border all around it
    or something.  Anyway, we just add the 2 pixels back here; seems we
    have to do this for linux but not MSW; not sure about MacOS. */

#ifdef MSW
#define HORIZBORDER 0
#define VERTBORDER 0
#else
#define HORIZBORDER 2
#define VERTBORDER 2
#endif

static void canvas_relocate(t_canvas *x, t_symbol *canvasgeom,
    t_symbol *topgeom)
{
#ifdef ROCKBOX
    (void) x;
    (void) canvasgeom;
    (void) topgeom;
#else /* ROCKBOX */
    int cxpix, cypix, cw, ch, txpix, typix, tw, th;
    if (sscanf(canvasgeom->s_name, "%dx%d+%d+%d", &cw, &ch, &cxpix, &cypix)
    	< 4 ||
    	sscanf(topgeom->s_name, "%dx%d+%d+%d", &tw, &th, &txpix, &typix) < 4)
    	bug("canvas_relocate");
    	    /* for some reason this is initially called with cw=ch=1 so
	    we just suppress that here. */
    if (cw > 5 && ch > 5)
    	canvas_setbounds(x, txpix, typix,
    	    txpix + cw - HORIZBORDER, typix + ch - VERTBORDER);
#endif /* ROCKBOX */
}

void canvas_popabstraction(t_canvas *x)
{
    newest = &x->gl_pd;
    pd_popsym(&x->gl_pd);
    x->gl_loading = 0;
    canvas_resortinlets(x);
    canvas_resortoutlets(x);
}

void canvas_logerror(t_object *y)
{
#ifdef ROCKBOX
    (void) y;
#endif
#ifdef LATER
    canvas_vis(x, 1);
    if (!glist_isselected(x, &y->ob_g))
    	glist_select(x, &y->ob_g);
#endif
}

/* -------------------------- subcanvases ---------------------- */

static void *subcanvas_new(t_symbol *s)
{
    t_atom a[6];
    t_canvas *x, *z = canvas_getcurrent();
    if (!*s->s_name) s = gensym("/SUBPATCH/");
    SETFLOAT(a, 0);
    SETFLOAT(a+1, GLIST_DEFCANVASYLOC);
    SETFLOAT(a+2, GLIST_DEFCANVASWIDTH);
    SETFLOAT(a+3, GLIST_DEFCANVASHEIGHT);
    SETSYMBOL(a+4, s);
    SETFLOAT(a+5, 1);
    x = canvas_new(0, 0, 6, a);
    x->gl_owner = z;
    canvas_pop(x, 1);
    return (x);
}

static void canvas_click(t_canvas *x,
    t_floatarg xpos, t_floatarg ypos,
    	t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
#ifdef ROCKBOX
    (void) xpos;
    (void) ypos;
    (void) shift;
    (void) ctrl;
    (void) alt;
#endif
    canvas_vis(x, 1);
}


    /* find out from subcanvas contents how much to fatten the box */
void canvas_fattensub(t_canvas *x,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
#ifdef ROCKBOX
    (void) x;
    (void) xp1;
    (void) yp1;
#else /* ROCKBOX */
    t_gobj *y;
#endif /* ROCKBOX */
    *xp2 += 50;     /* fake for now */
    *yp2 += 50;
}

static void canvas_rename_method(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
#ifdef ROCKBOX
    (void) s;
#endif
    if (ac && av->a_type == A_SYMBOL)
    	canvas_rename(x, av->a_w.w_symbol, 0);
    else canvas_rename(x, gensym("Pd"), 0);
}

/* ------------------ table ---------------------------*/

static int tabcount = 0;

static void *table_new(t_symbol *s, t_floatarg f)
{
    t_atom a[9];
    t_glist *gl;
    t_canvas *x, *z = canvas_getcurrent();
    if (s == &s_)
    {
	 char  tabname[255];
	 t_symbol *t = gensym("table"); 
#ifdef ROCKBOX
        snprintf(tabname, sizeof(tabname), "%s%d", t->s_name, tabcount++);
#else /* ROCKBOX */
	 sprintf(tabname, "%s%d", t->s_name, tabcount++);
#endif /* ROCKBOX */
	 s = gensym(tabname); 
    }
    if (f <= 1)
    	f = 100;
    SETFLOAT(a, 0);
    SETFLOAT(a+1, GLIST_DEFCANVASYLOC);
    SETFLOAT(a+2, 600);
    SETFLOAT(a+3, 400);
    SETSYMBOL(a+4, s);
    SETFLOAT(a+5, 0);
    x = canvas_new(0, 0, 6, a);

    x->gl_owner = z;

    	/* create a graph for the table */
    gl = glist_addglist((t_glist*)x, &s_, 0, -1, (f > 1 ? f-1 : 1), 1,
    	50, 350, 550, 50);

    graph_array(gl, s, &s_float, f, 0);

    canvas_pop(x, 0); 

    return (x);
}

    /* return true if the "canvas" object is an abstraction (so we don't
    save its contents, fogr example.)  */
int canvas_isabstraction(t_canvas *x)
{
    return (x->gl_env != 0);
}

    /* return true if the "canvas" object is a "table". */
int canvas_istable(t_canvas *x)
{
    t_atom *argv = (x->gl_obj.te_binbuf? binbuf_getvec(x->gl_obj.te_binbuf):0);
    int argc = (x->gl_obj.te_binbuf? binbuf_getnatom(x->gl_obj.te_binbuf) : 0);
    int istable = (argc && argv[0].a_type == A_SYMBOL &&
    	argv[0].a_w.w_symbol == gensym("table"));
    return (istable);
}

    /* return true if the "canvas" object should be treated as a text
    object.  This is true for abstractions but also for "table"s... */
int canvas_showtext(t_canvas *x)
{
    t_atom *argv = (x->gl_obj.te_binbuf? binbuf_getvec(x->gl_obj.te_binbuf):0);
    int argc = (x->gl_obj.te_binbuf? binbuf_getnatom(x->gl_obj.te_binbuf) : 0);
    int isarray = (argc && argv[0].a_type == A_SYMBOL &&
    	argv[0].a_w.w_symbol == gensym("graph"));
    return (!isarray);
}

static void canvas_dodsp(t_canvas *x, int toplevel, t_signal **sp);
static void canvas_dsp(t_canvas *x, t_signal **sp)
{
    canvas_dodsp(x, 0, sp);
}

    /* get the document containing this canvas */
t_canvas *canvas_getrootfor(t_canvas *x)
{
    if ((!x->gl_owner) || canvas_isabstraction(x))
    	return (x);
    else return (canvas_getrootfor(x->gl_owner));
}

/* ------------------------- DSP chain handling ------------------------- */

EXTERN_STRUCT _dspcontext;
#define t_dspcontext struct _dspcontext

void ugen_start(void);
void ugen_stop(void);

t_dspcontext *ugen_start_graph(int toplevel, t_signal **sp,
    int ninlets, int noutlets);
void ugen_add(t_dspcontext *dc, t_object *x);
void ugen_connect(t_dspcontext *dc, t_object *x1, int outno,
    t_object *x2, int inno);
void ugen_done_graph(t_dspcontext *dc);

    /* schedule one canvas for DSP.  This is called below for all "root"
    canvases, but is also called from the "dsp" method for sub-
    canvases, which are treated almost like any other tilde object.  */

static void canvas_dodsp(t_canvas *x, int toplevel, t_signal **sp)
{
    t_linetraverser t;
    t_outconnect *oc;
    t_gobj *y;
    t_object *ob;
    t_symbol *dspsym = gensym("dsp");
    t_dspcontext *dc;    

    	/* create a new "DSP graph" object to use in sorting this canvas.
	If we aren't toplevel, there are already other dspcontexts around. */

    dc = ugen_start_graph(toplevel, sp,
    	obj_nsiginlets(&x->gl_obj),
    	obj_nsigoutlets(&x->gl_obj));

    	/* find all the "dsp" boxes and add them to the graph */
    
    for (y = x->gl_list; y; y = y->g_next)
    	if ((ob = pd_checkobject(&y->g_pd)) && zgetfn(&y->g_pd, dspsym))
    	    ugen_add(dc, ob);

    	/* ... and all dsp interconnections */
    linetraverser_start(&t, x);
    while((oc = linetraverser_next(&t)))
    	if (obj_issignaloutlet(t.tr_ob, t.tr_outno))
    	    ugen_connect(dc, t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);

    	/* finally, sort them and add them to the DSP chain */
    ugen_done_graph(dc);
}

    /* this routine starts DSP for all root canvases. */
static void canvas_start_dsp(void)
{
    t_canvas *x;
    if (canvas_dspstate) ugen_stop();
#ifndef ROCKBOX
    else sys_gui("pdtk_pd_dsp ON\n");
#endif
    ugen_start();
    
    for (x = canvas_list; x; x = x->gl_next)
    	canvas_dodsp(x, 1, 0);
    
    canvas_dspstate = 1;
}

static void canvas_stop_dsp(void)
{
    if (canvas_dspstate)
    {
    	ugen_stop();
#ifndef ROCKBOX
    	sys_gui("pdtk_pd_dsp OFF\n");
#endif
    	canvas_dspstate = 0;
    }
}

    /* DSP can be suspended before, and resumed after, operations which
    might affect the DSP chain.  For example, we suspend before loading and
    resume afterward, so that DSP doesn't get resorted for every DSP object
    int the patch. */

int canvas_suspend_dsp(void)
{
    int rval = canvas_dspstate;
    if (rval) canvas_stop_dsp();
    return (rval);
}

void canvas_resume_dsp(int oldstate)
{
    if (oldstate) canvas_start_dsp();
}

    /* this is equivalent to suspending and resuming in one step. */
void canvas_update_dsp(void)
{
    if (canvas_dspstate) canvas_start_dsp();
}

void glob_dsp(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    int newstate;
#ifdef ROCKBOX
    (void) dummy;
    (void) s;
#endif
    if (argc)
    {
    	newstate = atom_getintarg(0, argc, argv);
    	if (newstate && !canvas_dspstate)
	{
	    sys_set_audio_state(1);
    	    canvas_start_dsp();
	}
    	else if (!newstate && canvas_dspstate)
	{
    	    canvas_stop_dsp();
	    sys_set_audio_state(0);
	}
    }
    else post("dsp state %d", canvas_dspstate);
}

    /* LATER replace this with a queueing scheme */
void glist_redrawitem(t_glist *owner, t_gobj *gobj)
{
    if (glist_isvisible(owner))
    {
    	gobj_vis(gobj, owner, 0);
    	gobj_vis(gobj, owner, 1);
    }
}

    /* redraw all "scalars" (do this if a drawing command is changed.) 
    LATER we'll use the "template" information to select which ones we
    redraw.  */
static void glist_redrawall(t_glist *gl)
{
    t_gobj *g;
    int vis = glist_isvisible(gl);
    for (g = gl->gl_list; g; g = g->g_next)
    {
#ifndef ROCKBOX
    	t_class *cl;
#endif
    	if (vis && g->g_pd == scalar_class)
    	    glist_redrawitem(gl, g);
    	else if (g->g_pd == canvas_class)
    	    glist_redrawall((t_glist *)g);
    }
}

    /* public interface for above */
void canvas_redrawallfortemplate(t_canvas *templatecanvas)
{
    t_canvas *x;
#ifdef ROCKBOX
    (void) templatecanvas;
#endif
    	/* find all root canvases */
    for (x = canvas_list; x; x = x->gl_next)
    	glist_redrawall(x);
}

/* ------------------------------- setup routine ------------------------ */

    /* why are some of these "glist" and others "canvas"? */
extern void glist_text(t_glist *x, t_symbol *s, int argc, t_atom *argv);
extern void canvas_obj(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_bng(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_toggle(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_hslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
    /* old version... */
extern void canvas_hdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_hdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
    /* new version: */
extern void canvas_hradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vumeter(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_mycnv(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_numbox(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_msg(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_floatatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_symbolatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void glist_scalar(t_glist *canvas, t_symbol *s, int argc, t_atom *argv);

void g_graph_setup(void);
void g_editor_setup(void);
void g_readwrite_setup(void);
extern void graph_properties(t_gobj *z, t_glist *owner);

void g_canvas_setup(void)
{
    	/* we prevent the user from typing "canvas" in an object box
	by sending 0 for a creator function. */
    canvas_class = class_new(gensym("canvas"), 0,
    	(t_method)canvas_free, sizeof(t_canvas), CLASS_NOINLET, 0);
	    /* here is the real creator function, invoked in patch files
	    by sending the "canvas" message to #N, which is bound
	    to pd_camvasmaker. */
    class_addmethod(pd_canvasmaker, (t_method)canvas_new, gensym("canvas"),
    	A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_restore,
    	gensym("restore"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_coords,
    	gensym("coords"), A_GIMME, 0);

/* -------------------------- objects ----------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_obj,
    	gensym("obj"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_msg,
    	gensym("msg"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_floatatom,
    	gensym("floatatom"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_symbolatom,
    	gensym("symbolatom"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_text,
    	gensym("text"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_glist, gensym("graph"),
    	A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_scalar,
    	gensym("scalar"), A_GIMME, A_NULL);

    /* -------------- Thomas Musil's GUI objects ------------ */
    class_addmethod(canvas_class, (t_method)canvas_bng, gensym("bng"),
		    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_toggle, gensym("toggle"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vslider, gensym("vslider"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_hslider, gensym("hslider"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_hdial, gensym("hdial"),
		    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vdial, gensym("vdial"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_hradio, gensym("hradio"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vradio, gensym("vradio"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vumeter, gensym("vumeter"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_mycnv, gensym("mycnv"),
		    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_numbox, gensym("numbox"),
                    A_GIMME, A_NULL);

/* ------------------------ gui stuff --------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_pop, gensym("pop"),
    	A_DEFFLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_loadbang,
    	gensym("loadbang"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_relocate,
    	gensym("relocate"), A_SYMBOL, A_SYMBOL, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vis,
    	gensym("vis"), A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_menu_open,
    	gensym("menu-open"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_map,
    	gensym("map"), A_FLOAT, A_NULL);
    class_setpropertiesfn(canvas_class, graph_properties);

/* ---------------------- list handling ------------------------ */
    class_addmethod(canvas_class, (t_method)glist_clear, gensym("clear"),
    	A_NULL);

/* ----- subcanvases, which you get by typing "pd" in a box ---- */
    class_addcreator((t_newmethod)subcanvas_new, gensym("pd"), A_DEFSYMBOL, 0);
    class_addcreator((t_newmethod)subcanvas_new, gensym("page"),  A_DEFSYMBOL, 0);

    class_addmethod(canvas_class, (t_method)canvas_click,
    	gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_dsp, gensym("dsp"), 0);
    class_addmethod(canvas_class, (t_method)canvas_rename_method,
    	gensym("rename"), A_GIMME, 0);

/*---------------------------- tables -- GG ------------------- */

    class_addcreator((t_newmethod)table_new, gensym("table"),
	A_DEFSYM, A_DEFFLOAT, 0);

/* -------------- setups from other files for canvas_class ---------------- */
    g_graph_setup();
    g_editor_setup();
    g_readwrite_setup();
}

