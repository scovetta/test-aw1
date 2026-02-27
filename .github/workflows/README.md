# Agentic Workflows for zlib Maintenance

This repository uses [GitHub Agentic Workflows](https://github.github.com/gh-aw/introduction/overview/) to automate daily maintenance, security auditing, testing, and code quality monitoring of the zlib compression library.

## Agent Team

The following agents work together, coordinated by the Product Manager orchestrator:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Daily Schedule (6:00 UTC)                          │
│                                                                     │
│                   ┌──────────────────┐                              │
│                   │  PM Orchestrator  │                              │
│                   │  (Coordinator)    │                              │
│                   └────────┬─────────┘                              │
│                            │ dispatches                              │
│        ┌─────────┬─────────┼─────────┬─────────┼─────────┐  │
│        ▼         ▼         ▼         ▼         ▼         ▼  │
│  ┌────────┐ ┌───────┐ ┌────────┐ ┌───────┐ ┌────────┐ ┌───────┐  │
│  │Security│ │ Test  │ │  Code  │ │ Fixer │ │Builder │ │  Dep  │  │
│  │Auditor │ │Guard. │ │Maint. │ │ Agent │ │ Agent  │ │Update │  │
│  └────┬───┘ └───┬───┘ └────┬───┘ └───┬───┘ └────┬───┘ └───┬───┘  │
│       │         │        │        │         │        │   │
│       │   issues│  PRs   │assigns │  issues │  PRs   │   │
│       ▼         ▼        ▼ Copilot▼         ▼        ▼   │
│                                                           │
│                   ┌──────────────────┐  (8:00 UTC)           │
│                   │   Daily Report   │                     │
│                   │  (Consolidator)  │                     │
│                   └──────────────────┘                     │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                    Event-Driven Agents                              │
│                                                                     │
│   ┌──────────────────┐    ┌──────────────────┐                      │
│   │ Issue Responder   │    │ Issue Comment    │                      │
│   │ (on issue open)   │    │ Handler          │                      │
│   └──────────────────┘    └──────────────────┘                      │
│                                                                     │
│   ┌──────────────────┐                                              │
│   │   PM Review      │  Reviews & approves all agent PRs            │
│   │ (on PR open)     │  before human merge                          │
│   └──────────────────┘                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Scheduled Agents (DailyOps)

| Agent | File | Schedule | Purpose |
|-------|------|----------|---------|
| **PM Orchestrator** | `pm-orchestrator.md` | Daily 6:00 UTC | Coordinates all agents, makes decisions, creates daily status issues |
| **Security Auditor** | `security-auditor.md` | Dispatched by PM | Scans C source for buffer overflows, integer overflows, memory safety issues |
| **Test Guardian** | `test-guardian.md` | Dispatched by PM | Analyzes test coverage, proposes test improvements via draft PRs |
| **Code Maintainer** | `code-maintainer.md` | Dispatched by PM | Identifies documentation, build system, and code quality improvements |
| **Fixer Agent** | `fixer-agent.md` | Dispatched by PM + issues | Evaluates actionable issues and assigns Copilot to implement fixes |
| **Builder Agent** | `builder-agent.md` | Dispatched by PM | Analyzes build systems, verifies release readiness, proposes build fixes |
| **Dependency Updater** | `dependency-updater.md` | Dispatched by PM | Checks for outdated dependencies, proposes version updates |
| **Daily Report** | `daily-report.md` | Daily 8:00 UTC | Consolidates all agent findings into a daily health report issue |

### Event-Driven Agents

| Agent | File | Trigger | Purpose |
|-------|------|---------|---------|
| **Issue Responder** | `issue-responder.md` | Issue opened/reopened | Triages issues, applies labels, provides initial response |
| **Issue Comment Handler** | `issue-comment-handler.md` | Comment on issue | Responds to follow-up questions, updates labels |
| **PM Review** | `pm-review.md` | PR opened/updated | Reviews all PRs against project goals, provides code review |

## How It Works

1. **Daily at 6:00 UTC**, the PM Orchestrator wakes up and:
   - Checks repository state (open issues, PRs, recent activity)
   - Dispatches all worker agents: Security Auditor, Test Guardian, Code Maintainer, Fixer Agent, Builder Agent, and Dependency Updater
   - Creates a `[pm-daily]` status issue

2. **Worker agents** each analyze their domain:
   - **Security Auditor** — scans for vulnerabilities, creates `[security]` issues
   - **Test Guardian** — finds coverage gaps, creates draft test PRs
   - **Code Maintainer** — identifies quality issues, creates `[maintenance]` issues and cleanup PRs
   - **Fixer Agent** — picks up actionable issues, assigns them to Copilot for automated fixing
   - **Builder Agent** — reviews build systems, verifies release readiness, creates `[build]` issues
   - **Dependency Updater** — checks for outdated dependencies, creates `[dependencies]` issues and update PRs

3. **At 8:00 UTC**, the Daily Report agent consolidates all findings into a single health report issue.

4. **When Copilot creates PRs** from Fixer Agent assignments, the **PM Review agent**:
   - Reviews code changes against project goals
   - **APPROVES** safe changes (docs, tests, cosmetic, build fixes)
   - Flags library code changes with `needs-human-review` for human merge decision

5. **When issues are opened**, the Issue Responder immediately triages and responds.

6. **Humans only need to merge** — all PRs are created as drafts and reviewed by PM Review before any merge.

## Project Goals (enforced by PM)

All agent decisions are evaluated against these zlib project goals:

1. **Stability** — No breaking changes to critical infrastructure
2. **Correctness** — RFC 1950/1951/1952 compliance, bit-exact behavior
3. **Performance** — No throughput or memory regressions
4. **Portability** — Cross-platform support (Linux, Windows, macOS, embedded)
5. **Security** — Memory safety, input validation
6. **Minimal footprint** — No unnecessary dependencies

## Safety Constraints

- All agents use **read-only permissions** by default
- Write operations go through **safe-outputs** (validated, sandboxed)
- Agents **NEVER write to other repositories** — same-repo only
- PRs are always created as **drafts** — human review required
- The PM agent **never approves API changes** — always flags for human review
- Daily issues **auto-expire** to prevent clutter

## Issue Labels

| Label | Used By | Meaning |
|-------|---------|---------|
| `automation` | All agents | Marks agent-generated content |
| `security` | Security Auditor, Issue Responder | Security-related findings |
| `testing` | Test Guardian | Testing-related findings |
| `maintenance` | Code Maintainer | Code quality and housekeeping |
| `build` | Builder Agent | Build system and release findings |
| `dependencies` | Dependency Updater | Dependency update tracking |
| `fix-assigned` | Fixer Agent | Issue assigned to Copilot for fixing |
| `fix-in-progress` | Fixer Agent | Copilot is working on a fix |
| `pm-orchestrator` | PM Orchestrator | PM coordination issues |
| `daily-report` | Daily Report | Consolidated daily reports |
| `needs-triage` | Issue Responder | Needs human classification |
| `needs-human-review` | PM Review | PR requires human approval |
| `security-sensitive` | PM Review | PR touches security-critical code |
| `api-change` | PM Review | PR modifies public API |

## Setup

### Prerequisites

1. Install the [GitHub Agentic Workflows CLI](https://github.github.com/gh-aw/setup/quick-start/)
2. Ensure GitHub Actions is enabled on the repository

### Compile Workflows

```bash
gh aw compile
```

This generates `.lock.yml` files for each `.md` workflow. Commit both the `.md` source and `.lock.yml` compiled files.

### Create Labels

The agents expect these labels to exist. Create them manually or via the GitHub API:

```bash
gh label create automation --color 0e8a16 --description "Agent-generated content"
gh label create security --color d93f0b --description "Security-related"
gh label create testing --color 0075ca --description "Testing-related"
gh label create maintenance --color e4e669 --description "Code quality and housekeeping"
gh label create build --color 006b75 --description "Build system and release"
gh label create dependencies --color 0366d6 --description "Dependency updates"
gh label create fix-assigned --color c2e0c6 --description "Assigned to Copilot for fixing"
gh label create fix-in-progress --color fbca04 --description "Copilot is working on a fix"
gh label create pm-orchestrator --color 5319e7 --description "PM coordination"
gh label create daily-report --color 1d76db --description "Daily health reports"
gh label create needs-triage --color fbca04 --description "Needs human classification"
gh label create needs-human-review --color b60205 --description "Requires human approval"
gh label create security-sensitive --color d93f0b --description "Touches security-critical code"
gh label create api-change --color b60205 --description "Modifies public API"
gh label create needs-more-info --color fbca04 --description "Needs additional information"
gh label create needs-changes --color b60205 --description "PR needs changes"
gh label create approved --color 0e8a16 --description "Approved by PM Review"
gh label create answered --color 0e8a16 --description "Question has been answered"
gh label create good-first-issue --color 7057ff --description "Good for newcomers"
gh label create wontfix --color ffffff --description "Will not be fixed"
```

### Manual Trigger

Any workflow can be triggered manually for testing:

```bash
gh workflow run pm-orchestrator.lock.yml
```
