#ifndef PLAYER_H
#define PLAYER_H

// Initialize the player state
int player_init(void);

// Start playback for a direct MP4 stream URL
// This is a stub for now that prints the URL and simulates playback
void player_play_stream(const char *stream_url);

// Clean up player state
void player_exit(void);

#endif // PLAYER_H
