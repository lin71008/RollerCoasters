/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <Fl/fl.h>
#include <time.h>
#include <cstdlib>
#include <math.h>


// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
#include "GL/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"
#include "Utilities/Pnt3f.H"

#include "DEBUG.h"


// #ifdef EXAMPLE_SOLUTION
// #	include "TrainExample/TrainExample.H"
// #endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );
	this->selectedCube = -1;
	this->seed = (unsigned) time(NULL);
	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				if (k == 's') {
					printf("Original Speed (%.2lfx): %lf\n", this->tw->speed->value(), this->tw->origional_speed);
					printf("Physics Effected Speed: %lf\n", this->tw->physics_effected_speed);
				}
				break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	// if (gladLoadGL())
	// {
	// 	//initiailize VAO, VBO, Shader...
	// }
	// else
	// 	throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	GLfloat lightPosition1[]	= {0,1,1,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[]	= {1, 0, 0, 0};
	GLfloat lightPosition3[]	= {0, -1, 0, 0};
	GLfloat yellowLight[] = {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[] = {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[] = {.1f,.1f,.3f,1.0};
	GLfloat grayLight[] = {.3f, .3f, .3f, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
	glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);



	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	// glUseProgram(0);

	setupFloor();
	glDisable(GL_LIGHTING);
	drawFloor(200,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
}

void TrainView::
getCurvesPoint(const float t, Pnt3f* pos, Pnt3f* dir, Pnt3f* up)
{
	int i = (int) fmod(t, this->tw->m_Track.points.size());
	const float p = t - i;

	const int s0 = i > 0 ?
				(i - 1) % this->tw->m_Track.points.size() : this->tw->m_Track.points.size() - 1;
	const int s1 = (i + 0) % this->tw->m_Track.points.size();
	const int s2 = (i + 1) % this->tw->m_Track.points.size();
	const int s3 = (i + 2) % this->tw->m_Track.points.size();

	const ControlPoint* p0 = &(this->m_pTrack->points[s0]);
	const ControlPoint* p1 = &(this->m_pTrack->points[s1]);
	const ControlPoint* p2 = &(this->m_pTrack->points[s2]);
	const ControlPoint* p3 = &(this->m_pTrack->points[s3]);

	static const float mat44_Carriental[16] = {
		-1 / 2.0,  3 / 2.0, -3 / 2.0,  1 / 2.0,
		 2 / 2.0, -5 / 2.0,  4 / 2.0, -1 / 2.0,
		-1 / 2.0,  0 / 2.0,  1 / 2.0,  0 / 2.0,
		 0 / 2.0,  2 / 2.0,  0 / 2.0,  0 / 2.0
	};
	static const glm::mat4x4 Carriental = glm::make_mat4x4(mat44_Carriental);

	static const float mat44_B_Spline[16] = {
		-1 / 6.0,  3 / 6.0, -3 / 6.0,  1 / 6.0,
		 3 / 6.0, -6 / 6.0,  3 / 6.0,  0 / 6.0,
		-3 / 6.0,  0 / 6.0,  3 / 6.0,  0 / 6.0,
		 1 / 6.0,  4 / 6.0,  1 / 6.0,  0 / 6.0
	};
	static const glm::mat4x4 B_Spline = glm::make_mat4x4(mat44_B_Spline);

	const float mat43_G[12] = {
		p0->pos.x, p0->pos.y, p0->pos.z,
		p1->pos.x, p1->pos.y, p1->pos.z,
		p2->pos.x, p2->pos.y, p2->pos.z,
		p3->pos.x, p3->pos.y, p3->pos.z
	};
	const glm::mat4x3 G = glm::make_mat4x3(mat43_G);

	const float mat43_O[12] = {
		p0->orient.x, p0->orient.y, p0->orient.z,
		p1->orient.x, p1->orient.y, p1->orient.z,
		p2->orient.x, p2->orient.y, p2->orient.z,
		p3->orient.x, p3->orient.y, p3->orient.z
	};
	const glm::mat4x3 O = glm::make_mat4x3(mat43_O);

	const float vec4_T[4] = {
		(float) std::pow(p, 3),
		(float) std::pow(p, 2),
		(float) std::pow(p, 1),
		(float) std::pow(p, 0)
	};
	const glm::vec4 T = glm::make_vec4(vec4_T);

	const float vec4_dT[4] = {
		(float) (3.0 * std::pow(p, 2)),
		(float) (2.0 * std::pow(p, 1)),
		(float) (1.0 * std::pow(p, 0)),
		(float) (0.0 * std::pow(p, 0))
	};
	const glm::vec4 dT = glm::make_vec4(vec4_dT);

	if (this->tw->splineBrowser->selected(1))
	{
		if (pos != NULL)
		{
			(*pos) = (1 - p) * p1->pos + p * p2->pos;
		}
		if (dir != NULL)
		{
			(*dir) = (p2->pos + -1.0 * p1->pos);
			(*dir).normalize();
		}
		if (up != NULL)
		{
			(*up) = (1 - p) * p1->orient + p * p2->orient;
			(*up).normalize();
		}
	}
	else if (this->tw->splineBrowser->selected(2))
	{
		const glm::vec3 Q = G * Carriental * T;
		const glm::vec3 dQ = G * Carriental * dT;
		const glm::vec3 Ot = O * Carriental * T;

		if (pos != NULL)
		{
			(*pos).x = Q.x;
			(*pos).y = Q.y;
			(*pos).z = Q.z;
		}
		if (dir != NULL)
		{
			(*dir).x = dQ.x;
			(*dir).y = dQ.y;
			(*dir).z = dQ.z;
			(*dir).normalize();
		}
		if (up != NULL)
		{
			(*up).x = Ot.x;
			(*up).y = Ot.y;
			(*up).z = Ot.z;
			(*up).normalize();
		}
	}
	else if (this->tw->splineBrowser->selected(3))
	{
		const glm::vec3 Q = G * B_Spline * T;
		const glm::vec3 dQ = G * B_Spline * dT;
		const glm::vec3 Ot = O * B_Spline * T;

		if (pos != NULL)
		{
			(*pos).x = Q.x;
			(*pos).y = Q.y;
			(*pos).z = Q.z;
		}
		if (dir != NULL)
		{
			(*dir).x = dQ.x;
			(*dir).y = dQ.y;
			(*dir).z = dQ.z;
			(*dir).normalize();
		}
		if (up != NULL)
		{
			(*up).x = Ot.x;
			(*up).y = Ot.y;
			(*up).z = Ot.z;
			(*up).normalize();
		}
	}
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(70, aspect, 0.1, 1000);

		Pnt3f pos, dir, up;
		getCurvesPoint(this->tw->m_Track.trainU, &pos, &dir, &up);
		pos = pos + (up * Train_Height * 0.5) + (dir * Train_Length * 0.5);
		dir = pos + dir;

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(pos.x, pos.y, pos.z, dir.x, dir.y, dir.z, up.x, up.y, up.z);
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

// #ifdef EXAMPLE_SOLUTION
	drawTrack(doingShadows);
// #endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
// #ifdef EXAMPLE_SOLUTION
// 	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(doingShadows);
// #endif
	// DEBUG_INFO("%d\n", tw->trainCam->value());

	drawOthers(doingShadows);
}

// x-> u, y -> v
static inline void drawOwO(Pnt3f np, Pnt3f nu, Pnt3f nv, Pnt3f fp, Pnt3f fu, Pnt3f fv,
					  float hw, float hh, bool dnfs)
{
	glPushMatrix();
	glEnable(GL_NORMALIZE);
	glBegin(GL_QUADS);

	glNormal3f(nu.y * (fp.z - np.z + hh * (fv.z - nv.z)) - nu.z * (fp.y - np.y + hh * (fv.y - nv.y)),
			 nu.z * (fp.x - np.x + hh * (fv.x - nv.x)) - nu.x * (fp.z - np.z + hh * (fv.z - nv.z)),
			 nu.x * (fp.y - np.y + hh * (fv.y - nv.y)) - nu.y * (fp.x - np.x + hh * (fv.x - nv.x)));
	glVertex3f(np.x - hw * nu.x + hh * nv.x, np.y - hw * nu.y + hh * nv.y, np.z - hw * nu.z + hh * nv.z);
	glVertex3f(fp.x - hw * fu.x + hh * fv.x, fp.y - hw * fu.y + hh * fv.y, fp.z - hw * fu.z + hh * fv.z);
	glVertex3f(fp.x + hw * fu.x + hh * fv.x, fp.y + hw * fu.y + hh * fv.y, fp.z + hw * fu.z + hh * fv.z);
	glVertex3f(np.x + hw * nu.x + hh * nv.x, np.y + hw * nu.y + hh * nv.y, np.z + hw * nu.z + hh * nv.z);

	glNormal3f(nu.z * (fp.y - np.y - hh * (fv.y - nv.y) - nu.y * (fp.z - np.z - hh * (fv.z - nv.z))),
			 nu.x * (fp.z - np.z - hh * (fv.z - nv.z) - nu.z * (fp.x - np.x - hh * (fv.x - nv.x))),
			 nu.y * (fp.x - np.x - hh * (fv.x - nv.x) - nu.x * (fp.y - np.y - hh * (fv.y - nv.y))));
	glVertex3f(np.x - hw * nu.x - hh * nv.x, np.y - hw * nu.y - hh * nv.y, np.z - hw * nu.z - hh * nv.z);
	glVertex3f(np.x + hw * nu.x - hh * nv.x, np.y + hw * nu.y - hh * nv.y, np.z + hw * nu.z - hh * nv.z);
	glVertex3f(fp.x + hw * fu.x - hh * fv.x, fp.y + hw * fu.y - hh * fv.y, fp.z + hw * fu.z - hh * fv.z);
	glVertex3f(fp.x - hw * fu.x - hh * fv.x, fp.y - hw * fu.y - hh * fv.y, fp.z - hw * fu.z - hh * fv.z);
	
	glNormal3f((fp.y - np.y - hw * (fv.y - nu.y)) * nv.z - (fp.z - np.z - hw * (fv.z - nu.z)) * nv.y,
			 (fp.z - np.z - hw * (fv.z - nu.z)) * nv.x - (fp.x - np.x - hw * (fv.x - nu.x)) * nv.z,
			 (fp.x - np.x - hw * (fv.x - nu.x)) * nv.y - (fp.y - np.y - hw * (fv.y - nu.y)) * nv.x);
	glVertex3f(np.x - hw * nu.x - hh * nv.x, np.y - hw * nu.y - hh * nv.y, np.z - hw * nu.z - hh * nv.z);
	glVertex3f(fp.x - hw * fu.x - hh * fv.x, fp.y - hw * fu.y - hh * fv.y, fp.z - hw * fu.z - hh * fv.z);
	glVertex3f(fp.x - hw * fu.x + hh * fv.x, fp.y - hw * fu.y + hh * fv.y, fp.z - hw * fu.z + hh * fv.z);
	glVertex3f(np.x - hw * nu.x + hh * nv.x, np.y - hw * nu.y + hh * nv.y, np.z - hw * nu.z + hh * nv.z);
	
	glNormal3f((fp.y - np.y + hw * (fv.y - nu.y)) * -nv.z - (fp.z - np.z + hw * (fv.z - nu.z)) * -nv.y,
			 (fp.z - np.z + hw * (fv.z - nu.z)) * -nv.x - (fp.x - np.x + hw * (fv.x - nu.x)) * -nv.z,
			 (fp.x - np.x + hw * (fv.x - nu.x)) * -nv.y - (fp.y - np.y + hw * (fv.y - nu.y)) * -nv.x);
	glVertex3f(np.x + hw * nu.x + hh * nv.x, np.y + hw * nu.y + hh * nv.y, np.z + hw * nu.z + hh * nv.z);
	glVertex3f(fp.x + hw * fu.x + hh * fv.x, fp.y + hw * fu.y + hh * fv.y, fp.z + hw * fu.z + hh * fv.z);
	glVertex3f(fp.x + hw * fu.x - hh * fv.x, fp.y + hw * fu.y - hh * fv.y, fp.z + hw * fu.z - hh * fv.z);
	glVertex3f(np.x + hw * nu.x - hh * nv.x, np.y + hw * nu.y - hh * nv.y, np.z + hw * nu.z - hh * nv.z);
	
	if (dnfs)
	{
		glNormal3f(np.x - fp.x, np.y - fp.y, np.z - fp.z);
		glVertex3f(np.x - hw * nu.x - hh * nv.x, np.y - hw * nu.y - hh * nv.y, np.z - hw * nu.z - hh * nv.z);
		glVertex3f(np.x - hw * nu.x + hh * nv.x, np.y - hw * nu.y + hh * nv.y, np.z - hw * nu.z + hh * nv.z);
		glVertex3f(np.x + hw * nu.x + hh * nv.x, np.y + hw * nu.y + hh * nv.y, np.z + hw * nu.z + hh * nv.z);
		glVertex3f(np.x + hw * nu.x - hh * nv.x, np.y + hw * nu.y - hh * nv.y, np.z + hw * nu.z - hh * nv.z);	
	
		glNormal3f(fp.x - np.x, fp.y - np.y, fp.z - np.z);
		glVertex3f(fp.x - hw * fu.x - hh * fv.x, fp.y - hw * fu.y - hh * fv.y, fp.z - hw * fu.z - hh * fv.z);
		glVertex3f(fp.x + hw * fu.x - hh * fv.x, fp.y + hw * fu.y - hh * fv.y, fp.z + hw * fu.z - hh * fv.z);
		glVertex3f(fp.x + hw * fu.x + hh * fv.x, fp.y + hw * fu.y + hh * fv.y, fp.z + hw * fu.z + hh * fv.z);
		glVertex3f(fp.x - hw * fu.x + hh * fv.x, fp.y - hw * fu.y + hh * fv.y, fp.z - hw * fu.z + hh * fv.z);	
	}
	
	glEnd();
	glPopMatrix();
}

void TrainView::
drawTrack(bool doingShadows)
{
	Pnt3f pos, pos_next;
	Pnt3f dir, dir_next;
	Pnt3f up, up_next;
	Pnt3f cross, cross_next;
	Pnt3f on, on_next;

	Pnt3f p0, p1;

	float l = 0.0;

	for (int i = 0; i < this->m_pTrack->points.size(); i++)
	{
		for (int j = 0; j < N_dT; ++j)
		{
			const float t0 = fmod(i + (j + 0.0) / N_dT, this->m_pTrack->points.size());
			const float t1 = fmod(i + (j + 1.0) / N_dT, this->m_pTrack->points.size());

			getCurvesPoint(t0, &pos, &dir, &up);
			getCurvesPoint(t1, &pos_next, &dir_next, &up_next);

			cross = dir * up;
			cross.normalize();

			cross_next = dir_next * up_next;
			cross_next.normalize();

			on = cross * dir;
			on.normalize();

			on_next = cross_next * dir_next;
			on_next.normalize();

			// track

			if (!doingShadows)
			{
				const float p = (i + ((float) j) / N_dT) / this->m_pTrack->points.size();
				const float r = 0.0 / 3.0 <= p && p <= 2.0 / 3.0 ? 255.0 * 3.0 * (1.0 / 3.0 - abs(1.0 / 3.0 - p)) : 0.0;
				const float g = 1.0 / 3.0 <= p && p <= 3.0 / 3.0 ? 255.0 * 3.0 * (1.0 / 3.0 - abs(2.0 / 3.0 - p)) : 0.0;
				const float b = 2.0 / 3.0 <= p || p <= 1.0 / 3.0 ? 255.0 * 3.0 * (1.0 / 6.0 - abs(1.0 / 2.0 - p)) : 0.0;
				glColor3ub(r, g, b);
			}

			// left hand side
			p0 = pos + on * -(Track_Height / 2.0) + cross * -(Track_Gauge / 2.0);
			p1 = pos_next + on_next * -(Track_Height / 2.0) + cross_next * -(Track_Gauge / 2.0);
			drawOwO(p0, cross, on, p1, cross_next, on_next, Track_Width / 2.0, Track_Height / 2.0, false);

			// right hand side
			p0 = pos + on * -(Track_Height / 2.0) + cross * (Track_Gauge / 2.0);
			p1 = pos_next + on_next * -(Track_Height / 2.0) + cross_next * (Track_Gauge / 2.0);
			drawOwO(p0, cross, on, p1, cross_next, on_next, Track_Width / 2.0, Track_Height / 2.0, false);

			// cross-tie

			if (!doingShadows)
			{
				glColor3ub(90, 50, 0);
			}

			l += sqrt(pow(pos_next.x - pos.x, 2) + pow(pos_next.y - pos.y, 2) + pow(pos_next.z - pos.z, 2));

			if ((this->tw->arcLength->value() == 1 && l >= Crosstie_Spacing) || (this->tw->arcLength->value() == 0 && j % (N_dT / 10) == 0))
			{
				p0 = pos + on * -(Track_Height + Crosstie_Height / 2.0) + dir * -(Crosstie_Width / 2.0);
				p1 = pos + on * -(Track_Height + Crosstie_Height / 2.0) + dir * (Crosstie_Width / 2.0);
				drawOwO(p0, cross, on, p1, cross, on, Crosstie_Lenght / 2.0, Crosstie_Height / 2.0, true);
				l -= Crosstie_Spacing;
			}
		}
	}
}

void TrainView::
drawTrain(bool doingShadows)
{
	Pnt3f pos, pos_next, dir, up;
	Pnt3f cross, on, p0, p1;

	float l = 0.0;

	for (int k = 0, i = 0; k < this->tw->train_amount && i < this->m_pTrack->points.size(); i++)
	{
		for (int j = 0; k < this->tw->train_amount && j < N_dT; ++j)
		{
			const float t0 = fmod(this->tw->m_Track.trainU + this->m_pTrack->points.size() - 1.0 - i + (N_dT - j - 0.0) / N_dT, this->m_pTrack->points.size());
			const float t1 = fmod(this->tw->m_Track.trainU + this->m_pTrack->points.size() - 1.0 - i + (N_dT - j - 1.0) / N_dT, this->m_pTrack->points.size());
			
			getCurvesPoint(t0, &pos, &dir, &up);
			getCurvesPoint(t1, &pos_next, NULL, NULL);

			if (l >= 0.0)
			{
				this->tw->train_position[k] = t0;

				cross = dir * up;
				cross.normalize();

				on = cross * dir;
				on.normalize();

				if (!doingShadows)
				{
					glColor3ub(160, 120, 0);
				}

				p0 = pos + dir * -(Train_Length / 2.0) + on * (Train_Height / 2.0);
				p1 = pos + dir * (Train_Length / 2.0) + on * (Train_Height / 2.0);
				drawOwO(p0, cross, on, p1, cross, on, Train_Width / 2.0, Train_Height / 2.0, true);

				l -= Train_Length + Train_Gap;
				k++;
			}
			l += sqrt(pow(pos_next.x - pos.x, 2) + pow(pos_next.y - pos.y, 2) + pow(pos_next.z - pos.z, 2));
		}
	}
}

void TrainView::
drawOthers(bool doingShadows)
{
	srand( this->seed );

	unsigned stone_amount = 16 + rand() % 32;

	for (int i = 0; i < stone_amount; ++i)
	{
		if (!doingShadows)
		{
			glColor3ub(80, 80, 80);
		}

		float x = 100.0 - rand() % 200 + 0.01 * (rand() % 100);
		float z = 100.0 - rand() % 200 + 0.01 * (rand() % 100);
		
		float w = 1.0 + 0.1 * (rand() % 50);
		float h = 0.5 + 0.1 * (rand() % 30);
		float r = (rand() % 360) * M_PI / 180.0;

		Pnt3f u, v, pos(x, 0.0, z), dir(0.0, 1.0, 0.0), _x(1.0, 0.0, 0.0), _z(0.0, 0.0, 1.0);

		u = cos(r) * _x + sin(-r) * _z;
		v = sin(r) * _x + cos(r) * _z;

		drawOwO(pos, u, v, pos + dir * h, u * 0.8, v * 0.8, w, w, true);
	}

	unsigned tree_amount = 4 + rand() % 8;

	for (int i = 0; i < tree_amount; ++i)
	{
		float x = 100.0 - rand() % 200 + 0.01 * (rand() % 100);
		float z = 100.0 - rand() % 200 + 0.01 * (rand() % 100);
		
		float w0 = 2.0 + 0.1 * (rand() % 20);
		float h0 = 4.0 + 0.2 * (rand() % 40);
		float r = (rand() % 360) * M_PI / 180.0;
		unsigned n = 2 + rand() % 5;

		Pnt3f u, v, pos(x, 0.0, z), dir(0.0, 1.0, 0.0), _x(1.0, 0.0, 0.0), _z(0.0, 0.0, 1.0);

		u = cos(r) * _x + sin(-r) * _z;
		v = sin(r) * _x + cos(r) * _z;

		if (!doingShadows)
		{
			glColor3ub(100, 70, 0);
		}

		drawOwO(pos, u, v, pos + dir * h0, u, v, w0, w0, true);

		if (!doingShadows)
		{
			glColor3ub(0, 80, 0);
		}

		for (int j = 0; j < n; ++j)
		{
			drawOwO(pos + dir * (h0 * (1.0 + j * 3.0 / n)), u, v, pos + dir * (h0 * (1.0 + (j + 1) * 3.0 / n)), u * (1.0 - ((float) j) / n), v * (1.0 - ((float) j) / n), 2.0 * w0, 2.0 * w0, true);
		}
		// drawOwO(pos + dir * h0, u, v, pos + dir * (h0 + 2), u * 0.0, v * 0.0, 2.0 * w0, 2.0 * w0, true);
	}
}
// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	// DEBUG_INFO("Selected Cube %d\n",selectedCube);
}