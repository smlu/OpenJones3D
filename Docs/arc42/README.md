# Jones3D EX architecture

This is the architecture documentation of Jones3D EX, written to the [arc42](https://arc42.org) template.
One file per chapter. Diagrams are [Mermaid](https://mermaid.js.org) and render on GitHub.

| # | Chapter | What it answers |
|---|---|---|
| 1 | [Introduction and goals](01-introduction-and-goals.md) | What Jones3D EX is, and what matters most |
| 2 | [Constraints](02-constraints.md) | What we can't choose freely |
| 3 | [Context and scope](03-context-and-scope.md) | What the system talks to |
| 4 | [Solution strategy](04-solution-strategy.md) | The few ideas everything else follows from |
| 5 | [Building block view](05-building-blocks/README.md) | The modules and their interfaces |
| 6 | [Runtime view](06-runtime/README.md) | Boot, frame loop, save and load |
| 7 | [Deployment view](07-deployment.md) | Builds per OS, packages, game data |
| 8 | [Crosscutting concepts](08-crosscutting/README.md) | Fidelity and traces, memory, logging, config, testing |
| 9 | [Architecture decisions](09-decisions/README.md) | Why it is built this way |
| 10 | [Quality requirements](10-quality-requirements.md) | Quality tree and scenarios |
| 11 | [Risks and technical debt](11-risks-and-technical-debt.md) | What could go wrong |
| 12 | [Glossary](12-glossary.md) | Terms |

Reference material that isn't architecture stays outside arc42:

- [Formats](../Formats/README.md): game file formats
- [COG](../COG/README.md): the scripting language
- [Jones.cfg](../Jones.cfg.md): configuration
- [Roadmap](../Roadmap.md): milestones and status
