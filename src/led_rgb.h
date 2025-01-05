#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class LedRGB
{
public:
    LedRGB(uint8_t outputPin)
        : m_pixel(1, outputPin, NEO_RGB | NEO_KHZ800)
    {
    }

    virtual void begin()
    {
        m_pixel.begin();
    }


    virtual void blinkAsync()
    {
        m_lastOn = millis();
        m_ledState = true;
        m_pixel.setBrightness(150);
        m_pixel.setPixelColor(0, Adafruit_NeoPixel::Color(100, 100, 0)); // Moderately bright yellow/orange color.
        m_pixel.show();
    }

    virtual void setBackgroundLight(bool enabled)
    {
        if (enabled)
        {
            m_backgroundColor = Adafruit_NeoPixel::Color(0, 100, 0);
            m_backgroundBrightness = 20;
        }
        else
        {
            m_backgroundColor = Adafruit_NeoPixel::Color(0, 0, 0);
            m_backgroundBrightness = 0;
        }
    }

    virtual void update()
    {
        if (m_ledState)
        {
            if (millis() - m_lastOn > 50)
            {
                m_ledState = false;
                m_pixel.setBrightness(m_backgroundBrightness);
                m_pixel.setPixelColor(0, m_backgroundColor);
                m_pixel.show();
            }
        }
    }

private:
    Adafruit_NeoPixel m_pixel;
    long m_lastOn = 0;
    bool m_ledState = false;
    uint32_t m_backgroundColor = 0;
    uint8_t m_backgroundBrightness = 0;
};