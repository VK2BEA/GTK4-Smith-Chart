/*
 * Copyright (c) 2026 Michael G. Katzmann
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file GTKsmithChart.c
 * @brief GtkDrawingArea widget support for Smith chart
 *
 * @author Michael G. Katzmann
 *
 */

/*
 * Adapted by Michael Katzmann from a Postscript program posted
 * on the Usenet group sci.electronics in 1990 by Marshall Jose (mjj@stda.jhuapl.edu)
 * with the following notice:
 *
 * The following PostScript(R) program produces a Smith chart which is
 * functionally equivalent to the ubiquitous Smith chart created by
 * Phillip Smith and marketed by the Kay Electric Company [#82-BSPR (9-66)]
 * and others.  It is reproduced here without permission & is intended for
 * use by cheap students; any plagiarizer incorporating this code into
 * marketed products is welcome to risk his/her own legal neck, but please
 * leave me out of it.
 *
 * 18 December 1990
 * Marshall Jose
 * JHU - Applied Physics Laboratory, Laurel, MD
 *
 * In 2015 the rights to the Smith chart were assigned to the
 * IEEE's Microwave Theory and Techniques Society
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include "GTKsmithChart.h"

static tSmithOptions defaultOptions = {
        .flags.bShowRX      = TRUE,
        .flags.bShowGB      = FALSE,
        .flags.bShowLabels  = TRUE,
        .flags.bShowStrings = TRUE,
        .flags.bDrawRing    = TRUE,
        .flags.bSparceGB    = TRUE,    // use when both RX and GB are shown together (like Form ZY-01-N)

        .lineWidth = 0.25,   // Percentage of radius
        .pointWidth = 0.6,
                        // red/green/blue/alpha
        .colorRXgrid     = { 0.7, 0.0, 0.0, 1.0 },
        .colorGBgrid     = { 0.0, 0.5, 0.5, 1.0 },
        .colorRXtext     = { 0.5, 0.0, 0.0, 1.0 },
        .colorGBtext     = { 0.0, 0.5, 0.5, 1.0 },
        .colorRing       = { 0.0, 0.0, 0.0, 1.0 },
        .colorLine       = { 0.0, 0.0, 0.5, 1.0 },
        .colorAnnotation = { 0.0, 0.5, 0.0, 1.0 },

        .annotationFontSize = 0.4
};

/*
 *   "labels" are the numbers which index the R & X circles.
 *   "region", "minorDiv", and "majorDiv" define the boundaries
 *   and divisions of the various regions within with the same grid density is kept.
 */

static tLabel labels[] = {
        { 0.0,   "0" }, { 0.1, "0.1" }, { 0.2, "0.2" }, { 0.3, "0.3" }, { 0.4, "0.4" },
        { 0.5, "0.5" }, { 0.6, "0.6" }, { 0.7, "0.7" }, { 0.8, "0.8" }, { 0.9, "0.9" },
        { 1.0, "1.0" }, { 1.2, "1.2" }, { 1.4, "1.4" }, { 1.6, "1.6" }, { 1.8, "1.8" },
        { 2.0, "2.0" }, { 3.0, "3.0" }, { 4.0, "4.0" }, { 5.0, "5.0" },
        { 10.0, "10" }, { 20.0, "20" }, { 50.0, "50" }, { 0.0,  NULL } };

static tRegion stdGrid[] = {
        { 0.0, 0.01, 5 }, { 0.2, 0.02, 5 }, { 0.5, 0.05, 2 }, { 1.0,  0.10, 2 },
        { 2.0, 0.20, 5 }, { 5.0, 1.00, 5 }, {10.0, 2.00, 5 }, {20.0, 10.00, 5 },
        {50.0, 0.00, END } };

static tRegion sparseGrid[] = {
        {  0, 0.1, 5 }, {  1, 0.2, 5 }, {  2,  0.5, 2 },
        {  4, 1.0, 6 }, { 10, 5.0, 2 }, { 20, 30.0, SPECIAL_CASE },
        { 50, 0.0, END } };

/*!     \brief  Convert normalized R+jX to Complex Gamma
 *
 * Convert normalized R+jX to the complex gamma space.
 * Gamma = (Z-1)/(Z+1). i.e. (R-1 + jX)/(R+1 + jX)
 * Multiply the numerator and denominator by the complex conjugate of
 * the denominator (R+1 - jX) which makes the denominator purely real.
 *
 * \ingroup Smith
 *
 * \param rx    structure containing the normalized R and X
 * \return      the tUV structure containing the normalized cartesian
 *              points on the gamma plane (UV)
 */
tUV
RXtoUV( tRX rx ) {
    gdouble Zplus1_magSqu;
    tUV rtn;

    // This is (Z+1) * its complex conjugate
    // i.e (R+1 + jX) * (R+1 - jX)
    Zplus1_magSqu = SQU(rx.R) + SQU(rx.X) + (rx.R * 2.0) + 1.0;
    // The real (resistive) part of (R-1 + jX) * (R+1 - jX)
    // divided by the squared magnitude of (Z+1)
    rtn.U = (SQU(rx.R) + SQU(rx.X) - 1.0) / Zplus1_magSqu;
    // The imaginary (reactive) part
    rtn.V = rx.X * 2.0 / Zplus1_magSqu;
    return rtn;
}

/*!     \brief  Turn off font metrics hinting
 *
 * Remove font metric hinting so that the font size remains the same
 * in relation to the window size when resizing the drawing area
 *
 * \ingroup drawing
 *
 * \param cr        pointer to cairo context
 */
static void
removeFontHinting( cairo_t *cr ) {
    // Remove hinting so that resize of window does not change
    cairo_font_options_t *pFontOptions = cairo_font_options_create();
    cairo_get_font_options (cr, pFontOptions);
    cairo_font_options_set_hint_style( pFontOptions, CAIRO_HINT_STYLE_NONE );
    cairo_font_options_set_hint_metrics( pFontOptions, CAIRO_HINT_METRICS_OFF );
    cairo_set_font_options (cr, pFontOptions);
    cairo_font_options_destroy( pFontOptions );
}

/*!     \brief  find the width of the string in the current font
 *
 * Determine the width of the string in the current font and scaling
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param sLabel    NULL terminated string to show
 *
 */
static gdouble
stringWidthCairoText(cairo_t *cr, gchar *sLabel)
{
    cairo_text_extents_t extents;
    cairo_text_extents (cr, sLabel, &extents);
    return( extents.x_advance );
}

/*!     \brief  Change the font size
 *
 * Set the font size using the current scaling
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param fSize     font size (scaled)
 *
 */
static void
setCairoFontSize( cairo_t *cr, gdouble fSize ) {
    cairo_matrix_t fMatrix = {0, .xx=fSize, .yy=-fSize};
    cairo_set_font_matrix(cr, &fMatrix);
}


/*!     \brief  Render a text string left justified from the specified point
 *
 * Show the string on the widget left justified clearing the background
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param sLabel    NULL terminated string to show
 * \param x         x position of the right edge of the label
 * \param y         y position of the bottom of the label
 *
 */
static void
leftJustifiedClearText(cairo_t *cr, gchar *sLabel, gdouble x, gdouble y)
{
    cairo_text_extents_t extents;

    cairo_text_extents (cr, sLabel, &extents);

    cairo_save( cr );
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_new_path( cr );
        cairo_rectangle( cr, x, y, (extents.width + extents.x_bearing),
                extents.height + extents.y_bearing );
        cairo_stroke_preserve(cr);
        cairo_fill( cr );
    cairo_restore( cr );

    cairo_move_to(cr, x, y );
    cairo_show_text (cr, sLabel);
}

