---
title: "Commit Regulations"
description: "Commit message, commit size, and collaboration expectations for LocalGen development."
date: 2026-04-06T17:55:07+08:00
draft: false
weight: 30
---

You can also [read the commit regulations on GitHub](https://github.com/SZXC-WG/LocalGen-new/blob/master/docs/commit-regulations.md).

## Goal

The project’s commit regulations exist to keep the history consistent, understandable, and maintainable.

## Commit message structure

Use the following commit-message layout:

```text
<type>(<scope>): <subject>

<body>

<footer>
```

### Common types

- `feat` — new feature
- `upd` — update to an existing feature
- `fix` — bug fix
- `docs` — documentation change
- `style` — formatting or style-only change
- `refactor` — internal restructuring without feature change
- `chore` — maintenance or dependency work
- `test` — test additions or updates
- `ci` — continuous integration changes

## Subject-line guidance

- use the imperative mood
- keep it concise and descriptive
- avoid vague messages such as “Fix stuff”
- prefer lowercase unless a proper noun requires otherwise

## Commit size and frequency

The project prefers commits that are:

- **small and focused**
- **atomic**, so they can be reviewed or reverted independently
- **frequent enough** to show progress without flooding the history with trivial noise

## Additional practices

- write meaningful commit messages
- rebase before merging when appropriate
- squash overly fragmented work before landing on the main branch

