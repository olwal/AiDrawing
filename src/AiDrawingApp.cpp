#include "AiDrawingApp.h"
using namespace cinder::gl;

AiDrawingApp::AiDrawingApp() : spoutOutSketch("", app::getWindowSize()), spoutOutViewport("", app::getWindowSize()), showText(true)
{       
    showDrawing = false;
    showSpoutTexture = true;
    sendPrompt = true;
    showText = true;
    doContinuousGeneration = true;
}

using protocol = asio::ip::udp;
void AiDrawingApp::setup()
{
    result = "";
    semanticAverage = "";
    semanticAverageDt = 0;
    ready = true;
    
    // Initialize stroke tracking
    totalStrokeDistance = 0.0f;
    strokeDistanceSinceLastInterpretation = 0.0f;
    hasActiveStroke = false;
    minimumStrokeDistanceForInterpretation = 100.0f; // pixels
    doStreaming = true;
    prompt = "provide a concise, but creative description of what is being drawn, no more than 10 words"; 

    DrawingApp::setup();
    createConsole();
    params->hide();
    model = "llava:7b";
    ollama.setVisionModel(model);

    bool touchDesigner = false;

    if (touchDesigner)
    {
        addressPrompt = "/project1/StreamDiffusionTD"; //TouchDesigner
        propertyPrompt = "Promptdict0concept"; //TouchDesigner
    }
    else
    {
        addressPrompt = "/prompt";
        propertyPrompt = "";
    }

    injectionPrompt = ""; //" in sketch style using pencil on white paper";

    fontSize = 32;

    try {
        font = Font("Inter", fontSize);
        cout << "Loaded Inter font successfully" << endl;
    }
    catch (const std::exception& e) {
        cout << "Couldn't load font" << endl;
        font = Font("Arial", fontSize);
    }
    
    sender = new osc::SenderUdp(localPort, destinationHost, destinationPort);
    sender->bind();

    receiver = new osc::ReceiverUdp(receiverPort);

    receiver->setListener("/fps",
        [&](const osc::Message& msg) {
//            cout << msg << endl;
            //cout << msg.getArgFloat(0) << endl;
            imageGenerationFps = msg.getArgFloat(0);
        });

    receiver->bind();
    receiver->listen(NULL);

    //sendOsc("/project1/StreamDiffusionTD", "Promptdict0concept", "apple, sketch style and pencil drawing");

    sendOsc(addressPrompt, propertyPrompt, "apple");

    //spoutIn.getSpoutReceiver().SetActiveSender("TDSyphonSpoutOut"); 
    
    // 
    // 
    glm::ivec2 dimensions = app::getWindowSize();
    dimensions.x = 32;
    dimensions.y = 32;
    spoutOutSketch.getSpoutSender().CreateSender(spoutOutSketchName.c_str(), dimensions.x, dimensions.y);
    spoutOutViewport.getSpoutSender().CreateSender(spoutOutViewportName.c_str(), dimensions.x, dimensions.y);

    //spoutIn.getSpoutReceiver().SetActiveSender(spoutInName.c_str());
    sender->send(osc::Message("/x"));
    //spoutStartReceiver();
}

void AiDrawingApp::sendOsc(string op, string property, string value)
{
    osc::Message msg(op);
    msg.append(property);
    msg.append(value);
    sender->send(msg);
}

void AiDrawingApp::sendOsc(string op, string message)
{
    osc::Message msg(op);
    msg.append(message);
    sender->send(msg);
}

void AiDrawingApp::mouseDown(ci::app::MouseEvent event)
{
    DrawingApp::mouseDown(event);
    
    hasActiveStroke = true;
    lastStrokePos = event.getPos();
}

void AiDrawingApp::mouseDrag(ci::app::MouseEvent event)
{
    DrawingApp::mouseDrag(event);
    
    updateStrokeDistance(event.getPos());

    if (doStreaming && hasSignificantDrawing()) {
        cout << "Significant drawing detected (" << strokeDistanceSinceLastInterpretation << " pixels), interpreting canvas..." << endl;
        interpretCanvas(true);
        resetStrokeDistance();
    }
}