/*!     \brief  Render a text string right justified from the specified point
 *
 * Show the string on the widget right justified
 *
 * \ingroup drawing
 *
 * \param cr        pointer to cairo context
 * \param sLabel    NULL terminated string to show
 * \param x         x position of the right edge of the label
 * \param y         y position of the bottom of the label
 *
 */
static void
rightJustifiedClearText(cairo_t *cr, gchar *sLabel, gdouble x, gdouble y)
{
    cairo_text_extents_t extents;

    cairo_text_extents (cr, sLabel, &extents);

    cairo_save( cr );
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_new_path( cr );
        cairo_rectangle( cr, x-(extents.width + extents.x_bearing),
                y, (extents.width + extents.x_bearing),
                extents.height + extents.y_bearing );
        cairo_stroke_preserve(cr);
        cairo_fill( cr );
    cairo_restore( cr );

    cairo_move_to(cr, x - stringWidthCairoText(cr, sLabel), y );
    cairo_show_text (cr, sLabel);
}

/*!     \brief  Render a text string center justified around the specified point
 *
 * Show the string on the widget center justified
 *
 * \ingroup drawing
 *
 * \param cr        pointer to cairo context
 * \param sLabel    NULL terminated string to show
 * \param x         x position of the right edge of the label
 * \param y         y position of the bottom of the label
 *
 */
static void
centreJustifiedCairoText(cairo_t *cr, gchar *sLabel, gdouble x, gdouble y)
{
    cairo_move_to(cr, x - stringWidthCairoText(cr, sLabel)/2.0, y);
    cairo_show_text (cr, sLabel);
}


/*!     \brief  Finds the angle of the line from the center of the R=r circle to (r + jx)
 *
 * Finds the angle of the line from the center of the R=r circle to (r + jx)
 *
 * \ingroup Smith
 *
 * \param rx    structure containing the normalized R and X
 * \return      angle (in radians)
 */
static gdouble
angleR( tRX rx ) {
    // convert r+jx to gamma Cartesian coordinates
    tUV uv = RXtoUV( rx );
    // find the angle to the origin
    return atan2( uv.V, uv.U - (rx.R / (rx.R + 1.0)) );
}

//
/*!     \brief  Finds the angle of the line from the center of the X=x circle to (r + jx)
 *
 * Finds the angle of the line from the center of the X=x circle to (r + jx)
 *
 * \ingroup Smith
 *
 * \param rx    structure containing the normalized R and X
 * \return      angle (in radians)
 */
static gdouble
angleX( tRX rx ) {
    // convert r+jx to gamma Cartesian coordinates
    tUV uv = RXtoUV( rx );
    // find the angle to the X point on the U=1 line
    return atan2( uv.V - 1.0 / rx.X, uv.U - 1.0 );
}

/*!     \brief  Draw an arc, scaling for the size of the unit circle
 *
 * Draw an arc, scaling for the size of the unit circle
 *
 * \ingroup Smith
 *
 * \param cr            pointer to cairo context
 * \param radius        radius of the arc
 * \param angleStart    starting at this angle
 * \param angleEnd      ending at this angle
 */
static void
drawArc( cairo_t *cr, tUV uv, gdouble radius, gdouble angleStart, gdouble angleEnd) {
    cairo_arc( cr, uv.U * SMITH_RADIUS,  uv.V * SMITH_RADIUS, radius * SMITH_RADIUS, angleStart, angleEnd);
    cairo_stroke( cr );
}


/*!     \brief  Draw a resistance arc between two reactance arcs
 *
 * Draw a resistance arc between two reactance arcs
 *
 * \ingroup Smith
 *
 * \param cr        pointer to cairo context
 * \param rArc      The resistance arc to plot
 * \param xFrom     Starting on this reactance arc
 * \param xTo       Ending on this reactance arc
 */
static void
drawRarc( cairo_t *cr, gdouble rArc, gdouble xFrom, gdouble xTo ) {
    tUV uv;
    tRX rx;
    gdouble radius, theta1, theta2;

    uv = (tUV){ rArc/(rArc+1.0), 0.0 };
    radius = 1.0 / (rArc+1.0);

    rx = (tRX){rArc, xFrom};
    theta1 = angleR( rx );
    rx.X = xTo;
    theta2 = angleR( rx );
    drawArc( cr, uv, radius, theta1, theta2 );
}

/*!     \brief  Draw a reactance arc between two resistance arcs
 *
 * Draw a reactance arc between two resistance arcs (circles)
 *
 * \ingroup Smith
 *
 * \param cr        pointer to cairo context
 * \param xArc      The reactance arc to plot
 * \param rFrom     Starting on this resistance arc
 * \param rTo       Ending on this resistance arc
 */
static void
drawXarc( cairo_t *cr, gdouble xArc, gdouble rFrom, gdouble rTo ) {
    tUV uv;
    tRX rx;
    gdouble radius, theta1, theta2;

    // Converting to Gamma (UV) coordinates
    // The center of the X curves are on all the U=1 line
    uv = (tUV){ 1.0, 1.0/xArc };
    radius = fabs( uv.V );

    // determine the angle range of the curve to draw
    rx = (tRX){ rFrom, xArc };
    theta1 = angleX( rx );
    rx.R = rTo;
    theta2 = angleX( rx );
    // draw the arc
    drawArc( cr, uv, radius, theta1, theta2 );
}


/*!     \brief  Draw a grid block
 *
 * Draws two grid blocks either side of the X=0 line bounded by RXstart and RXend
 *
 * From RXstart.R to RXend.R, incrementing by minorInc:
 *      - draw resistance curves from RXend.X to RXstart.X
 *      - draw the resistance curves from  -RXstart.X -RXend.X
 *
 * From RXstart.X to RXend.X, incrementing by minorInc:
 *      - draw reactance curves from RXstart.R to RXend.R
 *      - draw the negative reactance curves from  RXend.R RXstart.R
 *
 * for every multiple of majorInc curves, render a thicker line
 *
 * \ingroup Smith
 *
 * \param cr        pointer to cairo context
 * \param RXstart   Start of block on the Smith chart (in R+jX space)
 * \param RXend     End of block on the Smith chart (in R+jX space)
 * \param minorInc  The spacing between the finest grid lines
 * \param majorInc  The spacing between the bold grid lines
 */
static void
drawBlock( cairo_t *cr, tRX RXstart, tRX RXend, gdouble minorInc, gint minorPerMajor ) {
    gint rticks = 1;
    gint xticks = 1;

    for( gdouble r = RXstart.R + minorInc ; r <=  RXend.R + minorInc/2.0; r += minorInc, rticks++ ) {
        if( (rticks % minorPerMajor) == 0 )
            cairo_set_line_width( cr, STROKE_WIDTH_MAJOR );
        else
            cairo_set_line_width( cr, STROKE_WIDTH_MINOR );

        drawRarc( cr, r, RXend.X, RXstart.X );
        drawRarc( cr, r, -RXstart.X, -RXend.X );
    }

    for( gdouble x = RXstart.X + minorInc ; x <=  + RXend.X + minorInc/2.0; x += minorInc, xticks++ ) {
        if( (xticks % minorPerMajor) == 0 )
            cairo_set_line_width( cr, STROKE_WIDTH_MAJOR );
        else
            cairo_set_line_width( cr, STROKE_WIDTH_MINOR );

        drawXarc( cr, x, RXstart.R, RXend.R );
        drawXarc( cr, -x, RXend.R, RXstart.R );
    }

}

