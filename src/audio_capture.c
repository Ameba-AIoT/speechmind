#define LOG_TAG "AudioCapture"

#include "platform_autoconf.h"
#include "log/log.h"

#include "audio_capture.h"

#include "common/audio_errnos.h"
#include "osal_c/osal_mem.h"
#include "osal_c/osal_time.h"
#include "osal_c/osal_thread.h"
#include "audio/audio_record.h"
#include "audio/audio_control.h"

#define FRAME_MS 16

struct AudioCapture {
    struct AudioRecord *audio_record;
    uint32_t sample_rate;
    uint32_t channels;
    bool record_thread_running;
    osal_thread_t *record_thread;
    AudioCaptureCallback callback;
    void* user_data;
};

static void AudioCapture_startRecord(AudioCapture* audio_capture)
{
    MEDIA_LOGV("%s Enter", __FUNCTION__);
    //*create audio_record*/
    audio_capture->audio_record = AudioRecord_Create();
    if (!audio_capture->audio_record) {
        MEDIA_LOGE("record create failed");
        return;
    }

    AudioRecordConfig record_config;
    record_config.sample_rate = audio_capture->sample_rate;
    record_config.format = AUDIO_FORMAT_PCM_16_BIT;
    record_config.channel_count = audio_capture->channels;

    record_config.device = DEVICE_IN_MIC;
    record_config.buffer_bytes = 0;
    AudioRecord_Init(audio_capture->audio_record, &record_config, AUDIO_INPUT_FLAG_NONE);
    AudioRecord_Start(audio_capture->audio_record);

    /* Replace AUDIO_AMICS with your board's actual MIC category defines. */
#if defined (CONFIG_AMEBALITE)
    AudioControl_SetChannelMicCategory(0, AUDIO_AMIC1);
    AudioControl_SetChannelMicCategory(1, AUDIO_AMIC2);
    AudioControl_SetChannelMicCategory(2, AUDIO_AMIC3);

    AudioControl_SetMicBstGain(AUDIO_AMIC1, AUDIO_MICBST_GAIN_15DB);
    AudioControl_SetMicBstGain(AUDIO_AMIC2, AUDIO_MICBST_GAIN_15DB);

#elif defined(CONFIG_AMEBASMART)
    AudioControl_SetChannelMicCategory(0, AUDIO_AMIC1);
    AudioControl_SetMicBstGain(AUDIO_AMIC1, AUDIO_MICBST_GAIN_15DB);
    AudioControl_SetChannelMicCategory(1, AUDIO_AMIC3); // EA
    AudioControl_SetMicBstGain(AUDIO_AMIC3, AUDIO_MICBST_GAIN_15DB); // EA
    // AudioControl_SetChannelMicCategory(1, AUDIO_AMIC4);  // EL
    // AudioControl_SetMicBstGain(AUDIO_AMIC4, AUDIO_MICBST_GAIN_0DB); // EL
    AudioControl_SetChannelMicCategory(2, AUDIO_AMIC5);
    AudioControl_SetMicBstGain(AUDIO_AMIC5, AUDIO_MICBST_GAIN_5DB);
#endif
    AudioRecord_SetParameters(audio_capture->audio_record, "cap_mode=no_afe_pure_data");
    MEDIA_LOGD("RecordStartTask.. END");
}

static bool AudioCapture_recordLoop(void *param)
{
    AudioCapture *audio_capture = (AudioCapture *)param;
    MEDIA_LOGD("%s Enter, record_thread_running=%d\n", __FUNCTION__, audio_capture->record_thread_running);

    int data_size = (audio_capture->sample_rate * audio_capture->channels * 2 * FRAME_MS ) / 1000L;
    uint8_t* record_data = osal_malloc(data_size);
    while (audio_capture->record_thread_running) {
        AudioRecord_Read(audio_capture->audio_record, record_data, data_size, true);
        if (audio_capture->callback) {
            audio_capture->callback(record_data, data_size, NULL);
        }

    }

    osal_free(record_data);
    AudioRecord_Stop(audio_capture->audio_record);
    AudioRecord_Destroy(audio_capture->audio_record);
    audio_capture->audio_record = NULL;
    MEDIA_LOGD("%s Enter, exit", __FUNCTION__);
    return false;
}

AudioCapture* AudioCapture_create(uint32_t sample_rate, uint32_t channels)
{
    AudioCapture* capture = (AudioCapture*)osal_calloc(sizeof(AudioCapture));
    if (!capture) {
        MEDIA_LOGE("create audio capture error");
        return NULL;
    }
    capture->sample_rate = sample_rate;
    capture->channels = channels;

    AudioCapture_startRecord(capture);
    return capture;
}

int32_t AudioCapture_setCallback(AudioCapture* audio_capture, AudioCaptureCallback callback, void* user_data)
{
    audio_capture->callback = callback;
    audio_capture->user_data = user_data;
    return AUDIO_OK;
}

int32_t AudioCapture_start(AudioCapture* audio_capture)
{
    audio_capture->record_thread_running = true;
    osal_thread_param param;
    param.priority = OSAL_THREAD_PRI_AUDIO;
    param.stack_size = 1024 * 20;
    param.joinable = false;
    param.name = (char *)"AudioCapture_recordLoop";
    int32_t res = osal_thread_create(&audio_capture->record_thread, AudioCapture_recordLoop, audio_capture, &param);
    if (res) {
        MEDIA_LOGE("create record task fail");
        return AUDIO_ERR_NO_MEMORY;
    }
    return AUDIO_OK;

}

int32_t AudioCapture_stop(AudioCapture* audio_capture)
{
    if (audio_capture->record_thread) {
        audio_capture->record_thread_running = false;
        osal_thread_request_exitAndWait(audio_capture->record_thread);
    }
    return AUDIO_OK;
}

int32_t AudioCapture_destroy(AudioCapture* capture)
{
    if (capture) {
        osal_free(capture);
    }
    return AUDIO_OK;
}