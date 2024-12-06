#include <uri/UriBraces.h>
#include <esplog.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "apiserver.h"

void ApiServer::begin()
{
    m_server.begin(80);

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
    // Handle form submission and update configuration data
    if (m_server.method() == HTTP_POST)
    {
        JsonDocument doc;
        String jsonString = m_server.arg("plain");
        deserializeJson(doc, jsonString);

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
    // Respond with the current configuration in JSON format
    String configJson;
    JsonDocument doc;
    // TODO set joints
    // Set the values in the document
    doc["joint"] = 1l;

    // doc["version"] = SYSTEM_NAME " " VERSION " (" __DATE__ ")";

    serializeJson(doc, configJson);
    m_server.send(200, "application/json", configJson);
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
