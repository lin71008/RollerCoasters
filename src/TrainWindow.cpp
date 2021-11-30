/************************************************************************
     File:        TrainWindow.H

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						this class defines the window in which the project 
						runs - its the outer windows that contain all of 
						the widgets, including the "TrainView" which has the 
						actual OpenGL window in which the train is drawn

						You might want to modify this class to add new widgets
						for controlling	your train

						This takes care of lots of things - including installing 
						itself into the FlTk "idle" loop so that we get periodic 
						updates (if we're running the train).


     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <FL/fl.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Dial.h>

// for using the real time clock
#include <time.h>
#include <math.h>

#include "TrainWindow.H"
#include "TrainView.H"
#include "CallBacks.H"
#include "DEBUG.h"


//************************************************************************
//
// * Constructor
//========================================================================
TrainWindow::
TrainWindow(const int x, const int y) 
	: Fl_Double_Window(x,y,800,600,"Train and Roller Coaster")
//========================================================================
{
	// make all of the widgets
	begin();	// add to this widget
	{
		int pty=5;			// where the last widgets were drawn

		trainView = new TrainView(5,5,590,590);
		trainView->tw = this;
		trainView->m_pTrack = &m_Track;
		this->resizable(trainView);

		// to make resizing work better, put all the widgets in a group
		widgets = new Fl_Group(600,5,200,590);
		widgets->begin();

		// camera buttons - in a radio button group
		Fl_Group* camGroup = new Fl_Group(600,pty,200,20);
		camGroup->begin();
		worldCam = new Fl_Button(605, pty, 60, 20, "World");
		worldCam->type(FL_RADIO_BUTTON);		// radio button
		worldCam->value(1);			// turned on
		worldCam->selection_color((Fl_Color)3); // yellow when pressed
		worldCam->callback((Fl_Callback*)damageCB,this);
		trainCam = new Fl_Button(670, pty, 60, 20, "Train");
		trainCam->type(FL_RADIO_BUTTON);
		trainCam->value(0);
		trainCam->selection_color((Fl_Color)3);
		trainCam->callback((Fl_Callback*)damageCB,this);
		topCam = new Fl_Button(735, pty, 60, 20, "Top");
		topCam->type(FL_RADIO_BUTTON);
		topCam->value(0);
		topCam->selection_color((Fl_Color)3);
		topCam->callback((Fl_Callback*)damageCB,this);
		camGroup->end();

		pty += 30;

		runButton = new Fl_Button(605,pty,60,20,"Run");
		togglify(runButton);
		Fl_Button* fb = new Fl_Button(703,pty,27,20,"@>>");
		fb->callback((Fl_Callback*)forwCB,this);
		Fl_Button* rb = new Fl_Button(670,pty,27,20,"@<<");
		rb->callback((Fl_Callback*)backCB,this);		
		physics = new Fl_Button(735,pty,60,20,"Physics");
		togglify(physics,1);
  
		pty+=25;

		Fl_Button* sub_train = new Fl_Button(605, pty, 60, 20, "-");
		sub_train->callback((Fl_Callback*)sub_trainCB,this);		
		trainBox = new Fl_Box(670, pty, 55, 20, "1");
		Fl_Button* add_train = new Fl_Button(735, pty, 60, 20, "+");
		add_train->callback((Fl_Callback*)add_trainCB,this);		
		train_amount = 1;

		pty+=25;

		speed = new Fl_Value_Slider(650,pty,145,20,"speed");
		speed->range(0,10);
		speed->value(2);
		speed->align(FL_ALIGN_LEFT);
		speed->type(FL_HORIZONTAL);

		pty += 40;

		arcLength = new Fl_Button(605,pty,190,20,"ArcLength");
		togglify(arcLength,1);

		pty += 25;

		// browser to select spline types
		// TODO: make sure these choices are the same as what the code supports
		splineBrowser = new Fl_Browser(605,pty,190,75,"Spline Type");
		splineBrowser->type(2);		// select
		splineBrowser->callback((Fl_Callback*)damageCB,this);
		splineBrowser->add("Linear");
		splineBrowser->add("Cardinal Cubic");
		splineBrowser->add("Cubic B-Spline");
		splineBrowser->select(2);

		pty += 105;

		// reset the points
		resetButton = new Fl_Button(735,pty,60,20,"Reset");
		resetButton->callback((Fl_Callback*)resetCB,this);
		Fl_Button* loadb = new Fl_Button(605,pty,60,20,"Load");
		loadb->callback((Fl_Callback*) loadCB, this);
		Fl_Button* saveb = new Fl_Button(670,pty,60,20,"Save");
		saveb->callback((Fl_Callback*) saveCB, this);

		pty += 30;

		// add and delete points
		Fl_Button* ap = new Fl_Button(605,pty,90,20,"Add Point");
		ap->callback((Fl_Callback*)addPointCB,this);
		Fl_Button* dp = new Fl_Button(705,pty,90,20,"Delete Point");
		dp->callback((Fl_Callback*)deletePointCB,this);

		pty += 30;

		// roll the points

		// move
		Fl_Button* mxp = new Fl_Button(605, pty, 60, 20, "mX+");
		mxp->callback((Fl_Callback*)mxpCB,this);
		Fl_Button* myp = new Fl_Button(670, pty, 60, 20, "mY+");
		myp->callback((Fl_Callback*)mypCB,this);
		Fl_Button* mzp = new Fl_Button(735, pty, 60, 20, "mZ+");
		mzp->callback((Fl_Callback*)mzpCB,this);
		
		pty += 25;

		Fl_Button* mxn = new Fl_Button(605, pty, 60, 20, "mX-");
		mxn->callback((Fl_Callback*)mxnCB,this);
		Fl_Button* myn = new Fl_Button(670, pty, 60, 20, "mY-");
		myn->callback((Fl_Callback*)mynCB,this);
		Fl_Button* mzn = new Fl_Button(735, pty, 60, 20, "mZ-");
		mzn->callback((Fl_Callback*)mznCB,this);
		
		pty += 25;

		// rotate
		Fl_Button* rxp = new Fl_Button(605, pty, 90, 20, "rX+");
		rxp->callback((Fl_Callback*)rxpCB,this);
		Fl_Button* rzp = new Fl_Button(705, pty, 90, 20, "rZ+");
		rzp->callback((Fl_Callback*)rzpCB,this);
		
		pty += 25;

		Fl_Button* rxn = new Fl_Button(605, pty, 90, 20, "rX-");
		rxn->callback((Fl_Callback*)rxnCB,this);
		Fl_Button* rzn = new Fl_Button(705, pty, 90, 20, "rZ-");
		rzn->callback((Fl_Callback*)rznCB,this);

		pty+=40;

		Fl_Button* rng = new Fl_Button(605, pty, 190, 20, "Randomization");
		rng->callback((Fl_Callback*)rngCB,this);

		// TODO: add widgets for all of your fancier features here
// #ifdef EXAMPLE_SOLUTION
// 		makeExampleWidgets(this,pty);
// #endif

		// we need to make a little phantom widget to have things resize correctly
		Fl_Box* resizebox = new Fl_Box(600,pty,200,590);
		widgets->resizable(resizebox);

		widgets->end();
	}
	end();	// done adding to this widget

	// set up callback on idle
	Fl::add_idle((void (*)(void*))runButtonCB,this);
}

//************************************************************************
//
// * handy utility to make a button into a toggle
//========================================================================
void TrainWindow::
togglify(Fl_Button* b, int val)
//========================================================================
{
	b->type(FL_TOGGLE_BUTTON);		// toggle
	b->value(val);		// turned off
	b->selection_color((Fl_Color)3); // yellow when pressed	
	b->callback((Fl_Callback*)damageCB,this);
}

//************************************************************************
//
// *
//========================================================================
void TrainWindow::
damageMe()
//========================================================================
{
	if (trainView->selectedCube >= ((int)m_Track.points.size()))
		trainView->selectedCube = 0;
	trainView->damage(1);
}

//************************************************************************
//
// * This will get called (approximately) 30 times per second
//   if the run button is pressed
//========================================================================
void TrainWindow::
advanceTrain(float dir)
//========================================================================
{
	const float x = this->m_Track.trainU;
	this->origional_speed = dir * ((float)speed->value() * .1f);
	Pnt3f pos, dir2, up, pos_next;

	this->physics_effected_speed = 0.0;	
	if (this->physics->value())
	{
		for (int i = 0; i < this->train_amount; ++i)
		{
			this->trainView->getCurvesPoint(this->train_position[i], NULL, &dir2, &up);
			this->physics_effected_speed += Train_Weight * dir2.y * up.y * -9.8;
		}
		this->physics_effected_speed /= this->train_amount;
	}

	float s = this->origional_speed + this->physics_effected_speed;
	s = fmod(this->m_Track.points.size() + s, this->m_Track.points.size());
	if (abs(s) < Min_Speed)
	{
		s = Min_Speed * pow(-1.0, signbit(dir));
	}
	if (abs(s) > Max_Speed)
	{
		s = Max_Speed * pow(-1.0, signbit(dir));
	}

	if (arcLength->value())
	{
		float l = 0.0;
		for (int i = 0; l <= s && i < this->trainView->m_pTrack->points.size(); i++)
		{
			for (int j = 0; l <= s * 75 && j < N_dT; ++j)
			{
				const float t0 = fmod(i + x + (j + 0.0) / N_dT, this->trainView->m_pTrack->points.size());
				const float t1 = fmod(i + x + (j + 1.0) / N_dT, this->trainView->m_pTrack->points.size());
				this->trainView->getCurvesPoint(t0, &pos, NULL, NULL);
				this->trainView->getCurvesPoint(t1, &pos_next, NULL, NULL);
				l += sqrt(pow(pos_next.x - pos.x, 2) + pow(pos_next.y - pos.y, 2) + pow(pos_next.z - pos.z, 2));
				this->m_Track.trainU += 1.0 / N_dT;
			}
		}
	}
	else
	{
		this->m_Track.trainU += s;
	}

	this->m_Track.trainU = fmod(this->m_Track.points.size() + this->m_Track.trainU, this->m_Track.points.size());
}