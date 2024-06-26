/************************************************************************
     File:        CallBacks.H

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu
     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     Header file to define callback functions.
						define the callbacks for the TrainWindow

						these are little functions that get called when the 
						various widgets
						get accessed (or the fltk timer ticks). these 
						functions are used 
						when TrainWindow sets itself up.

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/
#pragma once

// Preclarify to prevent the compiler error
class TrainWindow;
class Fl_Widget;

// Reset the train window
void resetCB(Fl_Widget*, TrainWindow* tw);
// Something change and thus we need to update the view
void damageCB(Fl_Widget*, TrainWindow* tw);

// Callback that adds a new point to the spline
// idea: add the point AFTER the selected point
void addPointCB(Fl_Widget*, TrainWindow* tw);


// Callback that deletes a point from the spline
void deletePointCB(Fl_Widget*, TrainWindow* tw);

// Callbacks for advancing/pulling back train
void forwCB(Fl_Widget*, TrainWindow* tw);
void backCB(Fl_Widget*, TrainWindow* tw);

// Idle callback: for run the step of the window
void runButtonCB(TrainWindow* tw);

// For load and save buttons
void loadCB(Fl_Widget*, TrainWindow* tw);
void saveCB(Fl_Widget*, TrainWindow* tw);

// roll the control points
// Move the selected control point about x axis by one more unit
void mxpCB(Fl_Widget*, TrainWindow* tw);
// Move the selected control point about x axis by one less unit
void mxnCB(Fl_Widget*, TrainWindow* tw);
// Move the selected control point about y axis by one more unit
void mypCB(Fl_Widget*, TrainWindow* tw);
// Move the selected control point about y axis by one less unit
void mynCB(Fl_Widget*, TrainWindow* tw);
// Move the selected control point about z axis by one more unit
void mzpCB(Fl_Widget*, TrainWindow* tw);
// Move the selected control point about z axis by one less unit
void mznCB(Fl_Widget*, TrainWindow* tw);
// Rotate the selected control point  about x axis by one more degree
void rxpCB(Fl_Widget*, TrainWindow* tw);
// Rotate the selected control point  about x axis by one less degree
void rxnCB(Fl_Widget*, TrainWindow* tw);
// Rotate the selected control point  about y axis by one more degree
void rypCB(Fl_Widget*, TrainWindow* tw);
// Rotate the selected control point  about y axis by one less degree
void rynCB(Fl_Widget*, TrainWindow* tw);
// Rotate the selected control point  about the z axis one more degree
void rzpCB(Fl_Widget*, TrainWindow* tw);
// Rotate the selected control point  about the z axis one less degree
void rznCB(Fl_Widget*, TrainWindow* tw);

// add one car to train
void add_trainCB(Fl_Widget*, TrainWindow *tw);
// remove one car from train
void sub_trainCB(Fl_Widget*, TrainWindow *tw);

// RNG
void rngCB(Fl_Widget*, TrainWindow *tw);