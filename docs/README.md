# WISENET Documentation Hub

This is the single source of truth for framework documentation.

## Start Here

- New to the project: read [README.md](../README.md), then [app/README.md](../app/README.md), then [app/frontend/README.md](../app/frontend/README.md).
- Working on backend API: start at [app/api/README.md](../app/api/README.md).
- Running a demo: start at [demos/README.md](../demos/README.md).
- Investigating runtime issues: start at [docs/runbooks/operations.md](runbooks/operations.md).

## Documentation Principles

- Keep docs close to code: each major folder has a short overview README.
- Optimize for 5-minute onboarding: new team members should run and navigate quickly.
- Prefer examples over theory: include one realistic request or command per page.
- Keep docs alive: every feature PR updates docs when behavior changes.

## Information Architecture

This is the recommended page map and ownership model.

| Page | Path | Purpose | Owner Role | Update Trigger |
| --- | --- | --- | --- | --- |
| Project overview | [README.md](../README.md) | What WISENET is, quick start, top-level architecture | Tech Lead | Release or setup changes |
| Docs hub | [docs/README.md](README.md) | Navigation, ownership, publishing workflow | Tech Lead | Quarterly review |
| Backend app overview | [app/README.md](../app/README.md) | Request flow, modules, local run/test steps | Backend Lead | API or service changes |
| API surface | [app/api/README.md](../app/api/README.md) | Endpoints, auth model, contracts | Backend Lead | Endpoint or schema changes |
| Frontend overview | [app/frontend/README.md](../app/frontend/README.md) | App structure, routes, data flow, local run | Frontend Lead | UX, route, or state changes |
| Demo index | [demos/README.md](../demos/README.md) | Demo catalog and how to execute each one | Demo Owner | New or changed demos |
| Stream-control demo | [demos/stream-control-2025-10/README.md](../demos/stream-control-2025-10/README.md) | End-to-end demo setup and script | Demo Owner | Contract/script changes |
| Operations runbook | [docs/runbooks/operations.md](runbooks/operations.md) | Troubleshooting, restart order, incident checks | Platform/DevOps | Ops findings or incidents |

## Section README Standard

Every section README should be 1-2 pages and follow this shape:

1. What this section owns
2. How data/control flows through it
3. Key files and why they matter
4. How to run and test locally
5. Known pitfalls and troubleshooting
6. Links to related docs

Use the template in [docs/templates/section-readme-template.md](templates/section-readme-template.md).

## Teams Publishing Workflow

Use Teams as a broadcast channel, not as the canonical source.

1. Write or update docs in this repository first.
2. Merge via PR so changes are reviewed.
3. Post a short Teams summary with:
   - what changed
   - why it matters
   - who is impacted
   - direct links to updated pages
4. Pin a message in Teams pointing to this hub page.

Template:

```text
Docs update: <topic>

What changed:
- ...

Why it matters:
- ...

Who is impacted:
- ...

Links:
- <doc link 1>
- <doc link 2>
```

## Definition Of Done For Documentation

A feature or change is doc-complete when:

- Relevant section README is updated.
- Any API or behavior change includes examples.
- Troubleshooting notes are updated if operational behavior changed.
- Teams summary is posted with links to updated docs.