/*!     \brief  Draw either the RX or GB grid
 *
 * Draw either the RX or GB grid based upon the information in the
 * zones array. This indicates what density of grid is to be rendered
 * in region of the Smith chart.
 *
 * \ingroup Smith
 *
 * \param cr        pointer to cairo context
 */
static void
drawImmittanceGrid( cairo_t *cr, tRegion zones[], tSmithOptions *pOptions )
{
    gdouble minorinc;
    gint minorPerMajor;
    tRX rxFrom, rxTo;
    // draw grids in each zone
    for( gint index=0; zones[ index ].minorPerMajorDiv != END; index++ ) {

        minorinc = zones[ index ].minorDiv;
        minorPerMajor = zones[ index ].minorPerMajorDiv;

        if( minorPerMajor == SPECIAL_CASE ) {
            // This is a hack to handle the area on the sparse grid
            // near the G=20 circle to match Form ZY-01-N
            cairo_set_line_width( cr, STROKE_WIDTH_MAJOR );
            drawRarc( cr, 20, 50, 20 );
            drawRarc( cr, 20, -20, -50 );

            drawXarc( cr,  20, 20, 50 );
            drawXarc( cr, -20, 50, 20 );
        } else {
            rxFrom = (tRX){ 0.0, zones[ index ].region };
            rxTo = (tRX){ zones[ index + 1 ].region, zones[ index + 1 ].region };
            drawBlock( cr, rxFrom, rxTo, minorinc, minorPerMajor );

            // draw grid blocks around the centerline between R=0.2 and infinity
            rxFrom = (tRX){ zones[ index ].region, 0.0 };
            rxTo = (tRX){ zones[ index + 1 ].region, zones[ index ].region };

            if( index == 7 )
                minorPerMajor = 3; // nobody likes this
            drawBlock( cr, rxFrom, rxTo, minorinc, minorPerMajor );
        }
    }

    cairo_set_line_width( cr, STROKE_WIDTH_MAJOR );
    // center resistance / conductance line ( X=0)
    cairo_move_to( cr, -SMITH_RADIUS, 0 );
    cairo_line_to( cr, SMITH_RADIUS, 0 );
    // outer circle
    cairo_stroke( cr );
    cairo_arc( cr, 0, 0, SMITH_RADIUS, 0, 2.0 * M_PI );
    cairo_stroke( cr );

    // special case for arcs / circles at r and x = 50
    drawRarc( cr, 50, 10000, 0 );
    drawRarc( cr, 50, 0, -10000 );

    drawXarc( cr, 50, 0, 10000 );
    drawXarc( cr, -50, 10000, 0 );

    // Another hack
    if( zones == sparseGrid ) {
        drawRarc( cr, 10, 10, 0 );
        drawRarc( cr, 10, 0, -10 );
        drawXarc( cr, 4, 4, 10 );
        drawXarc( cr, -4, 10, 4 );
    }

    // dot at center
    cairo_new_path( cr );
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_arc( cr, 0, 0, SMITH_RADIUS / 150, 0, 2.0 * M_PI );
    cairo_fill( cr );
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_set_line_width( cr, STROKE_WIDTH_THIN );
    cairo_arc( cr, 0, 0, SMITH_RADIUS / 150, 0, 2.0 * M_PI );
    cairo_stroke( cr );
    cairo_arc( cr, 0, 0, SMITH_RADIUS / 800, 0, 2.0 * M_PI );
    cairo_stroke( cr );
}


/*!     \brief  Render a text string center justified around the specified point
 *
 * Show the string on the widget center justified
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param sLabel    NULL terminated string to show
 * \param x         x position of the right edge of the label
 * \param y         y position of the bottom of the label
 *
 */
static void
circleCairoText(cairo_t *cr, gchar *sLabel, gdouble radius, gdouble angle, gdouble centerX, gdouble centerY )
{
    cairo_text_extents_t extents;
    cairo_text_extents (cr, sLabel, &extents);
    gchar sChar[2] = "A";
    gchar *thisChar;

    gdouble sweepAngle = extents.x_advance / radius;

    cairo_save( cr ); {
        cairo_new_path( cr );
        cairo_set_line_width( cr, 0 );
        // Clear the background
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        // text is rendered on an arc centered at centerX, centerY
        cairo_translate( cr, centerX, centerY );
        // turn the text arc space CW so that the end of the arc meets the x-axis
        cairo_rotate( cr, angle - sweepAngle/2.0 );
        // arc from the lower left to the lower right along an arc
        cairo_arc_negative( cr, centerX, centerY, radius + extents.y_bearing, sweepAngle, 0.0 );
        // lower right to upper right (since we are rotated, this is straight on the x-axis
        cairo_rel_line_to( cr, extents.height, 0.0 );
        // arc back upper right to upper left
        cairo_arc( cr, centerX, centerY, radius + extents.height, 0.0, sweepAngle );
        // upper left back to lower left
        cairo_close_path( cr );
        cairo_stroke_preserve(cr);
        cairo_fill( cr );
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        // turn the text space back so that the lower left is actually left
        cairo_rotate( cr, sweepAngle-M_PI/2 );
        for( thisChar = sLabel, sChar[0] = *sLabel; *thisChar != 0; sChar[ 0 ] = *(++thisChar) ) {
            cairo_text_extents (cr, sChar, &extents);
            // center of the letter tangential to the curve
            cairo_rotate( cr, -(extents.x_advance/2.0) / radius );
            // center the letter
            cairo_move_to( cr, -(extents.x_advance/2.0), radius );
            cairo_show_text (cr, sChar);
            // complete the rotation required by thefull letter width
            cairo_rotate( cr, -(extents.x_advance / 2.0) / radius );
        }
    } cairo_restore( cr );
}

/*!     \brief  draw the value designator labels on the chart
 *
 * Draw the value designator labels on the chart.
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param pOptions  pointer to options settings
 *
 */
