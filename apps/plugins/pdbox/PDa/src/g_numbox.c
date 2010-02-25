/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* my_numbox.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */

#ifdef ROCKBOX
#include "plugin.h"
#include "../../pdbox.h"
#include "m_pd.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#else /* ROCKBOX */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "t_tk.h"
#include "g_all_guis.h"
#include <math.h>

#ifdef MSW
#include <io.h>
#else
#include <unistd.h>
#endif
#endif /* ROCKBOX */

/*------------------ global varaibles -------------------------*/


/*------------------ global functions -------------------------*/

static void my_numbox_key(void *z, t_floatarg fkey);

/* ------------ nmx gui-my number box ----------------------- */

t_widgetbehavior my_numbox_widgetbehavior;
static t_class *my_numbox_class;

/* widget helper functions */

static void my_numbox_tick_reset(t_my_numbox *x)
{
    if(x->x_gui.x_fsf.x_change)
    {
	x->x_gui.x_fsf.x_change = 0;
	glist_grab(x->x_gui.x_glist, 0, 0, 0, 0, 0);
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
}

static void my_numbox_tick_wait(t_my_numbox *x)
{
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

void my_numbox_clip(t_my_numbox *x)
{
    if(x->x_val < x->x_min)
	x->x_val = x->x_min;
    if(x->x_val > x->x_max)
	x->x_val = x->x_max;
}

void my_numbox_calc_fontwidth(t_my_numbox *x)
{
    int w, f=31;

    if(x->x_gui.x_fsf.x_font_style == 1)
	f = 27;
    else if(x->x_gui.x_fsf.x_font_style == 2)
	f = 25;

    w = x->x_gui.x_fontsize * f * x->x_gui.x_w;
    w /= 36;
    x->x_numwidth = w + (x->x_gui.x_h / 2) + 4;
}

void my_numbox_ftoa(t_my_numbox *x)
{
    double f=x->x_val;
    int bufsize, is_exp=0, i, idecimal;

#ifdef ROCKBOX
    ftoan(f, x->x_buf, 10);
#else
    sprintf(x->x_buf, "%g", f);
#endif
    bufsize = strlen(x->x_buf);
    if(bufsize >= 5)/* if it is in exponential mode */
    {
	i = bufsize - 4;
	if((x->x_buf[i] == 'e') || (x->x_buf[i] == 'E'))
	    is_exp = 1;
    }
    if(bufsize > x->x_gui.x_w)/* if to reduce */
    {
	if(is_exp)
	{
	    if(x->x_gui.x_w <= 5)
	    {
		x->x_buf[0] = (f < 0.0 ? '-' : '+');
		x->x_buf[1] = 0;
	    }
	    i = bufsize - 4;
	    for(idecimal=0; idecimal < i; idecimal++)
		if(x->x_buf[idecimal] == '.')
		    break;
	    if(idecimal > (x->x_gui.x_w - 4))
	    {
		x->x_buf[0] = (f < 0.0 ? '-' : '+');
		x->x_buf[1] = 0;
	    }
	    else
	    {
		int new_exp_index=x->x_gui.x_w-4, old_exp_index=bufsize-4;

		for(i=0; i < 4; i++, new_exp_index++, old_exp_index++)
		    x->x_buf[new_exp_index] = x->x_buf[old_exp_index];
		x->x_buf[x->x_gui.x_w] = 0;
	    }

	}
	else
	{
	    for(idecimal=0; idecimal < bufsize; idecimal++)
		if(x->x_buf[idecimal] == '.')
		    break;
	    if(idecimal > x->x_gui.x_w)
	    {
		x->x_buf[0] = (f < 0.0 ? '-' : '+');
		x->x_buf[1] = 0;
	    }
	    else
		x->x_buf[x->x_gui.x_w] = 0;
	}
    }
}

static void my_numbox_draw_update(t_my_numbox *x, t_glist *glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
    if (glist_isvisible(glist))
    {
	if(x->x_gui.x_fsf.x_change)
	{
	    if(x->x_buf[0])
	    {
		char *cp=x->x_buf;
		int sl = strlen(x->x_buf);

		x->x_buf[sl] = '>';
		x->x_buf[sl+1] = 0;
		if(sl >= x->x_gui.x_w)
		    cp += sl - x->x_gui.x_w + 1;
		sys_vgui(
		    ".x%x.c itemconfigure %xNUMBER -fill #%6.6x -text {%s} \n",
			 glist_getcanvas(glist), x, IEM_GUI_COLOR_EDITED, cp);
		x->x_buf[sl] = 0;
	    }
	    else
	    {
		my_numbox_ftoa(x);
		sys_vgui(
		    ".x%x.c itemconfigure %xNUMBER -fill #%6.6x -text {%s} \n",
		    glist_getcanvas(glist), x, IEM_GUI_COLOR_EDITED, x->x_buf);
		x->x_buf[0] = 0;
	    }
	}
	else
	{
	    my_numbox_ftoa(x);
            sys_vgui(
	    	".x%x.c itemconfigure %xNUMBER -fill #%6.6x -text {%s} \n",
		glist_getcanvas(glist), x,
		x->x_gui.x_fsf.x_selected?
		    IEM_GUI_COLOR_SELECTED:x->x_gui.x_fcol,
		x->x_buf);
	    x->x_buf[0] = 0;
	}
    }
#endif /* ROCKBOX */
}

static void my_numbox_draw_new(t_my_numbox *x, t_glist *glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
    int half=x->x_gui.x_h/2, d=1+x->x_gui.x_h/34;
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(
".x%x.c create polygon %d %d %d %d %d %d %d %d %d %d -outline #%6.6x \
-fill #%6.6x -tags %xBASE1\n",
	     canvas, xpos, ypos,
	     xpos + x->x_numwidth-4, ypos,
	     xpos + x->x_numwidth, ypos+4,
	     xpos + x->x_numwidth, ypos + x->x_gui.x_h,
	     xpos, ypos + x->x_gui.x_h,
	     IEM_GUI_COLOR_NORMAL, x->x_gui.x_bcol, x);
    sys_vgui(
    	".x%x.c create line %d %d %d %d %d %d -fill #%6.6x -tags %xBASE2\n",
	canvas, xpos, ypos,
	xpos + half, ypos + half,
	xpos, ypos + x->x_gui.x_h,
	x->x_gui.x_fcol, x);
    sys_vgui(".x%x.c create text %d %d -text {%s} -anchor w \
	-font {%s %d bold} -fill #%6.6x -tags %xLABEL\n",
	canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
	strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
	x->x_gui.x_font, x->x_gui.x_fontsize, x->x_gui.x_lcol, x);
    my_numbox_ftoa(x);
    sys_vgui(".x%x.c create text %d %d -text {%s} -anchor w \
	-font {%s %d bold} -fill #%6.6x -tags %xNUMBER\n",
	canvas, xpos+half+2, ypos+half+d,
	x->x_buf, x->x_gui.x_font, x->x_gui.x_fontsize, x->x_gui.x_fcol, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xOUT%d\n",
	     canvas,
	     xpos, ypos + x->x_gui.x_h-1,
	     xpos+IOWIDTH, ypos + x->x_gui.x_h,
	     x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
	sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xIN%d\n",
	     canvas,
	     xpos, ypos,
	     xpos+IOWIDTH, ypos+1,
	     x, 0);
#endif /* ROCKBOX */
}

static void my_numbox_draw_move(t_my_numbox *x, t_glist *glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
    int half = x->x_gui.x_h/2, d=1+x->x_gui.x_h/34;
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%x.c coords %xBASE1 %d %d %d %d %d %d %d %d %d %d\n",
	     canvas, x, xpos, ypos,
	     xpos + x->x_numwidth-4, ypos,
	     xpos + x->x_numwidth, ypos+4,
	     xpos + x->x_numwidth, ypos + x->x_gui.x_h,
	     xpos, ypos + x->x_gui.x_h);
    sys_vgui(".x%x.c coords %xBASE2 %d %d %d %d %d %d\n",
	     canvas, x, xpos, ypos,
	     xpos + half, ypos + half,
	     xpos, ypos + x->x_gui.x_h);
    sys_vgui(".x%x.c coords %xLABEL %d %d\n",
	     canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);
    sys_vgui(".x%x.c coords %xNUMBER %d %d\n",
	     canvas, x, xpos+half+2, ypos+half+d);
   if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c coords %xOUT%d %d %d %d %d\n",
	     canvas, x, 0,
	     xpos, ypos + x->x_gui.x_h-1,
	     xpos+IOWIDTH, ypos + x->x_gui.x_h);
   if(!x->x_gui.x_fsf.x_rcv_able)
	sys_vgui(".x%x.c coords %xIN%d %d %d %d %d\n",
	     canvas, x, 0,
	     xpos, ypos,
	     xpos+IOWIDTH, ypos+1);
#endif /* ROCKBOX */
}