void AiDrawingApp::mouseUp(ci::app::MouseEvent event)
{
    DrawingApp::mouseUp(event);
    
    hasActiveStroke = false;
    
    // Check if we should interpret after this stroke
    if (doStreaming && hasSignificantDrawing()) {
        cout << "Significant drawing detected (" << strokeDistanceSinceLastInterpretation << " pixels), interpreting canvas..." << endl;
        interpretCanvas(true);
        resetStrokeDistance();
    } else if (doStreaming) {
        cout << "Stroke distance: " << strokeDistanceSinceLastInterpretation << " pixels (minimum: " << minimumStrokeDistanceForInterpretation << ")" << endl;
    }
}

void AiDrawingApp::callback(const std::string& result)
{
    dt = ci::app::getElapsedSeconds() * 1000.0 - currentMillis;
    results.push_front(result);
    if (results.size() > 30)
        results.pop_back();
    
    this->result = result;
    //cout << dt << " ms" << endl;
    //cout << "[" << result << "]" << endl;

    // Calculate semantic average when we get a new interpretation
    calculateSemanticAverage();
    
    if (sendPrompt)
    {
        if (propertyPrompt.size() > 0)
            sendOsc(addressPrompt, propertyPrompt, result + injectionPrompt);
        else
            sendOsc(addressPrompt, result + injectionPrompt);
    }

    ready = true;
}

void AiDrawingApp::calculateSemanticAverage()
{
    if (results.size() == 0) {
        return;
    }
    
    cout << "computing semantic average..." << endl;
    semanticAverageStartMillis = ci::app::getElapsedSeconds() * 1000.0;
    
    string prompt = "Given these drawing interpretation results (most recent first), provide a single concise description that represents the semantic average or most likely interpretation of what is being drawn. Weight more recent results higher:\n\n";
    
    int index = 0;
    results.forEach([&prompt, &index](const std::string& line) {
        float weight = 1.0f / (1.0f + index * 0.1f); // Exponential decay for recency weighting
        prompt += "Result " + to_string(index + 1) + " (weight: " + to_string(weight) + "): " + line + "\n";
        index++;
    });
    
    if (!semanticAverage.empty()) {
        prompt += "\nCurrent semantic average: \"" + semanticAverage + "\"\n";
        prompt += "\nBased on these results, what is most likely being drawn? Do not provide any explanations or other information other than the semantic average. If your interpretation is similar or almost the same as the current semantic average, just return the current semantic average unchanged to avoid unnecessary updates. Otherwise, respond with a brief, clear description (2-4 words):";
    } else {
        prompt += "\nBased on these results, what is most likely being drawn? Respond with a brief, clear description (2-4 words):";
    }
    
    ollama.sendPrompt(prompt, [this](const std::string& result, void* userData) {
        this->semanticAverageCallback(result);
    }, this);
}

bool AiDrawingApp::isSemanticallySimilar(const string& newResult, const string& previousAverage)
{
    if (previousAverage.empty()) {
        return false;
    }
    
    // Exact match
    if (newResult == previousAverage) {
        return true;
    }
    
    // Convert to lowercase for comparison
    string newLower = newResult;
    string prevLower = previousAverage;
    transform(newLower.begin(), newLower.end(), newLower.begin(), ::tolower);
    transform(prevLower.begin(), prevLower.end(), prevLower.begin(), ::tolower);
    
    // Check if one is contained in the other (e.g. "cat" vs "cat drawing")
    if (newLower.find(prevLower) != string::npos || prevLower.find(newLower) != string::npos) {
        return true;
    }
    
    // Check for common word overlap (simple approach)
    // Split by spaces and check if main words overlap
    size_t newSpacePos = newLower.find(' ');
    size_t prevSpacePos = prevLower.find(' ');
    
    if (newSpacePos != string::npos && prevSpacePos != string::npos) {
        string newFirstWord = newLower.substr(0, newSpacePos);
        string prevFirstWord = prevLower.substr(0, prevSpacePos);
        if (newFirstWord == prevFirstWord && newFirstWord.length() > 2) {
            return true;
        }
    }
    
    return false;
}

void AiDrawingApp::resetStrokeDistance()
{
    strokeDistanceSinceLastInterpretation = 0.0f;
}

void AiDrawingApp::updateStrokeDistance(const ci::vec2& newPos)
{
    if (hasActiveStroke) {
        float distance = ci::distance(lastStrokePos, newPos);
        totalStrokeDistance += distance;
        strokeDistanceSinceLastInterpretation += distance;
    }
    lastStrokePos = newPos;
}

