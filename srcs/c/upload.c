/* ============================================================================
 * upload.c — Setup -> "Upload data (S-records)"
 *
 * Receives a Motorola S-record file over the serial port and writes its data
 * straight into memory. The user gives a base address that is ADDED to every
 * record's own address (0 = take the file's addresses as they are). There is
 * deliberately NO validity check on the target: the user is expected to know
 * the area (ROM, open bus, chip registers... all accepted and silently
 * whatever the hardware makes of a byte write).
 *
 * Why S-records and not X/Y/ZMODEM: plain text, so it works from ANY terminal
 * (minicom/picocom paste, PiKVM web console, `cat file.srec > /dev/ttyUSBx`)
 * with no protocol support on the host; every record carries its address, a
 * length and a checksum, so a damaged line is reported and skipped instead of
 * written. `m68k-amigaos-objcopy -O srec` or srec_cat produce them.
 *
 * Receive discipline (the part that matters on a 68000 at 9600-19200 baud):
 * Paula buffers ONE received byte, so anything that stalls polling for more
 * than a byte time (~1 ms at 9600) loses data. Therefore
 *   - the stream is parsed byte by byte with a small state machine, no line
 *     buffer, and the record is written only once its checksum verified;
 *   - all on-screen progress goes through a tiny output queue that is drained
 *     ONE character per poll when no serial byte is pending;
 *   - globals->SerNoEcho mutes rs232_out() for the duration, so screen output
 *     never waits on the serial transmitter (which would block ~1 ms/char and
 *     collide with the incoming stream on the same wire).
 * Idle timeout: 10 s without a byte after the first record ends the upload
 * (files without an S7/S8/S9 end record still work). ESC on the Amiga
 * keyboard aborts at any time. Platform-neutral: uses readSerial()/SerBuf,
 * so the DeMoN FT245 backend works unchanged.
 *
 * MANTRA: no writable statics (they land in ROM) — all state is on the stack.
 * ========================================================================== */
#include "generic.h"

#define SREC_IDLE_LINES   156000UL   /* raster lines (~64us) of silence = ~10 s  */
#define SREC_KEYPOLL      4096UL     /* poll the keyboard every N idle lines      */
#define SREC_OUTQ         96
#define SREC_PROGRESS_ROW 9
#define SREC_ERROR_ROW    12
#define SREC_MAX_ERRLINES 8

typedef struct {
    char     q[SREC_OUTQ];
    int      qHead, qTail;
    uint8_t  color;
} OutQueue;

static void oqPut(OutQueue *o, const char *s, uint8_t color)
{
    o->color = color;
    while (*s) {
        int next = o->qTail + 1;                 /* no __modsi3 in this freestanding build */
        if (next == SREC_OUTQ) next = 0;
        if (next == o->qHead) break;             /* full: drop the tail, never stall */
        o->q[o->qTail] = *s++;
        o->qTail = next;
    }
}

static int oqDrainOne(OutQueue *o)
{
    if (o->qHead == o->qTail) return 0;
    char c = o->q[o->qHead];
    o->qHead = o->qHead + 1;
    if (o->qHead == SREC_OUTQ) o->qHead = 0;
    if (c == '\n') printCharNewLine();
    else           printChar(c, o->color);
    return 1;
}

