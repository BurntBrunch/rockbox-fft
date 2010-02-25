/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* changes by Thomas Musil IEM KUG Graz Austria 2001 */
/* the methods for calling the gui-objects from menu are implemented */
/* all changes are labeled with      iemlib      */

#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "t_tk.h"
#include "g_canvas.h"

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#else /* ROCKBOX */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#endif /* ROCKBOX */

static t_class *text_class;
static t_class *message_class;
static t_class *gatom_class;
static void text_vis(t_gobj *z, t_glist *glist, int vis);
static void text_displace(t_gobj *z, t_glist *glist,
    int dx, int dy);
static void text_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);

void canvas_startmotion(t_canvas *x);
t_widgetbehavior text_widgetbehavior;

/* ----------------- the "text" object.  ------------------ */

    /* add a "text" object (comment) to a glist.  While this one goes for any glist,
    the other 3 below are for canvases only.  (why?)  This is called
    without args if invoked from the GUI; otherwise at least x and y
    are provided.  */

void glist_text(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    t_text *x = (t_text *)pd_new(text_class);
    t_atom at;

#ifdef ROCKBOX
    (void) s;
#endif

    x->te_width = 0;	    	    	    	/* don't know it yet. */
    x->te_type = T_TEXT;
    x->te_binbuf = binbuf_new();
    if (argc > 1)
    {
    	x->te_xpix = atom_getfloatarg(0, argc, argv);
    	x->te_ypix = atom_getfloatarg(1, argc, argv);
    	if (argc > 2) binbuf_restore(x->te_binbuf, argc-2, argv+2);
    	else
    	{
    	    SETSYMBOL(&at, gensym("comment"));
    	    binbuf_restore(x->te_binbuf, 1, &at);
    	}
    	glist_add(gl, &x->te_g);
    }
    else
    {
    	int xpix, ypix;
	pd_vmess((t_pd *)glist_getcanvas(gl), gensym("editmode"), "i", 1);
    	SETSYMBOL(&at, gensym("comment"));
    	glist_noselect(gl);
    	glist_getnextxy(gl, &xpix, &ypix);
    	x->te_xpix = glist_pixelstox(gl, xpix-3);
    	x->te_ypix = glist_pixelstoy(gl, ypix-3);
    	binbuf_restore(x->te_binbuf, 1, &at);
    	glist_add(gl, &x->te_g);
    	glist_noselect(gl);
    	glist_select(gl, &x->te_g);
	    /* it would be nice to "activate" here, but then the second,
	    "put-me-down" click changes the text selection, which is quite
	    irritating, so I took this back out.  It's OK in messages
	    and objects though since there's no text in them at menu
	    creation. */
    	    /* gobj_activate(&x->te_g, gl, 1); */
    	canvas_startmotion(glist_getcanvas(gl));
    }
}

/* ----------------- the "object" object.  ------------------ */

extern t_pd *newest;
void canvas_getargs(int *argcp, t_atom **argvp);

static void canvas_objtext(t_glist *gl, int xpix, int ypix, int selected,
    t_binbuf *b)
{
    t_text *x;
    int argc;
    t_atom *argv;
    newest = 0;
    canvas_setcurrent((t_canvas *)gl);
    canvas_getargs(&argc, &argv);
    binbuf_eval(b, &pd_objectmaker, argc, argv);
    if (binbuf_getnatom(b))
    {
	if (!newest)
	{
    	    binbuf_print(b);
    	    post("... couldn't create");
    	    x = 0;
	}
	else if (!(x = pd_checkobject(newest)))
	{
    	    binbuf_print(b);
    	    post("... didn't return a patchable object");
	}
    }
    else x = 0;
    if (!x)
    {
    	    
    	    /* LATER make the color reflect this */
    	x = (t_text *)pd_new(text_class);
    }
    x->te_binbuf = b;
    x->te_xpix = xpix;
    x->te_ypix = ypix;
    x->te_width = 0;
    x->te_type = T_OBJECT;
    glist_add(gl, &x->te_g);
    if (selected)
    {
    	    /* this is called if we've been created from the menu. */
    	glist_select(gl, &x->te_g);
    	gobj_activate(&x->te_g, gl, 1);
    }
    if (pd_class(&x->ob_pd) == vinlet_class)
    	canvas_resortinlets(glist_getcanvas(gl));
    if (pd_class(&x->ob_pd) == voutlet_class)
    	canvas_resortoutlets(glist_getcanvas(gl));
    canvas_unsetcurrent((t_canvas *)gl);
}

    /* object creation routine.  These are called without any arguments if
    they're invoked from the
    gui; when pasting or restoring from a file, we get at least x and y. */

void canvas_obj(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#else
    t_text *x;
#endif

    if (argc >= 2)
    {
    	t_binbuf *b = binbuf_new();
    	binbuf_restore(b, argc-2, argv+2);
    	canvas_objtext(gl, atom_getintarg(0, argc, argv),
    	    atom_getintarg(1, argc, argv), 0, b);
    }
    else
    {
    	t_binbuf *b = binbuf_new();
    	int xpix, ypix;
	pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
    	glist_noselect(gl);
    	glist_getnextxy(gl, &xpix, &ypix);
    	canvas_objtext(gl, xpix, ypix, 1, b);
    	canvas_startmotion(glist_getcanvas(gl));
    }
}

/* make an object box for an object that's already there. */

/* iemlib */
void canvas_iemguis(t_glist *gl, t_symbol *guiobjname)
{
    t_atom at;
    t_binbuf *b = binbuf_new();
    int xpix, ypix;

    pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
    glist_noselect(gl);
    SETSYMBOL(&at, guiobjname);
    binbuf_restore(b, 1, &at);
    glist_getnextxy(gl, &xpix, &ypix);
    canvas_objtext(gl, xpix, ypix, 1, b);
    canvas_startmotion(glist_getcanvas(gl));
}

void canvas_bng(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("bng"));
}

void canvas_toggle(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("tgl"));
}

void canvas_vslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("vsl"));
}

void canvas_hslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("hsl"));
}

void canvas_hdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("hdl"));
}

