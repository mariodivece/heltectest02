#include "ProgramState.hh"

ProgramState::ProgramState() {
    IsPendingShutdown = false;
    LoopCount = 0;

    StatusMessage = new String();
    StatusMessage->reserve(64);

    WiFiStatusMessage = new String();
    WiFiStatusMessage->reserve(64);
}

ProgramState::~ProgramState() {
    
    if (IsPendingShutdown)
        return;

    IsPendingShutdown = true;
    delete StatusMessage;
    delete WiFiStatusMessage;
    Display->displayOff();
    Display->end();
    delete Display;
    vSemaphoreDelete(SyncRoot);
}

void ProgramState::begin() {
    Display = createDisplay();
    
    SyncRoot = xSemaphoreCreateBinary();
    xSemaphoreGive(SyncRoot);

    xTaskCreateUniversal(BlinkTask, "BlinkTask", STACK_SIZE_DEFAULT, this, 1, &BlinkTaskHandle, AUTO_CPU_NUM);
    xTaskCreateUniversal(DisplayTask, "DisplayTask", STACK_SIZE_DEFAULT, this, 2, &DisplayTaskHandle, AUTO_CPU_NUM);
    xTaskCreateUniversal(WirelessTask, "WirelessTask", STACK_SIZE_DEFAULT, this, 3, &WirelessTaskHandle, AUTO_CPU_NUM);
}

void ProgramState::lockWait() {
    // TODO: use ulTaskNotifyTake instead
    while (!IsPendingShutdown && xSemaphoreTake(SyncRoot, portMAX_DELAY) == pdFALSE) {
        // empty loop
    }
}

void ProgramState::lockRelease() {
    if (IsPendingShutdown) return;
    xSemaphoreGive(SyncRoot);
}

SSD1306Wire* ProgramState::createDisplay() {
    auto display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, RST_OLED, GEOMETRY_128_64);
    display->init();
    // display->flipScreenVertically();
    display->setFont(ArialMT_Plain_10);
    display->setBrightness(128);
    display->clear();

    return display;
}

void ProgramState::BlinkTask(void* argument) {
    auto instance = (ProgramState*)argument;
    pinMode(LED_BUILTIN, OUTPUT);
    auto ledState = true;

    while(!instance->IsPendingShutdown)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(LED_BUILTIN, ledState);
        ledState = !ledState;
        instance->lockWait();
        instance->LoopCount++;
        instance->lockRelease();
    }

    vTaskDelete(nullptr);
}

void ProgramState::DisplayTask(void* argument) {
    auto instance = (ProgramState*)argument;

    instance->Display->setFont(ArialMT_Plain_10);
    instance->Display->setTextAlignment(TEXT_ALIGN_LEFT);

    while (!instance->IsPendingShutdown)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        instance->lockWait();
        instance->Display->clear();

        instance->StatusMessage->clear();
        instance->StatusMessage->concat("Current Count: ");
        instance->StatusMessage->concat(instance->LoopCount);
        instance->Display->drawString(0, 10, *(instance->StatusMessage));
        instance->Display->drawString(0, 20, *(instance->WiFiStatusMessage));
        
        instance->lockRelease();
        instance->Display->display();
    }

    vTaskDelete(nullptr);
}

void ProgramState::WirelessTask(void* argument) {
    auto instance = (ProgramState*)argument;

    WiFi.setAutoConnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE); // disable power saving for WiFi, as enabling it makes comms unreliable.
    WiFi.begin("Highbird", "74a42f28a1");
    instance->lockWait();
    instance->WiFiStatusMessage->clear();
    instance->WiFiStatusMessage->concat("Connecting . . .");
    instance->lockRelease();

    auto status = WiFi.waitForConnectResult();
    
    instance->lockWait();
    instance->WiFiStatusMessage->clear();
    instance->WiFiStatusMessage->concat("Connection result complete");
    instance->lockRelease();

    while (!instance->IsPendingShutdown)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        status = WiFi.status();
        instance->lockWait();
        instance->WiFiStatusMessage->clear();
        if (status == WL_CONNECTED) {
            instance->WiFiStatusMessage->concat(WiFi.localIP().toString());
        } else {
            instance->WiFiStatusMessage->concat("WiFi Unavaliable");
        }
        
        instance->lockRelease();
    }

    vTaskDelete(nullptr);
}