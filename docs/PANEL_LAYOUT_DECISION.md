# Zenith Panel Layout Decision

Date: 2026-02-20
Status: Active

## Decision

- Browser belongs on the left as the primary asset navigation zone.
- Wingman belongs on the right as contextual AI assistance.
- Mini mixer/inspector behavior should share the right-side ecosystem (tabbed/stacked), not create a separate competing slide system.
- Transport Wingman button should visually map to the right-side Wingman location.

## Rationale

- Left-side asset browsing matches common DAW mental models.
- Right-side contextual tools reduce conflict with core arrangement/navigation.
- One panel system per side avoids overlapping slide interactions and spatial confusion.

## UX Rules

- Do not have Browser and Wingman both originate from the same side.
- Keep panel collapse/expand memory per side.
- Keep Wingman globally reachable by shortcut and transport button.
- Prefer a single right-side shell for Wingman + related context tools.

## Implementation Notes

- Wingman panel ID: `right_sidebar`.
- Transport Wingman button grouped with right-side tools cluster.
