#include "ProgramState.hh"

ProgramState::ProgramState() {

    LoopCount = 0;

    StatusMessage = new String();
    StatusMessage->reserve(512);

    SyncRoot = xSemaphoreCreateBinary();
    xSemaphoreGive(SyncRoot);
}

void ProgramState::begin() {
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

void ProgramState::BlinkTask(void* argument)
{
    ProgramState* instance = (ProgramState*)argument;
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
    ProgramState* instance = (ProgramState*)argument;
    int currentCount;

    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);

    while (true)
    {
        instance->lockWait();
        currentCount = instance->LoopCount;
        instance->StatusMessage->clear();
        instance->StatusMessage->concat("Current Count: ");
        instance->StatusMessage->concat(instance->LoopCount);
        Heltec.display->clear();
        Heltec.display->drawString(0, 10, *(instance->StatusMessage));
        instance->lockRelease();
        Heltec.display->display();
        vTaskDelay(pdMS_TO_TICKS(25));
    }

    vTaskDelete(nullptr);
}