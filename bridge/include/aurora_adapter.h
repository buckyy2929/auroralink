/* ----------------------------------------------------------------------------
 * aurora_adapter.h
 *
 * PS-side adapter for the Aurora link on KR260.
 *
 * Scope of this header:
 *   This is the M2-A deliverable described in docs/milestone2/M2-A_auroralink_findings.md
 *   section 7 ("Configurable Field Map") and section 8 step 5 ("Implement
 *   aurora_adapter PS module"). It declares the public interface only --
 *   the runtime implementation in bridge/src/aurora_adapter.c is intentionally
 *   a stub (every function returns AURORA_ERR_NOT_IMPLEMENTED) until M2-D
 *   wires it against the cross-compiled libEGD (M2-C) and the field map at
 *   config/aurora_fieldmap.example.yaml is parsed by a real loader.
 *
 *   The header is committed early so:
 *     (a) the reviewer can see the contract this module promises;
 *     (b) the M2-D author starts against a stable interface;
 *     (c) the M2-C cross-compile flow has something concrete to link against
 *         when integration begins.
 *
 * Concurrency model (planned, not yet implemented):
 *   The Aurora receive path is interrupt-driven on the PL side and word-pull
 *   on the PS side (see docs/milestone2/M2-A_auroralink_findings.md \u00a73 and
 *   docs/BRIDGE_SOFTWARE_DESIGN.md \u00a72.3). Under Linux on the A53 this
 *   maps to a thread blocked on poll(/dev/uioN); when the UIO driver
 *   reports an interrupt, the thread calls aurora_adapter_drain_rx() which
 *   reads N words from the FIFO MMIO, decodes them per the loaded field
 *   map, and invokes the registered on_frame callback.
 *
 *   The on_frame callback runs in the adapter's RX thread, NOT in interrupt
 *   context. Callers must not block in the callback; instead they post to
 *   the next stage (EGD publisher) via a lock-free latest-value buffer.
 *
 * What is intentionally NOT defined here:
 *   - The concrete PEBB_Control_U / _Y struct layouts. The adapter does NOT
 *     write directly into the controller's structs (that is the CHIL
 *     reference's pattern). It writes into a translator-owned, name-keyed
 *     buffer; the EGD publisher then reads by name. This decoupling is the
 *     whole point of the "configurable field map".
 *
 * Sources reviewed when designing this interface:
 *   - RTDS_Aurora_Link/.../xllfifo_interrupt_example_1/src/Axi_IO.c (the
 *     existing bare-metal pattern -- type-puning, per-word descriptors).
 *   - RTDS_Aurora_Link/.../xllfifo_interrupt_example_1/src/Axi_IO.h
 *     (RECEIVE_LENGTH=37, TRANSMIT_LENGTH=23, WORD_SIZE=4).
 *   - Xilinx PG080 (axi_fifo_mm_s) for the FIFO register interface.
 *   - Xilinx UIO subsystem docs for the Linux IRQ delivery path.
 * --------------------------------------------------------------------------*/

#ifndef AURORA_ADAPTER_H_
#define AURORA_ADAPTER_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------
 * Error codes
 *
 * Negative on failure, AURORA_OK (0) on success. Callers must check the
 * return value of every aurora_adapter_* function. No errno usage; this
 * module is bare-metal- and Linux-portable.
 * --------------------------------------------------------------------------*/
typedef enum {
    AURORA_OK                       =  0,
    AURORA_ERR_NOT_IMPLEMENTED      = -1, /* stub; real impl pending M2-D */
    AURORA_ERR_BAD_ARG              = -2, /* NULL or out-of-range parameter */
    AURORA_ERR_CONFIG_LOAD          = -3, /* YAML missing, parse error, or schema mismatch */
    AURORA_ERR_CONFIG_SCHEMA        = -4, /* rx.word_count != 37, tx.word_count != 23, etc. */
    AURORA_ERR_UIO_OPEN             = -5, /* /dev/uioN not accessible */
    AURORA_ERR_FIFO_MMAP            = -6, /* mmap of FIFO BAR failed */
    AURORA_ERR_FIFO_LENGTH_MISMATCH = -7, /* received frame length != configured word_count */
    AURORA_ERR_FIFO_OVERRUN         = -8, /* new RC IRQ while previous frame still draining */
    AURORA_ERR_NAN_IN_FRAME         = -9, /* float field with NaN bit pattern; matches CHIL behavior */
    AURORA_ERR_INTERNAL             = -99
} aurora_status_t;

/* ----------------------------------------------------------------------------
 * Wire types (matches Axi_IO.c type_ enum: float_, s32_, u32_)
 * --------------------------------------------------------------------------*/
typedef enum {
    AURORA_WIRE_S32     = 0,
    AURORA_WIRE_U32     = 1,
    AURORA_WIRE_FLOAT32 = 2
} aurora_wire_type_t;

typedef enum {
    AURORA_LOCAL_S32     = 0,
    AURORA_LOCAL_U32     = 1,
    AURORA_LOCAL_FLOAT32 = 2,
    AURORA_LOCAL_FLOAT64 = 3,
    AURORA_LOCAL_PACKED_BITS = 4   /* for the TX bit-pack word (see Axi_IO.c packBits) */
} aurora_local_type_t;

/* ----------------------------------------------------------------------------
 * Decoded field value, delivered to the on_frame callback.
 *
 * The local_type tag tells the consumer which union member is valid for
 * a given field. The adapter has already performed:
 *   1. Wire->local reinterpretation (e.g. raw u32 -> IEEE 754 float).
 *   2. Scale-factor multiplication.
 *   3. NaN detection (for float fields).
 *
 * The name pointer's lifetime is tied to the adapter (it points into the
 * loaded config); do NOT free it from the callback.
 * --------------------------------------------------------------------------*/
