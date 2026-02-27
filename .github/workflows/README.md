# Agentic Workflows for zlib Maintenance

This repository uses [GitHub Agentic Workflows](https://github.github.com/gh-aw/introduction/overview/) to automate daily maintenance, security auditing, testing, and code quality monitoring of the zlib compression library.

## Agent Team

The following agents work together, coordinated by the Product Manager orchestrator:

```
┌─────────────────────────────────────────────────────────────┐
│                    Daily Schedule (6:00 UTC)                 │
│                                                             │
│                   ┌──────────────────┐                      │
│                   │  PM Orchestrator  │                      │
│                   │  (Coordinator)    │                      │
│                   └────────┬─────────┘                      │
│                            │ dispatches                     │
│              ┌─────────────┼─────────────┐                  │
│              ▼             ▼             ▼                   │
│   ┌──────────────┐ ┌─────────────┐ ┌──────────────┐        │
│   │   Security   │ │    Test     │ │     Code     │        │
│   │   Auditor    │ │  Guardian   │ │  Maintainer  │        │
│   └──────────────┘ └─────────────┘ └──────────────┘        │
│              │             │             │                   │
│              └─────────────┼─────────────┘                  │
│                            ▼                                │
│                   ┌──────────────────┐                      │
│                   │   Daily Report   │                      │
│                   │  (Consolidator)  │                      │
│                   └──────────────────┘                      │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    Event-Driven Agents                       │
│                                                             │
│   ┌──────────────────┐    ┌──────────────────┐              │
│   │ Issue Responder   │    │ Issue Comment    │              │
│   │ (on issue open)   │    │ Handler          │              │
│   └──────────────────┘    └──────────────────┘              │
│                                                             │
│   ┌──────────────────┐                                      │
│   │   PM Review      │                                      │
│   │ (on PR open)     │                                      │
│   └──────────────────┘                                      │
└─────────────────────────────────────────────────────────────┘
```

### Scheduled Agents (DailyOps)

| Agent | File | Schedule | Purpose |
|-------|------|----------|---------|
| **PM Orchestrator** | `pm-orchestrator.md` | Daily 6:00 UTC | Coordinates all agents, makes decisions, creates daily status issues |
| **Security Auditor** | `security-auditor.md` | Dispatched by PM | Scans C source for buffer overflows, integer overflows, memory safety issues |
| **Test Guardian** | `test-guardian.md` | Dispatched by PM | Analyzes test coverage, proposes test improvements via draft PRs |
| **Code Maintainer** | `code-maintainer.md` | Dispatched by PM | Identifies documentation, build system, and code quality improvements |
| **Daily Report** | `daily-report.md` | Dispatched by PM | Consolidates all agent findings into a daily health report issue |

### Event-Driven Agents

| Agent | File | Trigger | Purpose |
|-------|------|---------|---------|
| **Issue Responder** | `issue-responder.md` | Issue opened/reopened | Triages issues, applies labels, provides initial response |
| **Issue Comment Handler** | `issue-comment-handler.md` | Comment on issue | Responds to follow-up questions, updates labels |
| **PM Review** | `pm-review.md` | PR opened/updated | Reviews all PRs against project goals, provides code review |

## How It Works

1. **Daily at 6:00 UTC**, the PM Orchestrator wakes up and:
   - Checks repository state (open issues, PRs, recent activity)
   - Dispatches the Security Auditor, Test Guardian, and Code Maintainer
   - Dispatches the Daily Report to consolidate findings
   - Creates a `[pm-daily]` status issue

2. **Worker agents** (Security, Testing, Maintenance) each:
   - Analyze their specific domain
   - Report findings via GitHub issues (prefixed with `[security]`, `[testing]`, `[maintenance]`)
   - May create draft PRs for safe improvements
   - Save state to cache memory for continuity

3. **When issues are opened**, the Issue Responder immediately:
   - Classifies the issue (bug, feature, question, security, etc.)
   - Applies appropriate labels
   - Posts a helpful initial response

4. **When PRs are opened**, the PM Review agent:
   - Reviews code changes against project goals
   - Adds inline review comments
   - Labels PRs appropriately (`approved`, `needs-human-review`, `security-sensitive`)

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
gh label create pm-orchestrator --color 5319e7 --description "PM coordination"
gh label create daily-report --color 1d76db --description "Daily health reports"
gh label create needs-triage --color fbca04 --description "Needs human classification"
gh label create needs-human-review --color b60205 --description "Requires human approval"
gh label create security-sensitive --color d93f0b --description "Touches security-critical code"
gh label create api-change --color b60205 --description "Modifies public API"
gh label create needs-more-info --color fbca04 --description "Needs additional information"
gh label create answered --color 0e8a16 --description "Question has been answered"
gh label create good-first-issue --color 7057ff --description "Good for newcomers"
```

### Manual Trigger

Any workflow can be triggered manually for testing:

```bash
gh workflow run pm-orchestrator.lock.yml
```
