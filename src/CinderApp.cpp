#include "CinderApp.h"

void CinderApp::setup()
{
	CinderConsole::create();
}

void CinderApp::mouseDown( MouseEvent event )
{
}

void CinderApp::update()
{
}

void CinderApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

void CinderApp::keyDown(KeyEvent event)
{
	switch (event.getCode())
	{
		case 27: exit(0); break;
	}

}