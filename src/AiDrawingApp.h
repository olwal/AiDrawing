#pragma once
#include "DrawingApp.h"
#include <OllamaClient/OllamaClientCinder.h>
#include "ThreadSafeList.h"

#include "CiSpoutOut.h"
#include "CiSpoutIn.h"
#include "cinder/osc/Osc.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class AiDrawingApp : public DrawingApp {
public:
	AiDrawingApp();

	void setup() override;
	void mouseDown(ci::app::MouseEvent event) override;
	void mouseDrag(ci::app::MouseEvent event) override;
	void mouseUp(ci::app::MouseEvent event) override;
	void interpretCanvas(bool async = true);
	void keyDown(KeyEvent event) override;
	void draw() override;

	void sendOsc(string op, string property, string value);
	void sendOsc(string op, string message);
	void callback(const string& result);
	string result;
	
	void calculateSemanticAverage();
	bool isSemanticallySimilar(const string& newResult, const string& previousAverage);
	
	// Drawing detection methods
	void resetStrokeDistance();
	void updateStrokeDistance(const ci::vec2& newPos);
	bool hasSignificantDrawing();
	void semanticAverageCallback(const string& result);
	string semanticAverage;
	double semanticAverageStartMillis;
	float semanticAverageDt;

	// Stroke tracking for meaningful drawing detection
	float totalStrokeDistance;
	float strokeDistanceSinceLastInterpretation;
	ci::vec2 lastStrokePos;
	bool hasActiveStroke;
	float minimumStrokeDistanceForInterpretation;

	// on-screen text
	Font font;
	int fontSize;
	float dt;
	ThreadSafeList<string> results;
	bool showText;

	//Spout
	glm::ivec2 spoutDimensions = glm::ivec2(512, 512);
	SpoutOut spoutOutSketch;
	SpoutOut spoutOutViewport;
	string spoutOutSketchName = "SourceImage";
	string spoutOutViewportName = "AiViewport";
	bool doContinuousGeneration;
	cinder::gl::TextureRef texSolid;

	SpoutIn	spoutIn;
	bool showSpoutTexture;
	string spoutInName = "StreamDiffusion";
	bool spoutInConnected = false;
	bool spoutStartReceiver(string spoutInName = "");

	//OSC
	osc::SenderUdp* sender;
	osc::ReceiverUdp* receiver;
	const string destinationHost = "127.0.0.1";
	const int destinationPort = 7000;
	const int localPort = 7001;
	const int receiverPort = 7002;

	string addressPrompt;
	string propertyPrompt;
	string injectionPrompt;
	bool sendPrompt;

protected:
	bool ready;
	bool doStreaming;
	double currentMillis;
	string model;
	string prompt;
	OllamaClientCinder ollama;

	float imageGenerationFps;

};
