# Documentation Index

This `docs/` folder is the checked-in source of truth for contributor-facing project documentation.

## Read This First

1. [Roadmap](roadmap.md): defines product goals, scope boundaries, milestones, and acceptance gates
2. [Architecture](architecture.md): defines runtime shape, dependencies, module boundaries, and repository structure
3. [Interfaces](interfaces.md): defines the contributor-facing backend contract summary
4. [Foundation Decisions](decisions/0001-foundation.md): records the baseline choices locked in for V1

## Quick Links

- [Target platform](roadmap.md#target-platform)
- [V1 scope](roadmap.md#v1-scope)
- [Runtime shape](architecture.md#runtime-shape)
- [API split](interfaces.md#transport-split)

## What Each Doc Owns

- `roadmap.md` owns scope, milestones, and delivery order.
- `architecture.md` owns the backend shape and repository layout.
- `interfaces.md` owns the public state model summary and transport responsibilities.
- `decisions/0001-foundation.md` owns the baseline choices from project setup.

## Documentation Rules

- Do not duplicate major product decisions across files unless a short restatement is needed for context.
- If a product or implementation decision changes, update the decision record first and then adjust the affected docs.
- Keep the docs lean and contributor-focused. Avoid turning them into a task board or agent playbook.
