#include "ProgramState.hh"

ProgramState::ProgramState() {
    IsPendingShutdown = false;
    LoopCount = 0;

    StatusMessage = new String();
    StatusMessage->reserve(512);

    SyncRoot = xSemaphoreCreateBinary();
    xSemaphoreGive(SyncRoot);
}

ProgramState::~ProgramState() {
    if (IsPendingShutdown) return;

    IsPendingShutdown = true;
    delete StatusMessage;
    Display->displayOff();
    Display->end();
    delete Display;
    vSemaphoreDelete(SyncRoot);
}

void ProgramState::begin() {
    Display = createDisplay();
    
    xTaskCreateUniversal(BlinkTask, "BlinkTask", STACK_SIZE_DEFAULT, this, 1, &BlinkTaskHandle, APP_CPU_NUM);
    xTaskCreateUniversal(DisplayTask, "DisplayTask", STACK_SIZE_DEFAULT, this, 2, &DisplayTaskHandle, APP_CPU_NUM);
    xTaskCreateUniversal(WirelessTask, "WirelessTask", 4096, this, 3, &WirelessTaskHandle, PRO_CPU_NUM);
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
        vTaskDelay(pdMS_TO_TICKS(500));
        digitalWrite(LED_BUILTIN, ledState);
        ledState = !ledState;
        instance->lockWait();
        instance->LoopCount++;
        instance->StatusMessage->clear();
        instance->StatusMessage->concat("Current Count: ");
        instance->StatusMessage->concat(instance->LoopCount);
        instance->lockRelease();
    }

    vTaskDelete(instance->BlinkTaskHandle);
}

void ProgramState::DisplayTask(void* argument) {
    auto instance = (ProgramState*)argument;

    instance->Display->setFont(ArialMT_Plain_10);
    instance->Display->setTextAlignment(TEXT_ALIGN_LEFT);

    while (!instance->IsPendingShutdown)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        instance->lockWait();
        instance->Display->clear();
        instance->Display->drawString(0, 10, *(instance->StatusMessage));
        instance->lockRelease();
        instance->Display->display();
    }

    vTaskDelete(instance->DisplayTaskHandle);
}

void ProgramState::WirelessTask(void* argument) {
    auto instance = (ProgramState*)argument;
    WiFi.setAutoConnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin("Highbird", "74a42f28a1");
    instance->lockWait();
    instance->StatusMessage->clear();
    instance->StatusMessage->concat("Connecting . . .");
    instance->lockRelease();

    auto status = WiFi.waitForConnectResult();
    
    instance->lockWait();
    instance->StatusMessage->clear();
    instance->StatusMessage->concat("Connection result complete");
    instance->lockRelease();

    while (!instance->IsPendingShutdown)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        status = WiFi.status();
        instance->lockWait();
        instance->StatusMessage->clear();
        if (status == WL_CONNECTED) {
            instance->StatusMessage->concat(WiFi.localIP().toString());
        } else {
            instance->StatusMessage->concat("WiFi Unavaliable");
        }
        
        instance->lockRelease();
    }

    vTaskDelete(instance->WirelessTaskHandle);
}