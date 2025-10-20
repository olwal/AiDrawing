#include "DrawingApp.h"
#include "cinder/Utilities.h"
#include "cinder/Timeline.h"
#include "cinder/ip/Flip.h"

using namespace ci;
using namespace ci::app;
using namespace std;

DrawingApp::DrawingApp()
    : currentColor(0, 0, 0),
    strokeWidth(10.0f),
    smoothingLevel(0),
    dynamicWidth(false), showDrawing(true), isMouseDown(false) {
}

void DrawingApp::setup() {
    // Setup the drawing with initial values
    drawing.setColor(vdraw::Color(currentColor.r, currentColor.g, currentColor.b));
    drawing.setStrokeWidth(strokeWidth);
    drawing.setSmoothing(smoothingLevel);
    drawing.setDynamicWidth(dynamicWidth);

    // Create FBO with same size as window
    auto windowSize = getWindowSize();
    gl::Fbo::Format fboFormat;
    fboFormat.colorTexture();  // Ensure color attachment is accessible as a texture
    fboFormat.setColorTextureFormat(gl::Texture2d::Format().internalFormat(GL_RGBA8));
    canvasFbo = gl::Fbo::create(windowSize.x, windowSize.y, fboFormat);

    // Initialize canvas with white background
    resetCanvas();

    // Setup parameter interface
    setupParams();

    // Set window title
    getWindow()->setTitle("Vector Drawing App");
}

void DrawingApp::setupParams() {
    params = params::InterfaceGl::create("Drawing Controls", ivec2(200, 200));
    params->addParam("Stroke Color", &currentColor)
        .updateFn([this]() {
        drawing.setColor(vdraw::Color(currentColor.r, currentColor.g, currentColor.b));
            });

    params->addParam("Stroke Width", &strokeWidth)
        .min(0.5f)
        .max(20.0f)
        .step(0.5f)
        .updateFn([this]() {
        drawing.setStrokeWidth(strokeWidth);
            });

    params->addParam("Smoothing", &smoothingLevel)
        .min(0)
        .max(10)
        .step(1)
        .updateFn([this]() {
        drawing.setSmoothing(smoothingLevel);
            });

    params->addParam("Dynamic Width", &dynamicWidth)
        .updateFn([this]() {
        drawing.setDynamicWidth(dynamicWidth);
            });
}

double DrawingApp::getCurrentTime() {
    return getElapsedSeconds();
}

void DrawingApp::mouseDown(MouseEvent event) {
    vec2 pos = event.getPos();
    float pressure = 1.0f; // Default pressure if not available

#if defined(CINDER_COCOA_TOUCH) //|| defined(CINDER_MSW_DESKTOP)
    // On platforms with pressure sensitivity
    if (event.getNative().isPressureSupported()) {
        pressure = event.getNative().getPressure();
    }
#endif

    drawing.beginStroke(vdraw::Vec2(pos.x, pos.y), pressure, getCurrentTime());

    isMouseDown = true;
}

void DrawingApp::mouseDrag(MouseEvent event) {
    vec2 pos = event.getPos();
    float pressure = 1.0f; // Default pressure if not available

#if defined(CINDER_COCOA_TOUCH) //|| defined(CINDER_MSW_DESKTOP)
    // On platforms with pressure sensitivity
    if (event.getNative().isPressureSupported()) {
        pressure = event.getNative().getPressure();
    }
#endif

    drawing.continueStroke(vdraw::Vec2(pos.x, pos.y), pressure, getCurrentTime());

    // Draw the active stroke (with safety check)
    const auto& strokes = drawing.getStrokes();
    if (!strokes.empty()) {
        gl::ScopedFramebuffer fbScp(canvasFbo);
        gl::ScopedViewport viewport(vec2(0), canvasFbo->getSize());
        gl::ScopedMatrices matrices;
        gl::setMatricesWindow(canvasFbo->getSize());

        // Only render the active stroke
        renderStroke(*strokes.back());

        // Reset color state to white
        gl::color(ColorA(1, 1, 1, 1));
    }
}


void DrawingApp::mouseUp(MouseEvent event) {
    drawing.endStroke();

    isMouseDown = false;
}

