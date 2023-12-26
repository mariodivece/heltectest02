#include "ProgramState.hh"

ProgramState::ProgramState() {
    IsPendingShutdown = false;
    LoopCount = 0;

    LoopStatusMessage = new String();
    LoopStatusMessage->reserve(64);

    WiFiStatusMessage = new String();
    WiFiStatusMessage->reserve(64);
}

ProgramState::~ProgramState() {
    
    if (IsPendingShutdown)
        return;

    IsPendingShutdown = true;
    delete LoopStatusMessage;
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
    xTaskCreateUniversal(WirelessTask, "WirelessTask", STACK_SIZE_DEFAULT, this, 3, &WirelessTaskHandle, PRO_CPU_NUM);
}

void ProgramState::lockWait() {
    // TODO: use ulTaskNotifyTake instead
    while (!IsPendingShutdown && xSemaphoreTake(SyncRoot, portMAX_DELAY) == pdFALSE) {
        NOP();
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
        instance->updateLoopStatusMessage();
        instance->Display->clear();
        instance->lockWait();
        instance->Display->drawString(0, 10, *(instance->LoopStatusMessage));
        instance->Display->drawString(0, 20, *(instance->WiFiStatusMessage));
        instance->lockRelease();

        instance->Display->display();
    }

    vTaskDelete(nullptr);
}

void ProgramState::WirelessTask(void* argument) {
    auto instance = (ProgramState*)argument;

    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE); // disable power saving for WiFi, as enabling it makes comms unreliable.
    WiFi.begin("Highbird", "74a42f28a1");
    
    instance->setWiFiStatusMessage("Connecting . . . ");
    auto status = WiFi.waitForConnectResult();
    instance->setWiFiStatusMessage("Connection result completed");

    while (!instance->IsPendingShutdown) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        status = WiFi.status();

        if (status == WL_CONNECTED) {
            instance->setWiFiStatusMessage(WiFi.localIP().toString().c_str());
            break;
        } else {
            instance->setWiFiStatusMessage("WiFi Unavailable");
        }
    }

    WiFiServer httpServer;
    ulong currentTime = millis();
    ulong previousTime = 0;
    const ulong timeout = 2000; //ms.
    
    httpServer.begin(80);

    while (!instance->IsPendingShutdown) {
        WiFiClient client = httpServer.available();
        
        String readBuffer;
        readBuffer.reserve(4);

        const int MaxHeaderCount = 16;
        String httpHeaders[MaxHeaderCount];
        String httpBody;
        int currentHeaderIndex = 0;
        bool isReadingHeaders = true;

        if (!client) continue;
        currentTime = millis();
        previousTime = currentTime;

        while (client.connected() && currentTime - previousTime <= timeout) {
            currentTime = millis();

            if (client.available()) {
                char readChar = client.read();

                if (isReadingHeaders) {
                    if (readChar == '\r') continue;

                }

                if (httpHeaders[currentHeaderIndex].endsWith("\n") && && )
                httpHeader += readChar;
                // TODO NOT BUILDING!!!
                
            }
        }
    }

    vTaskDelete(nullptr);
}

void ProgramState::updateLoopStatusMessage() {
    lockWait();
    LoopStatusMessage->clear();
    LoopStatusMessage->concat("Current Count: ");
    LoopStatusMessage->concat(LoopCount);
    lockRelease();
}

void ProgramState::setWiFiStatusMessage(const char* cstr) {
    lockWait();
    WiFiStatusMessage->clear();
    if (cstr) {
        WiFiStatusMessage->concat(cstr);
    }

    lockRelease();
}