static void my_numbox_draw_erase(t_my_numbox* x,t_glist* glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%x.c delete %xBASE1\n", canvas, x);
    sys_vgui(".x%x.c delete %xBASE2\n", canvas, x);
    sys_vgui(".x%x.c delete %xLABEL\n", canvas, x);
    sys_vgui(".x%x.c delete %xNUMBER\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c delete %xOUT%d\n", canvas, x, 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%x.c delete %xIN%d\n", canvas, x, 0);
#endif /* ROCKBOX */
}

static void my_numbox_draw_config(t_my_numbox* x,t_glist* glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
    t_canvas *canvas=glist_getcanvas(glist);

    sys_vgui(".x%x.c itemconfigure %xLABEL -font {%s %d bold} -fill #%6.6x -text {%s} \n",
	     canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize,
	     x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_lcol,
	     strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%x.c itemconfigure %xNUMBER -font {%s %d bold} -fill #%6.6x \n",
	     canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize,
	     x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_fcol);
    sys_vgui(".x%x.c itemconfigure %xBASE1 -fill #%6.6x\n", canvas,
	     x, x->x_gui.x_bcol);
    sys_vgui(".x%x.c itemconfigure %xBASE2 -fill #%6.6x\n", canvas,
	     x, x->x_gui.x_fsf.x_selected?IEM_GUI_COLOR_SELECTED:x->x_gui.x_fcol);
#endif /* ROCKBOX */
}

static void my_numbox_draw_io(t_my_numbox* x,t_glist* glist, int old_snd_rcv_flags)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
    (void) old_snd_rcv_flags;
#else /* ROCKBOX */
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xOUT%d\n",
	     canvas,
	     xpos, ypos + x->x_gui.x_h-1,
	     xpos+IOWIDTH, ypos + x->x_gui.x_h,
	     x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%x.c delete %xOUT%d\n", canvas, x, 0);
    if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xIN%d\n",
	     canvas,
	     xpos, ypos,
	     xpos+IOWIDTH, ypos+1,
	     x, 0);
    if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
	sys_vgui(".x%x.c delete %xIN%d\n", canvas, x, 0);