void DrawingApp::keyDown(KeyEvent event) {
    // Delegate to handler methods
    handleColorKeys(event);
    handleStrokeKeys(event);
    handleUndoRedoKeys(event);

    // Handle clear drawing
    if (event.getCode() == KeyEvent::KEY_DELETE ||
        event.getCode() == KeyEvent::KEY_BACKSPACE) {
        drawing.clearDrawing();
        resetCanvas();
    }

    if (event.getCode() == KeyEvent::KEY_p)
    {
        if (event.isShiftDown())
        {
            params->hide();
            cout << "hide" << endl;
        }
        else
            params->show();
    }

    // Save drawing as PNG file with 'S' key
    if (event.getCode() == KeyEvent::KEY_SPACE && (event.isControlDown() || event.isMetaDown())) {
        // Generate filename with timestamp
        auto timestamp = (int)time(nullptr);
        std::string filename = "drawing_" + std::to_string(timestamp) + ".png";

        // Save drawing
        saveDrawingToDisk(filename);
    }
}

void DrawingApp::handleColorKeys(KeyEvent event) {

    float I = 0.8f;
    float O = 0.2f;

    switch (event.getCode()) {
    case KeyEvent::KEY_r:
        // Red color
        currentColor = Color(I, O, O);
        drawing.setColor(vdraw::Color(I, O, O));
        break;

    case KeyEvent::KEY_g:
        // Green color
        currentColor = Color(O, I, O);
        drawing.setColor(vdraw::Color(O, I, O));
        break;

    case KeyEvent::KEY_b:
        // Blue color
        currentColor = Color(O, O, I);
        drawing.setColor(vdraw::Color(O, O, I));
        break;

    case KeyEvent::KEY_k:
        // Black color
        currentColor = Color(O, O, O);
        drawing.setColor(vdraw::Color(O, O, O));
        break;

    case KeyEvent::KEY_w:
        // White color
        currentColor = Color(I, I, I);
        drawing.setColor(vdraw::Color(O, O, O));
        break;
    }
}

void DrawingApp::handleStrokeKeys(KeyEvent event) {
    switch (event.getCode()) {
    case KeyEvent::KEY_PLUS:
    case KeyEvent::KEY_EQUALS:
        // Increase stroke width
        strokeWidth = min(strokeWidth + 0.5f, 50.0f);
        drawing.setStrokeWidth(strokeWidth);
        break;

    case KeyEvent::KEY_MINUS:
        // Decrease stroke width
        strokeWidth = max(strokeWidth - 0.5f, 0.5f);
        drawing.setStrokeWidth(strokeWidth);
        break;

    case KeyEvent::KEY_d:
        // Toggle dynamic width
        dynamicWidth = !dynamicWidth;
        drawing.setDynamicWidth(dynamicWidth);
        break;

    case KeyEvent::KEY_a:
        // Only if not combined with Ctrl/Cmd
        if (!(event.isControlDown() || event.isMetaDown())) {
            // Increase smoothing
            smoothingLevel = min(smoothingLevel + 1, 10);
            drawing.setSmoothing(smoothingLevel);
        }
        break;

    case KeyEvent::KEY_z:
        // Decrease smoothing
        smoothingLevel = max(smoothingLevel - 1, 0);
        drawing.setSmoothing(smoothingLevel);
        break;
    }
}

void DrawingApp::handleUndoRedoKeys(KeyEvent event) {
    if (event.isControlDown() || event.isMetaDown()) {
        switch (event.getCode()) {
        case KeyEvent::KEY_z:
            // Undo
            drawing.undo();
            resetCanvas();
            break;

        case KeyEvent::KEY_y:
            // Redo
            drawing.redo();
            resetCanvas();
            break;
        }
    }
}

void DrawingApp::update() {
    // No per-frame updates needed
}

void DrawingApp::draw() {
    // Clear the screen
    gl::clear(Color(1, 1, 1));


    gl::color(ColorA(1, 1, 1, 1));

    // Draw the FBO to the screen
    gl::draw(canvasFbo->getColorTexture());

    // Draw parameter window
    params->draw();
}