static void
drawLabels( cairo_t *cr, tSmithOptions *pOptions ) {
    tUV uv;
    tRX rx;
    gdouble angle;
    gdouble labelMargin = LABELFONTSIZE / 4.0;

    cairo_select_font_face(cr, LABEL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL );
    setCairoFontSize( cr, LABELFONTSIZE );

    for( gint index=1; labels[ index ].text != NULL; index++ ) {
        // +X
        rx = (tRX){ 0.0, labels[ index ].value };
        uv = RXtoUV( rx );
        angle = atan2( uv.V, uv.U );
        cairo_save( cr ); {
            cairo_rotate( cr, angle );
            rightJustifiedClearText( cr,  labels[ index ].text, SMITH_RADIUS - labelMargin, 0.0 + labelMargin );
        } cairo_restore( cr );
        // -X
        rx = (tRX){ 0.0, -labels[ index ].value };
        uv = RXtoUV( rx );
        angle = atan2( uv.V, uv.U ) + M_PI;
        cairo_save( cr ); {
            cairo_rotate( cr, angle );
            leftJustifiedClearText( cr, labels[ index ].text, -SMITH_RADIUS + labelMargin, 0.0 + labelMargin  );
        } cairo_restore( cr );
        // R (along the U axis)
        rx = (tRX){ labels[ index ].value, 0.0 };
        uv = RXtoUV( rx );
        cairo_save( cr ); {
            cairo_rotate( cr, M_PI / 2.0 );
            leftJustifiedClearText( cr, labels[ index ].text, labelMargin, -uv.U + labelMargin );
        } cairo_restore( cr );
    }

    for( gint index=2; index <= 10; index += 2 ) {
        // R labels on the X=1 arc (upper - inductive hemisphere)
        rx = (tRX){ labels[ index ].value, 1.0 };
        uv = RXtoUV( rx );
        cairo_save( cr ); {
            cairo_translate( cr, uv.U * SMITH_RADIUS, uv.V * SMITH_RADIUS );
            cairo_rotate( cr, angleX( rx ) + M_PI );
            leftJustifiedClearText( cr, labels[ index ].text, labelMargin, labelMargin );
        } cairo_restore( cr );

        // R labels on the X=-1 arc (lower - capacitive hemisphere)
        rx = (tRX){ labels[ index ].value, -1.0 };
        uv = RXtoUV( rx );
        cairo_save( cr ); {
            cairo_translate( cr, uv.U * SMITH_RADIUS, uv.V * SMITH_RADIUS );
            cairo_rotate( cr, angleX( rx ) );
            rightJustifiedClearText( cr, labels[ index ].text, -labelMargin, +labelMargin );
        } cairo_restore( cr );

        // X labels on the R=1 circle (upper - inductive hemisphere)
        rx = (tRX){ 1.0, labels[ index ].value };
        uv = RXtoUV( rx );
        cairo_save( cr ); {
            cairo_translate( cr, uv.U * SMITH_RADIUS, uv.V * SMITH_RADIUS );
            cairo_rotate( cr, angleR( rx )  );
            rightJustifiedClearText( cr, labels[ index ].text, -labelMargin, labelMargin );
        } cairo_restore( cr );
        // -X labels on the R=1 circle (lower - capacitive hemisphere)
        rx = (tRX){ 1.0, -labels[ index ].value };
        uv = RXtoUV( rx );
        cairo_save( cr ); {
            cairo_translate( cr, uv.U * SMITH_RADIUS, uv.V * SMITH_RADIUS );
            cairo_rotate( cr, angleR( rx ) + M_PI );
            leftJustifiedClearText( cr, labels[ index ].text, labelMargin, labelMargin );
        } cairo_restore( cr );
    }
}

/*!     \brief  Draw the resistance / impedance Smith grid
 *
 * Draw the resistance / impedance Smith grid
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param areas     array of parameters describing the density of the grid
 * \param pOptions  pointer to options settings
 *
 */
static void
drawRXgrid( cairo_t *cr, tRegion areas[], tSmithOptions *pOptions ) {
    cairo_save( cr ); {
        cairo_set_source_rgba(cr, pOptions->colorRXgrid.red, pOptions->colorRXgrid.green, pOptions->colorRXgrid.blue, pOptions->colorRXgrid.alpha);
        drawImmittanceGrid( cr, areas, pOptions );
    } cairo_restore( cr );
}

/*!     \brief  Draw the text on the resistance / impedance Smith grid
 *
 * Draw the text on the resistance / impedance Smith grid
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param areas     array of parameters describing the density of the grid
 * \param pOptions  pointer to options settings
 *
 */
static void
drawRXgridText( cairo_t *cr, tRegion areas[], tSmithOptions *pOptions ) {
    cairo_save( cr ); {
        cairo_set_source_rgba(cr, pOptions->colorRXtext.red, pOptions->colorRXtext.green, pOptions->colorRXtext.blue, pOptions->colorRXtext.alpha);
        cairo_set_line_width( cr, 0.0);
        if( pOptions->flags.bShowLabels )
            drawLabels( cr, pOptions );

        if( pOptions->flags.bShowStrings ) {
            gdouble resistanceTextVpos = -( LABELFONTSIZE + SRpct(0.8) );

            if( pOptions->flags.bShowGB ) {
                resistanceTextVpos -= ( LABELFONTSIZE + SRpct(0.4) );
            }

            circleCairoText( cr, "INDUCTIVE REACTANCE COMPONENT (+jX/Zo)", SRpct(94), DEGtoRAD(141.7), 0, 0 );
            circleCairoText( cr, "CAPACITIVE REACTANCE COMPONENT (-jX/Zo)", SRpct(94), DEGtoRAD(-141.7), 0, 0 );
            leftJustifiedClearText( cr, "RESISTANCE COMPONENT (R/Zo)", SRpct(-32.5), resistanceTextVpos );
        }
    } cairo_restore( cr );
}

/*!     \brief  Draw the conductance / susceptance Smith grid
 *
 * Draw the conductance / susceptance Smith grid
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param areas     array of parameters describing the density of the grid
 * \param pOptions  pointer to options settings
 *
 */
static void
drawGBgrid( cairo_t *cr, tRegion areas[], tSmithOptions *pOptions ) {
    cairo_save( cr ); {
        cairo_set_source_rgba(cr, pOptions->colorGBgrid.red, pOptions->colorGBgrid.green, pOptions->colorGBgrid.blue, pOptions->colorGBgrid.alpha);
        cairo_rotate( cr, M_PI );
        drawImmittanceGrid( cr, areas, pOptions );
    } cairo_restore( cr);
}

/*!     \brief  Draw the text on the conductance / susceptance Smith grid
 *
 * Draw the text on the conductance / susceptance Smith grid
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param areas     array of parameters describing the density of the grid
 * \param pOptions  pointer to options settings
 *
 */
static void
drawGBgridText( cairo_t *cr, tRegion areas[], tSmithOptions *pOptions ) {
    cairo_save( cr ); {
        cairo_rotate( cr, M_PI );
        cairo_set_source_rgba(cr, pOptions->colorGBtext.red, pOptions->colorGBtext.green, pOptions->colorGBtext.blue, pOptions->colorGBtext.alpha);
        cairo_set_line_width( cr, 0.0);

        if( pOptions->flags.bShowLabels )
            drawLabels( cr, pOptions );

        if( pOptions->flags.bShowStrings ) {
            gdouble reactanceTextAngle = 141.7;
            gdouble coductanceTextVpos = SRpct(0.8);
            cairo_rotate( cr, -M_PI );

            if( pOptions->flags.bShowRX ) {
                reactanceTextAngle -= 27.0;
                coductanceTextVpos += LABELFONTSIZE + SRpct(0.4);
            }
            circleCairoText( cr, "CAPACITIVE SUSCEPTANCE COMPONENT (+jX/Yo)", SRpct(94),
                    DEGtoRAD(reactanceTextAngle), 0, 0 );
            circleCairoText( cr, "INDUCTIVE SUSCEPTANCE COMPONENT (-jB/Yo)", SRpct(94),
                    DEGtoRAD(-reactanceTextAngle), 0, 0 );
            leftJustifiedClearText( cr, "CONDUCTANCE COMPONENT (G/Yo)", SRpct(-32.5),
                    coductanceTextVpos + SRpct(0.8) );
        }
    } cairo_restore( cr);
}

/*!     \brief  Draw a curved arrow in the outer ring
 *
 * Draw an curved (arc) arrow in the outer ring
 *
 * \ingroup plot
 *
 * \param cr            pointer to cairo context
 * \param radius        radius of the arrow arc
 * \param startAngle    start angle of arrow arc
 * \param stopAngle     end (head) of arrow
 *
 */