void canvas_vdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("vdl"));
}

void canvas_hradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("hradio"));
}

void canvas_vradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("vradio"));
}

void canvas_vumeter(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("vu"));
}

void canvas_mycnv(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("cnv"));
}

void canvas_numbox(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
    (void) argc;
    (void) argv;
#endif
    canvas_iemguis(gl, gensym("nbx"));
}

/* iemlib */

void canvas_objfor(t_glist *gl, t_text *x, int argc, t_atom *argv)
{
    x->te_width = 0;	    	    	    	/* don't know it yet. */
    x->te_type = T_OBJECT;
    x->te_binbuf = binbuf_new();
    x->te_xpix = atom_getfloatarg(0, argc, argv);
    x->te_ypix = atom_getfloatarg(1, argc, argv);
    if (argc > 2) binbuf_restore(x->te_binbuf, argc-2, argv+2);
    glist_add(gl, &x->te_g);
}

/* ---------------------- the "message" text item ------------------------ */

typedef struct _messresponder
{
    t_pd mr_pd;
    t_outlet *mr_outlet;
} t_messresponder;

typedef struct _message
{
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist *m_glist;
    t_clock *m_clock;
} t_message;

static t_class *message_class, *messresponder_class;

static void messresponder_bang(t_messresponder *x)
{
    outlet_bang(x->mr_outlet);
}

static void messresponder_float(t_messresponder *x, t_float f)
{
    outlet_float(x->mr_outlet, f);
}

static void messresponder_symbol(t_messresponder *x, t_symbol *s)
{
    outlet_symbol(x->mr_outlet, s);
}

static void messresponder_list(t_messresponder *x, 
    t_symbol *s, int argc, t_atom *argv)
{
    outlet_list(x->mr_outlet, s, argc, argv);
}

static void messresponder_anything(t_messresponder *x,
    t_symbol *s, int argc, t_atom *argv)
{
    outlet_anything(x->mr_outlet, s, argc, argv);
}

static void message_bang(t_message *x)
{
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 0, 0);
}

static void message_float(t_message *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 1, &at);
}

static void message_symbol(t_message *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 1, &at);
}

static void message_list(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, argc, argv);
}

static void message_set(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    binbuf_clear(x->m_text.te_binbuf);
    binbuf_add(x->m_text.te_binbuf, argc, argv);
    glist_retext(x->m_glist, &x->m_text);
}

static void message_add2(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    binbuf_add(x->m_text.te_binbuf, argc, argv);
    glist_retext(x->m_glist, &x->m_text);
}

static void message_add(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    binbuf_add(x->m_text.te_binbuf, argc, argv);
    binbuf_addsemi(x->m_text.te_binbuf);
    glist_retext(x->m_glist, &x->m_text);
}

static void message_click(t_message *x,
    t_floatarg xpos, t_floatarg ypos, t_floatarg shift,
    	t_floatarg ctrl, t_floatarg alt)
{
#ifdef ROCKBOX
    (void) xpos;
    (void) ypos;
    (void) shift;
    (void) ctrl;
    (void) alt;
#endif
    message_float(x, 0);
    if (glist_isvisible(x->m_glist))
    {
#ifndef ROCKBOX
    	t_rtext *y = glist_findrtext(x->m_glist, &x->m_text);
    	sys_vgui(".x%x.c itemconfigure %sR -width 5\n", 
	    glist_getcanvas(x->m_glist), rtext_gettag(y));
#endif
    	clock_delay(x->m_clock, 120);
    }
}

static void message_tick(t_message *x)
{
    if (glist_isvisible(x->m_glist))
    {
#ifndef ROCKBOX
    	t_rtext *y = glist_findrtext(x->m_glist, &x->m_text);
    	sys_vgui(".x%x.c itemconfigure %sR -width 1\n",
	    glist_getcanvas(x->m_glist), rtext_gettag(y));
#endif
    }
}

static void message_free(t_message *x)
{
    clock_free(x->m_clock);
}

void canvas_msg(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
#ifdef ROCKBOX
    (void) s;
#endif
    t_message *x = (t_message *)pd_new(message_class);
    x->m_messresponder.mr_pd = messresponder_class;
    x->m_messresponder.mr_outlet = outlet_new(&x->m_text, &s_float);
    x->m_text.te_width = 0;	    	    	    	/* don't know it yet. */
    x->m_text.te_type = T_MESSAGE;
    x->m_text.te_binbuf = binbuf_new();
    x->m_glist = gl;
    x->m_clock = clock_new(x, (t_method)message_tick);
    if (argc > 1)
    {
    	x->m_text.te_xpix = atom_getfloatarg(0, argc, argv);
    	x->m_text.te_ypix = atom_getfloatarg(1, argc, argv);
    	if (argc > 2) binbuf_restore(x->m_text.te_binbuf, argc-2, argv+2);
    	glist_add(gl, &x->m_text.te_g);
    }
    else
    {
    	int xpix, ypix;
	pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
    	glist_noselect(gl);
    	glist_getnextxy(gl, &xpix, &ypix);
    	x->m_text.te_xpix = xpix-3;
    	x->m_text.te_ypix = ypix-3;
    	glist_add(gl, &x->m_text.te_g);
    	glist_noselect(gl);
    	glist_select(gl, &x->m_text.te_g);
    	gobj_activate(&x->m_text.te_g, gl, 1);
    	canvas_startmotion(glist_getcanvas(gl));
    }
}

/* ---------------------- the "atom" text item ------------------------ */

#define ATOMBUFSIZE 40
#define ATOM_LABELLEFT 0
#define ATOM_LABELRIGHT 1
#define ATOM_LABELUP 2
#define ATOM_LABELDOWN 3

