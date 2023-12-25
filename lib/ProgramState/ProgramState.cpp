#include "ProgramState.hh"

ProgramState::ProgramState() {

    LoopCount = 0;

    StatusMessage = new String();
    StatusMessage->reserve(512);

    SyncRoot = xSemaphoreCreateBinary();
    xSemaphoreGive(SyncRoot);
}

void ProgramState::begin() {
    Display = createDisplay();
    xTaskCreateUniversal(BlinkTask, "BlinkTask", STACK_SIZE_DEFAULT, this, 1, &BlinkTaskHandle, CORE_ID_AUTO);
    xTaskCreateUniversal(DisplayTask, "DisplayTask", STACK_SIZE_DEFAULT, this, 2, &DisplayTaskHandle, CORE_ID_AUTO);
}

void ProgramState::lockWait() {
    // TODO: use ulTaskNotifyTake instead
    while (xSemaphoreTake(SyncRoot, portMAX_DELAY) == pdFALSE) {}
}

void ProgramState::lockRelease() {
    xSemaphoreGive(SyncRoot);
}

SSD1306Wire* ProgramState::createDisplay() {
    auto display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, RST_OLED, GEOMETRY_128_64);
    display->init();
    display->flipScreenVertically();
    display->setFont(ArialMT_Plain_10);
    display->setBrightness(128);
    display->clear();

    return display;
}

void ProgramState::BlinkTask(void* argument)
{
    auto instance = (ProgramState*)argument;
    pinMode(LED_BUILTIN, OUTPUT);
    bool ledState = true;

    while(true)
    {
        digitalWrite(LED_BUILTIN, ledState);
        ledState = !ledState;
        instance->lockWait();
        instance->LoopCount++;
        instance->lockRelease();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelete(nullptr);
}

void ProgramState::DisplayTask(void* argument)
{
    auto instance = (ProgramState*)argument;
    int currentCount;

    instance->Display->setFont(ArialMT_Plain_10);
    instance->Display->setTextAlignment(TEXT_ALIGN_LEFT);

    while (true)
    {
        instance->lockWait();
        currentCount = instance->LoopCount;
        instance->StatusMessage->clear();
        instance->StatusMessage->concat("Current Count: ");
        instance->StatusMessage->concat(instance->LoopCount);
        instance->Display->clear();
        instance->Display->drawString(0, 10, *(instance->StatusMessage));
        instance->lockRelease();
        instance->Display->display();
        vTaskDelay(pdMS_TO_TICKS(25));
    }

    vTaskDelete(nullptr);
}