void DrawingApp::resize() {
    // Recreate FBO with new window size
    auto windowSize = getWindowSize();
    gl::Fbo::Format fboFormat;
    fboFormat.colorTexture();

    // Create new FBO
    auto newFbo = gl::Fbo::create(windowSize.x, windowSize.y, fboFormat);

    // Copy existing content if we had a previous FBO
    if (canvasFbo) {
        gl::ScopedFramebuffer fbScp(newFbo);
        gl::ScopedViewport viewport(vec2(0), newFbo->getSize());
        gl::ScopedMatrices matrices;
        gl::setMatricesWindow(newFbo->getSize());

        // Clear the new FBO to white
        gl::clear(Color(1, 1, 1));

        // Draw the old FBO content stretched to the new size
        gl::draw(canvasFbo->getColorTexture(), newFbo->getBounds());
    }

    // Replace the old FBO with the new one
    canvasFbo = newFbo;

    // If we didn't have a previous FBO, make sure to initialize it
    if (!canvasFbo) {
        resetCanvas();
    }
}

void DrawingApp::resetCanvas() {
    // Clear the canvas to transparent
    gl::ScopedFramebuffer fbScp(canvasFbo);
    gl::ScopedViewport viewport(vec2(0), canvasFbo->getSize());
    gl::ScopedMatrices matrices;
    gl::setMatricesWindow(canvasFbo->getSize());

    // Enable alpha blending within the FBO context
    gl::enableAlphaBlending();

    // Clear to transparent (R, G, B, Alpha)
    gl::clear(ColorA(0, 0, 0, 0));

    // Redraw all strokes
    for (const auto& stroke : drawing.getStrokes()) {
        renderStroke(*stroke);
    }

    // Restore blending state
    gl::disableAlphaBlending();
}

void DrawingApp::renderStroke(const vdraw::Stroke& stroke) {
    const auto& points = stroke.getProcessedPoints();
    if (points.size() < 2) return;

    // Get stroke color
    vdraw::Color color = stroke.getColor();
    gl::color(ColorA(color.r, color.g, color.b, color.a));

    // For line segments
    for (size_t i = 1; i < points.size(); ++i) {
        const auto& p1 = points[i - 1];
        const auto& p2 = points[i];

        float width1 = stroke.getWidthAt(i - 1);
        float width2 = stroke.getWidthAt(i);

        // Draw line segment with varying width
        gl::pushModelMatrix();

        vec2 v1(p1.position.x, p1.position.y);
        vec2 v2(p2.position.x, p2.position.y);

        // Calculate direction vector
        vec2 dir = normalize(v2 - v1);
        // Calculate perpendicular vector
        vec2 perp(-dir.y, dir.x);

        // Create polyline for the stroke segment
        gl::begin(GL_TRIANGLE_STRIP);

        // First point - left side
        gl::vertex(v1 + perp * width1 * 0.5f);
        // First point - right side
        gl::vertex(v1 - perp * width1 * 0.5f);
        // Second point - left side
        gl::vertex(v2 + perp * width2 * 0.5f);
        // Second point - right side
        gl::vertex(v2 - perp * width2 * 0.5f);

        gl::end();

        gl::popModelMatrix();
    }
}