typedef struct _gatom
{
    t_text a_text;
    t_atom a_atom;  	    /* this holds the value and the type */
    t_glist *a_glist;	    /* owning glist */
    t_float a_toggle;	    /* value to toggle to */
    t_float a_draghi;	    /* high end of drag range */
    t_float a_draglo;	    /* low end of drag range */
    t_symbol *a_label;	    /* symbol to show as label next to box */
    t_symbol *a_symfrom;    /* "receive" name -- bind ourselvs to this */
    t_symbol *a_symto;	    /* "send" name -- send to this on output */
    char a_buf[ATOMBUFSIZE];/* string buffer for typing */
    char a_shift;   	    /* was shift key down when dragging started? */
    char a_wherelabel;	    /* 0-3 for left, right, above, below */
    t_symbol *a_expanded_to; /* a_symto after $0, $1, ...  expansion */
} t_gatom;

    /* prepend "-" as necessary to avoid empty strings, so we can
    use them in Pd messages.  A more complete solution would be
    to introduce some quoting mechanism; but then we'd be much more
    complicated. */
static t_symbol *gatom_escapit(t_symbol *s)
{
    if (!*s->s_name)
    	return (gensym("-"));
    else if (*s->s_name == '-')
    {
    	char shmo[100];
	shmo[0] = '-';
	strncpy(shmo+1, s->s_name, 99);
	shmo[99] = 0;
    	return (gensym(shmo));
    }
    else return (iemgui_dollar2raute(s));
}

    /* undo previous operation: strip leading "-" if found. */
static t_symbol *gatom_unescapit(t_symbol *s)
{
    if (*s->s_name == '-')
    	return (gensym(s->s_name+1));
    else return (iemgui_raute2dollar(s));
}

#if 0 /* ??? */
    /* expand leading $0, $1, etc. in the symbol */
static t_symbol *gatom_realizedollar(t_gatom *x, t_symbol *s)
{
    return (canvas_realizedollar(x->a_glist, s)); 
}
#endif

static void gatom_retext(t_gatom *x, int senditup)
{
    binbuf_clear(x->a_text.te_binbuf);
    binbuf_add(x->a_text.te_binbuf, 1, &x->a_atom);
    if (senditup)
    	glist_retext(x->a_glist, &x->a_text);
}

static void gatom_set(t_gatom *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom oldatom = x->a_atom;
    int senditup = 0;
#ifdef ROCKBOX
    (void) s;
#endif
    if (!argc) return;
    if (x->a_atom.a_type == A_FLOAT)
    	x->a_atom.a_w.w_float = atom_getfloat(argv),
	    senditup = (x->a_atom.a_w.w_float != oldatom.a_w.w_float);
    else if (x->a_atom.a_type == A_SYMBOL)
    	x->a_atom.a_w.w_symbol = atom_getsymbol(argv),
	    senditup = (x->a_atom.a_w.w_symbol != oldatom.a_w.w_symbol);
    gatom_retext(x, senditup);
    x->a_buf[0] = 0;
}

static void gatom_bang(t_gatom *x)
{
    if (x->a_atom.a_type == A_FLOAT)
    {
    	if (x->a_text.te_outlet)
	    outlet_float(x->a_text.te_outlet, x->a_atom.a_w.w_float);
	if (*x->a_expanded_to->s_name && x->a_expanded_to->s_thing)
	{
	    if (x->a_symto == x->a_symfrom)
	    	pd_error(x,
		    "%s: atom with same send/receive name (infinite loop)",
		    	x->a_symto->s_name);
	    else pd_float(x->a_expanded_to->s_thing, x->a_atom.a_w.w_float);
    	}
    }
    else if (x->a_atom.a_type == A_SYMBOL)
    {
    	if (x->a_text.te_outlet)
    	    outlet_symbol(x->a_text.te_outlet, x->a_atom.a_w.w_symbol);
	if (*x->a_symto->s_name && x->a_expanded_to->s_thing)
	{
	    if (x->a_symto == x->a_symfrom)
	    	pd_error(x,
		    "%s: atom with same send/receive name (infinite loop)",
		    	x->a_symto->s_name);
	    else pd_symbol(x->a_expanded_to->s_thing, x->a_atom.a_w.w_symbol);
    	}
    }
}

static void gatom_float(t_gatom *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    gatom_set(x, 0, 1, &at);
    gatom_bang(x);
}

static void gatom_clipfloat(t_gatom *x, t_float f)
{
    if (x->a_draglo != 0 || x->a_draghi != 0)
    {
	if (f < x->a_draglo)
    	    f = x->a_draglo;
	if (f > x->a_draghi)
    	    f = x->a_draghi;
    }
    gatom_float(x, f);
}

static void gatom_symbol(t_gatom *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    gatom_set(x, 0, 1, &at);
    gatom_bang(x);
}

static void gatom_motion(void *z, t_floatarg dx, t_floatarg dy)
{
#ifdef ROCKBOX
    (void) dx;
#endif
    t_gatom *x = (t_gatom *)z;
    if (dy == 0) return;
    if (x->a_atom.a_type == A_FLOAT)
    {
    	if (x->a_shift)
    	{
    	    double nval = x->a_atom.a_w.w_float - 0.01 * dy;
    	    double trunc = 0.01 * (floor(100. * nval + 0.5));
    	    if (trunc < nval + 0.0001 && trunc > nval - 0.0001) nval = trunc;
    	    gatom_clipfloat(x, nval);
    	}
    	else
    	{
    	    double nval = x->a_atom.a_w.w_float - dy;
    	    double trunc = 0.01 * (floor(100. * nval + 0.5));
    	    if (trunc < nval + 0.0001 && trunc > nval - 0.0001) nval = trunc;
    	    trunc = floor(nval + 0.5);
    	    if (trunc < nval + 0.001 && trunc > nval - 0.001) nval = trunc;
    	    gatom_clipfloat(x, nval);
    	}
    }
}

