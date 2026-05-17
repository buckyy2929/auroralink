#!/usr/bin/env bash
# Create GitHub Milestone 2 and four tracking issues for protocol_translator.
# Prerequisites: gh auth login, push access to IOTSecurityHQ/protocol_translator

set -euo pipefail

REPO="${GITHUB_REPO:-IOTSecurityHQ/protocol_translator}"
MILESTONE_TITLE="Milestone 2 — Protocol deep-dive & bridge design"
MILESTONE_DESC="AuroraLink + libEGD investigation, A53 cross-compile, bidirectional bridge application design. See docs/MILESTONE_2_TASKS.md."

if ! gh auth status &>/dev/null; then
  echo "Error: gh is not authenticated. Run: gh auth login" >&2
  exit 1
fi

echo "Creating milestone..."
MILESTONE_URL=$(gh api "repos/${REPO}/milestones" \
  -f title="$MILESTONE_TITLE" \
  -f description="$MILESTONE_DESC" \
  -f state=open \
  --jq '.html_url')

MILESTONE_NUMBER=$(gh api "repos/${REPO}/milestones" --jq ".[] | select(.title==\"$MILESTONE_TITLE\") | .number" | head -1)
echo "Milestone: $MILESTONE_URL (#$MILESTONE_NUMBER)"

create_issue() {
  local title="$1"
  local body_file="$2"
  local labels="$3"

  gh issue create \
    --repo "$REPO" \
    --title "$title" \
    --body-file "$body_file" \
    --label "$labels" \
    --milestone "$MILESTONE_NUMBER"
}

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# --- M2-A ---
cat > "$TMP/m2a.md" <<'EOF'
## Objective

Gain a working understanding of **AuroraLink** on Kria: how data moves from FPGA (Aurora IP + fabric) to the **Cortex-A53** application, and how the reference CHIL code interacts with the AXI FIFO.

## Activities

- [ ] Study `RTDS_Aurora_Link/Kria_CHIL_Firmware/PEBB_CHIL/xllfifo_interrupt_example_1/` (`Axi_IO.c`, `main.c`, FIFO ISRs)
- [ ] Read RTDS Simulator documentation for AuroraLink (capture manual URL in deliverable)
- [ ] Review Xilinx Aurora PG / integration docs for PL vs PS responsibilities
- [ ] Document IP-core / FIFO configuration relevant to our bitstream
- [ ] Use AI-assisted walkthroughs; **cite sources** for every conclusion

## Key questions

1. How does AuroraLink work end-to-end on our platform?
2. **Push vs pull:** Does the IP push into PS memory, or does software poll AXI regions? Distinguish PL stream push vs CPU FIFO read vs interrupt notification.
3. What defines a **logical packet/frame** for software?
4. What **configuration parameters** exist for the IP core and bridge?

## Deliverable

`docs/milestone2/M2-A_auroralink_findings.md` (≤3 pages) with explicit push/pull answer and source citations.

## References

- [docs/MILESTONE_2_TASKS.md](https://github.com/IOTSecurityHQ/protocol_translator/blob/main/docs/MILESTONE_2_TASKS.md)
- [docs/BRIDGE_SOFTWARE_DESIGN.md](https://github.com/IOTSecurityHQ/protocol_translator/blob/main/docs/BRIDGE_SOFTWARE_DESIGN.md) §2.3
- In-repo: `RTDS_Aurora_Link/`
EOF

# --- M2-B ---
cat > "$TMP/m2b.md" <<'EOF'
## Objective

Understand **libEGD**: how EGD messages are sent/received over UDP, which APIs to use, typical payload sizes, and sync vs async behavior.

## Activities

- [ ] Deep-read `libEGD-master/` (README, examples, client implementation)
- [ ] Compare with `EgdSend.c` / `EgdTest.h` (port 18246, 1400-byte cap in-tree)
- [ ] Trace one **publish** and one **subscribe** path call-by-call
- [ ] Document JsonClient vs EgdClient recommendation
- [ ] Incorporate official EGD spec when available

## Key questions

1. Encode/decode path for production and consumption?
2. Functions to **send** — what runs internally?
3. Functions to **receive** — callback, block, or poll?
4. Typical **EGD data size** limits?
5. **Synchronous or asynchronous** API model?

## Deliverable

`docs/milestone2/M2-B_libegd_findings.md` with API cheat sheet and call-flow diagram.

## References

- `libEGD-master/README.md`, `src/examples/`
- `EgdSend.c`, `EgdTest.h`
EOF

# --- M2-C ---
cat > "$TMP/m2c.md" <<'EOF'
## Objective

Build **libEGD** for **ARM64 Linux on Kria Cortex-A53** (Ubuntu in PS), not R5 or FPGA.

## Activities

- [ ] Inventory CMake/Docker deps (`libEGD-master/`, `Dockerfile.builder`)
- [ ] Choose cross-compile vs on-board native vs SDK approach
- [ ] Document toolchain, sysroot, and install layout
- [ ] Produce at least one runnable example on A53
- [ ] Record smoke-test command and result

## Deliverable

`docs/milestone2/M2-C_cross_compile.md` with exact build commands and verification steps.

## Depends on

Rough understanding from M2-B (can start in parallel once build files are identified).
EOF

# --- M2-D ---
cat > "$TMP/m2d.md" <<'EOF'
## Objective

Design the **bidirectional bridge application**: Aurora (AXI/FIFO) ↔ translation ↔ EGD (libEGD), including scheduling and buffer model.

## Activities

- [ ] Module split: Aurora adapter, translation core, EGD adapter (see BRIDGE_SOFTWARE_DESIGN.md)
- [ ] Evaluate: single-thread round-robin vs multi-thread event-driven vs sidecar process
- [ ] Define shared buffers (latest-value, timestamps, VALID/STALE/INVALID per DESIGN_INTENT.md)
- [ ] Per-boundary sync vs async decision table
- [ ] Thread/ISR diagram and degraded-mode behavior

## Key questions

1. How to avoid starving Aurora RX or EGD RX under load?
2. Where may blocking calls occur?
3. ISR vs worker thread for FIFO handling?
4. Reconcile event-driven Aurora with cyclic EGD publish?

## Deliverable

`docs/milestone2/M2-D_application_design.md` with architecture diagram and explicit threading recommendation.

## Depends on

Draft findings from M2-A, M2-B, and constraints from M2-C.
EOF

# Ensure labels exist (ignore errors if already present)
for label in "milestone-2:FBCA04" "aurora:1D76DB" "egd:0E8A16" "documentation:0075CA" "build:D4C5F9" "kria:C5DEF5" "design:EDEDED"; do
  name="${label%%:*}"
  color="${label##*:}"
  gh label create "$name" --repo "$REPO" --color "$color" 2>/dev/null || true
done

echo "Creating issues..."
create_issue "M2-A: AuroraLink protocol & IP-core investigation" "$TMP/m2a.md" "milestone-2,aurora,documentation"
create_issue "M2-B: libEGD deep dive & API map" "$TMP/m2b.md" "milestone-2,egd,documentation"
create_issue "M2-C: Cross-compile libEGD for Kria A53" "$TMP/m2c.md" "milestone-2,build,kria"
create_issue "M2-D: Bridge application architecture design" "$TMP/m2d.md" "milestone-2,design"

echo "Done. Milestone: $MILESTONE_URL"
gh issue list --repo "$REPO" --milestone "$MILESTONE_TITLE" --limit 10
