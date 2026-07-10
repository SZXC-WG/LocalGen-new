#!/usr/bin/env python3
from __future__ import annotations

import json
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen

OWNER = "SZXC-WG"
REPO = "LocalGen-new"
API_ROOT = f"https://api.github.com/repos/{OWNER}/{REPO}"
SITE_ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = SITE_ROOT / "data"
TOKEN = os.getenv("GITHUB_TOKEN") or os.getenv("GH_TOKEN")
USER_AGENT = "LocalGen-website-sync/1.0"


def fetch_json(url: str, params: dict[str, Any] | None = None) -> Any:
    if params:
        url = f"{url}?{urlencode(params)}"
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": USER_AGENT,
    }
    if TOKEN:
        headers["Authorization"] = f"Bearer {TOKEN}"
    request = Request(url, headers=headers)
    try:
        with urlopen(request) as response:
            return json.load(response)
    except HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise SystemExit(f"GitHub API request failed: {exc.code} {exc.reason}\n{body}") from exc
    except URLError as exc:
        raise SystemExit(f"Network request failed: {exc.reason}") from exc


def human_size(size: int) -> str:
    units = ["B", "KB", "MB", "GB", "TB"]
    value = float(size)
    for unit in units:
        if value < 1024 or unit == units[-1]:
            if unit == "B":
                return f"{int(value)} {unit}"
            return f"{value:.1f} {unit}"
        value /= 1024
    return f"{size} B"


def normalize_repo(repo: dict[str, Any]) -> dict[str, Any]:
    topics = repo.get("topics") or []
    return {
        "name": repo.get("name"),
        "full_name": repo.get("full_name"),
        "description": repo.get("description"),
        "html_url": repo.get("html_url"),
        "homepage": repo.get("homepage"),
        "license": "GPL-3.0-or-later",
        "default_branch": repo.get("default_branch"),
        "created_at": repo.get("created_at"),
        "updated_at": repo.get("updated_at"),
        "pushed_at": repo.get("pushed_at"),
        "stargazers_count": repo.get("stargazers_count", 0),
        "watchers_count": repo.get("subscribers_count", 0),
        "forks_count": repo.get("forks_count", 0),
        "open_issues_count": repo.get("open_issues_count", 0),
        "topics": topics,
        "project_name": "Local Generals.io",
        "short_name": "LocalGen",
        "organization": OWNER,
        "version": "6.0.0-dev",
        "version_line": "master / v6",
        "legacy_line": "v5.0",
        "languages": ["English", "Chinese"],
        "bot_count": 10,
        "documented_bot_count": 13,
        "map_count": 46,
        "built_with": ["C++17", "Qt6", "CMake"],
        "download_url": f"https://github.com/{OWNER}/{REPO}/releases",
        "issues_url": f"https://github.com/{OWNER}/{REPO}/issues",
        "pulls_url": f"https://github.com/{OWNER}/{REPO}/pulls",
        "discussions_url": f"https://github.com/{OWNER}/{REPO}/discussions",
        "actions_url": f"https://github.com/{OWNER}/{REPO}/actions",
        "contributors_url": f"https://github.com/{OWNER}/{REPO}/graphs/contributors",
    }


def normalize_release(release: dict[str, Any], latest_stable_tag: str | None) -> dict[str, Any]:
    assets = []
    for asset in release.get("assets") or []:
        assets.append(
            {
                "name": asset.get("name"),
                "size": asset.get("size", 0),
                "size_human": human_size(asset.get("size", 0)),
                "download_count": asset.get("download_count", 0),
                "content_type": asset.get("content_type"),
                "browser_download_url": asset.get("browser_download_url"),
                "updated_at": asset.get("updated_at"),
            }
        )
    body = release.get("body") or ""
    summary = " ".join(line.strip() for line in body.splitlines() if line.strip())
    return {
        "name": release.get("name") or release.get("tag_name"),
        "tag_name": release.get("tag_name"),
        "html_url": release.get("html_url"),
        "body": body,
        "summary": summary,
        "draft": release.get("draft", False),
        "prerelease": release.get("prerelease", False),
        "published_at": release.get("published_at"),
        "created_at": release.get("created_at"),
        "author": (release.get("author") or {}).get("login"),
        "is_latest": release.get("tag_name") == latest_stable_tag,
        "zipball_url": release.get("zipball_url"),
        "tarball_url": release.get("tarball_url"),
        "assets": assets,
    }


def normalize_contributor(contributor: dict[str, Any]) -> dict[str, Any]:
    return {
        "login": contributor.get("login"),
        "html_url": contributor.get("html_url"),
        "avatar_url": contributor.get("avatar_url"),
        "contributions": contributor.get("contributions", 0),
        "type": contributor.get("type"),
    }


def main() -> None:
    repo = fetch_json(API_ROOT)
    releases = fetch_json(f"{API_ROOT}/releases", {"per_page": 100})
    contributors = fetch_json(f"{API_ROOT}/contributors", {"per_page": 100})

    stable_releases = [release for release in releases if not release.get("prerelease") and not release.get("draft")]
    latest_stable_tag = stable_releases[0].get("tag_name") if stable_releases else None

    normalized_project = normalize_repo(repo)
    normalized_releases = [normalize_release(release, latest_stable_tag) for release in releases if not release.get("draft")]
    normalized_contributors = [normalize_contributor(contributor) for contributor in contributors]
    normalized_project["release_count"] = len(normalized_releases)
    normalized_project["contributor_count"] = len(normalized_contributors)

    DATA_DIR.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now(timezone.utc).isoformat()

    (DATA_DIR / "project.json").write_text(
        json.dumps({**normalized_project, "synced_at": timestamp}, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    (DATA_DIR / "releases.json").write_text(
        json.dumps({"synced_at": timestamp, "items": normalized_releases}, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    (DATA_DIR / "contributors.json").write_text(
        json.dumps({"synced_at": timestamp, "items": normalized_contributors}, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )

    print(f"Synced {len(normalized_releases)} releases and {len(normalized_contributors)} contributors.")


if __name__ == "__main__":
    main()