static void gatom_key(void *z, t_floatarg f)
{
    t_gatom *x = (t_gatom *)z;
    int c = f;
    int len = strlen(x->a_buf);
    t_atom at;
    char sbuf[ATOMBUFSIZE + 4];
    if (c == 0)
    {
    	/* we're being notified that no more keys will come for this grab */
	if (x->a_buf[0])
	    gatom_retext(x, 1);
	return;
    }
    else if (c == ' ') return;
    else if (c == '\b')
    {
    	if (len > 0)
	x->a_buf[len-1] = 0;
	goto redraw;
    }
    else if (c == '\n')
    {
	if (x->a_atom.a_type == A_FLOAT)
    	    x->a_atom.a_w.w_float = atof(x->a_buf);
	else if (x->a_atom.a_type == A_SYMBOL)
    	    x->a_atom.a_w.w_symbol = gensym(x->a_buf);
    	else bug("gatom_key");
    	gatom_bang(x);
	gatom_retext(x, 1);
    	x->a_buf[0] = 0;
    }
    else if (len < (ATOMBUFSIZE-1))
    {
    	    /* for numbers, only let reasonable characters through */
    	if ((x->a_atom.a_type == A_SYMBOL) ||
	    ((c >= '0' && c <= '9') || c == '.' || c == '-'
	    	|| c == 'e' || c == 'E'))
	{
    	    x->a_buf[len] = c;
    	    x->a_buf[len+1] = 0;
    	    goto redraw;
	}
    }
    return;
redraw:
    	/* LATER figure out how to avoid creating all these symbols! */
#ifdef ROCKBOX
    snprintf(sbuf, sizeof(sbuf), "%s...", x->a_buf);
#else /* ROCKBOX */
    sprintf(sbuf, "%s...", x->a_buf);
#endif
    SETSYMBOL(&at, gensym(sbuf));
    binbuf_clear(x->a_text.te_binbuf);
    binbuf_add(x->a_text.te_binbuf, 1, &at);
    glist_retext(x->a_glist, &x->a_text);
}

static void gatom_click(t_gatom *x,
    t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl,
    t_floatarg alt)
{
#ifdef ROCKBOX
    (void) ctrl;
#endif
    if (x->a_text.te_width == 1)
    {
    	if (x->a_atom.a_type == A_FLOAT)
	    gatom_float(x, (x->a_atom.a_w.w_float == 0));
    }
    else
    {
	if (alt)
	{
    	    if (x->a_atom.a_type != A_FLOAT) return;
    	    if (x->a_atom.a_w.w_float != 0)
    	    {
    		x->a_toggle = x->a_atom.a_w.w_float;
    		gatom_float(x, 0);
    		return;
    	    }
    	    else gatom_float(x, x->a_toggle);
	}
	x->a_shift = shift;
	x->a_buf[0] = 0;
	glist_grab(x->a_glist, &x->a_text.te_g, gatom_motion, gatom_key,
    	    xpos, ypos);
    }
}

    /* message back from dialog window */
static void gatom_param(t_gatom *x, t_symbol *sel, int argc, t_atom *argv)
{
    t_float width = atom_getfloatarg(0, argc, argv);
    t_float draglo = atom_getfloatarg(1, argc, argv);
    t_float draghi = atom_getfloatarg(2, argc, argv);
    t_symbol *label = gatom_unescapit(atom_getsymbolarg(3, argc, argv));
    t_float wherelabel = atom_getfloatarg(4, argc, argv);
    t_symbol *symfrom = gatom_unescapit(atom_getsymbolarg(5, argc, argv));
    t_symbol *symto = gatom_unescapit(atom_getsymbolarg(6, argc, argv));

#ifdef ROCKBOX
    (void) sel;
#endif

    gobj_vis(&x->a_text.te_g, x->a_glist, 0);
    if (!*symfrom->s_name && *x->a_symfrom->s_name)
    	inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
    else if (*symfrom->s_name && !*x->a_symfrom->s_name && x->a_text.te_inlet)
    {
    	canvas_deletelinesforio(x->a_glist, &x->a_text,
	    x->a_text.te_inlet, 0);
	inlet_free(x->a_text.te_inlet);
    }
    if (!*symto->s_name && *x->a_symto->s_name)
    	outlet_new(&x->a_text, 0);
    else if (*symto->s_name && !*x->a_symto->s_name && x->a_text.te_outlet)
    {
    	canvas_deletelinesforio(x->a_glist, &x->a_text,
	    0, x->a_text.te_outlet);
	outlet_free(x->a_text.te_outlet);
    }
    if (draglo >= draghi)
    	draglo = draghi = 0;
    x->a_draglo = draglo;
    x->a_draghi = draghi;
    if (width < 0)
    	width = 4;
    else if (width > 80)
    	width = 80;
    x->a_text.te_width = width;
    x->a_wherelabel = ((int)wherelabel & 3);
    x->a_label = label;
    if (*x->a_symfrom->s_name)
    	pd_unbind(&x->a_text.te_pd,
	    canvas_realizedollar(x->a_glist, x->a_symfrom));
    x->a_symfrom = symfrom;
    if (*x->a_symfrom->s_name)
    	pd_bind(&x->a_text.te_pd,
	    canvas_realizedollar(x->a_glist, x->a_symfrom));
    x->a_symto = symto;
    x->a_expanded_to = canvas_realizedollar(x->a_glist, x->a_symto);
    gobj_vis(&x->a_text.te_g, x->a_glist, 1);

    /* glist_retext(x->a_glist, &x->a_text); */
}

    /* ---------------- gatom-specific widget functions --------------- */
static void gatom_getwherelabel(t_gatom *x, t_glist *glist, int *xp, int *yp)
{
    int x1, y1, x2, y2, width, height;
    text_getrect(&x->a_text.te_g, glist, &x1, &y1, &x2, &y2);
    width = x2 - x1;
    height = y2 - y1;
    if (x->a_wherelabel == ATOM_LABELLEFT)
    {
    	*xp = x1 - 3 -
	    strlen(canvas_realizedollar(x->a_glist, x->a_label)->s_name) *
#ifdef ROCKBOX
            8;
#else
	    sys_fontwidth(glist_getfont(glist));
#endif
	*yp = y1 + 2;
    }
    else if (x->a_wherelabel == ATOM_LABELRIGHT)
    {
    	*xp = x2 + 2;
	*yp = y1 + 2;
    }
    else if (x->a_wherelabel == ATOM_LABELUP)
    {
    	*xp = x1 - 1;
#ifdef ROCKBOX
        *yp = y1 - 1 - 10;
#else
	*yp = y1 - 1 - sys_fontheight(glist_getfont(glist));;
#endif
    }
    else
    {
    	*xp = x1 - 1;
	*yp = y2 + 3;
    }
}

