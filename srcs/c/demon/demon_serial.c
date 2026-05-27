/* ============================================================================
 * demon_serial.c — DeMoN II USB-C (FT245) serial driver
 *
 * This file provides a drop-in replacement for the Amiga internal SERDAT
 * serial port output in srcs/c/amiga/amiga.c, redirected to the FT245
 * USB FIFO on the DeMoN II cartridge.
 *
 * Hardware registers (from DeMoN.v):
 *   $BBFFF0/F1   USB serial DATA register (byte access, LDS)
 *   $BBFFF2      USB serial STATUS register
 *     bit 0 = ft_txe : 1 = TX FIFO FULL,  0 = ready to accept byte
 *     bit 1 = ft_rxf : 1 = RX FIFO empty, 0 = byte available to read
 *
 * Writing to $BBFFF2 triggers ft_siwu (send immediate) on the FT245,
 * which forces the chip to flush its internal latency buffer.  This is
 * useful after sending a logical "chunk" of diagnostic output.
 *
 * Activation: this file is compiled into the build only when
 *   -DTARGET_DEMON=1
 * is passed to gcc (set by Makefile.demon).  See README_DEMON.md.
 *
 * To switch the runtime serial backend, this file defines rs232_out()
 * and readSerial() with the same prototypes as amiga.c, and the build
 * system EXCLUDES amiga.c's versions via -DTARGET_DEMON guard inside
 * amiga.c (see patch file demon_amiga_overlay.c).
 * ========================================================================== */

#include "c/generic.h"
#include "c/platform.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef TARGET_DEMON

/* ----- DeMoN cartridge register addresses ----- */
#define DEMON_USB_DATA  (*(volatile uint8_t  *)0xbbfff1)  /* odd addr = LDS */
#define DEMON_USB_STAT  (*(volatile uint16_t *)0xbbfff2)
#define DEMON_USB_SIWU  (*(volatile uint16_t *)0xbbfff2)  /* write triggers SIWU */

#define USB_TX_FULL   0x0001    /* bit 0 of status: 1 = TX FIFO full   */
#define USB_RX_EMPTY  0x0002    /* bit 1 of status: 1 = RX FIFO empty  */

/* ============================================================================
 * Low-level FT245 primitives
 * ========================================================================== */

static inline bool usb_tx_ready(void)
{
    return (DEMON_USB_STAT & USB_TX_FULL) == 0;
}

static inline bool usb_rx_available(void)
{
    return (DEMON_USB_STAT & USB_RX_EMPTY) == 0;
}

/* ============================================================================
 * rs232_out — public API, called by sendSerial() and putChar() in Diagrom.
 *
 * Same prototype as the Amiga SERDAT version in srcs/c/amiga/amiga.c, so
 * the rest of Diagrom (which calls rs232_out hundreds of times) doesn't
 * need to change at all.
 *
 * NOTE on the timeout: the FT245 TX FIFO is 256 bytes deep.  If the host
 * PC has not opened the serial port, the FIFO will eventually fill and
 * usb_tx_ready() will never return true.  We use a finite (large) timeout
 * to avoid hanging Diagrom forever in that case; if it expires we just
 * drop the byte.  The NoSerial-style "silent fail after N timeouts"
 * mechanism from earlystart.s is therefore preserved in spirit.
 * ========================================================================== */

/* Sticky flag in DeMoN RAM: 0=USB OK, 1=USB disconnected/full (drop fast).
 * Matches the same flag used by usb_putc in demon_boot.s ($BBFFE0). */
#define USB_STICKY_FLAG (*(volatile uint8_t *)0xBBFFE0)

void rs232_out(char character __asm("d0"))
{
    /* Diagrom uses globals->NoSerial to disable serial entirely.  Honour it. */
    if (globals->NoSerial == 1)
        return;

    /* Sticky-disconnect: if a previous call timed out, probe once and
       either recover or drop instantly without any wait loop. */
    if (USB_STICKY_FLAG != 0) {
        if (usb_tx_ready()) {
            USB_STICKY_FLAG = 0;
            DEMON_USB_DATA = (uint8_t)character;
        }
        return;
    }

    /* Normal path: wait for TX FIFO room with a short timeout. */
    uint32_t timeout = 0x1000;     /* ~140us, matches usb_putc */
    while (timeout-- != 0) {
        if (usb_tx_ready()) {
            DEMON_USB_DATA = (uint8_t)character;
            return;
        }
    }
    /* Timeout: mark USB disconnected. Subsequent calls drop fast. */
    USB_STICKY_FLAG = 1;
}

/* ============================================================================
 * readSerial — public API, called by various polling loops.
 *
 * Reads one byte from FT245 RX FIFO if available, pushes into globals
 * SerBuf, sets SerData / BUTTON flags.  Matches the semantics of the
 * SERDAT-based version in amiga.c.
 * ========================================================================== */

void readSerial(void)
{
    if (!usb_rx_available())
        return;

    uint8_t data = DEMON_USB_DATA;

    globals->SerData   = 1;
    globals->OldSerial = data;
    globals->BUTTON    = 1;

    uint8_t bufpos = globals->SerBufLen;
    globals->SerBufLen = bufpos + 1;
    if (bufpos < sizeof(globals->SerBuf))
        globals->SerBuf[bufpos] = data;
}

/* ============================================================================
 * setSerialPort — public API, called once during init.
 *
 * The Amiga version configures SERPER baud rate.  The FT245 doesn't have
 * a baud rate concept (it's a USB FIFO bridge — the "baud rate" the host
 * sees is the USB throughput, typically several Mbps).  So this is a
 * no-op, but we drain any stale RX data just to be tidy.
 * ========================================================================== */

void setSerialPort(void)
{
    /* Drain RX FIFO of any leftover bytes from previous session */
    uint32_t safety = 4096;
    while (usb_rx_available() && safety-- > 0) {
        (void)DEMON_USB_DATA;
    }
    /* Trigger SIWU once to ensure any in-flight TX is flushed */
    DEMON_USB_SIWU = 0;
}

/* ============================================================================
 * flushSerial — bonus public API, can be called after blocks of output
 * to force the FT245 to send its latency buffer.
 * ========================================================================== */

void flushSerial(void)
{
    DEMON_USB_SIWU = 0;
}

#endif /* TARGET_DEMON */

#ifdef TARGET_DEMON
/* Stub: FT245 doesn't need init, but generic.s calls initSerial() */
void initSerial(void)
{
    /* No-op: FT245 is always ready when powered.
     * setSerialPort() (called separately) drains stale RX data. */
}
#endif
