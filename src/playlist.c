#define LOG_TAG "PlayList"
#include "log/log.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "osal_c/osal_errnos.h"
#include "osal_c/osal_mem.h"
#include "osal_c/osal_mutex.h"
#include "osal_c/osal_thread.h"
#include "osal_c/osal_condition.h"


#define MAX_TITLE_LENGTH 1024

#define MAX_SUB_TTS (10)

enum PlayingStatus {
    IDLE,
    PLAYING,
    PAUSED,
    PLAYING_COMPLETED,
    REWIND_COMPLETE,
    STOPPED,
    RESET,
};

typedef struct Song {
    char* title;
    struct Song* next;
} Song;

typedef struct Playlist {
    char* name;
    Song* head;
    Song* tail;
    int count;
    osal_mutex_t mutex;
    osal_cond_t cond;
    bool isPlaying;
    bool shouldExit;
    osal_thread_t* playlist_task;
} Playlist;

Playlist* Playlist_create(void) {
    Playlist* playlist = (Playlist*)osal_malloc(sizeof(Playlist));
    if (!playlist) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "malloc playlist fail\n");
        return NULL;
    }
    playlist->head = NULL;
    playlist->tail = NULL;
    playlist->count = 0;
    osal_mutex_init(&playlist->mutex);
    osal_cond_init(&playlist->cond);
    playlist->isPlaying = false;
    playlist->shouldExit = false;
    return playlist;
}

void Playlist_addSong(Playlist* playlist, const char* title) {
    osal_mutex_lock(&playlist->mutex);
    Song* newSong = (Song*)osal_malloc(sizeof(Song));
    if (!newSong) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "malloc new Song fail!\n");
        return;
    }
    newSong->title = strdup(title);
    newSong->next = NULL;

    if (playlist->tail == NULL) {
        playlist->head = playlist->tail = newSong;
    } else {
        playlist->tail->next = newSong;
        playlist->tail = newSong;
    }
    playlist->count++;

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "song %s added to the playlist\n", title);
    // osal_cond_signal(&playlist->cond);
    osal_mutex_unlock(&playlist->mutex);
    RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "Playlist_addSong finish\n");
}

void Playlist_clear(Playlist* playlist) {
    osal_mutex_lock(&playlist->mutex);

    Song* current = playlist->head;
    while (current != NULL) {
        Song* next = current->next;
        free(current);
        current = next;
    }
    playlist->head = playlist->tail = NULL;
    playlist->count = 0;

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "play list is clear\n");
    osal_mutex_unlock(&playlist->mutex);
}

void Playlist_display(Playlist* playlist) {
    osal_mutex_lock(&playlist->mutex);
    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Current PlayList:\n");
    Song* current = playlist->head;
    int index = 1;
    while (current != NULL) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "%d. %s\n", index++, current->title);
        current = current->next;
    }
    osal_mutex_unlock(&playlist->mutex);
}

void Playlist_removeSong(Playlist* playlist, Song* songToRemove) {
    if (playlist == NULL || songToRemove == NULL) return;

    osal_mutex_lock(&playlist->mutex);

    if (playlist->head == songToRemove) {
        playlist->head = songToRemove->next;
        if (playlist->head == NULL) {
            playlist->tail = NULL;
        }
    } else {
        Song* prev = playlist->head;
        while (prev != NULL && prev->next != songToRemove) {
            prev = prev->next;
        }
        if (prev != NULL) {
            prev->next = songToRemove->next;
            if (prev->next == NULL) {
                playlist->tail = prev;
            }
        }
    }
    playlist->count--;

    osal_free(songToRemove->title);
    osal_free(songToRemove);

    osal_mutex_unlock(&playlist->mutex);
}

void Playlist_destroy(Playlist* playlist) {
    if (!playlist) {
        return;
    }

    Playlist_clear(playlist);
    osal_mutex_destroy(&playlist->mutex);
    osal_cond_destroy(&playlist->cond);
    osal_free(playlist);
}

static Song* Playlist_getFirstSong(Playlist* playlist) {
    if (!playlist) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "Playlist is NULL\n");
        return NULL;
    }

    osal_mutex_lock(&playlist->mutex);

    Song* firstSong = playlist->head;

    if (!firstSong) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Playlist is empty\n");
    }

    osal_mutex_unlock(&playlist->mutex);

    return firstSong;
}

int Playlist_size(Playlist* playlist) {
    int list_size = 0;
    osal_mutex_lock(&playlist->mutex);
    list_size = playlist->count;
    osal_mutex_unlock(&playlist->mutex);
    return list_size;
}

char* Playlist_getFirstSongTitle(Playlist* playlist) {
    Song* song = Playlist_getFirstSong(playlist);
    if (song) {
        return song->title;
    } else {
        return NULL;
    }
}

int Playlist_removeFirstSong(Playlist* playlist) {
    if (!playlist) {
        RTK_LOGS(LOG_TAG, RTK_LOG_ERROR, "Playlist is NULL\n");
        return OSAL_ERR_INVALID_PARAM;
    }

    osal_mutex_lock(&playlist->mutex);

    if (playlist->head == NULL) {
        RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Playlist is already empty\n");
        osal_mutex_unlock(&playlist->mutex);
        return OSAL_ERR_INVALID_OPERATION;
    }

    Song* firstSong = playlist->head;
    playlist->head = firstSong->next;
    playlist->count--;

    if (playlist->head == NULL) {
        // If the list becomes empty after removal
        playlist->tail = NULL;
    }

    RTK_LOGS(LOG_TAG, RTK_LOG_INFO, "Removed song: %s\n", firstSong->title);

    osal_free(firstSong);

    osal_mutex_unlock(&playlist->mutex);

    return OSAL_OK;
}