static void gatom_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
#ifndef ROCKBOX
    t_gatom *x = (t_gatom*)z;
#endif
    text_displace(z, glist, dx, dy);
#ifndef ROCKBOX
    sys_vgui(".x%x.c move %x.l %d %d\n", glist_getcanvas(glist), 
    	x, dx, dy);
#endif
}

static void gatom_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_gatom *x = (t_gatom*)z;
    text_vis(z, glist, vis);
    if (*x->a_label->s_name)
    {
	if (vis)
	{
	    int x1, y1;
	    gatom_getwherelabel(x, glist, &x1, &y1);
#ifndef ROCKBOX
    	    sys_vgui("pdtk_text_new .x%x.c %x.l %f %f {%s} %d %s\n",
		glist_getcanvas(glist), x,
		(double)x1, (double)y1,
		canvas_realizedollar(x->a_glist, x->a_label)->s_name,
		sys_hostfontsize(glist_getfont(glist)),
		"black");
#endif
	}
#ifndef ROCKBOX
	else sys_vgui(".x%x.c delete %x.l\n", glist_getcanvas(glist), x);
#endif
    }
}

void canvas_atom(t_glist *gl, t_atomtype type,
    t_symbol *s, int argc, t_atom *argv)
{
    t_gatom *x = (t_gatom *)pd_new(gatom_class);
    t_atom at;

#ifdef ROCKBOX
    (void) s;
#endif

    x->a_text.te_width = 0;	    	    	   /* don't know it yet. */
    x->a_text.te_type = T_ATOM;
    x->a_text.te_binbuf = binbuf_new();
    x->a_glist = gl;
    x->a_atom.a_type = type;
    x->a_toggle = 1;
    x->a_draglo = 0;
    x->a_draghi = 0;
    x->a_wherelabel = 0;
    x->a_label = &s_;
    x->a_symfrom = &s_;
    x->a_symto = x->a_expanded_to = &s_;
    if (type == A_FLOAT)
    {
    	x->a_atom.a_w.w_float = 0;
	x->a_text.te_width = 5;
    	SETFLOAT(&at, 0);
    }
    else
    {
    	x->a_atom.a_w.w_symbol = &s_symbol;
	x->a_text.te_width = 10;
    	SETSYMBOL(&at, &s_symbol);
    }
    binbuf_add(x->a_text.te_binbuf, 1, &at);
    if (argc > 1)
    	/* create from file. x, y, width, low-range, high-range, flags,
	    label, receive-name, send-name */
    {
    	x->a_text.te_xpix = atom_getfloatarg(0, argc, argv);
    	x->a_text.te_ypix = atom_getfloatarg(1, argc, argv);
    	x->a_text.te_width = atom_getintarg(2, argc, argv);
	    /* sanity check because some very old patches have trash in this
	    field... remove this in 2003 or so: */
	if (x->a_text.te_width < 0 || x->a_text.te_width > 500)
	    x->a_text.te_width = 4;
    	x->a_draglo = atom_getfloatarg(3, argc, argv);
    	x->a_draghi = atom_getfloatarg(4, argc, argv);
	x->a_wherelabel = (((int)atom_getfloatarg(5, argc, argv)) & 3);
	x->a_label = gatom_unescapit(atom_getsymbolarg(6, argc, argv));
	x->a_symfrom = gatom_unescapit(atom_getsymbolarg(7, argc, argv));
	if (*x->a_symfrom->s_name)
    	    pd_bind(&x->a_text.te_pd,
	    	canvas_realizedollar(x->a_glist, x->a_symfrom));

	x->a_symto = gatom_unescapit(atom_getsymbolarg(8, argc, argv));
	x->a_expanded_to = canvas_realizedollar(x->a_glist, x->a_symto);
	if (x->a_symto == &s_)
	    outlet_new(&x->a_text,
		x->a_atom.a_type == A_FLOAT ? &s_float: &s_symbol);
	if (x->a_symfrom == &s_)
	    inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
    	glist_add(gl, &x->a_text.te_g);
    }
    else
    {
    	int xpix, ypix;
	outlet_new(&x->a_text,
	    x->a_atom.a_type == A_FLOAT ? &s_float: &s_symbol);
	inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
	pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
    	glist_noselect(gl);
    	glist_getnextxy(gl, &xpix, &ypix);
    	x->a_text.te_xpix = xpix;
    	x->a_text.te_ypix = ypix;
    	glist_add(gl, &x->a_text.te_g);
    	glist_noselect(gl);
    	glist_select(gl, &x->a_text.te_g);
    	canvas_startmotion(glist_getcanvas(gl));
    }
}

void canvas_floatatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_atom(gl, A_FLOAT, s, argc, argv);
}

void canvas_symbolatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_atom(gl, A_SYMBOL, s, argc, argv);
}

static void gatom_free(t_gatom *x)
{
    if (*x->a_symfrom->s_name)
    	pd_unbind(&x->a_text.te_pd,
	    canvas_realizedollar(x->a_glist, x->a_symfrom));
#ifndef ROCKBOX
    gfxstub_deleteforkey(x);
#endif
}

static void gatom_properties(t_gobj *z, t_glist *owner)
{
#ifdef ROCKBOX
    (void) z;
    (void) owner;
#else /* ROCKBOX */
    t_gatom *x = (t_gatom *)z;
    char buf[200];
    sprintf(buf, "pdtk_gatom_dialog %%s %d %g %g %d %s %s %s\n",
	x->a_text.te_width, x->a_draglo, x->a_draghi,
	    x->a_wherelabel, gatom_escapit(x->a_label)->s_name,
	    	gatom_escapit(x->a_symfrom)->s_name,
		    gatom_escapit(x->a_symto)->s_name);
    gfxstub_new(&x->a_text.te_pd, x, buf);
#endif /* ROCKBOX */
}


/* -------------------- widget behavior for text objects ------------ */

