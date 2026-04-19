/* MIKTEST.C  MikMod playback test for DOS (Watcom/DOS4GW)
 *
 * Mirrors the structure of EXAMPL32.C but uses plain libmikmod API
 * instead of DSMI.  Build with the Watcom toolchain; link mikmod-static.
 *
 * Controls
 * --------
 *  +/-     master volume up / down
 *  n       next order (pattern)
 *  p       previous order
 *  m       toggle mute on channel 0
 *  ESC     stop and quit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>      /* getch / kbhit */
#include <ctype.h>

#include "MIKMOD.H"

 /* -----------------------------------------------------------------------
  * Globals
  * --------------------------------------------------------------------- */
static MODULE* g_mod = NULL;
static int      g_volume = 128;   /* md_musicvolume range: 0-128 */
static int      g_muted = 0;

/* -----------------------------------------------------------------------
 * cleanup – registered with atexit
 * --------------------------------------------------------------------- */
static void cleanup(void)
{
    if (g_mod)
    {
        Player_Stop();
        Player_Free(g_mod);
        g_mod = NULL;
    }
    MikMod_Exit();
}

/* -----------------------------------------------------------------------
 * print_status  – one-line status bar, re-printed each tick
 * --------------------------------------------------------------------- */
static void print_status(void)
{
    int order = Player_GetOrder();
    int row = Player_GetRow();
    int paused = Player_Paused();

    printf("\r  ord:%3d  row:%3d  vol:%3d  %s%s   ",
        order, row, g_volume,
        paused ? "[PAUSED]" : "[PLAYING]",
        g_muted ? " ch0:MUTE" : "         ");

    fflush(stdout);
}

/* -----------------------------------------------------------------------
 * main
 * --------------------------------------------------------------------- */
int main(int argc, char* argv[])
{
    char filename[256];
    int  ch;

    setbuf(stdout, NULL);

    puts("MikMod Test  (Command line)");
    puts("---------------------------------");

    /* ------------------------------------------------------------------
     * Choose module file
     * ---------------------------------------------------------------- */
    if (argc >= 2)
    {
        strncpy(filename, argv[1], sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
    else
    {
        printf("Module filename: ");
        if (fgets(filename, sizeof(filename), stdin) == NULL) return 1;
        /* strip trailing newline */
        filename[strcspn(filename, "\r\n")] = '\0';
    }

    /* ------------------------------------------------------------------
     * Initialise MikMod
     * drv_nos  = null driver; on real DOS hardware swap for drv_sb or
     *            drv_ultra as appropriate.  Under DOSBox drv_nos is fine
     *            because DOSBox intercepts the SB port itself – use
     *            drv_sb if you want audible output on real iron.
     * ---------------------------------------------------------------- */
    MikMod_RegisterAllLoaders();
#ifdef DRV_SB
    MikMod_RegisterDriver(&drv_sb);   /* swap to &drv_sb for real HW */
    md_mode = DMODE_SOFT_MUSIC;
    md_mixfreq = 22050;
    md_volume = 128;
    md_musicvolume = g_volume;

    if (MikMod_Init("port=220 irq=7 dma=1 hidma=5") != 0)
    {
        fprintf(stderr, "MikMod_Init failed: %s\n", MikMod_strerror(MikMod_errno));
        return 1;
    }
#else
    MikMod_RegisterDriver(&drv_nos);   /* swap to &drv_sb for real HW */
    md_mode = DMODE_SOFT_MUSIC;
    md_mixfreq = 44100;
    md_volume = 128;
    md_musicvolume = g_volume;

    if (MikMod_Init("") != 0)
    {
        fprintf(stderr, "MikMod_Init failed: %s\n", MikMod_strerror(MikMod_errno));
        return 1;
    }
#endif

    atexit(cleanup);

    /* ------------------------------------------------------------------
     * Load module
     * ---------------------------------------------------------------- */
    printf("Loading: %s\n", filename);
    g_mod = Player_Load(filename, 64, 0);
    if (!g_mod)
    {
        fprintf(stderr, "Player_Load failed: %s\n",
            MikMod_strerror(MikMod_errno));
        return 2;
    }

    printf("Title  : %s\n", g_mod->songname ? g_mod->songname : "(none)");
    printf("Type   : %s\n", g_mod->modtype ? g_mod->modtype : "(unknown)");
    printf("Channels: %d    Patterns: %d    Positions: %d\n",
        g_mod->numchn, g_mod->numpat, g_mod->numpos);

    /* ------------------------------------------------------------------
     * Start playback
     * ---------------------------------------------------------------- */
    g_mod->loop = 1;      /* loop module */
    g_mod->fadeout = 0;
    Player_Start(g_mod);

    puts("\nControls:");
    puts("  +/-  volume up/down");
    puts("  n    next order");
    puts("  p    previous order");
    puts("  SPACE toggle pause");
    puts("  m    mute/unmute channel 0");
    puts("  ESC  quit\n");

    /* ------------------------------------------------------------------
     * Main loop – poll keyboard, call MikMod_Update each iteration
     * ---------------------------------------------------------------- */
    while (Player_Active())
    {
        /* Pump the software mixer */
        MikMod_Update();

        print_status();

        if (!kbhit())
            continue;

        ch = getch();

        switch (ch)
        {
        case '+':
            if (g_volume < 128) g_volume++;
            md_musicvolume = g_volume;
            break;

        case '-':
            if (g_volume > 0) g_volume--;
            md_musicvolume = g_volume;
            break;

        case 'n': case 'N':
            Player_NextPosition();
            break;

        case 'p': case 'P':
            Player_PrevPosition();
            break;

        case ' ':
            Player_TogglePause();
            break;

        case 'm': case 'M':
            g_muted = !g_muted;
            if (g_muted) Player_Mute(MUTE_INCLUSIVE, 0, 0, -1);
            else         Player_Unmute(MUTE_INCLUSIVE, 0, 0, -1);
            break;

        case 27:   /* ESC */
            goto done;

        default:
            break;
        }
    }

done:
    puts("\nStopping.");
    Player_Stop();
    return 0;
}