void
drawCurvedArrow( cairo_t *cr, gdouble radius, gdouble startAngle, gdouble stopAngle ) {
    gboolean bCW = FALSE;
    cairo_save( cr ); {
        cairo_new_path( cr );
        cairo_set_line_width( cr, SRpct(0.2) );

        if( startAngle > stopAngle )
            bCW = TRUE;

        if( bCW )
            cairo_arc_negative( cr, 0, 0, radius, startAngle, stopAngle );
        else
            cairo_arc( cr, 0, 0, radius, startAngle, stopAngle );

        cairo_stroke( cr );

        cairo_rotate( cr, stopAngle );
        cairo_set_line_width( cr, 0 );
        cairo_new_path( cr );
        cairo_move_to( cr, radius, 0 );
        cairo_rel_line_to( cr, SRpct(  0.7 ), SRpct( bCW ?  2   : -2   ) );
        cairo_rel_line_to( cr, SRpct( -0.7 ), SRpct( bCW ? -0.8 :  0.8 ) );
        cairo_rel_line_to( cr, SRpct( -0.7 ), SRpct( bCW ?  0.8 : -0.8 ) );
        cairo_close_path( cr );
        cairo_fill( cr );
    } cairo_restore( cr );
}

/*!     \brief  Draw the outer wavelength ring
 *
 * Draw the outer wavelength ring (toward generator / toward load)
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param pOptions pointer to options settings
 *
 */
void
drawWavelengthRing( cairo_t *cr, tSmithOptions *pOptions ) {
    gint ix;
    gdouble lstep;
    cairo_save( cr ); {
        cairo_set_line_width( cr, STROKE_WIDTH_MINOR );
        cairo_select_font_face(cr, LABEL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL );
        setCairoFontSize( cr, LABELFONTSIZE );

        cairo_new_path( cr );
        cairo_arc( cr, 0, 0, WAVE_RING_RADIUS, 0, 2.0 * M_PI );
        cairo_stroke( cr );

        for( ix=1, lstep = M_PI / 125; ix <= 250; ix++ ) {
            cairo_save( cr ); {
                cairo_rotate( cr, ix * lstep);
                cairo_move_to( cr, -(WAVE_RING_RADIUS + SRpct(0.8)) , 0 );
                cairo_rel_line_to( cr, SRpct(1.6), 0);
                cairo_stroke( cr );
            } cairo_restore( cr );

            if( ix % 5 == 0 && ix > 16 ) {
                gchar *sWaveNumber = g_strdup_printf( "%.2f", ix != 250 ? ((gdouble)ix) / 500.0 : 0.0 );

                cairo_save( cr ); {
                    cairo_rotate( cr, ix * lstep);
                    cairo_translate( cr, -(WAVE_RING_RADIUS - SRpct(1.25) - LABELFONTSIZE), 0 );
                    cairo_rotate( cr, M_PI / 2.0);
                    centreJustifiedCairoText( cr, sWaveNumber, 0.0, 0.0);
                } cairo_restore( cr );

                cairo_save( cr ); {
                    cairo_rotate( cr, -ix * lstep);
                    cairo_translate( cr, -(WAVE_RING_RADIUS + SRpct(1.5)), 0 );
                    cairo_rotate( cr, M_PI / 2.0);
                    centreJustifiedCairoText( cr, sWaveNumber, 0.0, 0.0);
                } cairo_restore( cr );

                g_free( sWaveNumber );
            }
        }

        circleCairoText( cr, "WAVELENGTHS TOWARD GENERATOR", WAVE_RING_RADIUS + SRpct(1.25), DEGtoRAD(165.6), 0, 0 );
        circleCairoText( cr, "WAVELENGTHS TOWARD LOAD", WAVE_RING_RADIUS - SRpct( 3 ), DEGtoRAD(-165.5), 0, 0 );

        drawCurvedArrow( cr, WAVE_RING_RADIUS + SRpct(2.0), DEGtoRAD(178.2), DEGtoRAD(174.9) );
        drawCurvedArrow( cr, WAVE_RING_RADIUS + SRpct(2.0), DEGtoRAD(156.3), DEGtoRAD(153.0) );

        drawCurvedArrow( cr, WAVE_RING_RADIUS - SRpct(2.1), DEGtoRAD(-176.8), DEGtoRAD(-173.6) );
        drawCurvedArrow( cr, WAVE_RING_RADIUS - SRpct(2.1), DEGtoRAD(-157.5), DEGtoRAD(-154.2) );

        cairo_new_path( cr );
        cairo_set_line_width( cr, STROKE_WIDTH_MINOR );
        cairo_arc( cr, 0, 0, OUTER_BOUNDARY_WITH_RING, 0, 2.0 * M_PI );
        cairo_stroke( cr );
    } cairo_restore( cr );
}

/*!     \brief  Show a string at 90 degrees to a radial (top of T)
 *
 * Show a string at 90 degrees to a radial (top of T)
 *
 * \ingroup plot
 *
 * \param cr                pointer to cairo context
 * \param radialAngle       angle of the radial
 * \param radialDistance    distance from the origin
 * \param sLable            pointer to sting to print
 *
 */
void
printNormalToRadial ( cairo_t *cr, gdouble radialAngle, gdouble radialDistance, gchar *sLabel ){
    cairo_save( cr ); {
        cairo_rotate( cr, radialAngle );
        cairo_translate( cr, radialDistance, 0.0 );
        cairo_rotate( cr, -M_PI/2.0 );   // print on the normal to the radial
        centreJustifiedCairoText( cr, sLabel, 0.0, 0.0);
    } cairo_restore( cr );
}

/*
 * Return the
 *
 */
/*!     \brief  Calculate the distance from the (-1,0) to the coefficient angle circle.
 *
 * Calculate the distance from the (-1,0) to the coefficient angle circle at
 * a particular angle. (for the start point of the tick mark).
 *
 * \ingroup plot
 *
 * \param angleDegrees      angle of the radial in degrees
 * \param unitRadius        radius of the smith chart (usually 1)
 * \param coeffRadiuus      radius of the coefficient circle
 *
 */
gdouble
findTCradial( gdouble angleDegrees, gdouble unitRadius, gdouble coeffRadiuus ) {
    gdouble inter, angleRadians;
    angleRadians = DEGtoRAD( angleDegrees );

    inter = sin( angleRadians ) * unitRadius / coeffRadiuus;
    inter = atan2( inter / sqrt( 1.0 - (inter * inter) ), 1.0);

    return( sin( M_PI - angleRadians - inter )
            * coeffRadiuus / sin( angleRadians ) );
}

/*!     \brief  Draw the coefficient ring
 *
 * Draw the coefficient ring (angle of reflection/transmission)
 *
 * \ingroup plot
 *
 * \param cr        pointer to cairo context
 * \param pOptions pointer to options settings
 *
 */
