#include "AiDrawingApp.h"

void prepareSettings(app::App::Settings* settings) {

    int scaleFactor = 3;
    int length = scaleFactor * 512;

    settings->setWindowSize(length, length);
    settings->setTitle("Drawing App w/ AI-driven Visual Completion");
    settings->setResizable(true);
}

// This line tells Cinder to create and run the application
CINDER_APP(AiDrawingApp, ci::app::RendererGl, prepareSettings)
