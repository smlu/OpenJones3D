# ADR-0014: Architecture documentation in arc42

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

The existing documentation described a reconstruction: subsystem pages, a testing guide for hooks and
reference binaries. The new direction needs goals, constraints and decisions written down.

## Decision

Architecture documentation follows arc42 in `Docs/arc42/`, one file per chapter. Decisions are ADRs in this
directory in Nygard format. Diagrams are Mermaid. Formats, COG, configuration and the roadmap stay outside
arc42. The subsystem pages move into chapters 5 and 6.

## Consequences

- Every lasting decision gets an ADR. Superseded ADRs stay, marked as superseded.
- Mermaid renders on GitHub without a build step.