void
drawAngleRing( cairo_t *cr, tSmithOptions *pOptions ) {
    gint deg;
#define SSTRLEN 20
    gchar sstr[ SSTRLEN ];
    cairo_save( cr ); {

        cairo_set_line_width( cr, STROKE_WIDTH_MINOR );
        // inner circle
        cairo_new_path( cr );
        cairo_arc( cr, 0, 0, ANGLE_RING_RADIUS, 0, 2.0 * M_PI );
        cairo_arc( cr, 0, 0, ANGLE_RING_RADIUS + SRpct( 3.5 ), 0, 2.0 * M_PI );
        cairo_stroke( cr );

        cairo_save( cr ); {
            for( deg = 0; deg <= 178; deg += 2 ) {
                cairo_move_to( cr, -ANGLE_RING_RADIUS, 0 );
                cairo_rel_line_to( cr, SRpct( -1.5 ), 0 );
                cairo_stroke( cr );
                cairo_move_to( cr, ANGLE_RING_RADIUS, 0 );
                cairo_rel_line_to( cr, SRpct( 1.5 ), 0 );
                cairo_stroke( cr );
                cairo_rotate( cr, DEGtoRAD( 2 ) );
            }
        } cairo_restore( cr );

        for( deg = 20; deg <= 170; deg += 10 ) {
            g_snprintf( sstr, SSTRLEN, "%d", deg );
            printNormalToRadial ( cr, DEGtoRAD( deg ), ANGLE_RING_RADIUS + SRpct( 1 ), sstr );
            g_snprintf( sstr, SSTRLEN, "%d", -deg );
            printNormalToRadial ( cr, DEGtoRAD( -deg ), ANGLE_RING_RADIUS + SRpct( 1 ), sstr );
        }
        printNormalToRadial ( cr, DEGtoRAD( 180 ), ANGLE_RING_RADIUS + SRpct( 1 ), "±180" );

        cairo_save( cr ); {
            cairo_translate( cr, -SMITH_RADIUS, 0 );
            for( deg = 90; deg >= 1; deg += -1 ) {
                gdouble TCradial = findTCradial( deg, SMITH_RADIUS, ANGLE_RING_RADIUS );
                cairo_save( cr ); {
                    cairo_rotate( cr, DEGtoRAD( deg ) );
                    cairo_move_to( cr, TCradial, 0 );
                    cairo_rel_line_to( cr, SRpct( deg <= 55 ? -1.5 : -2.0 ), 0 );
                    cairo_stroke( cr );
                    if( deg >= 10 && (deg % 5) == 0 ) {
                        cairo_move_to( cr, TCradial - SRpct( 0.85 ), 0);
                        g_snprintf( sstr, SSTRLEN, "%d", deg );
                        cairo_rel_move_to(cr,
                                -stringWidthCairoText(cr, sstr) - (LABELFONTSIZE * deg/90),
                                -LABELFONTSIZE * (deg <= 45 ? 0.33 : deg/90.0) );
                        cairo_show_text (cr, sstr);
                    }
                } cairo_restore( cr );
                cairo_save( cr ); {
                    cairo_rotate( cr, M_PI - DEGtoRAD( deg ) );
                    cairo_move_to( cr, -TCradial, 0 );
                    cairo_rel_line_to( cr, SRpct( deg <= 55 ? 1.5 : 2.0 ), 0 );
                    cairo_stroke( cr );
                    if( deg >= 10 && (deg % 5) == 0 ) {
                        cairo_move_to( cr, -TCradial + LABELFONTSIZE/(deg < 45 ? 3 : 2 ),
                                -LABELFONTSIZE * (deg <= 45 ? 0.5 : deg/90.0) );
                               // deg <= 45 ? (-LABELFONTSIZE * 0.5) : (-LABELFONTSIZE * deg/90));
                        g_snprintf( sstr, SSTRLEN, "%d", -deg );
                        cairo_show_text (cr, sstr);
                    }
                } cairo_restore( cr );
            }
        } cairo_restore( cr );

    } cairo_restore( cr );

    circleCairoText( cr, "ANGLE OF REFLECTION COEFFICIENT IN DEGREES", ANGLE_RING_RADIUS + SRpct( 1 ), DEGtoRAD(0), 0, 0 );
    circleCairoText( cr, "ANGLE OF TRANSMISSION COEFFICIENT IN DEGREES", ANGLE_RING_RADIUS - SRpct( 2.7 ), DEGtoRAD(0), 0, 0 );
}


/*!     \brief  Draw a line on the Smith chart
 *
 * Draw a line on the Smith chart from two points in Cartesian gamma space
 *
 * \ingroup plot
 *
 * \param cr                pointer to the cairo context
 * \param uvFrom            starting point
 * \param uvTo              ending point
 * \param pOptions          pointer to options settings
 *
 */
void
drawLineOnSmithChart( cairo_t *cr, tUV uvFrom, tUV uvTo, tSmithOptions *pOptions ) {
    if( pOptions == NULL )
        pOptions = &defaultOptions;

    cairo_save( cr ); {
        // restore scaling and transformation
        cairo_set_matrix( cr, &pOptions->matrix );

        cairo_set_line_width( cr, SRpct(pOptions->lineWidth) );
        cairo_set_source_rgba(cr, pOptions->colorLine.red, pOptions->colorLine.green, pOptions->colorLine.blue, pOptions->colorLine.alpha);

        cairo_new_path( cr );
        cairo_move_to( cr, uvFrom.U, uvFrom.V );
        cairo_line_to( cr, uvTo.U, uvTo.V );
        cairo_stroke( cr );

    } cairo_restore( cr );
}

/*
 * Annotating routines - lines, curves & points on the smith chart
 */


// This factor defines the "curviness". Play with it!
#define CURVE_F 0.25

/*! This function calculates the control points. It takes two lines g and l as
 * arguments but it takes three lines into account for calculation. This is
 * line g (P0/P1), line h (P1/P2), and line l (P2/P3). The control points being
 * calculated are actually those for the middle line h, this is from P1 to P2.
 * Line g is the predecessor and line l the successor of line h.
 *
 * \ingroup plot
 *
 * \param g     pointer to first line.
 * \param l     pointer to third line (the second line is connecting g and l).
 *
 * \param p1    pointer to where we write the first control point
 * \param p2    pointer to where we write the second control point
 */
void
bezierControlPoints(const tLine *g, const tLine *l, tUV *p1, tUV *p2)
{
   tLine h;
   double f = CURVE_F;
   double lgt, a;

   // calculate the angle of line in respect to the coordinate system
   #define angle(x) (atan2((x)->B.V - (x)->A.V, (x)->B.U - (x)->A.U))

   // calculate length of line (P1/P2)
   lgt = sqrt(pow(g->B.U - l->A.U, 2) + pow(g->B.V - l->A.V, 2));

   // end point of 1st tangent
   h.B = l->A;
   // start point of tangent at same distance as end point along 'g'
   h.A.U = g->B.U - lgt * cos(angle(g));
   h.A.V = g->B.V - lgt * sin(angle(g));

   // angle of tangent
   a = angle(&h);
   // 1st control point on tangent at distance 'lgt * f'
   p1->U = g->B.U + lgt * cos(a) * f;
   p1->V = g->B.V + lgt * sin(a) * f;

   // start point of 2nd tangent
   h.A = g->B;
   // end point of tangent at same distance as start point along 'l'
   h.B.U = l->A.U + lgt * cos(angle(l));
   h.B.V = l->A.V + lgt * sin(angle(l));

   // angle of tangent
   a = angle(&h);
   // 2nd control point on tangent at distance 'lgt * f'
   p2->U = l->A.U - lgt * cos(a) * f;
   p2->V = l->A.V - lgt * sin(a) * f;
}