#endif /* ROCKBOX */
}

static void my_numbox_draw_select(t_my_numbox *x, t_glist *glist)
{
#ifdef ROCKBOX
    (void) x;
    (void) glist;
#else /* ROCKBOX */
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
    {
	if(x->x_gui.x_fsf.x_change)
	{
	    x->x_gui.x_fsf.x_change = 0;
            clock_unset(x->x_clock_reset);
	    x->x_buf[0] = 0;
	    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	}
	sys_vgui(".x%x.c itemconfigure %xBASE1 -outline #%6.6x\n",
	    canvas, x, IEM_GUI_COLOR_SELECTED);
	sys_vgui(".x%x.c itemconfigure %xBASE2 -fill #%6.6x\n",
	    canvas, x, IEM_GUI_COLOR_SELECTED);
	sys_vgui(".x%x.c itemconfigure %xLABEL -fill #%6.6x\n",
	    canvas, x, IEM_GUI_COLOR_SELECTED);
	sys_vgui(".x%x.c itemconfigure %xNUMBER -fill #%6.6x\n",
	    canvas, x, IEM_GUI_COLOR_SELECTED);
    }
    else
    {
	sys_vgui(".x%x.c itemconfigure %xBASE1 -outline #%6.6x\n",
	    canvas, x, IEM_GUI_COLOR_NORMAL);
	sys_vgui(".x%x.c itemconfigure %xBASE2 -fill #%6.6x\n",
	    canvas, x, x->x_gui.x_fcol);
	sys_vgui(".x%x.c itemconfigure %xLABEL -fill #%6.6x\n",
	    canvas, x, x->x_gui.x_lcol);
	sys_vgui(".x%x.c itemconfigure %xNUMBER -fill #%6.6x\n",
	    canvas, x, x->x_gui.x_fcol);
    }
#endif /* ROCKBOX */
}

