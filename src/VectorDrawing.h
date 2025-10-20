// VectorDrawing.h
#pragma once

#include <vector>
#include <memory>
#include <deque>
#include <functional>
#include <algorithm>
#include <cmath>

// Create a namespace for our drawing classes to avoid conflicts
namespace vdraw {

    // Generic 2D point structure
    struct Vec2 {
        float x, y;

        Vec2();
        Vec2(float x, float y);

        Vec2 operator+(const Vec2& other) const;
        Vec2 operator-(const Vec2& other) const;
        Vec2 operator*(float scalar) const;
        float distanceTo(const Vec2& other) const;
    };

    // Color representation with RGBA values
    struct Color {
        float r, g, b, a;

        Color();
        Color(float r, float g, float b, float a = 1.0f);
    };

    // Represents a point in a stroke with position, pressure and timestamp
    struct StrokePoint {
        Vec2 position;
        float pressure;
        double timestamp;

        StrokePoint();
        StrokePoint(const Vec2& pos, float p = 1.0f, double t = 0);
    };

    // A single stroke with its properties
    class Stroke {
    public:
        Stroke(const Color& color = Color(0, 0, 0), float baseWidth = 1.0f);

        void addPoint(const StrokePoint& point);

        void setColor(const Color& color);
        Color getColor() const;

        void setBaseWidth(float width);
        float getBaseWidth() const;

        void setDynamicWidth(bool dynamic);
        bool getDynamicWidth() const;

        void setSmoothing(int smoothingLevel);
        int getSmoothing() const;

        const std::vector<StrokePoint>& getRawPoints() const;
        const std::vector<StrokePoint>& getProcessedPoints() const;

        bool isEmpty() const;

        // Get width at a specific point index
        float getWidthAt(size_t index) const;

    private:
        std::vector<StrokePoint> rawPoints;
        std::vector<StrokePoint> processedPoints;
        Color color;
        float baseWidth;
        bool dynamicWidth;
        int smoothing;

        void updateProcessedPoints();
    };

    // Command interface for undo/redo functionality
    class DrawingCommand {
    public:
        virtual ~DrawingCommand() {}
        virtual void execute() = 0;
        virtual void undo() = 0;
    };

    // Forward declarations
    class Drawing;

    // Command to add a stroke
    class AddStrokeCommand : public DrawingCommand {
    public:
        AddStrokeCommand(Drawing* drawing, std::unique_ptr<Stroke> stroke);
        void execute() override;
        void undo() override;

    private:
        Drawing* drawing;
        std::unique_ptr<Stroke> stroke;
    };

    // Command to clear all strokes
    class ClearDrawingCommand : public DrawingCommand {
    public:
        ClearDrawingCommand(Drawing* drawing);
        void execute() override;
        void undo() override;

    private:
        Drawing* drawing;
        std::vector<std::unique_ptr<Stroke>> savedStrokes;
        std::vector<Stroke*> savedPtrs;
    };

    // Drawing class that manages strokes and provides drawing functionality
    class Drawing {
    public:
        Drawing();
        ~Drawing();

        void beginStroke(const Vec2& position, float pressure = 1.0f, double timestamp = 0);
        void continueStroke(const Vec2& position, float pressure = 1.0f, double timestamp = 0);
        void endStroke();

        void setColor(const Color& color);
        void setStrokeWidth(float width);
        void setDynamicWidth(bool enabled);
        void setSmoothing(int level);

        void clearDrawing();

        void undo();
        void redo();

        const std::vector<Stroke*>& getStrokes() const;

        // For rendering by external systems
        void forEachStroke(const std::function<void(const Stroke&)>& callback) const;

    private:
        std::vector<std::unique_ptr<Stroke>> strokes;
        std::vector<Stroke*> strokePtrs;  // Non-owning pointers for quick access
        std::vector<std::unique_ptr<DrawingCommand>> commandHistory;

        Color currentColor;
        float currentWidth;
        bool dynamicWidth;
        int smoothingLevel;

        Stroke* activeStroke;
        size_t undoRedoIndex;

        void clearStrokes();
        void clearHistory();
        void executeCommand(std::unique_ptr<DrawingCommand> cmd);

        friend class AddStrokeCommand;
        friend class ClearDrawingCommand;
    };

} // namespace vdraw