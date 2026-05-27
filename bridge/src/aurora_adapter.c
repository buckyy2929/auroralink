/* ----------------------------------------------------------------------------
 * aurora_adapter.c -- STUB IMPLEMENTATION
 *
 * Every function here returns AURORA_ERR_NOT_IMPLEMENTED. The shape of the
 * interface and the error-reporting contract are stable; the real
 * implementation (UIO open, mmap, RX thread, YAML loader, field-map decode,
 * TX assembly) is M2-D scope and lands together with the libEGD publisher
 * wiring.
 *
 * The stub exists so that:
 *   - The bridge directory has something that compiles cleanly with the
 *     M2-C aarch64 cross-toolchain (sanity-check the include path / ABI).
 *   - Reviewers can see the contract is not vapourware; the header is real
 *     and there is a translation unit you can compile.
 *   - Downstream callers (M2-D supervisor, the EGD publisher) can link
 *     against the symbols and develop in parallel; they will get
 *     AURORA_ERR_NOT_IMPLEMENTED at run time until the impl lands.
 *
 * NOTHING in this file pretends to work on hardware. There is no I/O,
 * no thread, no YAML loader. The stub is honest about that.
 *
 * Coding standard notes (kept simple by design):
 *   - C11, no external deps beyond libc + libpthread (only when impl lands).
 *   - All public symbols start with aurora_*.
 *   - The opaque struct is forward-declared in the header; in the stub it
 *     is intentionally never instantiated (out_adapter is always set to NULL).
 * --------------------------------------------------------------------------*/

#include "aurora_adapter.h"

#include <stddef.h>

/* Mark unused params explicitly to keep -Wunused-parameter happy. */
#define AA_UNUSED(x) ((void)(x))

aurora_status_t
aurora_adapter_create(const char *yaml_path, aurora_adapter_t **out_adapter)
{
    if (out_adapter == NULL) {
        return AURORA_ERR_BAD_ARG;
    }
    *out_adapter = NULL;
    AA_UNUSED(yaml_path);
    return AURORA_ERR_NOT_IMPLEMENTED;
}

void
aurora_adapter_destroy(aurora_adapter_t *adapter)
{
    AA_UNUSED(adapter);
    /* Stub: no resources to release. Safe no-op. */
}

aurora_status_t
aurora_adapter_start(aurora_adapter_t      *adapter,
                     const char            *uio_path,
                     aurora_on_frame_cb_t   on_frame,
                     void                  *user)
{
    if (adapter == NULL || uio_path == NULL) {
        return AURORA_ERR_BAD_ARG;
    }
    AA_UNUSED(on_frame);
    AA_UNUSED(user);
    return AURORA_ERR_NOT_IMPLEMENTED;
}

aurora_status_t
aurora_adapter_stop(aurora_adapter_t *adapter)
{
    if (adapter == NULL) {
        return AURORA_ERR_BAD_ARG;
    }
    return AURORA_ERR_NOT_IMPLEMENTED;
}

aurora_status_t
aurora_adapter_send_tx(aurora_adapter_t        *adapter,
                       const aurora_tx_field_t *fields,
                       size_t                   field_count)
{
    if (adapter == NULL || (fields == NULL && field_count > 0)) {
        return AURORA_ERR_BAD_ARG;
    }
    AA_UNUSED(field_count);
    return AURORA_ERR_NOT_IMPLEMENTED;
}

aurora_status_t
aurora_adapter_get_stats(const aurora_adapter_t *adapter,
                         aurora_adapter_stats_t *out_stats)
{
    if (adapter == NULL || out_stats == NULL) {
        return AURORA_ERR_BAD_ARG;
    }
    /* Zero out so the caller does not see uninitialized counters. */
    out_stats->rx_frames_ok                = 0;
    out_stats->rx_frames_length_mismatch   = 0;
    out_stats->rx_frames_nan               = 0;
    out_stats->rx_irq_overrun              = 0;
    out_stats->tx_frames_sent              = 0;
    out_stats->tx_frames_failed            = 0;
    out_stats->last_rx_inter_arrival_ns    = 0;
    out_stats->channel_up_since_ns         = 0;
    return AURORA_ERR_NOT_IMPLEMENTED;
}

const char *
aurora_status_string(aurora_status_t s)
{
    switch (s) {
    case AURORA_OK:                       return "AURORA_OK";
    case AURORA_ERR_NOT_IMPLEMENTED:      return "AURORA_ERR_NOT_IMPLEMENTED (stub; see bridge/src/aurora_adapter.c)";
    case AURORA_ERR_BAD_ARG:              return "AURORA_ERR_BAD_ARG (NULL or out-of-range parameter)";
    case AURORA_ERR_CONFIG_LOAD:          return "AURORA_ERR_CONFIG_LOAD (YAML missing / unreadable / parse error)";
    case AURORA_ERR_CONFIG_SCHEMA:        return "AURORA_ERR_CONFIG_SCHEMA (word_count mismatch or unknown type)";
    case AURORA_ERR_UIO_OPEN:             return "AURORA_ERR_UIO_OPEN (/dev/uioN not accessible -- check DTBO load)";
    case AURORA_ERR_FIFO_MMAP:            return "AURORA_ERR_FIFO_MMAP (mmap of FIFO BAR failed)";
    case AURORA_ERR_FIFO_LENGTH_MISMATCH: return "AURORA_ERR_FIFO_LENGTH_MISMATCH (frame length != configured word_count)";
    case AURORA_ERR_FIFO_OVERRUN:         return "AURORA_ERR_FIFO_OVERRUN (new RC IRQ before previous frame drained)";
    case AURORA_ERR_NAN_IN_FRAME:         return "AURORA_ERR_NAN_IN_FRAME (float field with NaN bit pattern)";
    case AURORA_ERR_INTERNAL:             return "AURORA_ERR_INTERNAL";
    }
    return "AURORA_<unknown status>";
}
