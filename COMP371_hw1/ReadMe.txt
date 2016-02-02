###############################################################################

						 COMP 371 COMPUTER GRAPHICS

								ASSIGNMENT 1

###############################################################################

AUTHOR: Michael Deom
STUDENT ID: 29549641
DATE: February 2nd, 2016

OVERVIEW
--------
The application reads a text file "input_a1.txt" consisting of an integer
(number of spans), followed by another integer (number of profile polyline
vertices), followed by that number of lines consisting of three floats each.
Each line is the three coordinates of a profile line vertex.

If the number of spans was 0, the sweep will be a translational sweep, so the
file must then contain another integer (number of trajectory polyline vertices)
followed by the coordinates of the trajectory polyline vertices, in the same
format as for the profile polyline vertices above.

If the number of spans was positive, the sweep will be rotational.

CONTROLS
--------
-	Hold the left mouse button while moving the mouse forward or backward to
	move the camera respectively.
-	Use the left and right arrow keys to rotate the camera around the z-axis.
-	Use the up and down arrow keys to rotate the camera around the x-axis.
-	To change the display mode:
	-	T: triangles
	-	W: wireframe
	-	P: points
