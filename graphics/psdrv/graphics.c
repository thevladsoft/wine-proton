/*
 *	PostScript driver graphics functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */
#include <string.h>
#include <math.h>
#include "psdrv.h"
#include "debug.h"
#include "winspool.h"
#ifndef PI
#define PI M_PI
#endif

/**********************************************************************
 *	     PSDRV_MoveToEx
 */
BOOL32 PSDRV_MoveToEx(DC *dc, INT32 x, INT32 y, LPPOINT32 pt)
{
    TRACE(psdrv, "%d %d\n", x, y);
    if (pt)
    {
	pt->x = dc->w.CursPosX;
	pt->y = dc->w.CursPosY;
    }
    dc->w.CursPosX = x;
    dc->w.CursPosY = y;

    return TRUE;
}


/***********************************************************************
 *           PSDRV_LineTo
 */
BOOL32 PSDRV_LineTo(DC *dc, INT32 x, INT32 y)
{
    TRACE(psdrv, "%d %d\n", x, y);

    PSDRV_SetPen(dc);
    PSDRV_WriteMoveTo(dc, XLPTODP(dc, dc->w.CursPosX),
		          YLPTODP(dc, dc->w.CursPosY));
    PSDRV_WriteLineTo(dc, XLPTODP(dc, x), YLPTODP(dc, y));
    PSDRV_WriteStroke(dc);

    dc->w.CursPosX = x;
    dc->w.CursPosY = y;
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Rectangle
 */
BOOL32 PSDRV_Rectangle( DC *dc, INT32 left, INT32 top, INT32 right,
		       INT32 bottom )
{
    INT32 width = XLSTODS(dc, right - left);
    INT32 height = YLSTODS(dc, bottom - top);


    TRACE(psdrv, "%d %d - %d %d\n", left, top, right, bottom);

    PSDRV_WriteRectangle(dc, XLPTODP(dc, left), YLPTODP(dc, top),
			     width, height);

    PSDRV_Brush(dc,0);
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_RoundRect
 */
BOOL32 PSDRV_RoundRect( DC *dc, INT32 left, INT32 top, INT32 right,
			INT32 bottom, INT32 ell_width, INT32 ell_height )
{
    left = XLPTODP( dc, left );
    right = XLPTODP( dc, right );
    top = YLPTODP( dc, top );
    bottom = YLPTODP( dc, bottom );
    ell_width = XLSTODS( dc, ell_width );
    ell_height = YLSTODS( dc, ell_height );

    if( left > right ) { INT32 tmp = left; left = right; right = tmp; }
    if( top > bottom ) { INT32 tmp = top; top = bottom; bottom = tmp; }

    if(ell_width > right - left) ell_width = right - left;
    if(ell_height > bottom - top) ell_height = bottom - top;

    PSDRV_WriteMoveTo( dc, left, top + ell_height/2 );
    PSDRV_WriteArc( dc, left + ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 90.0, 180.0);
    PSDRV_WriteLineTo( dc, right - ell_width/2, top );
    PSDRV_WriteArc( dc, right - ell_width/2, top + ell_height/2, ell_width,
		    ell_height, 0.0, 90.0);
    PSDRV_WriteLineTo( dc, right, bottom - ell_height/2 );
    PSDRV_WriteArc( dc, right - ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, -90.0, 0.0);
    PSDRV_WriteLineTo( dc, right - ell_width/2, bottom);
    PSDRV_WriteArc( dc, left + ell_width/2, bottom - ell_height/2, ell_width,
		    ell_height, 180.0, -90.0);
    PSDRV_WriteClosePath( dc );

    PSDRV_Brush(dc,0);
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}

/***********************************************************************
 *           PSDRV_DrawArc
 *
 * Does the work of Arc, Chord and Pie. lines is 0, 1 or 2 respectively.
 */
static BOOL32 PSDRV_DrawArc( DC *dc, INT32 left, INT32 top, 
			     INT32 right, INT32 bottom,
			     INT32 xstart, INT32 ystart,
			     INT32 xend, INT32 yend,
			     int lines )
{
    INT32 x, y, h, w;
    double start_angle, end_angle, ratio;

    x = XLPTODP(dc, (left + right)/2);
    y = YLPTODP(dc, (top + bottom)/2);

    w = XLSTODS(dc, (right - left));
    h = YLSTODS(dc, (bottom - top));

    if(w < 0) w = -w;
    if(h < 0) h = -h;
    ratio = ((double)w)/h;

    /* angle is the angle after the rectangle is transformed to a square and is
       measured anticlockwise from the +ve x-axis */

    start_angle = atan2((double)(y - ystart) * ratio, (double)(xstart - x));
    end_angle = atan2((double)(y - yend) * ratio, (double)(xend - x));

    start_angle *= 180.0 / PI;
    end_angle *= 180.0 / PI;

    if(lines == 2) /* pie */
        PSDRV_WriteMoveTo(dc, x, y);
    PSDRV_WriteArc(dc, x, y, w, h, start_angle, end_angle);
    if(lines == 1 || lines == 2) { /* chord or pie */
        PSDRV_WriteClosePath(dc);
	PSDRV_Brush(dc,0);
    }
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Arc
 */
BOOL32 PSDRV_Arc( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
		  INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return PSDRV_DrawArc( dc, left, top, right, bottom, xstart, ystart,
			 xend, yend, 0 );
}

/***********************************************************************
 *           PSDRV_Chord
 */
BOOL32 PSDRV_Chord( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
		  INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return PSDRV_DrawArc( dc, left, top, right, bottom, xstart, ystart,
			 xend, yend, 1 );
}


/***********************************************************************
 *           PSDRV_Pie
 */
BOOL32 PSDRV_Pie( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom,
		  INT32 xstart, INT32 ystart, INT32 xend, INT32 yend )
{
    return PSDRV_DrawArc( dc, left, top, right, bottom, xstart, ystart,
			 xend, yend, 2 );
}


/***********************************************************************
 *           PSDRV_Ellipse
 */
BOOL32 PSDRV_Ellipse( DC *dc, INT32 left, INT32 top, INT32 right, INT32 bottom)
{
    INT32 x, y, w, h;

    TRACE(psdrv, "%d %d - %d %d\n", left, top, right, bottom);

    x = XLPTODP(dc, (left + right)/2);
    y = YLPTODP(dc, (top + bottom)/2);

    w = XLSTODS(dc, (right - left));
    h = YLSTODS(dc, (bottom - top));

    PSDRV_WriteArc(dc, x, y, w, h, 0.0, 360.0);
    PSDRV_WriteClosePath(dc);
    PSDRV_Brush(dc,0);
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_PolyPolyline
 */
BOOL32 PSDRV_PolyPolyline( DC *dc, const POINT32* pts, const DWORD* counts,
			   DWORD polylines )
{
    DWORD polyline, line;
    const POINT32* pt;
    TRACE(psdrv, "\n");

    pt = pts;
    for(polyline = 0; polyline < polylines; polyline++) {
        PSDRV_WriteMoveTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	pt++;
	for(line = 1; line < counts[polyline]; line++) {
	    PSDRV_WriteLineTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	    pt++;
	}
    }
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}   


/***********************************************************************
 *           PSDRV_Polyline
 */
BOOL32 PSDRV_Polyline( DC *dc, const POINT32* pt, INT32 count )
{
    return PSDRV_PolyPolyline( dc, pt, (LPDWORD) &count, 1 );
}


/***********************************************************************
 *           PSDRV_PolyPolygon
 */
BOOL32 PSDRV_PolyPolygon( DC *dc, const POINT32* pts, const INT32* counts,
			  UINT32 polygons )
{
    DWORD polygon, line;
    const POINT32* pt;
    TRACE(psdrv, "\n");

    pt = pts;
    for(polygon = 0; polygon < polygons; polygon++) {
        PSDRV_WriteMoveTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	pt++;
	for(line = 1; line < counts[polygon]; line++) {
	    PSDRV_WriteLineTo(dc, XLPTODP(dc, pt->x), YLPTODP(dc, pt->y));
	    pt++;
	}
	PSDRV_WriteClosePath(dc);
    }

    if(dc->w.polyFillMode == ALTERNATE)
        PSDRV_Brush(dc, 1);
    else /* WINDING */
        PSDRV_Brush(dc, 0);
    PSDRV_SetPen(dc);
    PSDRV_WriteStroke(dc);
    return TRUE;
}


/***********************************************************************
 *           PSDRV_Polygon
 */
BOOL32 PSDRV_Polygon( DC *dc, const POINT32* pt, INT32 count )
{
     return PSDRV_PolyPolygon( dc, pt, &count, 1 );
}


/***********************************************************************
 *           PSDRV_SetPixel
 */
COLORREF PSDRV_SetPixel( DC *dc, INT32 x, INT32 y, COLORREF color )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    PSCOLOR pscolor;

    x = XLPTODP(dc, x);
    y = YLPTODP(dc, y);

    PSDRV_WriteRectangle( dc, x, y, 0, 0 );
    PSDRV_CreateColor( physDev, &pscolor, color );
    PSDRV_WriteSetColor( dc, &pscolor );
    PSDRV_WriteFill( dc );
    return color;
}