static void text_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_text *x = (t_text *)z;
    int width, height, iscomment = (x->te_type == T_TEXT);
    float x1, y1, x2, y2;

    	/* for number boxes, we know width and height a priori, and should
	report them here so that graphs can get swelled to fit. */
    
    if (x->te_type == T_ATOM && x->te_width > 0)
    {
#ifdef ROCKBOX
        int fontwidth = 8, fontheight = 10;
#else
    	int font = glist_getfont(glist);
	int fontwidth = sys_fontwidth(font), fontheight = sys_fontheight(font);
#endif
	width = (x->te_width > 0 ? x->te_width : 6) * fontwidth + 2;
	height = fontheight + 1; /* borrowed from TMARGIN, etc, in g_rtext.c */
    }
	/* if we're invisible we don't know our size so we just lie about
	it.  This is called on invisible boxes to establish order of inlets
	and possibly other reasons.
	   To find out if the box is visible we can't just check the "vis"
	flag because we might be within the vis() routine and not have set
	that yet.  So we check directly whether the "rtext" list has been
	built.  LATER reconsider when "vis" flag should be on and off? */

    else if (glist->gl_editor && glist->gl_editor->e_rtext)
    {
    	t_rtext *y = glist_findrtext(glist, x);
    	width = rtext_width(y);
    	height = rtext_height(y) - (iscomment << 1);
    }
    else width = height = 10;
    x1 = text_xpix(x, glist);
    y1 = text_ypix(x, glist);
    x2 = x1 + width;
    y2 = y1 + height;
    y1 += iscomment;
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void text_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_text *x = (t_text *)z;
    x->te_xpix += dx;
    x->te_ypix += dy;
    if (glist_isvisible(glist))
    {
    	t_rtext *y = glist_findrtext(glist, x);
    	rtext_displace(y, dx, dy);
    	text_drawborder(x, glist, rtext_gettag(y),
	    rtext_width(y), rtext_height(y), 0);
    	canvas_fixlinesfor(glist_getcanvas(glist), x);
    }
}

static void text_select(t_gobj *z, t_glist *glist, int state)
{
    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    rtext_select(y, state);
    if (glist_isvisible(glist) && text_shouldvis(x, glist))
#ifdef ROCKBOX
    {
    }
#else /* ROCKBOX */
	sys_vgui(".x%x.c itemconfigure %sR -fill %s\n", glist, 
    	    rtext_gettag(y), (state? "blue" : "black"));
#endif /* ROCKBOX */
}

static void text_activate(t_gobj *z, t_glist *glist, int state)
{
    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class) rtext_activate(y, state);
}

static void text_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist, x);
}

    /* return true if the text box should be drawn.
    We don't show object boxes inside graphs. */
int text_shouldvis(t_text *x, t_glist *glist)
{
    return (glist->gl_havewindow ||
    	(x->te_pd != canvas_class && x->te_pd->c_wb != &text_widgetbehavior) ||
    	(x->te_pd == canvas_class && (((t_glist *)x)->gl_isgraph)));
}

static void text_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_text *x = (t_text *)z;
    if (vis)
    {
	if (text_shouldvis(x, glist))
	{
    	    t_rtext *y = glist_findrtext(glist, x);
    	    if (x->te_type == T_ATOM)
	    	glist_retext(glist, x);
    	    text_drawborder(x, glist, rtext_gettag(y),
		rtext_width(y), rtext_height(y), 1);
	    rtext_draw(y);
	}
    }
    else
    {
    	t_rtext *y = glist_findrtext(glist, x);
	if (text_shouldvis(x, glist))
	{
    	    text_eraseborder(x, glist, rtext_gettag(y));
	    rtext_erase(y);
	}
    }
}

static int text_click(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
#ifdef ROCKBOX
    (void) glist;
    (void) dbl;
#endif
    t_text *x = (t_text *)z;
    if (x->te_type == T_OBJECT)
    {
    	t_symbol *clicksym = gensym("click");
	if (zgetfn(&x->te_pd, clicksym))
	{
	    if (doit)
	    	pd_vmess(&x->te_pd, clicksym, "fffff",
		    (double)xpix, (double)ypix,
    	    	    	(double)shift, 0, (double)alt);
	    return (1);
	}
	else return (0);
    }
    else if (x->te_type == T_ATOM)
    {
    	if (doit)
	    gatom_click((t_gatom *)x, (t_floatarg)xpix, (t_floatarg)ypix,
	    	(t_floatarg)shift, 0, (t_floatarg)alt);
	return (1);
    }
    else if (x->te_type == T_MESSAGE)
    {
    	if (doit)
	    message_click((t_message *)x, (t_floatarg)xpix, (t_floatarg)ypix,
	    	(t_floatarg)shift, 0, (t_floatarg)alt);
	return (1);
    }
    else return (0);
}

