#pragma once

#include <SPI.h>
#include <WiFi.h>
#include "oled/SSD1306Wire.h"

#define CORE_ID_AUTO -1
#define STACK_SIZE_DEFAULT 2048

class ProgramState {

public:
    int LoopCount;
    String* StatusMessage;
    SSD1306Wire* Display;
    
private:
    SemaphoreHandle_t SyncRoot;
    TaskHandle_t BlinkTaskHandle;
    TaskHandle_t DisplayTaskHandle;
    TaskHandle_t WirelessTaskHandle;
    volatile bool IsPendingShutdown;

public:

    /// @brief Initializes the program state variables
    ProgramState();

    /// @brief Starts the tasks associated witht he program state
    void begin();

    /// @brief Acquires an exclusive lock for reading or writing to program state.
    /// A caller of this method must always call the lockRelease method.
    void lockWait();

    /// @brief Releases the previously acquired lock for reading or writing to program state
    void lockRelease();

    /// @brief Destory resources created by this Program
    ~ProgramState();

private:

    /// @brief Creates an OLED display object
    /// @return A display object 
    static SSD1306Wire* createDisplay();

    /// @brief Implementation logic for the blinking LED
    /// @param argument Must contain the ProgramState instance.
    static void BlinkTask(void* argument);

    /// @brief Implementation logic for the status display.
    /// @param argument Must contain the ProgramState instance.
    static void DisplayTask(void* argument);

    /// @brief Implementation logic for wireless connection.
    /// @param argument Must contain the ProgramState instance.
    static void WirelessTask(void* arguemnt);
};