static int hexVal(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Pull one byte out of the shared serial buffer (same shift as getSerial()). */
static int srecPopByte(void)
{
    uint8_t n = globals->SerBufLen;
    if (n == 0) return -1;
    uint8_t b = globals->SerBuf[0];
    for (int i = 0; i + 1 < n; i++) globals->SerBuf[i] = globals->SerBuf[i + 1];
    globals->SerBufLen = n - 1;
    return b;
}

typedef struct {
    uint32_t recOk, recBad, bytes, lineNo;
    uint32_t lo, hi;
    int      errLines;
    int      gotEnd;
} SrecStats;

static void srecProgress(OutQueue *oq, const SrecStats *st)
{
    setPos(0, SREC_PROGRESS_ROW);
    oqPut(oq, "Records OK: ", WHITE);  oqPut(oq, binDec((int32_t)st->recOk), GREEN);
    oqPut(oq, "   bad: ", WHITE);      oqPut(oq, binDec((int32_t)st->recBad), st->recBad ? RED : GREEN);
    oqPut(oq, "   bytes: ", WHITE);    oqPut(oq, binDec((int32_t)st->bytes), CYAN);
    oqPut(oq, "   ", WHITE);
}

static void srecError(OutQueue *oq, SrecStats *st, const char *why)
{
    st->recBad++;
    if (st->errLines < SREC_MAX_ERRLINES) {
        setPos(0, SREC_ERROR_ROW + st->errLines);
        oqPut(oq, "Line ", RED); oqPut(oq, binDec((int32_t)st->lineNo), RED);
        oqPut(oq, ": ", RED);    oqPut(oq, why, RED);
        st->errLines++;
    }
}

/* Parser states */
enum { ST_WAIT_S, ST_TYPE, ST_COUNT, ST_BODY };

void uploadSrec(void)
{
    OutQueue  oq;
    SrecStats st;
    uint8_t   rec[256];
    int32_t   base;

    clearScreen();
    print("\002Upload data (S-records)\n\n", CYAN);
    print("Writes a Motorola S-record file from the serial port into memory.\n", WHITE);
    print("Every record's own address is ADDED to the base you give here.\n", WHITE);
    print("No check is made on the target area - you must know it is valid.\n\n", YELLOW);
    print("Base address (hex, 0 = use the file's addresses), ESC cancels: $", WHITE);
    base = inputHexNum((void *)0);
    if (base == -1) return;

    print("\n\nSend the file now (ESC on the keyboard aborts, 10 s silence ends it).\n", CYAN);

    oq.qHead = oq.qTail = 0; oq.color = WHITE;
    st.recOk = st.recBad = st.bytes = 0; st.lineNo = 1;
    st.lo = 0xFFFFFFFFUL; st.hi = 0; st.errLines = 0; st.gotEnd = 0;

    /* From here on nothing may stall: mute the serial echo and drain stale input. */
    globals->SerNoEcho = 1;
    globals->SerBufLen = 0;
    srecProgress(&oq, &st);

    int      state = ST_WAIT_S, type = 0, count = 0, got = 0, hiNib = -1;
    uint32_t sum = 0;
    uint32_t idleLines = 0, keyPoll = 0;
    uint8_t  lastLine = *(volatile uint8_t *)0xDFF006;   /* VHPOSR high byte: raster line */
    int      seenData = 0, aborted = 0, timedOut = 0;

    for (;;) {
        readSerial();
        int b = srecPopByte();
        if (b < 0) {
            /* idle: emit one queued char, then keep time via the raster line counter */
            if (oqDrainOne(&oq)) continue;
            uint8_t line = *(volatile uint8_t *)0xDFF006;
            if (line != lastLine) {
                lastLine = line;
                idleLines++; keyPoll++;
                if (keyPoll >= SREC_KEYPOLL) {
                    keyPoll = 0;
                    if (getCharKey() == 27) { aborted = 1; break; }
                }
                if (seenData && idleLines >= SREC_IDLE_LINES) { timedOut = 1; break; }
            }
            continue;
        }
        idleLines = 0;
        seenData = 1;

        if (b == '\n') st.lineNo++;

        switch (state) {
        case ST_WAIT_S:
            if (b == 'S' || b == 's') { state = ST_TYPE; }
            break;

        case ST_TYPE:
            type = b;
            if (!((b >= '0' && b <= '9'))) { srecError(&oq, &st, "bad record type"); state = ST_WAIT_S; break; }
            state = ST_COUNT; hiNib = -1; sum = 0; got = 0;
            break;

        case ST_COUNT:
        case ST_BODY: {
            int v = hexVal((uint8_t)b);
            if (v < 0) {
                /* anything non-hex inside a record = damaged line; resync on next 'S' */
                if (b != '\r' && b != '\n') srecError(&oq, &st, "non-hex character in record");
                else                        srecError(&oq, &st, "record truncated");
                state = ST_WAIT_S;
                break;
            }
            if (hiNib < 0) { hiNib = v; break; }
            uint8_t byte = (uint8_t)((hiNib << 4) | v);
            hiNib = -1;
            if (state == ST_COUNT) {
                count = byte; sum = byte; got = 0;
                if (count < 1) { srecError(&oq, &st, "zero count"); state = ST_WAIT_S; break; }
                state = ST_BODY;
                break;
            }
            rec[got++] = byte;
            if (got < count) { sum += byte; break; }

            /* record complete: rec[0..count-2] = address+data, rec[count-1] = checksum */
            state = ST_WAIT_S;
            if (((sum + byte) & 0xFF) != 0xFF) { srecError(&oq, &st, "checksum mismatch"); break; }

            int addrLen;
            switch (type) {
                case '1': addrLen = 2; break;
                case '2': addrLen = 3; break;
                case '3': addrLen = 4; break;
                case '7': case '8': case '9':
                    st.gotEnd = 1; st.recOk++;
                    addrLen = -1; break;
                case '0': case '5': case '6':
                    st.recOk++; addrLen = -1; break;   /* header / counts: nothing to write */
                default:
                    srecError(&oq, &st, "unknown record type"); addrLen = -1; break;
            }
            if (addrLen < 0) {
                if (st.gotEnd) goto done;
                break;
            }
            if (count - 1 < addrLen) { srecError(&oq, &st, "record shorter than its address"); break; }

            uint32_t addr = 0;
            for (int i = 0; i < addrLen; i++) addr = (addr << 8) | rec[i];
            addr += (uint32_t)base;
            int dataLen = count - 1 - addrLen;
            volatile uint8_t *dst = (volatile uint8_t *)(uintptr_t)addr;
            for (int i = 0; i < dataLen; i++) dst[i] = rec[addrLen + i];
            if (dataLen) {
                if (addr < st.lo) st.lo = addr;
                if (addr + (uint32_t)dataLen - 1 > st.hi) st.hi = addr + (uint32_t)dataLen - 1;
            }
            st.bytes += (uint32_t)dataLen;
            st.recOk++;
            if ((st.recOk & 15) == 0) srecProgress(&oq, &st);
            break;
        }
        }
    }
done:
    /* flush whatever progress text is still queued, then speak normally again */
    while (oqDrainOne(&oq)) ;
    srecProgress(&oq, &st);
    while (oqDrainOne(&oq)) ;
    globals->SerNoEcho = 0;

    setPos(0, SREC_ERROR_ROW + SREC_MAX_ERRLINES + 1);
    if (aborted)        print("Aborted by ESC.\n", RED);
    else if (timedOut)  print("No end record (S7/S8/S9) - stopped after 10 s of silence.\n", YELLOW);
    else                print("End record received.\n", GREEN);
    print("Records OK: ", WHITE); print(binDec((int32_t)st.recOk), GREEN);
    print("  bad: ", WHITE);      print(binDec((int32_t)st.recBad), st.recBad ? RED : GREEN);
    print("  bytes written: ", WHITE); print(binDec((int32_t)st.bytes), CYAN);
    if (st.bytes) {
        print("\nAddress range written: ", WHITE); print(binHex(st.lo), CYAN);
        print(" - ", WHITE); print(binHex(st.hi), CYAN);
    }
    print("\n\nPress any key/button to return to Setup.\n", WHITE);

    /* let any trailing newline/CR of the file arrive, then discard it so it
       cannot dismiss the prompt by itself */
    for (int i = 0; i < 200; i++) { readSerial(); waitShort(); }
    globals->SerBufLen = 0;
    WaitButton();
}
