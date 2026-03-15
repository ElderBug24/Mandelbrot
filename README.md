# Mandelbrot set explorer for M5Stack Cardputer

A Mandelbrot set (https://en.wikipedia.org/wiki/Mandelbrot_set) explorer / visualizer written in c++ for the M5Stack Cardputer (https://docs.m5stack.com/en/core/Cardputer).

The program gives plenty of options and modes to render a visual of the mandelbrot set the 'usual way', with each pixel corresponding to a point on the complexe plane and its color, how close it is evaluated to be in the Mandelbrot Set.
It also is able to show the Julia set (https://en.wikipedia.org/wiki/Julia_set) associated with the point at the center of the screen.

# Controls
The program allows the user to move around the camera on the set, zoom in and out, as well as toggle different calculation / coloring algorithms.

## Movement / basic features
 The orange arrows in the bottom right (',' '.' '/' ';') allow for the camera to move respectively left, down, right or up, by 0.166 / [the current zoom]
 - '+' ('=' on the keyboard) zooms in, doubling the zoom level
 - '-' ('_' on the keyboard) zooms out, halving the zoom level
   + Note that the zoom level will only be powers of two (negative or posisitve)
 - 'r' resets the zoom level to default
 - ' ' will block the render allowing the user to change the camera position / zoom level multiple times witout having to recompute the whole frame as this can be quite expensive.
   + Note that a green 'crosshair' will allow the user to visualize the new frame and zoom level, showing precisely what will be visible and what won't
   + Also if the zoom level display mode is on, the current zoom level as well as the quotient of the current zoom by the one before the render blocking mode was toggled on
 - Enter ('&#8629;' on the keyboard) will cancel the render blocking mode and go back the the previous position and zoom level. Will not do anything if the render blocking mode was not on.
 - Points: the numbers from 0 to 9 and the Esc key ('`') each will teleport the camera to a different point on the complex plane:
   - Esc -> (0, 0), the origin of the complex plane
   - 1 -> (-0.75, 0), which is the default point when starting the program, as we can see the whole set from there
   - 2 -> (-0.1528459638, 1.0396947861), a Misiurewicz point (https://en.wikipedia.org/wiki/Misiurewicz_point) where the whole Mandelbrot set repeats itself
   - 3 -> (-0.5937632322, -0.4961385429), another Misiurewicz point, that looks like a flower: Queen Anne’s Lace (Daucus carota)
   - 4 -> (-0.9943835735, 2997867465), another Misiurewicz point
   - 5 -> (-0.7981594801, 0.1794066280), again a Misiurewicz point, looks like the sun
   - 6 -> (0.26711215318732706159, -0.0043170512292552603), a spiral
   - 7 -> (-0.743517833, 0.127094578), the seahorse valley
   - 8 -> (-0.3438543964986979, -0.6112538802506510), another spiral
   - 9 -> (-1.7687788000474986560, 0.00173890994736915347), a julia set inside the Mandelbrot set (if you zoom far enough)
   - 0 -> (-1.0069455230908423981, 0.31240071852809647712), self similar for two iterations, then if you keep zooming you will see the Mandelbrot set with some kind of 'aura' (use modular coloring for better resuts)
## Different calculations / coloring
 - '[' and ']' respecively decrease and increase the amount of iterations used to calculate the image:
    more iterations result in a higher quality imagein the sense that the result will get more accurate results and will not include in the set points that should not be part of it, but has a direct impact on performance
 - 'd' toggles double floating point arithmetic, using different data type to get double the accuracy, at the cost of a high increase in calculation time
 - 'j' toggles the Julia mode, which shows the Julia set corresponding to the point at the center of the screen instead of the Mandelbrot set
   + Note that there are independants coordinates, zoom level, double precision status and iteration count in this mode
 - 'o' and 'p' respectively switch to the previous and next color palettes each of 255 colors calculated when the progam lauches for additional runtime performaces. There are 5 currently available palettes.
 - 'v' toggles that the points that were confirmed inside the Mandelbrot set are totally black (its accuracy nevertheless depends on the iteration count).
 - 's' toggles the smoothing algorithm, doesnt do much when zoomed in far enough
 - 'm' toggles the modular coloring algorithm, makes it so that each color in the color palette can be used more than once to show more details. This mode takes the priority over and completely disables the smoothing algorithm.
 - 'h' toggles the histogram coloring algorithm which equally distributes the colors, not including the black of the Mandelbrot set. This mode takes the priority over and completely disables the modular coloring algorithm.
## Head-up display
 - 'a' toggles the axis display, which shows the axis at the center of the screen as well as some graduations to show where one step right, left, up or down will take the camera
 - 'c' toggles the coordinates display (off / on / extended accuracy), which display the coordinates at the center of the screen
 - 'z' toggles the zoom display
 - 'i' toggles the iterations count display, that also shows if the smooting, modular coloring, histogram coloring or double precision modes are on
 - 'b' toggles the battery level display

A dot is sometimes displayed in the top right corner of the screen:
 - Red when the render is blocked
 - Green when the program is computing
 - White when the program is coloring the results

The program uses two threads to speed up calculations and this is the reason that two progress bars are displayed at the bottom: one for each thread and the computing time may differ because some parts of the screen will take different times to compute
