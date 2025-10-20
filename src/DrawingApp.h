#pragma once

#include "CinderApp.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Color.h"
#include "cinder/params/Params.h"

#include "VectorDrawing.h"

#include <Windows.h>
#include <string>

using namespace std;
class DrawingApp : public CinderApp {
public:
    DrawingApp();
    virtual ~DrawingApp() = default;

    void setup() override;
    void mouseDown(ci::app::MouseEvent event) override;
    void mouseDrag(ci::app::MouseEvent event) override;
    void mouseUp(ci::app::MouseEvent event) override;
    void keyDown(ci::app::KeyEvent event) override;
    void update() override;
    void draw() override;
    void resize() override;

//    static void createConsole();

    static void createConsole() {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }

    ci::gl::TextureRef convertTransparentFboToSolidTexture(const ci::gl::FboRef& transparentFbo,
        const ci::ColorA& backgroundColor = ci::ColorA(1, 1, 1, 1));


    ci::gl::TextureRef captureLatestStrokeAsTexture();

    // Create texture/surface from drawing
    ci::gl::TextureRef captureDrawingAsTexture(bool solid = false);
    ci::Surface8u captureDrawingAsSurface();
    void saveDrawingToDisk(const std::string& filename);

protected:
    // Core drawing functionality
    vdraw::Drawing drawing;

    // FBO for capturing drawing
    ci::gl::FboRef canvasFboTransparent;
    ci::gl::FboRef canvasFbo;

    bool isMouseDown;

    // UI parameters
    ci::Color currentColor;
    float strokeWidth;
    int smoothingLevel;
    bool dynamicWidth;

    bool showDrawing;

    ci::params::InterfaceGlRef params;

    // Helper methods
    double getCurrentTime();
    virtual void renderStroke(const vdraw::Stroke& stroke);
    virtual void resetCanvas();

    // Initialize UI parameters
    virtual void setupParams();

    // Handle key input
    virtual void handleColorKeys(ci::app::KeyEvent event);
    virtual void handleStrokeKeys(ci::app::KeyEvent event);
    virtual void handleUndoRedoKeys(ci::app::KeyEvent event);
};