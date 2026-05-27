# Milestone 2 — Task files

Track work using these files (not GitHub Issues). Overview and dependency graph: [MILESTONE_2_TASKS.md](../MILESTONE_2_TASKS.md).

## Task overview + deliverables

| Task | Spec | Deliverable (English — reviewer/customer) | Hindi mirror (internal — for our own understanding, with train metaphor) |
| ---- | ---- | ------------------------------------------ | ------------------------------------------------------------------------- |
| **M2-A** | [M2-A_TASK.md](M2-A_TASK.md) | [`M2-A_auroralink_findings.md`](M2-A_auroralink_findings.md) | [`M2-A_auroralink_findings_hindi.md`](M2-A_auroralink_findings_hindi.md) |
| **M2-B** | [M2-B_TASK.md](M2-B_TASK.md) | [`M2-B_libegd_findings.md`](M2-B_libegd_findings.md) | [`M2-B_libegd_findings_hindi.md`](M2-B_libegd_findings_hindi.md) |
| **M2-C** | [M2-C_TASK.md](M2-C_TASK.md) | [`M2-C_cross_compile.md`](M2-C_cross_compile.md) | [`M2-C_cross_compile_hindi.md`](M2-C_cross_compile_hindi.md) |
| **M2-D** | [M2-D_TASK.md](M2-D_TASK.md) | [`M2-D_application_design.md`](M2-D_application_design.md) | [`M2-D_application_design_hindi.md`](M2-D_application_design_hindi.md) |

## Dependency graph

```text
M2-A ──┐
M2-B ──┼──► M2-D
M2-C ──┘
```

M2-A and M2-B can run in parallel. M2-D starts after draft A/B findings.

## Doc convention

For every English deliverable kept in this folder, there is a co-located `*_hindi.md` mirror written in simple Hindi with a train / railway-station metaphor. The Hindi mirrors are **for internal use only** (so we can each understand every piece of the design from scratch); they are not part of the customer/reviewer deliverable. The English documents are the source of truth — Hindi mirrors are derived explanations.

Task spec files (`M2-*_TASK.md`) and the project plan (`../MILESTONE_2_TASKS.md`) stay English-only — they are terse acceptance-criteria documents aimed at the reviewer.

## Other Hindi pairings in the repo

| Topic | English | Hindi |
| ----- | ------- | ----- |
| Bridge architecture | [`../BRIDGE_SOFTWARE_DESIGN.md`](../BRIDGE_SOFTWARE_DESIGN.md) | [`../BRIDGE_SOFTWARE_DESIGN_hindi.md`](../BRIDGE_SOFTWARE_DESIGN_hindi.md) |
| Full system deep-dive (RTDS → Aurora → FIFO → bridge → EGD) | `../../explaination_docs/detailed_explaination.md` | `../../explaination_docs/detailed_explaination_hindi.md` |
| KR260 board / AuroraLink setup guide | `../../explaination_docs/KR260_auroralink.md` | `../../explaination_docs/KR260_auroralink_hindi.md` |
