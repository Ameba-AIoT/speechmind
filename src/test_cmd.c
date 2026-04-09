#ifdef AEC_TEST
#define LOG_TAG "PlayerTest"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "osal_c/osal_time.h"
#include "osal_c/osal_mem.h"
#include "osal_c/osal_thread.h"

#include "ameba_soc.h"
#include "os_wrapper.h"
#include "platform_stdlib.h"
#include "basic_types.h"
#include "audio/audio_service.h"
#include "media/rtplayer.h"

#include "test_cmd.h"

#define MAX_URL_SIZE 1024
static char g_url[MAX_URL_SIZE];

enum PlayingStatus {
    IDLE,
    PLAYING,
    PAUSED,
    PLAYING_COMPLETED,
    REWIND_COMPLETE,
    STOPPED,
    RESET,
};
static int g_playing_status = IDLE;

static struct MediaPlayer *g_player;

static float g_left = 1.0;

static float g_right = 1.0;

static int g_loop = 0;

static osal_thread_t *g_player_thread;

void OnStateChanged(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int state)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "OnStateChanged(%p %p), (%d)\n", listener, player, state);

    switch (state) {
    case MEDIA_PLAYER_PREPARED: { //entered for async prepare
        break;
    }

    case MEDIA_PLAYER_PLAYBACK_COMPLETE: { //eos received, then stop
        g_playing_status = PLAYING_COMPLETED;
        break;
    }

    case MEDIA_PLAYER_STOPPED: { //stop received, then reset
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "start reset\n");
        g_playing_status = STOPPED;
        break;
    }

    case MEDIA_PLAYER_PAUSED: { //pause received when do pause or start rewinding
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "paused\n");
        g_playing_status = PAUSED;
        break;
    }

    case MEDIA_PLAYER_REWIND_COMPLETE: { //rewind done received, then start
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "rewind complete\n");
        g_playing_status = REWIND_COMPLETE;
        break;
    }
    }
}

void OnInfo(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int info, int extra)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "OnInfo (%p %p), (%d, %d)\n", listener, player, info, extra);

    switch (info) {
    case MEDIA_PLAYER_INFO_BUFFERING_START: {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "MEDIA_PLAYER_INFO_BUFFERING_START\n");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_END: {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "MEDIA_PLAYER_INFO_BUFFERING_END\n");
        break;
    }

    case MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE: {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "MEDIA_PLAYER_INFO_BUFFERING_INFO_UPDATE %d\n", extra);
        break;
    }

    case MEDIA_PLAYER_INFO_NOT_REWINDABLE: {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "MEDIA_PLAYER_INFO_NOT_REWINDABLE\n");
        break;
    }
    }
}

void OnError(const struct MediaPlayerCallback *listener, const struct MediaPlayer *player, int error, int extra)
{
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "OnError (%p %p), (%d, %d)\n", player, listener, error, extra);
}

void StartPlay(struct MediaPlayer *player, char *url)
{
    if (player == NULL) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "start play fail, player is NULL!\n");
        return;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "start to play: %s\n", url);
    int32_t ret = 0;

    g_playing_status = PLAYING;

    MediaPlayer_SetVolume(player, g_left, g_right);

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "SetSource\n");
    ret = MediaPlayer_SetSource(player, url);
    if (ret) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "SetDataSource fail:error=%d\n", (int)ret);
        return ;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Prepare\n");
    ret = MediaPlayer_Prepare(player);
    if (ret) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "prepare  fail:error=%d\n", (int)ret);
        return ;
    }

    if (g_loop) {
        ret = MediaPlayer_SetLooping(player, g_loop);
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Start\n");
    ret = MediaPlayer_Start(player);
    if (ret) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "start  fail:error=%d\n", (int)ret);
        return ;
    }

    int64_t duration = 0;
    MediaPlayer_GetDuration(player, &duration);
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "duration is %lldms\n", duration);

    while ((g_playing_status == PLAYING) || g_loop) {
        osal_msleep(1000);
    }

    if (g_playing_status == PLAYING_COMPLETED) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "play complete, now stop.\n");
        MediaPlayer_Stop(player);
    }

    while (g_playing_status == PLAYING_COMPLETED) {
        osal_msleep(1000);
    }

    if (g_playing_status == STOPPED) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "play stopped, now reset.\n");
        MediaPlayer_Reset(player);
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "play %s done!!!!\n", url);
}

