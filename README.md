# GTK4-Smith-Chart
A GTK4 Drawing Area widget displaying a detailed Smith chart.

The chart is cusomizable and can be annotated with traces, lines and points.
In the drawing callback of the GtkDrawingArea widget, an options scructure 
may be supplied to alter the color and other features.

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