/*!     \brief  Draw a smooth curve on the Smith chart
 *
 * Draw a smooth curve through an array of points (in Cartesian gamma space)
 * on the points Smith chart
 *
 * \ingroup plot
 *
 * \param cr            pointer to the cairo context
 * \param uvPoints      array of points in gamma space
 * \param length        number of points
 * \param pOptions      pointer to options settings
 *
 */
void
drawBezierCurveOnSmithChart(cairo_t *cr, const tUV uvPoints[], gint length, tSmithOptions *pOptions)
{
   // Helping variables for lines.
   tLine g, l;
   // Variables for control points.
   tUV c1, c2;

   if( pOptions == NULL )
       pOptions = &defaultOptions;

   cairo_save( cr ); {
       // restore scaling and transformation
       cairo_set_matrix( cr, &pOptions->matrix );

       cairo_set_line_width( cr, SRpct(pOptions->lineWidth) );
       cairo_set_source_rgba(cr, pOptions->colorLine.red, pOptions->colorLine.green, pOptions->colorLine.blue, pOptions->colorLine.alpha);

       cairo_new_path( cr );
       // Draw bezier curve through all points.
       cairo_move_to(cr, uvPoints[0].U, uvPoints[0].V);
       for (int i = 1; i < length; i++)
       {
          g.A = uvPoints[(i + length - 2) % length];
          g.B = uvPoints[(i + length - 1) % length];
          l.A = uvPoints[(i + length + 0) % length];
          l.B = uvPoints[(i + length + 1) % length];

          // Calculate controls points for points pt[i-1] and pt[i].
          bezierControlPoints(&g, &l, &c1, &c2);

          // Handle special case because points are not connected in a loop.
          if (i == 1)
              c1 = g.B;
          if (i == length - 1)
              c2 = l.A;

          // Create Cairo curve path.
          cairo_curve_to(cr, c1.U, c1.V, c2.U, c2.V, uvPoints[i].U, uvPoints[i].V);
       }
       // Actually draw curve.
       cairo_stroke(cr);
   } cairo_restore( cr );
}

/*!     \brief  Draw connected lines on the Smith chart
 *
 * Draw a lines on the Smith chart between points in Cartesian gamma space
 *
 * \ingroup plot
 *
 * \param cr                pointer to the cairo context
 * \param uvuvPointsFrom    array of points in gamma space
 * \param length            number of points
 * \param pOptions          pointer to options settings
 *
 */
void
drawLineArrayOnSmithChart( cairo_t *cr, tUV uvPoints[], gint length, tSmithOptions *pOptions ) {
    if( pOptions == NULL )
        pOptions = &defaultOptions;

    cairo_save( cr ); {
        // restore scaling and transformation
        cairo_set_matrix( cr, &pOptions->matrix );

        cairo_set_line_width( cr, SRpct(pOptions->lineWidth) );
        cairo_set_source_rgba(cr, pOptions->colorLine.red, pOptions->colorLine.green,
                pOptions->colorLine.blue, pOptions->colorLine.alpha);

        cairo_new_path( cr );

        for( gint i=0; i < length; i++ ) {
            if( i == 0 )
                cairo_move_to( cr, uvPoints[0].U, uvPoints[0].V );
            else
                cairo_line_to( cr, uvPoints[i].U, uvPoints[i].V );
        }
        cairo_stroke( cr );

    } cairo_restore( cr );
}

/*!     \brief  Draw a point on the Smith chart
 *
 * Draw a point on the Smith chart
 *
 * \ingroup plot
 *
 * \param cr                pointer to the cairo context
 * \param uvPoint           point to plot
 * \param pOptions          pointer to options settings
 *
 */
void
drawPointOnSmithChart( cairo_t *cr, tUV uvPoint, tSmithOptions *pOptions ) {
    if( pOptions == NULL )
        pOptions = &defaultOptions;

    cairo_save( cr ); {
        // restore scaling and transformation
        cairo_set_matrix( cr, &pOptions->matrix );

        cairo_set_line_width( cr, 0.0 );
        cairo_set_source_rgba(cr, pOptions->colorLine.red, pOptions->colorLine.green,
                pOptions->colorLine.blue, pOptions->colorLine.alpha);

        cairo_new_path( cr );

        cairo_arc( cr, uvPoint.U, uvPoint.V, SRpct( pOptions->pointWidth ), 0, 2.0 * M_PI );
        cairo_fill( cr );

        cairo_stroke( cr );

    } cairo_restore( cr );
}


/*!     \brief  Draw text label at the specified point
 *
 * Draw a smooth curve through an array of points (in Cartesian gamma space)
 * on the points Smith chart
 *
 * \ingroup plot
 *
 * \param cr        pointer to the cairo context
 * \param sLabel    string to display
 * \param uv        point in gamma Cartesian space to place the label
 * \param bLeft     left justify (TRUE) or right justify
 * \param pOptions  pointer to options settings
 *
 */
void
annotatePointOnSmithChart( cairo_t *cr, gchar *sLabel, tUV uv, gboolean bLeft, tSmithOptions *pOptions ) {
    gdouble fontSize;

    if( pOptions == NULL )
        pOptions = &defaultOptions;

    cairo_save( cr ); {
        // restore scaling and transformation
        cairo_set_matrix( cr, &pOptions->matrix );

        cairo_set_line_width( cr, 0.0 );
        cairo_set_source_rgba(cr, pOptions->colorAnnotation.red, pOptions->colorAnnotation.green,
                pOptions->colorAnnotation.blue, pOptions->colorAnnotation.alpha);

        cairo_select_font_face(cr,
                pOptions->annotationFont ? pOptions->annotationFont : LABEL_FONT,
                CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL );
        fontSize = pOptions->annotationFontSize ? pOptions->annotationFontSize/100.0 * SMITH_RADIUS : 2 * LABELFONTSIZE;
        setCairoFontSize( cr, fontSize );

        if( bLeft )
            leftJustifiedClearText( cr,  sLabel, uv.U + (fontSize * 0.5), uv.V - (fontSize * 0.3) );
        else
            rightJustifiedClearText( cr, sLabel, uv.U - (fontSize * 0.5), uv.V - (fontSize * 0.3) );
    } cairo_restore( cr );
}

/*!     \brief  Draw the Smith chart at the specified location
 *
 * Draw the Smith chart at the specified location and size on the drawing widget
 *
 * \ingroup plot
 *
 * \param cr                pointer to the cairo context
 * \param centerX           horizontal center of the chart
 * \param centerY           vertical center of the drawing area
 * \param radius            size of the whole chart (including rings)
 * \param pOptions          pointer to options settings
 *
 */
