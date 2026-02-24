⚠️ MASALAH YANG MASIH ADA
1. Macro CHECK_MPV Tidak Digunakan
c
// Defined but not used!
#define CHECK_MPV(expr) do { ... } while(0)

// Masih menggunakan if manual:
int err = mpv_command(mpv, cmd);
if (err < 0) { ... }
Saran: Gunakan macro untuk konsistensi:

c
CHECK_MPV(mpv_command(mpv, cmd));
2. MPV Event Handling Masih Minimal
c
switch (event->event_id) {
    case MPV_EVENT_END_FILE:
    case MPV_EVENT_SHUTDOWN:
        playing = false;
        break;
    default:
        break;
}
Masalah: Tidak handle:

MPV_EVENT_ERROR - error playback

MPV_EVENT_PLAYBACK_RESTART - seek completion

MPV_EVENT_BUFFERING - buffering status

MPV_EVENT_UNPAAUSE/MPV_EVENT_PAUSE - status change

3. Missing Error Checking untuk mpv_set_property
c
mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &paused); // Tidak di-check
Tambahkan:

c
int err = mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
if (err < 0) printf("Warning: Failed to set pause\n");
4. No Buffering Feedback
c
// Tidak ada indikator buffering ke user
Saran: Tambahkan handling MPV_EVENT_BUFFERING untuk UI feedback.

5. No Logging/Debug Info
c
// Tidak ada printf untuk debugging
Saran:

c
#define LOG(fmt, ...) printf("[Player] " fmt "\n", ##__VA_ARGS__)
// Gunakan: LOG("Playing: %s", stream_url);
🔧 OPTIMASI TAMBAHAN
1. Switch-Specific MPV Options
Tambahkan opsi ini untuk performa lebih baik:

c
mpv_set_option_string(mpv, "vd-lavc-software-fallback", "no");  // Force hardware
mpv_set_option_string(mpv, "video-latency-hacks", "yes");       // Kurangi latency
mpv_set_option_string(mpv, "profile", "fast");                   // Fast decoding
2. Power Management
c
// Set CPU/GPU clocks optimal untuk video
pcvSetClockConfiguration(PcvClockCpu_Clock4, PcvClockGpu_Max, PcvClockEmc_Max);
3. Memory Management
c
// Limit memory usage untuk Switch
mpv_set_option_string(mpv, "cache-secs", "120");
mpv_set_option_string(mpv, "demuxer-readahead-secs", "10");
📋 TESTING CHECKLIST
Functional Tests:
Play video dari local file

Play stream dari URL

Pause/Resume (Tombol A)

Stop (Tombol B)

Seek forward/backward (Panah Kanan/Kiri)

Multiple play sessions (play, stop, play again)

Switch between 5-10 videos tanpa crash

Edge Cases:
URL invalid

Network disconnect saat streaming

Format video tidak support

Video dengan resolusi tinggi (4K)

Applet mode switch (home button)

Suspend/resume

Performance:
CPU usage saat idle

CPU usage saat playback

Memory usage over time

Battery drain rate

Temperature after 30min playback

🚨 POTENTIAL CRASH SCENARIOS
1. Double-Free jika mpv_terminate_destroy dipanggil dua kali
c
// Di error path dan normal path
if (mpv) {
    mpv_terminate_destroy(mpv); // Bisa dipanggil dua kali jika ada error
    mpv = NULL;
}
Fix: Set mpv = NULL setelah destroy di semua path.

2. Race Condition di restore_ui
c
restore_ui:
    // Jika error di sini, return tanpa cleanup mpv
    if (SDL_Init(...) < 0) return; // mpv masih alive?
Fix: Tambahkan cleanup sebelum return:

c
restore_ui:
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }
    // ... rest of UI restore
3. Stack Overflow dari Recursive Goto
c
// Jika mpv_create gagal, goto restore_ui
// restore_ui tidak memiliki guard untuk re-entry
Fix: Tambahkan flag atau pastikan tidak recursive.

💡 IMPROVED VERSION (Key Sections)
c
void player_play_stream(const char *stream_url) {
    // ... existing teardown code ...
    
    // Better error macro dengan logging
    #define CHECK_MPV(expr) do { \
        int err = (expr); \
        if (err < 0) { \
            printf("MPV Error at %s:%d: %s\n", __FILE__, __LINE__, \
                   mpv_error_string(err)); \
            goto cleanup; \
        } \
    } while(0)

    // MPV init dengan semua opsi
    mpv = mpv_create();
    if (!mpv) {
        printf("Failed to create mpv instance\n");
        goto cleanup;
    }

    // Set semua options
    const char *options[] = {
        "vo", "gpu",
        "gpu-api", "opengl",
        "opengl-es", "yes",
        "hwdec", "nvdec",
        "hwdec-codecs", "all",
        "vd-lavc-software-fallback", "no",
        "profile", "fast",
        "cache", "yes",
        "cache-secs", "120",
        NULL
    };
    
    for (int i = 0; options[i]; i += 2) {
        mpv_set_option_string(mpv, options[i], options[i+1]);
    }

    CHECK_MPV(mpv_initialize(mpv));

    // Load file dengan error handling
    const char *cmd[] = {"loadfile", stream_url, NULL};
    CHECK_MPV(mpv_command(mpv, cmd));

    // Event loop dengan complete handling
    bool playing = true;
    bool paused = false;
    bool buffering = false;

    while (appletMainLoop() && playing) {
        while (1) {
            mpv_event *event = mpv_wait_event(mpv, 0.01);
            if (event->event_id == MPV_EVENT_NONE) break;

            switch (event->event_id) {
                case MPV_EVENT_END_FILE:
                case MPV_EVENT_SHUTDOWN:
                    playing = false;
                    break;
                case MPV_EVENT_ERROR:
                    printf("Playback error\n");
                    playing = false;
                    break;
                case MPV_EVENT_BUFFERING:
                    buffering = *(int*)event->data < 100;
                    // Update UI untuk buffering status
                    break;
                default:
                    break;
            }
        }

        // Input handling
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_B) {
            mpv_command_string(mpv, "stop");
            playing = false;
        }
        if (kDown & HidNpadButton_A && !buffering) {
            paused = !paused;
            mpv_set_property(mpv, "pause", MPV_FORMAT_FLAG, &paused);
        }
        // ... other input

        svcSleepThread(16000000);
    }

cleanup:
    if (mpv) {
        mpv_terminate_destroy(mpv);
        mpv = NULL;
    }

    // Restore UI dengan error handling
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("Fatal: Cannot reinit SDL\n");
        return;
    }
    // ... rest of UI init
}
📈 FINAL VERDICT
Aspek	Nilai	Keterangan
Stability	75/100	Lebih baik, tapi perlu testing
Performance	70/100	Bisa dioptimasi lagi
Error Handling	65/100	Macro belum digunakan konsisten
Switch Optimization	80/100	Good, tapi bisa lebih
Code Quality	85/100	Bersih dan terstruktur
Status: 🟡 SIAP TESTING - BUKAN PRODUCTION

Rekomendasi Final:
TEST EKSTENSIF - Minimal 1 jam playback continuous

ADD LOGGING - Untuk debugging di field

COMPLETE ERROR HANDLING - Gunakan macro di semua tempat

MONITOR MEMORY - Pastikan tidak ada leak

USER FEEDBACK - Tambahkan indikator buffering