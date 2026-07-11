# LocalGen project website

This repository hosts the bilingual **Hugo** website and English/Chinese guides for **Local Generals.io (LocalGen)**.

Repository:

- <https://github.com/SZXC-WG/szxc-wg.github.io>

Published site:

- <https://szxc-wg.github.io/>

## What the site includes

- English and Chinese content through Hugo bilingual mode
- a feature overview with clear notes about tools still in development
- download, source-build, first-game, controls, and Map Creator guides
- the current built-in Bot roster and complete simulator reference
- a compact release archive and contributor roster
- GitHub Pages deployment workflow

## Local development

Hugo is required locally. Once installed, run:

```text
hugo server
```

or build a production version with:

```text
hugo build --gc --minify
```

## Data refresh

Release and contributor data is generated into the `data/` directory by:

- `scripts/sync_localgen.py`

The site ships with seeded public data for local development. On GitHub Actions, the workflow runs the sync script with `GITHUB_TOKEN` so the published site stays fresh.

For authenticated local refreshes, copy `.env.example` to `.env` and set either `GITHUB_TOKEN` or `GH_TOKEN` before running the sync script.

## Workflows

- `hugo.yaml` — builds and deploys the site to the root `szxc-wg.github.io` GitHub Pages environment
- `sync-localgen-data.yaml` — refreshes project metadata on demand or on a schedule

## GitHub Pages setup

In the repository settings for `SZXC-WG/szxc-wg.github.io`, set **Pages → Source** to **GitHub Actions**. The committed workflow then handles the full build and deployment process.
