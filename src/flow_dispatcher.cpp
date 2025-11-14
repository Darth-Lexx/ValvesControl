#include "flow_dispatcher.h"
#include "event_queue.h"
#include "logs.h"
#include "main.h"

// Очередь событий установки расхода (FIFO)
static FIFOQueue<8> FlowQueue;

void requestFlowSend(uint8_t channelIndex)
{
    FIFOQueue<8>::Item it;
    it.channel = channelIndex;

    if (!FlowQueue.push(it))
    {
        logMessage(LOG_WARNING, "FlowQueue overflow, event dropped for CH" + String(channelIndex+1));
    }
}

void processFlowQueue()
{
    FIFOQueue<8>::Item it;

    // Обрабатываем максимум 1 команду за цикл loop()
    if (!FlowQueue.pop(it))
        return;

    uint8_t idx = it.channel;
    if (idx >= 4)
        return;

    ChannelStruct *ch = Channels[idx];

    // Отправляем новое значение расхода в AS200
    ch->AS200MbError = AS200Master.writeMultipleHoldingRegisters(
        ch->AS200MbAdr,
        AS200_SET_FLOW_H,
        ch->AS200SetFlowArray,
        2
    );

    if (ch->AS200MbError == 0)
    {
        ch->setIsFlowSet(true);
        logMessage(LOG_MODBUS, "SetFlow OK for CH" + String(idx + 1));
    }
    else
    {
        logMessage(LOG_ERROR, "SetFlow FAILED on CH" + String(idx + 1) +
                              ", err=" + String(ch->AS200MbError));
        // позже можно добавить retry/отключение канала по ошибкам
    }
}