u32 player_pause(u16 argc, u8 *argv[])
{
    (void) argc;
    (void) argv;

    if (g_player && g_playing_status == PLAYING) {
        MediaPlayer_Pause(g_player);
    }

    return 0;
}

u32 player_stop(u16 argc, u8 *argv[])
{
    (void) argc;
    (void) argv;

    if (g_player && (g_playing_status == PLAYING || g_playing_status == PAUSED)) {
        MediaPlayer_Stop(g_player);
    }

    return 0;
}

u32 player_resume(u16 argc, u8 *argv[])
{
    (void) argc;
    (void) argv;

    if (g_player && g_playing_status == PAUSED) {
        MediaPlayer_Start(g_player);
        g_playing_status = PLAYING;
    }

    return 0;
}

u32 player_set_volume(u16 argc, u8 *argv[])
{
    (void) argc;

    float left = 1.0;
    float right = 1.0;
    if (argv[0]) {
        left = (float)atof((const char *)argv[0]);
    }
    if (argv[1]) {
        right = (float)atof((const char *)argv[1]);
    }

    if (g_player) {
        MediaPlayer_SetVolume(g_player, left, right);
    }

    g_left = left;
    g_right = right;

    return 0;
}

u32 player_set_loop(u16 argc, u8 *argv[])
{
    (void) argc;

    int loop = (int)atoi((const char *)argv[0]);
    g_loop = loop;

    return 0;
}

int player_test(const char *url)
{
    AudioService_Init();
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "AudioService_Init done\n");

    struct MediaPlayerCallback *callback = (struct MediaPlayerCallback *)osal_malloc(sizeof(struct MediaPlayerCallback));
    if (!callback) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Calloc MediaPlayerCallback fail.\n");
        return -1;
    }

    callback->OnMediaPlayerStateChanged = OnStateChanged;
    callback->OnMediaPlayerInfo = OnInfo;
    callback->OnMediaPlayerError = OnError;

    g_player = MediaPlayer_Create();

    MediaPlayer_SetCallback(g_player, callback);

    StartPlay(g_player, (char *)url);

    osal_free(callback);
    MediaPlayer_Destory(g_player);
    g_player = NULL;

    osal_msleep(1000);

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "exit\n");
    return 0;
}

static bool example_player_thread(void *param)
{
    (void) param;

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "player test start......\n");

    player_test(g_url);
    osal_msleep(1 * 1000);

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "player test done......\n");

    return false;
}

void example_player_test_args_handle(char  *argv[])
{
    /* parse command line arguments */
    while (*argv) {
        if (strcmp(*argv, "-F") == 0) {
            argv++;
            if (*argv) {
                memset(g_url, 0, MAX_URL_SIZE);
                if (!strncasecmp("http://", *argv, 7) || !strncasecmp("https://", *argv, 8)) {
                    snprintf(g_url, MAX_URL_SIZE, "%s", *argv);
                } else {
                    snprintf(g_url, MAX_URL_SIZE, "%s", *argv);
                }
            }
        }
        if (*argv) {
            argv++;
        }
    }
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Usage: url is %s\n", g_url);

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "player test start......\n");

    osal_thread_param param;
    param.priority = OSAL_THREAD_PRI_NORMAL;
    param.stack_size = 1024 * 8;
    param.joinable = false;
    param.name = (char *)"example_player_thread";
    int32_t res = osal_thread_create(&g_player_thread, example_player_thread, NULL, &param);
    if (res) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "create example_player_thread fail\n");
        return;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "player test done......\n");

    return;
}

u32 example_player_test(u16 argc, u8 *argv[])
{
    (void) argc;
    example_player_test_args_handle((char **)argv);
    return TRUE;
}

CMD_TABLE_DATA_SECTION
const COMMAND_TABLE player_test_cmd_table[] = {
    {
        (const u8 *)"player",  1, example_player_test, (const u8 *)"\t player\n"
    },
    {
        (const u8 *)"player_pause",  1, player_pause, (const u8 *)"\t player_pause\n"
    },
    {
        (const u8 *)"player_stop",  1, player_stop, (const u8 *)"\t player_stop\n"
    },
    {
        (const u8 *)"player_resume",  1, player_resume, (const u8 *)"\t player_resume\n"
    },
    {
        (const u8 *)"player_set_volume",  1, player_set_volume, (const u8 *)"\t player_set_volume\n"
    },
    {
        (const u8 *)"player_set_loop",  1, player_set_loop, (const u8 *)"\t player_set_loop\n"
    },
};
#endif