ci::gl::TextureRef DrawingApp::convertTransparentFboToSolidTexture(const ci::gl::FboRef& transparentFbo,
    const ci::ColorA& backgroundColor) {
    // Get the size of the input FBO
    auto size = transparentFbo->getSize();

    // Create surface with background color
    ci::Surface8u surface(size.x, size.y, true);
    surface.setPremultiplied(true);

    // Fill with background color
    auto surfaceIter = surface.getIter();
    while (surfaceIter.line()) {
        while (surfaceIter.pixel()) {
            surfaceIter.r() = backgroundColor.r * 255;
            surfaceIter.g() = backgroundColor.g * 255;
            surfaceIter.b() = backgroundColor.b * 255;
            surfaceIter.a() = backgroundColor.a * 255;
        }
    }

    // Create texture from the surface
    auto bgTexture = ci::gl::Texture::create(surface);

    // Use a shader to composite the FBO over the background
    static ci::gl::GlslProgRef compositeShader = nullptr;
    if (!compositeShader) {
        try {
            compositeShader = ci::gl::GlslProg::create(
                ci::gl::GlslProg::Format()
                .vertex(CI_GLSL(150,
                    uniform mat4 ciModelViewProjection;
            in vec4 ciPosition;
            in vec2 ciTexCoord0;
            out vec2 TexCoord;

            void main() {
                gl_Position = ciModelViewProjection * ciPosition;
                TexCoord = ciTexCoord0;
            }
                ))
                .fragment(CI_GLSL(150,
                    uniform sampler2D uBackgroundTex;
            uniform sampler2D uForegroundTex;
            in vec2 TexCoord;
            out vec4 FragColor;

            void main() {
                vec4 bgColor = texture(uBackgroundTex, TexCoord);
                vec4 fgColor = texture(uForegroundTex, TexCoord);
                // Simple alpha compositing
                FragColor = mix(bgColor, fgColor, fgColor.a);
            }
                ))
            );
        }
        catch (ci::gl::GlslProgCompileExc& exc) {
            ci::app::console() << "Shader compile error: " << exc.what() << std::endl;
            // Fallback to simpler method if shader fails
            return ci::gl::Texture::create(surface);
        }
    }

    // Create output texture
    auto format = ci::gl::Texture2d::Format().internalFormat(GL_RGBA8);
    auto resultTexture = ci::gl::Texture::create(size.x, size.y, format);

    // Use FBO to render composite to texture
    {
        auto tempFbo = ci::gl::Fbo::create(size.x, size.y);
        ci::gl::ScopedFramebuffer fbScp(tempFbo);
        ci::gl::ScopedViewport viewScp(ci::vec2(0), size);
        ci::gl::ScopedMatrices matScp;
        ci::gl::setMatricesWindow(size);

        // Use the shader program
        ci::gl::ScopedGlslProg shaderScp(compositeShader);

        // Bind textures
        bgTexture->bind(0);
        transparentFbo->getColorTexture()->bind(1);

        // Set uniforms
        compositeShader->uniform("uBackgroundTex", 0);
        compositeShader->uniform("uForegroundTex", 1);

        // Draw fullscreen quad
        ci::gl::drawSolidRect(ci::Rectf(0, 0, size.x, size.y));

        // Get result
        resultTexture = tempFbo->getColorTexture();
    }

    return resultTexture;
}


ci::gl::TextureRef DrawingApp::captureLatestStrokeAsTexture()
{
    auto windowSize = getWindowSize();
    gl::Fbo::Format fboFormat;
    fboFormat.colorTexture();
    auto newFbo = gl::Fbo::create(windowSize.x, windowSize.y, fboFormat);

    // Draw the active stroke (with safety check)
    const auto& strokes = drawing.getStrokes();
    if (!strokes.empty()) {
        gl::ScopedFramebuffer fbScp(newFbo);
        gl::ScopedViewport viewport(vec2(0), newFbo->getSize());
        gl::ScopedMatrices matrices;
        gl::setMatricesWindow(newFbo->getSize());

//        gl::clear(Color(1, 1, 1));
        
        auto colorLive = vdraw::Color(137 / 255.f, 216 / 255.f, 238 / 255.f);

        auto activeStroke = *strokes.back();
        auto color = activeStroke.getColor();
        activeStroke.setColor(colorLive);

        // Only render the active stroke
        renderStroke(activeStroke);

        // Reset color state to white
        gl::color(ColorA(1, 1, 1, 1));
    }

    return newFbo->getColorTexture();
}

ci::gl::TextureRef DrawingApp::captureDrawingAsTexture(bool solid) {

    if (solid)
        return convertTransparentFboToSolidTexture(canvasFbo);
    else
        return canvasFbo->getColorTexture();
}

ci::Surface8u DrawingApp::captureDrawingAsSurface() {
    // Create a surface of the appropriate size
    auto size = canvasFbo->getSize();
    ci::Surface8u result(size.x, size.y, true); // Use alpha channel

    // Bind the FBO to read from it
    gl::ScopedFramebuffer fbScp(canvasFbo);

    // Get the pixels
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, result.getData());

    // Create a flipped surface (OpenGL has origin at bottom-left, Surface has origin at top-left)
    ci::Surface8u flipped(size.x, size.y, true);

    // Manual flip
    for (int y = 0; y < size.y; ++y) {
        uint8_t* srcRow = result.getData(ivec2(0, size.y - 1 - y));
        uint8_t* dstRow = flipped.getData(ivec2(0, y));
        memcpy(dstRow, srcRow, size.x * 4); // 4 bytes per pixel (RGBA)
    }

    return flipped;
}

void DrawingApp::saveDrawingToDisk(const std::string& filename) {
    try {
        // Get surface from drawing
        auto surface = captureDrawingAsSurface();

        // Write to file
        writeImage(filename, surface);
        console() << "Saved drawing to: " << filename << std::endl;
    }
    catch (std::exception& e) {
        console() << "Error saving drawing: " << e.what() << std::endl;
    }
}