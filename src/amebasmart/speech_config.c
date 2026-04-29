#define LOG_TAG "SpeechC"

#include "log/media_log.h"

#include <stdlib.h>
#include "cJSON/cJSON.h"

#include "ameba_soc.h"

#include "audio/audio_record.h"
#include "audio/audio_control.h"

#include "speech_config.h"

#include "vfs.h"

#define PARAM_FILE "vfs://param.json"
#define PARAM_FILE_SIZE_MAX 256

static SpeechConfig g_config;
static void SetDefaultParam(void)
{
    g_config.afe = 0;
    g_config.rpc = 0;
    g_config.dump_config.sd_dump = 0;
    g_config.dump_config.index = 0;
    g_config.dump_config.save_times = 0;
    g_config.amplifier_volume = 0.8f;
}

void SpeechConfig_save()
{
    FILE *param_file = fopen(PARAM_FILE, "r+");
    if (!param_file) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "%s is not exist.\n", PARAM_FILE);
        return;
    }

    char tmp[PARAM_FILE_SIZE_MAX];
    size_t res = fread(tmp, PARAM_FILE_SIZE_MAX, 1, param_file);

    cJSON *root = cJSON_Parse(tmp);
    if (!root) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse error.\n");
        fclose(param_file);
        return;
    }
    cJSON *index = cJSON_GetObjectItem(root, "index");
    if (index) {
        cJSON_DeleteItemFromObject(root, "index");
        cJSON_AddItemToObject(root, "index", cJSON_CreateNumber(g_config.dump_config.index));
    } else {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse index fail\n");
    }

    char *updated_json = cJSON_Print(root);
    updated_json[strlen(updated_json) + 1] = '\0';
    cJSON_Delete(root);
    fclose(param_file);

    param_file = fopen(PARAM_FILE, "w");
    if (!param_file) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "%s is not exist.\n", PARAM_FILE);
        free(updated_json);
        return;
    }
    res = fwrite(updated_json, strlen(updated_json) + 1, 1, param_file);
    if (res < strlen(updated_json) + 1) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "write fail\n");
    }
    fclose(param_file);
    free(updated_json);
}

void SpeechConfig_load()
{
    SetDefaultParam();
    FILE *param_file = fopen(PARAM_FILE, "r");
    if (!param_file) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "%s is not exist.\n", PARAM_FILE);
        g_config.dump_config.sd_dump = 0;
        return;
    }

    char tmp[PARAM_FILE_SIZE_MAX];
    size_t res = fread(tmp, PARAM_FILE_SIZE_MAX, 1, param_file);
    if (res < PARAM_FILE_SIZE_MAX) {
        tmp[res] = '\0';
    }

    cJSON *root = cJSON_Parse(tmp);
    if (!root) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse root error.\n");
        fclose(param_file);
        return;
    }

    cJSON *amplifier_volume = cJSON_GetObjectItem(root, "amplifier_volume");
    if (!amplifier_volume) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse volume fail.\n");
        g_config.amplifier_volume = 0.8f;
    } else {
        g_config.amplifier_volume = (float)amplifier_volume->valuedouble;
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "amplifier_volume is %d.\n", (int)(g_config.amplifier_volume * 10));
    }

    cJSON *sd_dump = cJSON_GetObjectItem(root, "sd_dump");
    if (!sd_dump) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse sd_dump fail.\n");
        goto exit;
    } else {
        g_config.dump_config.sd_dump = sd_dump->valueint;
        if (g_config.dump_config.sd_dump == 0) {
            RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "don't dump data to sd card.\n");
            goto exit;
        }
    }

    cJSON *index = cJSON_GetObjectItem(root, "index");
    if (!index) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse index fail.\n");
        goto exit;
    } else {
        g_config.dump_config.index = index->valueint;
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "dump file index is %d.\n", index->valueint);
    }

    cJSON *save_times = cJSON_GetObjectItem(root, "save_times");
    if (!save_times) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "parse save times fail.\n");
        goto exit;
    } else {
        g_config.dump_config.save_times = save_times->valueint;
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "dump save times is %d.\n", save_times->valueint);
    }

exit:
    cJSON_Delete(root);
    fclose(param_file);
}

SpeechConfig *SpeechConfig_getSpeechConfig(void)
{
    return &g_config;
}

float SpeechConfig_getAmplifierVolume(void)
{
    return g_config.amplifier_volume;
}
