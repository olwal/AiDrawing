// VectorDrawing.cpp
#include "VectorDrawing.h"

// Use the namespace for all implementations
namespace vdraw {

    //-------------------------------------------------------------------------
    // Vec2 Implementation
    //-------------------------------------------------------------------------
    Vec2::Vec2() : x(0), y(0) {}

    Vec2::Vec2(float x, float y) : x(x), y(y) {}

    Vec2 Vec2::operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }

    Vec2 Vec2::operator-(const Vec2& other) const {
        return Vec2(x - other.x, y - other.y);
    }

    Vec2 Vec2::operator*(float scalar) const {
        return Vec2(x * scalar, y * scalar);
    }

    float Vec2::distanceTo(const Vec2& other) const {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    //-------------------------------------------------------------------------
    // Color Implementation
    //-------------------------------------------------------------------------
    Color::Color() : r(0), g(0), b(0), a(1.0f) {}

    Color::Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

    //-------------------------------------------------------------------------
    // StrokePoint Implementation
    //-------------------------------------------------------------------------
    StrokePoint::StrokePoint() : pressure(1.0f), timestamp(0) {}

    StrokePoint::StrokePoint(const Vec2& pos, float p, double t)
        : position(pos), pressure(p), timestamp(t) {
    }

    //-------------------------------------------------------------------------
    // Stroke Implementation
    //-------------------------------------------------------------------------
    Stroke::Stroke(const Color& color, float baseWidth)
        : color(color), baseWidth(baseWidth), dynamicWidth(false), smoothing(0) {
    }

    void Stroke::addPoint(const StrokePoint& point) {
        rawPoints.push_back(point);
        updateProcessedPoints();
    }

    void Stroke::setColor(const Color& color) {
        this->color = color;
    }

    Color Stroke::getColor() const {
        return color;
    }

    void Stroke::setBaseWidth(float width) {
        baseWidth = width;
    }

    float Stroke::getBaseWidth() const {
        return baseWidth;
    }

    void Stroke::setDynamicWidth(bool dynamic) {
        dynamicWidth = dynamic;
        updateProcessedPoints();
    }

    bool Stroke::getDynamicWidth() const {
        return dynamicWidth;
    }

    void Stroke::setSmoothing(int smoothingLevel) {
        smoothing = std::max(0, smoothingLevel);
        updateProcessedPoints();
    }

    int Stroke::getSmoothing() const {
        return smoothing;
    }

    const std::vector<StrokePoint>& Stroke::getRawPoints() const {
        return rawPoints;
    }

    const std::vector<StrokePoint>& Stroke::getProcessedPoints() const {
        return processedPoints;
    }

    bool Stroke::isEmpty() const {
        return rawPoints.empty();
    }

    float Stroke::getWidthAt(size_t index) const {
        if (index >= processedPoints.size()) return baseWidth;

        if (dynamicWidth) {
            float speed = 1.0f;

            // Calculate speed if we have a previous point
            if (index > 0 && processedPoints[index].timestamp > processedPoints[index - 1].timestamp) {
                float distance = processedPoints[index].position.distanceTo(processedPoints[index - 1].position);
                double timeDiff = processedPoints[index].timestamp - processedPoints[index - 1].timestamp;

                if (timeDiff > 0) {
                    speed = distance / static_cast<float>(timeDiff);
                    // Map speed to stroke width (faster → thinner)
                    speed = std::max(0.1f, std::min(2.0f, 1.0f / (speed * 0.01f)));
                }
            }

            return baseWidth * speed * processedPoints[index].pressure;
        }
        else {
            return baseWidth * processedPoints[index].pressure;
        }
    }

    void Stroke::updateProcessedPoints() {
        if (rawPoints.empty()) {
            processedPoints.clear();
            return;
        }

        if (smoothing <= 0) {
            processedPoints = rawPoints;
            return;
        }

        // Apply smoothing algorithm
        processedPoints.clear();
        processedPoints.reserve(rawPoints.size());

        for (size_t i = 0; i < rawPoints.size(); ++i) {
            StrokePoint smoothedPoint = rawPoints[i];

            if (i > 0 && i < rawPoints.size() - 1) {
                // Apply smoothing window
                int windowStart = std::max(0, static_cast<int>(i) - smoothing);
                int windowEnd = std::min(static_cast<int>(rawPoints.size()) - 1, static_cast<int>(i) + smoothing);

                Vec2 avgPos(0, 0);
                float avgPressure = 0;
                float totalWeight = 0;

                for (int j = windowStart; j <= windowEnd; ++j) {
                    // Calculate weight based on distance from center
                    float weight = 1.0f - std::abs(static_cast<float>(j - i)) / (smoothing + 1.0f);

                    avgPos = avgPos + (rawPoints[j].position * weight);
                    avgPressure += rawPoints[j].pressure * weight;
                    totalWeight += weight;
                }

                if (totalWeight > 0) {
                    smoothedPoint.position = avgPos * (1.0f / totalWeight);
                    smoothedPoint.pressure = avgPressure / totalWeight;
                }
            }

            processedPoints.push_back(smoothedPoint);
        }
    }

    //-------------------------------------------------------------------------
    // Drawing Implementation
    //-------------------------------------------------------------------------
    Drawing::Drawing()
        : currentColor(0, 0, 0), currentWidth(2.0f),
        dynamicWidth(false), smoothingLevel(0),
        activeStroke(nullptr), undoRedoIndex(0) {
    }

    Drawing::~Drawing() {
        clearStrokes();
        clearHistory();
    }

    void Drawing::beginStroke(const Vec2& position, float pressure, double timestamp) {
        // Create a new stroke with current settings
        std::unique_ptr<Stroke> stroke = std::make_unique<Stroke>(currentColor, currentWidth);
        stroke->setDynamicWidth(dynamicWidth);
        stroke->setSmoothing(smoothingLevel);

        // Add the first point
        stroke->addPoint(StrokePoint(position, pressure, timestamp));

        activeStroke = stroke.get();

        // Create add stroke command
        auto cmd = std::make_unique<AddStrokeCommand>(this, std::move(stroke));
        executeCommand(std::move(cmd));
    }

    void Drawing::continueStroke(const Vec2& position, float pressure, double timestamp) {
        if (activeStroke) {
            activeStroke->addPoint(StrokePoint(position, pressure, timestamp));
        }
    }

    void Drawing::endStroke() {
        activeStroke = nullptr;
    }

    void Drawing::setColor(const Color& color) {
        currentColor = color;
    }

    void Drawing::setStrokeWidth(float width) {
        currentWidth = std::max(0.1f, width);
    }

    void Drawing::setDynamicWidth(bool enabled) {
        dynamicWidth = enabled;
    }

    void Drawing::setSmoothing(int level) {
        smoothingLevel = std::max(0, level);
    }

    void Drawing::clearDrawing() {
        if (strokes.empty()) {
            return;
        }

        auto cmd = std::make_unique<ClearDrawingCommand>(this);
        executeCommand(std::move(cmd));
    }

    void Drawing::undo() {
        if (undoRedoIndex > 0) {
            undoRedoIndex--;
            commandHistory[undoRedoIndex]->undo();
        }
    }

    void Drawing::redo() {
        if (undoRedoIndex < commandHistory.size()) {
            commandHistory[undoRedoIndex]->execute();
            undoRedoIndex++;
        }
    }

    const std::vector<Stroke*>& Drawing::getStrokes() const {
        return strokePtrs;
    }

    void Drawing::forEachStroke(const std::function<void(const Stroke&)>& callback) const {
        for (const auto& stroke : strokePtrs) {
            callback(*stroke);
        }
    }

    void Drawing::clearStrokes() {
        strokePtrs.clear();
        strokes.clear();
        activeStroke = nullptr;
    }

    void Drawing::clearHistory() {
        commandHistory.clear();
        undoRedoIndex = 0;
    }

    void Drawing::executeCommand(std::unique_ptr<DrawingCommand> cmd) {
        // Remove any redoable commands if we're executing a new command
        if (undoRedoIndex < commandHistory.size()) {
            commandHistory.resize(undoRedoIndex);
        }

        cmd->execute();
        commandHistory.push_back(std::move(cmd));
        undoRedoIndex = commandHistory.size();
    }

    //-------------------------------------------------------------------------
    // Command Implementation
    //-------------------------------------------------------------------------
    AddStrokeCommand::AddStrokeCommand(Drawing* drawing, std::unique_ptr<Stroke> stroke)
        : drawing(drawing), stroke(std::move(stroke)) {
    }

    void AddStrokeCommand::execute() {
        if (stroke) {
            drawing->strokePtrs.push_back(stroke.get());
            drawing->strokes.push_back(std::move(stroke));
            stroke = nullptr;
        }
    }

    void AddStrokeCommand::undo() {
        if (!drawing->strokes.empty()) {
            stroke = std::move(drawing->strokes.back());
            drawing->strokes.pop_back();
            drawing->strokePtrs.pop_back();
        }
    }

    ClearDrawingCommand::ClearDrawingCommand(Drawing* drawing)
        : drawing(drawing) {
    }

    void ClearDrawingCommand::execute() {
        savedStrokes.clear();
        std::swap(savedStrokes, drawing->strokes);

        // Also save the pointers
        savedPtrs.clear();
        std::swap(savedPtrs, drawing->strokePtrs);

        drawing->activeStroke = nullptr;
    }

    void ClearDrawingCommand::undo() {
        std::swap(savedStrokes, drawing->strokes);
        std::swap(savedPtrs, drawing->strokePtrs);
    }

} // namespace vdraw