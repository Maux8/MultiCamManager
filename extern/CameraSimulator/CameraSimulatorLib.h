#pragma once

#ifdef _WIN32
  #ifdef CAMERASIMULATORLIB_EXPORTS
    #define CAMERASIMULATORLIB_API __declspec(dllexport)
  #else
    #define CAMERASIMULATORLIB_API __declspec(dllimport)
  #endif
#else
  #define CAMERASIMULATORLIB_API
#endif
//#include "CameraSimulator.h"
#include <opencv2/opencv.hpp>

/**
 * @class CameraSimulatorDLL
 * @brief DLL-Schnittstelle, kapselt intern den CameraSimulator.
 *
 * Diese Klasse stellt die API für DLL-Nutzer bereit,
 * die Implementation wird an das reine Simulator-Objekt delegiert.
 */
 
 class CameraSimulator;
 
class CAMERASIMULATORLIB_API CameraSimulatorLib
{
public:
    /**
     * @brief Konstruktor: Erstellt den Simulator intern.
     */
	CameraSimulatorLib();

    /**
     * @brief Destruktor: Gibt die Ressourcen frei.
     */
	~CameraSimulatorLib();

    /**
     * @brief Verbindet die Kamera.
     * @return true wenn Verbindung erfolgreich war.
     */
    bool connect();

    /**
     * @brief Trennt die Kamera.
     */
    void disconnect();

    /**
     * @brief Startet die Bildaufnahme.
     * @return true wenn gestartet wurde.
     */
    bool start();

    /**
     * @brief Stoppt die Bildaufnahme.
     */
    void stop();

    /**
     * @brief Liefert den aktuellen Frame.
     * @return OpenCV Bild (cv::Mat).
     */
    cv::Mat getFrame();

    /**
     * @brief Liefert die Temperatur.
     * @return Temperatur in °C.
     */
    double getTemperature() const;

    /**
     * @brief Liefert die aktuelle Bildfrequenz.
     * @return FPS.
     */
    double getFPS() const;

    /**
     * @brief Liest die Belichtungszeit.
     * @return Belichtungszeit in µs.
     */
    double getExposureTime() const;

    /**
     * @brief Setzt die Belichtungszeit.
     * @param value Neue Belichtungszeit in µs.
     */
    void setExposureTime(double value);

    /**
     * @brief Liefert den Gain-Wert.
     * @return Verstärkungsfaktor.
     */
    double getGain() const;

    /**
     * @brief Setzt den Gain-Wert.
     * @param value Neuer Verstärkungsfaktor.
     */
    void setGain(double value);

    /**
     * @brief Prüft Stromversorgung.
     * @return true wenn Strom an.
     */
    bool getPowerStatus() const;

    /**
     * @brief Setzt Stromversorgung.
     * @param on true schaltet an, false aus.
     */
    void setPowerStatus(bool on);

    /**
     * @brief Liefert die Zahl der Frames.
     * @return Frame-Zähler.
     */
    uint64_t getFrameCounter() const;

    /**
     * @brief Liefert den Fehlercode.
     * @return Fehlercode.
     */
    int getErrorCode() const;

    /**
     * @brief Setzt den Fehlercode.
     * @param code Neuer Fehlercode.
     */
    void setErrorCode(int code);

private:
    CameraSimulator* m_simulator; ///< Internes Objekt
};