void
drawSmithChart( cairo_t *cr, gdouble centerX, gdouble centerY,
        gdouble radius, tSmithOptions *pOptions ) {

    if( pOptions == NULL )
        pOptions = &defaultOptions;

    // Adjust scale factor to account for the wavelength / angle rings
   if( pOptions->flags.bDrawRing )
       radius /= ( OUTER_BOUNDARY_WITH_RING / SMITH_RADIUS );

   cairo_save( cr ); {
       // this stops keeps the fonts in proportion to the size of the area
       removeFontHinting( cr );

       // Origin in the center of the drawing area
       cairo_translate( cr, centerX, centerY );
       // scale so the the UnitRadius is 1
       cairo_scale( cr, radius, -radius );
       // Set the font and font size
       cairo_select_font_face(cr, LABEL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL );
       setCairoFontSize( cr, LABELFONTSIZE );

       if( pOptions->flags.bShowGB ) {
           drawGBgrid( cr,  pOptions->flags.bSparceGB ? sparseGrid : stdGrid, pOptions );
       }
       if( pOptions->flags.bShowRX ) {
           drawRXgrid( cr,  stdGrid, pOptions );
       }

       if( pOptions->flags.bShowRX ) {
           drawRXgridText( cr,  stdGrid, pOptions );
       }
       if( pOptions->flags.bShowGB ) {
           drawGBgridText( cr,  pOptions->flags.bSparceGB ? sparseGrid : stdGrid, pOptions );
       }


       if( pOptions->flags.bDrawRing ) {
           cairo_set_source_rgba(cr, pOptions->colorRing.red, pOptions->colorRing.green, pOptions->colorRing.blue, pOptions->colorRing.alpha);
           drawWavelengthRing( cr, pOptions );
           drawAngleRing( cr, pOptions );
       }

       // Save the transformation matrix relevant to the Smith chart.
       // In this way, we can restore it so that annotations can me placed properly.
       cairo_get_matrix( cr, &pOptions->matrix );

   } cairo_restore( cr );
}

/*
 * Example implementation of the Smith chart GtkDrawingArea() widget
 */
#if 0
/*!     \brief  Callback of the GtkDrawing widget to draw
 *
 * Callback of the GtkDrawing widget to draw the Smith chart
 * and any other objects the user chooses on the drawing area.
 *
 * First the Smith grid is drawn scaled and positioned in the drawing area.
 * Curves, lines, points and annotations may be added.
 *
 * This callback is invoked on creation, resize etc.
 *
 * \ingroup plot
 *
 * \param wDrawingArea      pointer to the GtkDrawingArea widget
 * \param cr                pointer to the cairo context
 * \param width             width of the drawing area
 * \param height            height of the drawing area
 * \param pOptions          pointer to options settings
 *
 */
static void
CB_drawingArea_SmithS11 (GtkDrawingArea *wDrawingArea, cairo_t *cr, gint width, gint height, gpointer user_data) {

    // Customize the Smith chart.
    tSmithOptions optionsS11 = {
            .flags.bShowRX      = TRUE,
            .flags.bShowGB      = FALSE,
            .flags.bShowLabels  = TRUE,
            .flags.bShowStrings = TRUE,
            .flags.bDrawRing    = TRUE,
            .flags.bSparceGB    = TRUE,    // use when both RX and GB are shown together (like Form ZY-01-N)

            .lineWidth  = 0.25, // as a percentage of grid radius
            .pointWidth = 0.6,
            .annotationFontSize = 0.4,
                                //red/green/blue/alpha
            .colorRXgrid     =  { 0.7, 0.0, 0.0, 1.0 },     // red RX grid
            .colorGBgrid     =  { 0.0, 0.5, 0.5, 1.0 },     // cyan GB grid
            .colorRXtext     =  { 0.5, 0.0, 0.0, 1.0 },     // dark red text on RX grid
            .colorGBtext     =  { 0.0, 0.5, 0.5, 1.0 },     // dark cyan on GB grid
            .colorRing       =  { 0.0, 0.0, 0.0, 1.0 },     // outer ring in black
            .colorLine       =  { 0.0, 0.0, 0.5, 1.0 },     // curves, lines and points in dark blue
            .colorAnnotation =  { 0.0, 0.5, 0.0, 1.0 }      // annotations in dark green
    };

    // Pass the pointer to the options of this Smith chart to the drawing routines
    // Each Smith chart widget can have different options, or if pOptions is NULL
    // the default options.
    tSmithOptions *pOptions = &optionsS11;

#define SIZE_PCT    98
    // Set the size as percentage (say 98%) of the smallest dimension of the window.
    // If we have other information to display, we may choose to make this smaller
    // to accommodate.
    gdouble smithRadius = min( height, width )/2.0 * (SIZE_PCT / 100.0);
    // Position the Smith chart on the drawing area. e.g. in the middle
    gdouble centerX = width/2.0, centerY = height/2.0;
    // Draw the Smith chart
    drawSmithChart( cr, centerX, centerY, smithRadius, pOptions );

#define EXAMPLE
#ifdef EXAMPLE
    // TODO: display other information if appropriate or plot curves, lines or points onto Smith chart.
    // Use RXtoUV() to convert from normalized resistance & impedance to gamma space (UV) if needed.
#ifdef EXAMPLE_DRAW_LINE
    drawLineOnSmithChart( cr, (tUV){0.25, -0.25},(tUV){0.45, -0.30}, pOptions );
#endif
    // example exampleCurve on smith chart as
    // a series of points in gamma Cartesian coordinates
    tUV exampleCurve[] = {
            {-0.3000, 0.4000},
            {-0.2273, 0.4479},
            {-0.1545, 0.4826},
            {-0.0818, 0.5041},
            {-0.0091, 0.5124},
            { 0.0636, 0.5074},
            { 0.1364, 0.4893},
            { 0.2091, 0.4579},
            { 0.2818, 0.4132},
            { 0.3545, 0.3554},
            { 0.4273, 0.2843},
            { 0.5000, 0.2000}
    };

#define USE_BEZIER_INTERPOLATION
#ifdef  USE_BEZIER_INTERPOLATION
    // overlay our example curve
    // smoothly interpolate the array of points
    drawBezierCurveOnSmithChart( cr, exampleCurve, sizeof( exampleCurve )/ sizeof( tUV ), pOptions );
#else
    // join points by line segments
    drawLineArrayOnSmithChart( cr, exampleCurve, sizeof( exampleCurve )/ sizeof( tUV ), pOptions );

#endif
    // draw an example point on the smith chart
    drawPointOnSmithChart( cr, RXtoUV((tRX){0.9, 1.1}), pOptions );
    annotatePointOnSmithChart( cr, "70.25 MHz", RXtoUV((tRX){0.9, 1.1}), TRUE, pOptions );
#endif
}




/*!     \brief  Callback when the GTK program starts to create the GUI
 *
 * Callback when the GTK program starts to create the GUI
 *
 *
 * \param wApplication  pointer to the GtkApplication widget
 * \param gpOptions     pointer to the array of option
 */
static void CB_activate (GtkApplication *wApplication, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *wDrawingSmith;

    window = gtk_application_window_new (wApplication);
    gtk_window_set_title (GTK_WINDOW (window), "GTK4 / Cairo Graphics Smith Chart");

    // Create the drawing area, set it's size and options if
    // not using a GtkBuilder file.
    wDrawingSmith = gtk_drawing_area_new ();
    gtk_widget_set_size_request (wDrawingSmith, 1000, 1000);
    gtk_widget_set_hexpand (wDrawingSmith, TRUE); // Expand horizontally
    gtk_widget_set_vexpand (wDrawingSmith, TRUE); // Expand vertically
    // Connect the callback function for actually drawing in the widget
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (wDrawingSmith), CB_drawingArea_SmithS11, user_data, NULL);

    gtk_window_set_child (GTK_WINDOW (window), wDrawingSmith);
    gtk_window_present (GTK_WINDOW (window));
}


/*!     \brief  Test program to create the widget and display the Smith chart
 *
 * Test program to create the widget and display the Smith chart
 *
 *
 * \param argc      number of arguments
 * \param argv      pointer to the array of arguments
 * \return          status
 */
int main (int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.gtk.SmithChart", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (CB_activate), NULL );
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}

#endif
