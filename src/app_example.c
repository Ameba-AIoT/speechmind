#define LOG_TAG "AppE"
#include <stdio.h>

#include "log/media_log.h"
#include "common/audio_errnos.h"
#include "osal_c/osal_mem.h"

#include "speech_config.h"
#include "speech_mind.h"

void OnVad(SpeechMindCallback *callback, VadInfo *info)
{
    (void)callback;
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "vad info status %ld offset_ms %ld\n", info->status, info->offset_ms);
}

void OnWakeUp(SpeechMindCallback *callback, WakeUpInfo *info)
{
    (void)callback;
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "wake up info len %ld words %s\n", info->len, info->wakeup_words);
}

void OnAfe(SpeechMindCallback *callback, AfeInfo *info)
{
    (void)callback;
    (void)info;
}

void OnAsr(SpeechMindCallback *callback, AsrInfo *info)
{
    (void)callback;
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "asr info id %ld\n", info->id);
}

void OnAsrRecTimeout(SpeechMindCallback *callback)
{
    (void)callback;
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "asr timeout\n");
}

void app_example(void)
{
    SpeechConfig_load();

    SpeechMind_init();

    struct SpeechMindCallback* callback = (struct SpeechMindCallback*)osal_malloc(sizeof(struct SpeechMindCallback));
    if (!callback) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "malloc speechmind callback fail, no memory!\n");
        SpeechMind_deinit();
        return;
    }
    callback->onVad = OnVad;
    callback->onWakeUp = OnWakeUp;
    callback->onAfe = OnAfe;
    callback->onAsr = OnAsr;
    callback->onAsrRecTimeout = OnAsrRecTimeout;
    SpeechMind_setCallback(callback);

    SpeechMind_start();
}