bool AiDrawingApp::hasSignificantDrawing()
{
    return strokeDistanceSinceLastInterpretation >= minimumStrokeDistanceForInterpretation;
}

void AiDrawingApp::semanticAverageCallback(const string& result)
{
    semanticAverageDt = ci::app::getElapsedSeconds() * 1000.0 - semanticAverageStartMillis;
    
    // Mechanical check to reduce jitter even if Ollama didn't follow instructions perfectly
    //if (isSemanticallySimilar(result, semanticAverage)) {
    //    cout << "Mechanically detected similar results: [ new: " << result << ", old:" << semanticAverage << "]" << endl;
    //    return;
    //}
    
    semanticAverage = result;
    cout << "Updated semantic average: [" << result << "] " << semanticAverageDt << " ms" << endl;
}

bool AiDrawingApp::spoutStartReceiver(string spoutInName)
{
    if (spoutInName == "")
        spoutInName = this->spoutInName;

    unsigned int w;
    unsigned int h;

    sender->send(osc::Message("/s"));

    //spoutIn.getSpoutReceiver().GetSenderCount();

    bool success = spoutIn.getSpoutReceiver().CreateReceiver((char*)spoutInName.c_str(), w, h, false);
    //.SetActiveSender(spoutInName.c_str()); 

    if (success)
        cout << "Successfully connected to: " << spoutInName << " (" << w << "*" << h << ")" << endl;
    else
        cout << "Failed to connect to: " << spoutInName << endl;

    this->spoutInName = spoutInName;

    spoutInConnected = success;

    return success;
}

void AiDrawingApp::interpretCanvas(bool async)
{
    if (!ready)
        return;

    Surface8u surface = captureDrawingAsTexture(true)->createSource();
    cout << "sending canvas to " << ollama.getVisionModel() << "..." << endl;
    currentMillis = ci::app::getElapsedSeconds() * 1000.0;

    if (async)
    {
        ready = false;
        ollama.sendImageForInference(surface, prompt, [this](const std::string& result, void* userData) {
            this->callback(result);
        }, this);
    }
    else
    {
        ready = false;
        result = ollama.sendImageForInferenceSync(surface, prompt);
        cout << ci::app::getElapsedSeconds() * 1000.0 - currentMillis << " ms" << endl;
        cout << "result: " << result << endl;
        ready = true;
    }
}

static void variableToggle(bool * b, string text)
{
    *b = !*b;
    cout << text << " " << (*b ? "true" : "false") << endl;
}

void AiDrawingApp::keyDown(KeyEvent event) {
    
    

    switch (event.getCode())
    {
    case KeyEvent::KEY_ESCAPE: exit(0); break;
    case KeyEvent::KEY_RETURN: interpretCanvas(false); break;

    case KeyEvent::KEY_F1: variableToggle(&showDrawing, "showDrawing"); break;
    case KeyEvent::KEY_F2: variableToggle(&doStreaming, "doStreaming"); break;
    case KeyEvent::KEY_t: variableToggle(&showText, "showText");  break;
    case KeyEvent::KEY_o: variableToggle(&showSpoutTexture, "showSpoutTexture"); break;
    case KeyEvent::KEY_F3: variableToggle(&sendPrompt, "sendPrompt"); break;

    case KeyEvent::KEY_F5: variableToggle(&doContinuousGeneration, "doContinuousGeneration"); break;

    case KeyEvent::KEY_F6: 
        spoutOutSketch.sendTexture(texSolid);
        break;


    case KeyEvent::KEY_F4: calculateSemanticAverage(); break;

    //case KeyEvent::KEY_1: sender->send(osc::Message("/s") ); break;
    //case KeyEvent::KEY_2: sender->send(osc::Message("/S") ); break;
    //case KeyEvent::KEY_3: sender->send(osc::Message("/p")); break;
    //case KeyEvent::KEY_: sender->send(osc::Message("/P")); break;
    case KeyEvent::KEY_F12: spoutIn.getSpoutReceiver().SelectSenderPanel(); break;
//    case KeyEvent::KEY_6: spoutStartReceiver(); break;
    
    case KeyEvent::KEY_F11: sender->send(osc::Message("/x")); break;
    case KeyEvent::KEY_TAB: sender->send(osc::Message("/t")); break;

    case KeyEvent::KEY_DELETE: results.clear(); DrawingApp::keyDown(event); break;

    default: DrawingApp::keyDown(event); break;


    }
}

