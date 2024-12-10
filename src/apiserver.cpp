#include <uri/UriBraces.h>
#include <esplog.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "apiserver.h"

void ApiServer::begin()
{
    m_server.begin(80);
    m_server.enableDelay(false);

    // Webpages
    // m_server.on("/", HTTP_GET, std::bind(&ApiServer::handleRoot, this));
    // m_server.on(UriBraces("/{}"), HTTP_GET, std::bind(&ApiServer::handleWeb, this));

    m_server.on("/", HTTP_GET, std::bind(&ApiServer::handleMainPage, this));

    // Register API endpoints
    m_server.on("/api/v1/joints", HTTP_GET, std::bind(&ApiServer::handleJointsGet, this));
    m_server.on("/api/v1/joints", HTTP_POST, std::bind(&ApiServer::handleJointsPost, this));
    m_server.on("/api/v1/device/reboot", HTTP_GET, std::bind(&ApiServer::handleDeviceReboot, this));
}

// WEB PAGES

void ApiServer::handleMainPage()
{
    // TODO: Fix
    // m_server.send_P(200, "text/html", PAGE_MAIN);
}

void ApiServer::handleRoot()
{
    // Redirect to the main page URL
    m_server.sendHeader("Location", "/index.html", true);
    m_server.send(302, "text/plain", "");
}

void ApiServer::handleWeb()
{
    String fileName = m_server.pathArg(0);
    File file = LittleFS.open("/www/" + fileName, "r");
    if (!file)
    {
        m_server.send(404, "text/plain", "File not found");
        return;
    }
    String contentType = getContentType(fileName);
    m_server.streamFile(file, contentType);
    file.close();
}

void ApiServer::handleJointsPost()
{
    /* curl -X POST http://192.168.2.199/api/v1/joints \
    -H "Content-Type: application/json" \
     -d '{"joints": [90, 90, 90, 90, 90, 90, 90, 90]}'
    */
    if (m_server.method() == HTTP_POST)
    {
        JsonDocument doc;
        String jsonString = m_server.arg("plain");
        deserializeJson(doc, jsonString);

        // Access the "joints" array
        JsonArray joints = doc["joints"];
        
        Serial.println("Joints data:"); 
        for (size_t i = 0; i < joints.size(); i++) 
        {
            uint8_t value = joints[i]; 
            m_robotArm->setAngle(i, value);
        }

        // Send a response to the client
        String responseMessage = "Configuration updated successfully!";
        m_server.send(200, "application/json", "{\"message\":\"" + responseMessage + "\"}");
    }
}

void ApiServer::handleDeviceReboot()
{
    // wait a bit to save all files
    delay(100);
    m_server.send(200);
    // TODO: the restart should probably be
    // somewhere else to give a correct response
    delay(100);
    ESP.restart();
}

void ApiServer::handleJointsGet()
{
    // Respond with the current joint configuration in JSON format
    String configJson;
    JsonDocument doc;

    JsonArray joints = doc["joints"].to<JsonArray>();
    for(uint8_t i = 0; i < m_robotArm->NUM_JOINTS; i++)
    {
        joints.add(m_robotArm->getAngle(i));
    }
    serializeJson(doc, configJson);
    m_server.client().setNoDelay(true);
    m_server.send(200, "application/json", configJson);
    //m_server.client().print(configJson);
}

void ApiServer::handleClient()
{
    m_server.handleClient();
}

// HELPERS

String ApiServer::getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    else if (filename.endsWith(".css"))
        return "text/css";
    else if (filename.endsWith(".js"))
        return "application/javascript";
    else if (filename.endsWith(".png"))
        return "image/png";
    else if (filename.endsWith(".jpg"))
        return "image/jpeg";
    else if (filename.endsWith(".gif"))
        return "image/gif";
    else if (filename.endsWith(".ico"))
        return "image/x-icon";
    else if (filename.endsWith(".xml"))
        return "text/xml";
    else if (filename.endsWith(".pdf"))
        return "application/x-pdf";
    else if (filename.endsWith(".zip"))
        return "application/x-zip";
    else if (filename.endsWith(".gz"))
        return "application/x-gzip";
    return "text/plain";
}
