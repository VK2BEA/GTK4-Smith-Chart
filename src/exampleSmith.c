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
 * @file exampleSmith.c.c
 * @brief Example program using Smith chart GtkDrawingArea
 *
 * @author Michael G. Katzmann
 *
 * compile with:
 * $ gcc -o smith `pkg-config --cflags --libs gtk4` -lm GTKsmithChart.c exampleSmith.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "GTKsmithChart.h"

/*!     \brief  Callback of the GtkDrawing widget to draw
 *
 * Callback of the GtkDrawing widget to draw the Smith chart
 * and any other objects the user chooses on the drawing area.
 *
 * Each GtkDrawingArea widget will have its own callback to redraw the window.
 * Several GtkDrawingAreas may have Smith charts displaying different data
 * and with different options.
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
 * \param user_data         pass pointer to user data from the main application
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
 * \param user_data     pass pointer to user data from the main application
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