void AiDrawingApp::draw() {

    gl::enableAlphaBlending();

    float lineHeight = fontSize * 1.2f;
    int x = 10;
    int y = 10;

    int i = 0;

    float s1 = 0.5;
    float s2 = 1;
    //gl::clear(Color(s1, s1, s1));
    gl::clear(Color(s2, s2, s2));
    gl::color(ColorA(s2, s2, s2));

    bool matchingSpoutName = (spoutIn.getSenderName().compare(spoutInName) == 0);

    if (!matchingSpoutName)
    {
        static int lastTriggerTime = 0;
        double currentTime = ci::app::getElapsedSeconds();

        if (currentTime - lastTriggerTime >= 1.0) {
            spoutStartReceiver();
            lastTriggerTime = currentTime;
        }
    }

    if (showSpoutTexture)
    {
//        cout << "!" << endl;
        //cout << "[" << spoutIn.getSenderName() + " | " << spoutInName + " | " << matchingSpoutName << endl;

        Texture2dRef texSpout = spoutIn.receiveTexture();
        if (texSpout && matchingSpoutName)
        {
            float wf = this->getWindowWidth() / texSpout->getWidth();
            float hf = this->getWindowHeight() / texSpout->getHeight();
                
                
            //auto v = cout << v << endl;

            gl::pushMatrices();

            gl::scale(wf, hf);
            gl::draw(texSpout);

            gl::popMatrices();

        }
        else
            gl::color(ColorA(s2, s2, s2));
    }

    if (showDrawing)
    {
        auto texTransparent = captureDrawingAsTexture(false);
        gl::draw(texTransparent);
    }

    if (isMouseDown)
    {
        auto texTransparent = captureLatestStrokeAsTexture();
        gl::draw(texTransparent);
    }

    texSolid = captureDrawingAsTexture(true);


    if (doContinuousGeneration)
    {
//        auto texSolid = captureDrawingAsTexture(true);
        spoutOutSketch.sendTexture(texSolid);
    }

    //if (doStreaming && hasSignificantDrawing() && !isMouseDown) {
    //    cout << "Streaming: significant drawing detected, interpreting..." << endl;
    //    interpretCanvas();
    //    resetStrokeDistance();
    //}

    if (showText)
    {
        //drawString(spoutIn.getSenderName() + "=" + spoutInName + "[" + (matchingSpoutName ? "true" : "false") + "]", ci::vec2(getWindowWidth() * 0.6f, y + lineHeight), ColorA(0, 0, 0, 1), font);


        //string timing = to_string((int)dt) + " ms";
        //drawString(timing, ci::vec2(getWindowWidth() * 0.6f, y), ColorA(0, 0, 0, 1), font);

        //if (!semanticAverage.empty()) {
        //    string semantic = semanticAverage + " (" + to_string((int)semanticAverageDt) + " ms)";
        //    drawString(semantic, ci::vec2(x, y), ColorA(0, 0, 1, 1), font);
        //    y += lineHeight;
        //}

        //// Show stroke distance progress
        //string strokeInfo = "Stroke: " + to_string((int)strokeDistanceSinceLastInterpretation) + "/" + to_string((int)minimumStrokeDistanceForInterpretation) + " px";
        //ColorA strokeColor = hasSignificantDrawing() ? ColorA(0, 1, 0, 1) : ColorA(0.5f, 0.5f, 0.5f, 1);
        //drawString(strokeInfo, ci::vec2(x, y), strokeColor, font);
        //y += lineHeight;

        y += lineHeight;

        results.forEach([&y, this, lineHeight, x, &i](const std::string& line) {
            ColorA color = i++ == 0 ? ColorA(0, 0, 0, 1.0f) : ColorA(0, 0, 0, 0.3f);
            drawString(line, ci::vec2(x, y), color, font);
            y += lineHeight;
            });
    }
    gl::disableAlphaBlending();

    //for video capture in TouchDesigner
    spoutOutViewport.sendViewport();

    params->draw();
}