void text_save(t_gobj *z, t_binbuf *b)
{
    t_text *x = (t_text *)z;
    if (x->te_type == T_OBJECT)
    {
    	    /* if we have a "saveto" method, and if we don't happen to be
	    a canvas that's an abstraction, the saveto method does the work */
    	if (zgetfn(&x->te_pd, gensym("saveto")) &&
    	    !((pd_class(&x->te_pd) == canvas_class) && 
    	    	(canvas_isabstraction((t_canvas *)x)
		    || canvas_istable((t_canvas *)x))))
    	{  
    	    mess1(&x->te_pd, gensym("saveto"), b);
    	    binbuf_addv(b, "ssii", gensym("#X"), gensym("restore"),
    	    	(t_int)x->te_xpix, (t_int)x->te_ypix);
    	}
    	else	/* otherwise just save the text */
    	{
    	    binbuf_addv(b, "ssii", gensym("#X"), gensym("obj"),
    	    	(t_int)x->te_xpix, (t_int)x->te_ypix);
        }
        binbuf_addbinbuf(b, x->te_binbuf);
        binbuf_addv(b, ";");
    }
    else if (x->te_type == T_MESSAGE)
    {
    	binbuf_addv(b, "ssii", gensym("#X"), gensym("msg"),
    	    (t_int)x->te_xpix, (t_int)x->te_ypix);
        binbuf_addbinbuf(b, x->te_binbuf);
        binbuf_addv(b, ";");
    }
    else if (x->te_type == T_ATOM)
    {
    	t_atomtype t = ((t_gatom *)x)->a_atom.a_type;
    	t_symbol *sel = (t == A_SYMBOL ? gensym("symbolatom") :
    	    (t == A_FLOAT ? gensym("floatatom") : gensym("intatom")));
	t_symbol *label = gatom_escapit(((t_gatom *)x)->a_label);
	t_symbol *symfrom = gatom_escapit(((t_gatom *)x)->a_symfrom);
	t_symbol *symto = gatom_escapit(((t_gatom *)x)->a_symto);
    	binbuf_addv(b, "ssiiifffsss", gensym("#X"), sel,
    	    (t_int)x->te_xpix, (t_int)x->te_ypix, (t_int)x->te_width,
	    (double)((t_gatom *)x)->a_draglo,
	    (double)((t_gatom *)x)->a_draghi,
	    (double)((t_gatom *)x)->a_wherelabel,
	    label, symfrom, symto);
        binbuf_addv(b, ";");
    }    	
    else 	
    {
    	binbuf_addv(b, "ssii", gensym("#X"), gensym("text"),
    	    (t_int)x->te_xpix, (t_int)x->te_ypix);
        binbuf_addbinbuf(b, x->te_binbuf);
        binbuf_addv(b, ";");
    }    	
}

    /* this one is for everyone but "gatoms"; it's imposed in m_class.c */
t_widgetbehavior text_widgetbehavior =
{
    text_getrect,
    text_displace,
    text_select,
    text_activate,
    text_delete,
    text_vis,
    text_click,
};

static t_widgetbehavior gatom_widgetbehavior =
{
    text_getrect,
    gatom_displace,
    text_select,
    text_activate,
    text_delete,
    gatom_vis,
    text_click,
};

/* -------------------- the "text" class  ------------ */

#ifdef MACOSX
#define EXTRAPIX 2
#else
#define EXTRAPIX 1
#endif

    /* draw inlets and outlets for a text object or for a graph. */
void glist_drawiofor(t_glist *glist, t_object *ob, int firsttime,
    char *tag, int x1, int y1, int x2, int y2)
{
    int n = obj_noutlets(ob), nplus = (n == 1 ? 1 : n-1), i;
#ifdef ROCKBOX
    (void) glist;
    (void) firsttime;
    (void) tag;
    (void) x1;
    (void) y1;
    (void) x2;
    (void) y2;
#else /* ROCKBOX */
    int width = x2 - x1;
#endif /* ROCKBOX */
    for (i = 0; i < n; i++)
    {
#ifndef ROCKBOX
    	int onset = x1 + (width - IOWIDTH) * i / nplus;
    	if (firsttime)
    	    sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %so%d\n",
    	    	glist_getcanvas(glist),
    	    	onset, y2 - 1,
    	    	onset + IOWIDTH, y2,
    	    	tag, i);
    	else
    	    sys_vgui(".x%x.c coords %so%d %d %d %d %d\n",
    	    	glist_getcanvas(glist), tag, i,
    	    	onset, y2 - 1,
    	    	onset + IOWIDTH, y2);
#endif /* ROCKBOX */
    }
    n = obj_ninlets(ob);
    nplus = (n == 1 ? 1 : n-1);
    for (i = 0; i < n; i++)
    {
#ifndef ROCKBOX
    	int onset = x1 + (width - IOWIDTH) * i / nplus;
    	if (firsttime)
    	    sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %si%d\n",
    	    	glist_getcanvas(glist),
    	    	onset, y1,
    	    	onset + IOWIDTH, y1 + EXTRAPIX,
    	    	tag, i);
    	else
    	    sys_vgui(".x%x.c coords %si%d %d %d %d %d\n",
    	    	glist_getcanvas(glist), tag, i,
    	    	onset, y1,
    	    	onset + IOWIDTH, y1 + EXTRAPIX);
#endif /* ROCKBOX */
    }
}

void text_drawborder(t_text *x, t_glist *glist,
    char *tag, int width2, int height2, int firsttime)
{
    t_object *ob;
    int x1, y1, x2, y2, width, height;

#ifdef ROCKBOX
    (void) width2;
    (void) height2;
#endif

    text_getrect(&x->te_g, glist, &x1, &y1, &x2, &y2);
    width = x2 - x1;
    height = y2 - y1;
    if (x->te_type == T_OBJECT)
    {
#ifndef ROCKBOX
	if (firsttime)
    	    sys_vgui(".x%x.c create line\
 %d %d %d %d %d %d %d %d %d %d -tags %sR\n",
		glist_getcanvas(glist),
		    x1, y1,  x2, y1,  x2, y2,  x1, y2,  x1, y1,  tag);
	else
    	    sys_vgui(".x%x.c coords %sR\
 %d %d %d %d %d %d %d %d %d %d\n",
		glist_getcanvas(glist), tag,
		    x1, y1,  x2, y1,  x2, y2,  x1, y2,  x1, y1);
#endif
    }
    else if (x->te_type == T_MESSAGE)
    {
#ifndef ROCKBOX
	if (firsttime)
    	    sys_vgui(".x%x.c create line\
 %d %d %d %d %d %d %d %d %d %d %d %d %d %d -tags %sR\n",
		glist_getcanvas(glist),
		x1, y1,  x2+4, y1,  x2, y1+4,  x2, y2-4,  x2+4, y2,
		x1, y2,  x1, y1,
	    	    tag);
	else
    	    sys_vgui(".x%x.c coords %sR\
 %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
		glist_getcanvas(glist), tag,
		x1, y1,  x2+4, y1,  x2, y1+4,  x2, y2-4,  x2+4, y2,
		x1, y2,  x1, y1);
#endif
    }
    else if (x->te_type == T_ATOM)
    {
#ifndef ROCKBOX
	if (firsttime)
    	    sys_vgui(".x%x.c create line\
 %d %d %d %d %d %d %d %d %d %d %d %d -tags %sR\n",
		glist_getcanvas(glist),
		x1, y1,  x2-4, y1,  x2, y1+4,  x2, y2,  x1, y2,  x1, y1,
	    	    tag);
	else
    	    sys_vgui(".x%x.c coords %sR\
 %d %d %d %d %d %d %d %d %d %d %d %d\n",
		glist_getcanvas(glist), tag,
		x1, y1,  x2-4, y1,  x2, y1+4,  x2, y2,  x1, y2,  x1, y1);
#endif
    }
    	/* draw inlets/outlets */
    
    if ((ob = pd_checkobject(&x->te_pd)))
    	glist_drawiofor(glist, ob, firsttime, tag, x1, y1, x2, y2);
}

