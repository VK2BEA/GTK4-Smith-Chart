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

#ifndef GTKSMITHCHART_H_
#define GTKSMITHCHART_H_

#include <gtk/gtk.h>
#include <cairo.h>

typedef struct {
    struct {
        guint bShowRX      : 1;
        guint bShowGB      : 1;
        guint bShowLabels  : 1;
        guint bShowStrings : 1;
        guint bDrawRing    : 1;
        guint bSparceGB    : 1;
    } flags;

    gdouble lineWidth;  // as a percentage of the radius
    gdouble pointWidth; //

    GdkRGBA colorRXgrid, colorGBgrid,
            colorRXtext, colorGBtext, colorRing,
            colorLine, colorAnnotation;

    gchar   *annotationFont;
    gint    annotationFontSize; // as a percentage of the radius

    cairo_matrix_t matrix;
} tSmithOptions;

typedef struct {
    gdouble value;
    gchar   *text;
} tLabel;

typedef struct {
    gdouble region;
    gdouble minorDiv;
    gint    minorPerMajorDiv;
} tRegion;

typedef struct {
    gdouble U,V;
} tUV;

typedef struct {
    gdouble R, X;
} tRX;

typedef struct
{
    tUV A, B;
} tLine;

#define END (-1)
#define SPECIAL_CASE (0)

#define LABEL_FONT "Nimbus Sans"

// Set this to one and scale drawing area
// This makes drawing points and lines in Gamma space more straight forward.
#define SMITH_RADIUS 1.0
#define LABELFONTSIZE (SMITH_RADIUS/55.0)

#define STROKE_WIDTH_THIN  (SMITH_RADIUS / 2000) //0.01
#define STROKE_WIDTH_MINOR (SMITH_RADIUS / 1500) // 0.2
#define STROKE_WIDTH_MAJOR (SMITH_RADIUS / 500) // 0.5

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

#define SQU(x) ((x)*(x))
#define DEGtoRAD( x ) ( (x) / 180.0 * M_PI )

#define WAVE_RING_RADIUS  ( 1.115 * SMITH_RADIUS )
#define ANGLE_RING_RADIUS ( 1.038 * SMITH_RADIUS )
#define SRpct(x) ( SMITH_RADIUS / 100.0 * (x) )  // percentage of Smith gridradius

#define OUTER_BOUNDARY_WITH_RING (WAVE_RING_RADIUS + ((WAVE_RING_RADIUS - ANGLE_RING_RADIUS) / 2.0))

#define LEFT_JUSTIFIED  TRUE
#define RIGHT_JUSTIFIED FALSE

tUV RXtoUV( tRX );
void annotatePointOnSmithChart( cairo_t *, gchar *, tUV, gboolean, tSmithOptions * );
void drawSmithChart( cairo_t *, gdouble, gdouble, gdouble, tSmithOptions * );
void drawPointOnSmithChart( cairo_t *, tUV, tSmithOptions * );
void drawLineArrayOnSmithChart( cairo_t *, tUV [], gint, tSmithOptions * );
void drawBezierCurveOnSmithChart(cairo_t *, const tUV [], gint, tSmithOptions * );

#endif /* GTKSMITHCHART_H_ */