void my_numbox_draw(t_my_numbox *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
	my_numbox_draw_update(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
	my_numbox_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
	my_numbox_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
	my_numbox_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
	my_numbox_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
	my_numbox_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
	my_numbox_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ nbx widgetbehaviour----------------------------- */


static void my_numbox_getrect(t_gobj *z, t_glist *glist,
			      int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_my_numbox* x = (t_my_numbox*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_numwidth;
    *yp2 = *yp1 + x->x_gui.x_h;
}

static void my_numbox_save(t_gobj *z, t_binbuf *b)
{
    t_my_numbox *x = (t_my_numbox *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    if(x->x_gui.x_fsf.x_change)
    {
	x->x_gui.x_fsf.x_change = 0;
        clock_unset(x->x_clock_reset);
	glist_grab(x->x_gui.x_glist, 0, 0, 0, 0, 0);
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);

    }
    binbuf_addv(b, "ssiisiiffiisssiiiiiiifi", gensym("#X"),gensym("obj"),
		(t_int)x->x_gui.x_obj.te_xpix, (t_int)x->x_gui.x_obj.te_ypix,
		gensym("nbx"), x->x_gui.x_w, x->x_gui.x_h,
		(float)x->x_min, (float)x->x_max,
		x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
		srl[0], srl[1], srl[2],
		x->x_gui.x_ldx, x->x_gui.x_ldy,
		iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
		bflcol[0], bflcol[1], bflcol[2],
		x->x_val, x->x_log_height);
    binbuf_addv(b, ";");
}

int my_numbox_check_minmax(t_my_numbox *x, double min, double max)
{
    int ret=0;

    if(x->x_lin0_log1)
    {
	if((min == 0.0)&&(max == 0.0))
	    max = 1.0;
	if(max > 0.0)
	{
	    if(min <= 0.0)
		min = 0.01*max;
	}
	else
	{
	    if(min > 0.0)
		max = 0.01*min;
	}
    }
    x->x_min = min;
    x->x_max = max;
    if(x->x_val < x->x_min)
    {
	x->x_val = x->x_min;
	ret = 1;
    }
    if(x->x_val > x->x_max)
    {
	x->x_val = x->x_max;
        ret = 1;
    }
    if(x->x_lin0_log1)
	x->x_k = exp(log(x->x_max/x->x_min)/(double)(x->x_log_height));
    else
	x->x_k = 1.0;
    return(ret);
}

static void my_numbox_properties(t_gobj *z, t_glist *owner)
{
#ifdef ROCKBOX
    (void) z;
    (void) owner;
#else /* ROCKBOX */
    t_my_numbox *x = (t_my_numbox *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    if(x->x_gui.x_fsf.x_change)
    {
	x->x_gui.x_fsf.x_change = 0;
        clock_unset(x->x_clock_reset);
	glist_grab(x->x_gui.x_glist, 0, 0, 0, 0, 0);
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);

    }
    sprintf(buf, "pdtk_iemgui_dialog %%s NUMBERBOX \
	    -------dimensions(digits)(pix):------- %d %d width: %d %d height: \
	    -----------output-range:----------- %g min: %g max: %d \
	    %d lin log %d %d log-height: %d \
	    %s %s \
	    %s %d %d \
	    %d %d \
	    %d %d %d\n",
	    x->x_gui.x_w, 1, x->x_gui.x_h, 8,
	    x->x_min, x->x_max, 0,/*no_schedule*/
	    x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, -1,
	    	x->x_log_height, /*no multi, but iem-characteristic*/
	    srl[0]->s_name, srl[1]->s_name,
	    srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
	    x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
	    0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol,
	    	0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
#endif /* ROCKBOX */
}

static void my_numbox_bang(t_my_numbox *x)
{
    outlet_float(x->x_gui.x_obj.ob_outlet, x->x_val);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
	pd_float(x->x_gui.x_snd->s_thing, x->x_val);
}

static void my_numbox_dialog(t_my_numbox *x, t_symbol *s, int argc,
    t_atom *argv)
{
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int log_height = (int)atom_getintarg(6, argc, argv);
    int sr_flags;

#ifdef ROCKBOX
    (void) s;
#endif

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    if(w < 1)
	w = 1;
    x->x_gui.x_w = w;
    if(h < 8)
	h = 8;
    x->x_gui.x_h = h;
    if(log_height < 10)
        log_height = 10;
    x->x_log_height = log_height;
    my_numbox_calc_fontwidth(x);
    /*if(my_numbox_check_minmax(x, min, max))
     my_numbox_bang(x);*/
    my_numbox_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
}

static void my_numbox_motion(t_my_numbox *x, t_floatarg dx, t_floatarg dy)
{
    double k2=1.0;

#ifdef ROCKBOX
    (void) dx;
#endif

    if(x->x_gui.x_fsf.x_finemoved)
	k2 = 0.01;
    if(x->x_lin0_log1)
	x->x_val *= pow(x->x_k, -k2*dy);
    else
	x->x_val -= k2*dy;
    my_numbox_clip(x);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    my_numbox_bang(x);
    clock_unset(x->x_clock_reset);
}

static void my_numbox_click(t_my_numbox *x, t_floatarg xpos, t_floatarg ypos,
			    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
#ifdef ROCKBOX
    (void) shift;
    (void) ctrl;
    (void) alt;
#endif
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
    	(t_glistmotionfn)my_numbox_motion, my_numbox_key, xpos, ypos);
}

static int my_numbox_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_my_numbox* x = (t_my_numbox *)z;

#ifdef ROCKBOX
    (void) glist;
    (void) dbl;
#endif

    if(doit)
    {
	my_numbox_click( x, (t_floatarg)xpix, (t_floatarg)ypix,
	    (t_floatarg)shift, 0, (t_floatarg)alt);
	if(shift)
	    x->x_gui.x_fsf.x_finemoved = 1;
	else
	    x->x_gui.x_fsf.x_finemoved = 0;
	if(!x->x_gui.x_fsf.x_change)
	{
            clock_delay(x->x_clock_wait, 50);
	    x->x_gui.x_fsf.x_change = 1;
	    clock_delay(x->x_clock_reset, 3000);
            /* glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.ob_g,
	    	0, my_numbox_key, 0, 0); */

	    x->x_buf[0] = 0;
	}
        else
	{
	    x->x_gui.x_fsf.x_change = 0;
            clock_unset(x->x_clock_reset);
	    glist_grab(x->x_gui.x_glist, 0, 0, 0, 0, 0);
            x->x_buf[0] = 0;
            (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	}
    }
    return (1);
}

static void my_numbox_set(t_my_numbox *x, t_floatarg f)
{
    x->x_val = f;
    my_numbox_clip(x);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void my_numbox_log_height(t_my_numbox *x, t_floatarg lh)
{
    if(lh < 10.0)
	lh = 10.0;
    x->x_log_height = (int)lh;
    if(x->x_lin0_log1)
	x->x_k = exp(log(x->x_max/x->x_min)/(double)(x->x_log_height));
    else
	x->x_k = 1.0;
    
}

static void my_numbox_float(t_my_numbox *x, t_floatarg f)
{
    my_numbox_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
	my_numbox_bang(x);
}

static void my_numbox_size(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{
    int h, w;

#ifdef ROCKBOX
    (void) s;
#endif

    w = (int)atom_getintarg(0, ac, av);
    if(w < 1)
	w = 1;
    x->x_gui.x_w = w;
    if(ac > 1)
    {
	h = (int)atom_getintarg(1, ac, av);
	if(h < 8)
	    h = 8;
	x->x_gui.x_h = h;
    }
    my_numbox_calc_fontwidth(x);
    iemgui_size((void *)x, &x->x_gui);
}

static void my_numbox_delta(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void my_numbox_pos(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void my_numbox_range(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{
#ifdef ROCKBOX
    (void) s;
#endif
    if(my_numbox_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
			      (double)atom_getfloatarg(1, ac, av)))
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	/*my_numbox_bang(x);*/
    }
}

static void my_numbox_color(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void my_numbox_send(t_my_numbox *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void my_numbox_receive(t_my_numbox *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void my_numbox_label(t_my_numbox *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void my_numbox_label_pos(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void my_numbox_label_font(t_my_numbox *x,
    t_symbol *s, int ac, t_atom *av)
{
    int f = (int)atom_getintarg(1, ac, av);

    if(f < 4)
	f = 4;
    x->x_gui.x_fontsize = f;
    f = (int)atom_getintarg(0, ac, av);
    if((f < 0) || (f > 2))
	f = 0;
    x->x_gui.x_fsf.x_font_style = f;
    my_numbox_calc_fontwidth(x);
    iemgui_label_font((void *)x, &x->x_gui, s, ac, av);
}

static void my_numbox_log(t_my_numbox *x)
{
    x->x_lin0_log1 = 1;
    if(my_numbox_check_minmax(x, x->x_min, x->x_max))
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	/*my_numbox_bang(x);*/
    }
}

static void my_numbox_lin(t_my_numbox *x)
{
    x->x_lin0_log1 = 0;
}

static void my_numbox_init(t_my_numbox *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void my_numbox_loadbang(t_my_numbox *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
    {
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	my_numbox_bang(x);
    }
}

static void my_numbox_key(void *z, t_floatarg fkey)
{
    t_my_numbox *x = z;
    char c=fkey;
    char buf[3];
    buf[1] = 0;

    if (c == 0)
    {
	x->x_gui.x_fsf.x_change = 0;
	clock_unset(x->x_clock_reset);
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	return;
    }
    if(((c>='0')&&(c<='9'))||(c=='.')||(c=='-')||
	(c=='e')||(c=='+')||(c=='E'))
    {
	if(strlen(x->x_buf) < (IEMGUI_MAX_NUM_LEN-2))
	{
	    buf[0] = c;
	    strcat(x->x_buf, buf);
	    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
	}
    }
    else if((c=='\b')||(c==127))
    {
	int sl=strlen(x->x_buf)-1;

	if(sl < 0)
	    sl = 0;
	x->x_buf[sl] = 0;
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    else if((c=='\n')||(c==13))
    {
	x->x_val = atof(x->x_buf);
	x->x_buf[0] = 0;
	x->x_gui.x_fsf.x_change = 0;
	clock_unset(x->x_clock_reset);
        my_numbox_clip(x);
	my_numbox_bang(x);
	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    clock_delay(x->x_clock_reset, 3000);
}

static void my_numbox_list(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{
#ifdef ROCKBOX
    (void) s;
#endif
    if (IS_A_FLOAT(av,0))
    {
	my_numbox_set(x, atom_getfloatarg(0, ac, av));
	my_numbox_bang(x);
    }
}

static void *my_numbox_new(t_symbol *s, int argc, t_atom *argv)
{
    t_my_numbox *x = (t_my_numbox *)pd_new(my_numbox_class);
    int bflcol[]={-262144, -1, -1};
    int w=5, h=14;
#ifdef ROCKBOX
    int lilo=0, ldx=0, ldy=-6;
#else
    int lilo=0, f=0, ldx=0, ldy=-6;
#endif
    int fs=10;
    int log_height=256;
    double min=-1.0e+37, max=1.0e+37,v=0.0;
#ifndef ROCKBOX
    char str[144];
#endif

#ifdef ROCKBOX
    (void) s;
#endif

    if((argc >= 17)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
    {
	w = (int)atom_getintarg(0, argc, argv);
	h = (int)atom_getintarg(1, argc, argv);
	min = (double)atom_getfloatarg(2, argc, argv);
	max = (double)atom_getfloatarg(3, argc, argv);
	lilo = (int)atom_getintarg(4, argc, argv);
	iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
	iemgui_new_getnames(&x->x_gui, 6, argv);
	ldx = (int)atom_getintarg(9, argc, argv);
	ldy = (int)atom_getintarg(10, argc, argv);
	iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
	fs = (int)atom_getintarg(12, argc, argv);
	bflcol[0] = (int)atom_getintarg(13, argc, argv);
	bflcol[1] = (int)atom_getintarg(14, argc, argv);
	bflcol[2] = (int)atom_getintarg(15, argc, argv);
	v = atom_getfloatarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
    {
        log_height = (int)atom_getintarg(17, argc, argv);
    }
    x->x_gui.x_draw = (t_iemfunptr)my_numbox_draw;
    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if(x->x_gui.x_isa.x_loadinit)
	x->x_val = v;
    else
	x->x_val = 0.0;
    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(log_height < 10)
        log_height = 10;
    x->x_log_height = log_height;
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
    	x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
    	x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
	strcpy(x->x_gui.x_font, "courier"); }
    if (x->x_gui.x_fsf.x_rcv_able)
    	pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
	fs = 4;
    x->x_gui.x_fontsize = fs;
    if(w < 1)
	w = 1;
    x->x_gui.x_w = w;
    if(h < 8)
	h = 8;
    x->x_gui.x_h = h;
    x->x_buf[0] = 0;
    my_numbox_calc_fontwidth(x);
    my_numbox_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    x->x_clock_reset = clock_new(x, (t_method)my_numbox_tick_reset);
    x->x_clock_wait = clock_new(x, (t_method)my_numbox_tick_wait);
    x->x_gui.x_fsf.x_change = 0;
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}

static void my_numbox_free(t_my_numbox *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
	pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    clock_free(x->x_clock_reset);
    clock_free(x->x_clock_wait);
#ifndef ROCKBOX
    gfxstub_deleteforkey(x);
#endif
}

void g_numbox_setup(void)
{
    my_numbox_class = class_new(gensym("nbx"), (t_newmethod)my_numbox_new,
	(t_method)my_numbox_free, sizeof(t_my_numbox), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)my_numbox_new, gensym("my_numbox"),
    	A_GIMME, 0);
    class_addbang(my_numbox_class,my_numbox_bang);
    class_addfloat(my_numbox_class,my_numbox_float);
    class_addlist(my_numbox_class, my_numbox_list);
    class_addmethod(my_numbox_class, (t_method)my_numbox_click,
    	gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_motion,
    	gensym("motion"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_dialog,
    	gensym("dialog"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_loadbang,
    	gensym("loadbang"), 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_set,
    	gensym("set"), A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_size,
    	gensym("size"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_delta,
    	gensym("delta"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_pos,
    	gensym("pos"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_range,
    	gensym("range"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_color,
    	gensym("color"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_send,
    	gensym("send"), A_DEFSYM, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_receive,
    	gensym("receive"), A_DEFSYM, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_label,
    	gensym("label"), A_DEFSYM, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_label_pos,
    	gensym("label_pos"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_label_font,
    	gensym("label_font"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_log,
    	gensym("log"), 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_lin,
    	gensym("lin"), 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_init,
    	gensym("init"), A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_log_height,
    	gensym("log_height"), A_FLOAT, 0);
    my_numbox_widgetbehavior.w_getrectfn =    my_numbox_getrect;
    my_numbox_widgetbehavior.w_displacefn =   iemgui_displace;
    my_numbox_widgetbehavior.w_selectfn =     iemgui_select;
    my_numbox_widgetbehavior.w_activatefn =   NULL;
    my_numbox_widgetbehavior.w_deletefn =     iemgui_delete;
    my_numbox_widgetbehavior.w_visfn =        iemgui_vis;
    my_numbox_widgetbehavior.w_clickfn =      my_numbox_newclick;
    class_setwidget(my_numbox_class, &my_numbox_widgetbehavior);
    class_sethelpsymbol(my_numbox_class, gensym("numbox2"));
    class_setsavefn(my_numbox_class, my_numbox_save);
    class_setpropertiesfn(my_numbox_class, my_numbox_properties);
}