void glist_eraseiofor(t_glist *glist, t_object *ob, char *tag)
{
    int i, n;
#ifdef ROCKBOX
    (void) glist;
    (void) tag;
#endif
    n = obj_noutlets(ob);
    for (i = 0; i < n; i++)
#ifdef ROCKBOX
        ;
#else /* ROCKBOX */
    	sys_vgui(".x%x.c delete %so%d\n",
    	    glist_getcanvas(glist), tag, i);
#endif /* ROCKBOX */
    n = obj_ninlets(ob);
    for (i = 0; i < n; i++)
#ifdef ROCKBOX
        ;
#else /* ROCKBOX */
    	sys_vgui(".x%x.c delete %si%d\n",
    	    glist_getcanvas(glist), tag, i);
#endif /* ROCKBOX */
}

void text_eraseborder(t_text *x, t_glist *glist, char *tag)
{
    if (x->te_type == T_TEXT) return;
#ifndef ROCKBOX
    sys_vgui(".x%x.c delete %sR\n",
    	glist_getcanvas(glist), tag);
#endif
    glist_eraseiofor(glist, x, tag);
}

    /* change text; if T_OBJECT, remake it.  LATER we'll have an undo buffer
    which should be filled in here before making the change. */

void text_setto(t_text *x, t_glist *glist, char *buf, int bufsize)
{
    if (x->te_type == T_OBJECT)
    {
    	t_binbuf *b = binbuf_new();
	int natom1, natom2;
	t_atom *vec1, *vec2;
    	binbuf_text(b, buf, bufsize);
	natom1 = binbuf_getnatom(x->te_binbuf);
	vec1 = binbuf_getvec(x->te_binbuf);
	natom2 = binbuf_getnatom(b);
	vec2 = binbuf_getvec(b);
	    /* special case: if  pd args change just pass the message on. */
	if (natom1 >= 1 && natom2 >= 1 && vec1[0].a_type == A_SYMBOL
	    && !strcmp(vec1[0].a_w.w_symbol->s_name, "pd") &&
	     vec2[0].a_type == A_SYMBOL
	    && !strcmp(vec2[0].a_w.w_symbol->s_name, "pd"))
	{
	    typedmess(&x->te_pd, gensym("rename"), natom2-1, vec2+1);
	    binbuf_free(x->te_binbuf);
	    x->te_binbuf = b;
	}
	else  /* normally, just destroy the old one and make a new one. */
	{
	    int xwas = x->te_xpix, ywas = x->te_ypix;
    	    glist_delete(glist, &x->te_g);
    	    canvas_objtext(glist, xwas, ywas, 0, b);
	    	/* if it's an abstraction loadbang it here */
	    if (newest && pd_class(newest) == canvas_class)
    	    	canvas_loadbang((t_canvas *)newest);
    	    canvas_restoreconnections(glist_getcanvas(glist));
    	}
	    /* if we made a new "pd" or changed a window name,
		update window list */
	if (natom2 >= 1  && vec2[0].a_type == A_SYMBOL
	    && !strcmp(vec2[0].a_w.w_symbol->s_name, "pd"))
		canvas_updatewindowlist();
    }
    else binbuf_text(x->te_binbuf, buf, bufsize);
}

void g_text_setup(void)
{
    text_class = class_new(gensym("text"), 0, 0, sizeof(t_text),
    	CLASS_NOINLET | CLASS_PATCHABLE, 0);

    message_class = class_new(gensym("message"), 0, (t_method)message_free,
    	sizeof(t_message), CLASS_PATCHABLE, 0);
    class_addbang(message_class, message_bang);
    class_addfloat(message_class, message_float);
    class_addsymbol(message_class, message_symbol);
    class_addlist(message_class, message_list);
    class_addanything(message_class, message_list);

    class_addmethod(message_class, (t_method)message_click, gensym("click"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(message_class, (t_method)message_set, gensym("set"),
    	A_GIMME, 0);
    class_addmethod(message_class, (t_method)message_add, gensym("add"),
    	A_GIMME, 0);
    class_addmethod(message_class, (t_method)message_add2, gensym("add2"),
    	A_GIMME, 0);

    messresponder_class = class_new(gensym("messresponder"), 0, 0,
    	sizeof(t_text), CLASS_PD, 0);
    class_addbang(messresponder_class, messresponder_bang);
    class_addfloat(messresponder_class, (t_method) messresponder_float);
    class_addsymbol(messresponder_class, messresponder_symbol);
    class_addlist(messresponder_class, messresponder_list);
    class_addanything(messresponder_class, messresponder_anything);

    gatom_class = class_new(gensym("gatom"), 0, (t_method)gatom_free,
    	sizeof(t_gatom), CLASS_NOINLET | CLASS_PATCHABLE, 0);
    class_addbang(gatom_class, gatom_bang);
    class_addfloat(gatom_class, gatom_float);
    class_addsymbol(gatom_class, gatom_symbol);
    class_addmethod(gatom_class, (t_method)gatom_set, gensym("set"),
    	A_GIMME, 0);
    class_addmethod(gatom_class, (t_method)gatom_click, gensym("click"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(gatom_class, (t_method)gatom_param, gensym("param"),
    	A_GIMME, 0);
    class_setwidget(gatom_class, &gatom_widgetbehavior);
    class_setpropertiesfn(gatom_class, gatom_properties);
}

