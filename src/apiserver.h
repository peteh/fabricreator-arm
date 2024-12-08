#pragma once

#include <WebServer.h>
#include "RobotArm.h"
class ApiServer
{
private:
    WebServer m_server;
    RobotArm* m_robotArm;

    /**
     * @brief Get the web Content Type that corresponds to a file type.
     *
     * @param filename the file name like image.jpg
     * @return String the content type, e.g. text/html for index.html
     */
    String getContentType(String filename);

public:
    /**
     * @brief Construct a new Api Server that handles requests on the web /api/ endpoint.
     *
     * @param settings The settings of the device
     */
    ApiServer(RobotArm* robotArm)
    : m_robotArm(robotArm)
    {
    }

    /**
     * @brief Starts the webserver and registers all routes.
     *
     */
    void begin();

    /**
     * @brief Handles the root page and forwards to the main page.
     *
     */
    void handleRoot();

    /**
     * @brief Handles web page requests like index.html or style.css
     * and loads files files from the www dir of the filesystem.
     *
     */
    void handleWeb();

    void handleMainPage();

    void handleJointsGet();

    void handleJointsPost();

    /**
     * @brief Reboots the device, e.g. to reload the WiFi Config.
     *
     */
    void handleDeviceReboot();

    /**
     * @brief Handles active clients and responses.
     *
     */
    void handleClient();
};