# Superseded: separate Readings/Favorites execution plan

The owner requested one coordinated implementation including the full vehicle
profile. Use [VEHICLE_UI_IMPLEMENTATION_PLAN.md](VEHICLE_UI_IMPLEMENTATION_PLAN.md)
as the single authoritative technical specification and execution order.

It includes all former reading/group/default/layout/reset requirements, plus
complete profile schema/migration, equipment policy, confirmation, feature gates,
C1/BH display synchronization and coordinated commit behavior.

The former baseline/full-suite test commands are withdrawn. The joint plan
requires a test-impact ledger and a scoped allowlist of new or meaningfully
modified cases before execution. Do not run this older plan from Git history or
interpret “keep an invariant” as authorization to run an unchanged test.

This redirect replaces the old execution instructions to avoid contradictory
plans. Historical source evidence remains in the profile audit and layout summary.
No firmware or tests were modified by this planning update.
