#include "ui.h"
#include <switch.h>
#include <stdio.h>
#include <string.h>

void ui_init(void) {
    consoleInit(NULL);
}

void ui_exit(void) {
    consoleExit(NULL);
}

void ui_render_list(VideoItem *items, int count, int selected_index) {
    consoleClear();
    
    printf("\x1b[31;1m=== DarkTube [%d videos] ===\x1b[0m\n\n", count);
    
    if (count == 0) {
        printf("No videos loaded.\n");
        consoleUpdate(NULL);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        int h = items[i].lengthSeconds / 3600;
        int m = (items[i].lengthSeconds % 3600) / 60;
        int s = items[i].lengthSeconds % 60;
        
        char time_str[16];
        if (h > 0) snprintf(time_str, sizeof(time_str), "%d:%02d:%02d", h, m, s);
        else snprintf(time_str, sizeof(time_str), "%02d:%02d", m, s);

        // Truncate title to fit console width (80 cols)
        // Format: "> XX. <title> [HH:MM:SS]" = ~12 chars overhead + time_str
        int max_title_len = 80 - 12 - (int)strlen(time_str);
        if (max_title_len < 10) max_title_len = 10;
        
        char short_title[256];
        strncpy(short_title, items[i].title, sizeof(short_title) - 1);
        short_title[sizeof(short_title) - 1] = '\0';
        if ((int)strlen(short_title) > max_title_len) {
            short_title[max_title_len - 3] = '.';
            short_title[max_title_len - 2] = '.';
            short_title[max_title_len - 1] = '.';
            short_title[max_title_len] = '\0';
        }
        
        // Truncate author name too
        char short_author[80];
        strncpy(short_author, items[i].author, sizeof(short_author) - 1);
        short_author[sizeof(short_author) - 1] = '\0';
        if (strlen(short_author) > 60) {
            short_author[57] = '.';
            short_author[58] = '.';
            short_author[59] = '.';
            short_author[60] = '\0';
        }

        if (i == selected_index) {
            printf("\x1b[47;30m> %2d. %s [%s]\x1b[0m\n", i + 1, short_title, time_str);
            printf("\x1b[47;30m      (%s)\x1b[0m\n", short_author);
        } else {
            printf("  %2d. %s [%s]\n", i + 1, short_title, time_str);
            printf("      (%s)\n", short_author);
        }
    }
    
    printf("\n\x1b[36mControls: UP/DOWN (Navigate) | A (Play) | + (Exit)\x1b[0m\n");
    consoleUpdate(NULL);
}

void ui_render_loading(const char *message) {
    consoleClear();
    printf("\x1b[31;1m=== DarkTube ===\x1b[0m\n\n");
    printf("%s\n", message);
    consoleUpdate(NULL);
}