typedef struct {
    const char         *name;     /* e.g. "i_high", "P_CMD"; never NULL */
    aurora_wire_type_t  wire_type;
    aurora_local_type_t local_type;
    union {
        int32_t  i32;
        uint32_t u32;
        float    f32;
        double   f64;
        uint32_t bits;            /* AURORA_LOCAL_PACKED_BITS: raw 32-bit pattern */
    } value;
    int is_nan;                   /* 1 only if wire_type == FLOAT32 and value is NaN */
} aurora_field_t;

/* Whole decoded RX frame, passed to the on_frame callback. */
typedef struct {
    uint32_t              seq;            /* sequence number from last RX word */
    size_t                field_count;    /* equals rx.word_count from config */
    const aurora_field_t *fields;         /* owned by adapter; do not free */
    uint64_t              host_ns;        /* CLOCK_MONOTONIC at RC IRQ wakeup */
} aurora_rx_frame_t;

/* TX field, populated by the caller; passed to aurora_adapter_send_tx().
 * Either name OR word_index must be set; if both are set, name takes
 * precedence (more forgiving against frame layout changes). */
typedef struct {
    const char         *name;      /* may be NULL if word_index >= 0 */
    int                 word_index;/* -1 if not used */
    aurora_local_type_t local_type;
    union {
        int32_t  i32;
        uint32_t u32;
        float    f32;
        double   f64;
        uint32_t bits;
    } value;
} aurora_tx_field_t;

/* Callback type for delivered RX frames. */
typedef void (*aurora_on_frame_cb_t)(const aurora_rx_frame_t *frame, void *user);

/* ----------------------------------------------------------------------------
 * Opaque handle. The real implementation will allocate and own:
 *   - the parsed config (field map)
 *   - the UIO file descriptor + mmap'd FIFO BAR
 *   - the RX thread + lock-free decoded-frame buffer
 *   - per-field scratch storage for decode
 * --------------------------------------------------------------------------*/
typedef struct aurora_adapter aurora_adapter_t;

/* ----------------------------------------------------------------------------
 * Lifecycle
 * --------------------------------------------------------------------------*/

/* Create an adapter from a YAML field-map file (e.g. config/aurora_fieldmap.example.yaml).
 *
 * On success: *out_adapter is a non-NULL handle owned by the caller; returns
 *   AURORA_OK.
 * On failure: *out_adapter is set to NULL; returns one of the
 *   AURORA_ERR_CONFIG_* codes; no resources are leaked.
 *
 * Does NOT touch /dev/uio or any hardware -- it only loads + validates
 * the YAML. Call aurora_adapter_start() to begin RX/TX.
 */
aurora_status_t
aurora_adapter_create(const char *yaml_path, aurora_adapter_t **out_adapter);

/* Destroy the adapter. Safe to call on a NULL pointer (no-op). If the
 * adapter is started, aurora_adapter_stop() is invoked internally first. */
void
aurora_adapter_destroy(aurora_adapter_t *adapter);

/* Bind the adapter to a UIO device (e.g. "/dev/uio0") and start the RX
 * thread. The given on_frame callback is invoked once per validated RX
 * frame; it must NOT block. Pass NULL for on_frame to drop frames.
 *
 * Returns AURORA_OK once the RX thread is running and ready for the first
 * RC IRQ. On any failure, the adapter is left in the "created, not started"
 * state and aurora_adapter_destroy is still the correct cleanup. */
aurora_status_t
aurora_adapter_start(aurora_adapter_t      *adapter,
                     const char            *uio_path,
                     aurora_on_frame_cb_t   on_frame,
                     void                  *user);

/* Stop the RX thread and release UIO/mmap resources. Safe to call multiple
 * times; subsequent calls return AURORA_OK and are no-ops. */
aurora_status_t
aurora_adapter_stop(aurora_adapter_t *adapter);

/* ----------------------------------------------------------------------------
 * Transmit path
 *
 * Caller fills an aurora_tx_field_t[] (in any order; name OR word_index
 * identifies the slot). aurora_adapter_send_tx() looks up each field in
 * the loaded TX schema, performs local->wire conversion + scale division,
 * assembles the 23-word frame in the TX FIFO, sets the length register,
 * and returns.
 *
 * Sequence number is managed by the adapter -- callers MUST NOT set the
 * "seq" field; if they do, it is overwritten.
 * --------------------------------------------------------------------------*/
aurora_status_t
aurora_adapter_send_tx(aurora_adapter_t        *adapter,
                       const aurora_tx_field_t *fields,
                       size_t                   field_count);

/* ----------------------------------------------------------------------------
 * Introspection (for diagnostics + the M2-D supervisor / health endpoint).
 * --------------------------------------------------------------------------*/
typedef struct {
    uint64_t rx_frames_ok;
    uint64_t rx_frames_length_mismatch;
    uint64_t rx_frames_nan;
    uint64_t rx_irq_overrun;          /* RC IRQ while previous frame still draining */
    uint64_t tx_frames_sent;
    uint64_t tx_frames_failed;
    uint64_t last_rx_inter_arrival_ns;/* updated every successful RX */
    uint64_t channel_up_since_ns;     /* 0 if link is down */
} aurora_adapter_stats_t;

aurora_status_t
aurora_adapter_get_stats(const aurora_adapter_t *adapter,
                         aurora_adapter_stats_t *out_stats);

/* Human-readable name for an aurora_status_t, for log messages. Never NULL. */
const char *
aurora_status_string(aurora_status_t s);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AURORA_ADAPTER_H_ */
