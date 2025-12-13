# GTK4-Smith-Chart
A GTK4 Drawing Area widget displaying a detailed Smith chart.

The chart is cusomizable and can be annotated with traces, lines and points.
In the drawing callback of the GtkDrawingArea widget, an options scructure 
may be supplied to alter the color and other features.

Developed by Phillip H. Smith at Bell Telephone Laboratories in 1939,
the chart can be used to simultaneously display multiple parameters including impedances, admittances, reflection coefficients,
scattering parameters, noise figure circles, constant gain contours and regions for unconditional stability.

The code provided can be used to create an impedance (Z) Smith chart, a susceptace (Y) chart or one combining both.
To mimic the ZY-01-N chart published by Analog Instruments Company, a 'sparce' Y chart may be overlayed on a standard Z chart.

Routines are provided to plot lines, points, connected lines (from a series of cordinates in gamma space) or
a smooth bezier curve from a series of cordinates in gamma space.

The user incorporates one or more ```GtkDrawing``` widgets and connects the drawing callback to a routine
that creates the Smith chart and adds any curves or other annotations. Each ```GtkDrawing``` can have a
different set of options.

```
tSmithOptions options = {
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
```
Compile example with
```
$ gcc -o smith `pkg-config --cflags --libs gtk4` -lm GTKsmithChart.c exampleSmith.c
```

<img src="https://github.com/VK2BEA/GTK4-Smith-Chart/blob/main/Images/RX%2Bcurve.png" width="80%"/>
<img src="https://github.com/VK2BEA/GTK4-Smith-Chart/blob/main/Images/GB.png" width="80%"/>
<img src="https://github.com/VK2BEA/GTK4-Smith-Chart/blob/main/Images/dual.png" width="80%"/>
<img src="https://github.com/VK2BEA/GTK4-Smith-Chart/blob/main/Images/no-ring.png" width="80%"/>
