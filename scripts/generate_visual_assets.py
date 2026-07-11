from __future__ import annotations

import argparse
import json
import math
import textwrap
from datetime import datetime
from pathlib import Path
from typing import Any, Iterable
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
STATIC_DIR = ROOT / "static"
OUTPUT_DIR = STATIC_DIR / "img" / "generated"

# Shared with assets/css/main.css. The generated illustrations are intentionally
# dark so they remain legible in both site themes.
BG = "#101318"
BG_SOFT = "#14181e"
CARD_FILL = "#181d24"
CARD_RAISED = "#1e242c"
CARD_BORDER = "#364149"
GRID_LINE = "#293239"
TEXT_MAIN = "#f1f5f6"
TEXT_MUTED = "#a3adb2"
ACCENT = "#38c8b5"
BLUE = "#75aadb"
VIOLET = "#a58bd4"
SUCCESS = "#68d391"
WARNING = "#f2bd64"
DANGER = "#ef8792"


def load_json(name: str) -> dict[str, Any]:
    path = DATA_DIR / name
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"Missing data file: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON in {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise SystemExit(f"Expected a JSON object in {path}")
    return value


def require_items(document: dict[str, Any], source: str) -> list[dict[str, Any]]:
    items = document.get("items")
    if not isinstance(items, list) or not items:
        raise SystemExit(f"{source} must contain a non-empty 'items' list")
    if not all(isinstance(item, dict) for item in items):
        raise SystemExit(f"Every item in {source} must be an object")
    return items


def svg_text(value: object) -> str:
    return escape(str(value), {'"': "&quot;", "'": "&apos;"})


def wrap_words(text: str, max_chars: int, max_lines: int | None = None) -> list[str]:
    lines = textwrap.wrap(
        " ".join(text.split()),
        width=max_chars,
        break_long_words=True,
        break_on_hyphens=False,
    )
    if max_lines is None or len(lines) <= max_lines:
        return lines
    visible = lines[:max_lines]
    tail = visible[-1].rstrip(" ,.;:")
    visible[-1] = f"{tail[: max(1, max_chars - 1)].rstrip()}…"
    return visible


def text_block(
    x: float,
    y: float,
    text: str,
    *,
    max_chars: int,
    font_size: int,
    fill: str,
    line_height: int = 18,
    weight: str | None = None,
    anchor: str = "start",
    max_lines: int | None = None,
) -> str:
    weight_attr = f' font-weight="{weight}"' if weight else ""
    anchor_attr = f' text-anchor="{anchor}"' if anchor else ""
    return "".join(
        f'<text x="{x}" y="{y + index * line_height}" font-size="{font_size}" '
        f'fill="{fill}"{weight_attr}{anchor_attr}>{svg_text(line)}</text>'
        for index, line in enumerate(wrap_words(text, max_chars, max_lines))
    )


def parse_timestamp(value: object) -> datetime:
    if not isinstance(value, str):
        raise SystemExit(f"Expected ISO timestamp, received {value!r}")
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError as exc:
        raise SystemExit(f"Invalid ISO timestamp: {value}") from exc


def rounded_rect(
    x: float,
    y: float,
    width: float,
    height: float,
    radius: float,
    *,
    fill: str,
    stroke: str = CARD_BORDER,
    stroke_width: float = 1,
) -> str:
    return (
        f'<rect x="{x}" y="{y}" width="{width}" height="{height}" rx="{radius}" '
        f'fill="{fill}" stroke="{stroke}" stroke-width="{stroke_width}" />'
    )


def stat_card(x: float, y: float, label: str, value: object, color: str, width: int = 162) -> str:
    return (
        rounded_rect(x, y, width, 48, 12, fill=CARD_RAISED)
        + f'<text x="{x + 16}" y="{y + 20}" font-size="10" fill="{TEXT_MUTED}" '
        f'font-weight="700" letter-spacing="1.25">{svg_text(label.upper())}</text>'
        + f'<text x="{x + 16}" y="{y + 38}" font-size="15" fill="{color}" '
        f'font-weight="750">{svg_text(value)}</text>'
    )


def shared_defs() -> str:
    return f"""
    <linearGradient id="background" x1="0" x2="1" y1="0" y2="1">
      <stop offset="0%" stop-color="{BG_SOFT}" />
      <stop offset="100%" stop-color="{BG}" />
    </linearGradient>
    <linearGradient id="panel" x1="0" x2="0" y1="0" y2="1">
      <stop offset="0%" stop-color="{CARD_RAISED}" />
      <stop offset="100%" stop-color="{CARD_FILL}" />
    </linearGradient>
    <linearGradient id="accentLine" x1="0" x2="1" y1="0" y2="0">
      <stop offset="0%" stop-color="{ACCENT}" />
      <stop offset="100%" stop-color="{WARNING}" />
    </linearGradient>
    <linearGradient id="accentArea" x1="0" x2="0" y1="0" y2="1">
      <stop offset="0%" stop-color="{ACCENT}" stop-opacity="0.26" />
      <stop offset="100%" stop-color="{ACCENT}" stop-opacity="0.02" />
    </linearGradient>
    <pattern id="grid" width="44" height="44" patternUnits="userSpaceOnUse">
      <path d="M 44 0 L 0 0 0 44" fill="none" stroke="{GRID_LINE}" stroke-width="1" />
    </pattern>
    <filter id="softShadow" x="-20%" y="-20%" width="140%" height="150%">
      <feDropShadow dx="0" dy="10" stdDeviation="14" flood-color="#000000" flood-opacity="0.24" />
    </filter>
    """


def write_svg(
    path: Path,
    width: int,
    height: int,
    content: str,
    *,
    title: str,
    description: str,
    defs: str = "",
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    common_defs = textwrap.dedent(shared_defs()).strip()
    extra_defs = textwrap.dedent(defs).strip()
    clean_content = textwrap.dedent(content).strip()
    all_defs = f"{common_defs}\n{extra_defs}" if extra_defs else common_defs
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" fill="none" role="img" aria-labelledby="title desc">
  <title id="title">{svg_text(title)}</title>
  <desc id="desc">{svg_text(description)}</desc>
  <defs>
{all_defs}
  </defs>
  <g font-family="Inter, ui-sans-serif, system-ui, -apple-system, BlinkMacSystemFont, Segoe UI, sans-serif">
{clean_content}
  </g>
</svg>
'''
    path.write_text(svg, encoding="utf-8", newline="\n")


def generate_release_journey(items: list[dict[str, Any]]) -> Path:
    items = sorted(items, key=lambda item: parse_timestamp(item.get("published_at")))
    width, height = 780, 456
    left, right, top, bottom = 68, 36, 176, 58
    plot_width, plot_height = width - left - right, height - top - bottom
    dates = [parse_timestamp(item.get("published_at")) for item in items]
    min_date, max_date = dates[0], dates[-1]
    day_span = max((max_date - min_date).days, 1)

    def x_for(date: datetime) -> float:
        return left + ((date - min_date).days / day_span) * plot_width

    def y_for(value: float) -> float:
        return top + plot_height - (value / len(items)) * plot_height

    points = [(x_for(date), y_for(index), item) for index, (item, date) in enumerate(zip(items, dates), 1)]
    line_path = " ".join(
        [f"M {points[0][0]:.1f} {points[0][1]:.1f}"]
        + [f"L {x:.1f} {y:.1f}" for x, y, _ in points[1:]]
    )
    area_path = (
        f"M {points[0][0]:.1f} {top + plot_height:.1f} "
        + " ".join(f"L {x:.1f} {y:.1f}" for x, y, _ in points)
        + f" L {points[-1][0]:.1f} {top + plot_height:.1f} Z"
    )

    stable = [item for item in items if not bool(item.get("prerelease"))]
    previews = len(items) - len(stable)
    latest = next((item for item in reversed(items) if item.get("is_latest")), None)
    latest = latest or (stable[-1] if stable else items[-1])

    tick_step = max(1, math.ceil(len(items) / 5))
    grid_lines: list[str] = []
    for tick in range(0, len(items) + 1, tick_step):
        y = y_for(tick)
        grid_lines.append(
            f'<line x1="{left}" y1="{y:.1f}" x2="{width - right}" y2="{y:.1f}" '
            f'stroke="{GRID_LINE}" stroke-dasharray="3 7" />'
        )
        if tick:
            grid_lines.append(
                f'<text x="{left - 12}" y="{y + 4:.1f}" text-anchor="end" font-size="11" '
                f'fill="{TEXT_MUTED}">{tick}</text>'
            )

    years = {date.year for date in dates}
    year_ticks = []
    for year in sorted(years):
        first_date = next(date for date in dates if date.year == year)
        x = x_for(first_date)
        year_ticks.append(
            f'<line x1="{x:.1f}" y1="{top}" x2="{x:.1f}" y2="{top + plot_height}" stroke="{GRID_LINE}" />'
            f'<text x="{x:.1f}" y="{height - 20}" text-anchor="middle" font-size="11" fill="{TEXT_MUTED}">{year}</text>'
        )

    markers = []
    for x, y, item in points:
        color = WARNING if item.get("prerelease") else SUCCESS
        radius = 6 if item is latest else 4
        markers.append(
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{radius}" fill="{color}" '
            f'stroke="{BG}" stroke-width="2" />'
        )

    cards = "".join(
        stat_card(36 + index * 178, 28, label, value, color)
        for index, (label, value, color) in enumerate(
            [
                ("releases", len(items), ACCENT),
                ("stable", len(stable), SUCCESS),
                ("preview", previews, WARNING),
                ("latest", latest.get("tag_name", "—"), BLUE),
            ]
        )
    )

    content = f'''
    <rect width="{width}" height="{height}" rx="28" fill="url(#background)" />
    <rect width="{width}" height="{height}" rx="28" fill="url(#grid)" opacity="0.34" />
    <g filter="url(#softShadow)">{rounded_rect(18, 18, width - 36, height - 36, 22, fill=CARD_FILL)}</g>
    {cards}
    <text x="36" y="112" font-size="23" font-weight="750" fill="{TEXT_MAIN}">Release journey</text>
    {text_block(36, 136, "Cumulative published builds, from the first release to the active Qt era.", max_chars=72, font_size=12, fill=TEXT_MUTED, max_lines=2)}
    {''.join(grid_lines)}
    {''.join(year_ticks)}
    <path d="{area_path}" fill="url(#accentArea)" />
    <path d="{line_path}" stroke="url(#accentLine)" stroke-width="3.5" stroke-linecap="round" stroke-linejoin="round" />
    {''.join(markers)}
    <text x="{width - 40}" y="{top - 12}" text-anchor="end" font-size="11" fill="{TEXT_MUTED}">CUMULATIVE RELEASES</text>
    '''
    path = OUTPUT_DIR / "release-journey.svg"
    write_svg(
        path,
        width,
        height,
        content,
        title="LocalGen release journey",
        description=f"Timeline of {len(items)} LocalGen releases from {min_date.year} to {max_date.year}.",
    )
    return path


def generate_bot_spectrum(project: dict[str, Any], items: list[dict[str, Any]]) -> Path:
    score_colors = {1: BLUE, 2: WARNING, 3: DANGER}
    for item in items:
        score = item.get("complexity_score")
        if score not in score_colors:
            raise SystemExit(f"Bot {item.get('name', '<unnamed>')} has invalid complexity_score: {score!r}")
    items = sorted(items, key=lambda item: (item["complexity_score"], not bool(item.get("enabled")), str(item.get("name"))))

    width, row_height, top = 780, 29, 194
    height = max(560, top + len(items) * row_height + 58)
    columns = {1: 290, 2: 430, 3: 570}
    enabled_count = sum(bool(item.get("enabled")) for item in items)

    column_labels = []
    for score, label in enumerate(("low", "medium", "high"), 1):
        x = columns[score]
        column_labels.append(
            f'<text x="{x}" y="170" text-anchor="middle" font-size="11" fill="{TEXT_MUTED}" '
            f'font-weight="700" letter-spacing="1.2">{label.upper()}</text>'
            f'<line x1="{x}" y1="180" x2="{x}" y2="{height - 48}" stroke="{GRID_LINE}" stroke-dasharray="3 8" />'
        )

    rows = []
    for index, item in enumerate(items):
        y = top + index * row_height
        score = int(item["complexity_score"])
        color = score_colors[score]
        status_color = SUCCESS if item.get("enabled") else TEXT_MUTED
        status = "enabled" if item.get("enabled") else "disabled"
        rows.append(
            f'<text x="40" y="{y + 4}" font-size="12" fill="{TEXT_MAIN}" font-weight="650">{svg_text(item.get("name", "Unnamed"))}</text>'
            f'<rect x="{columns[score] - 48}" y="{y - 13}" width="96" height="21" rx="10.5" fill="{color}" fill-opacity="0.11" stroke="{color}" stroke-opacity="0.48" />'
            f'<text x="{columns[score]}" y="{y + 1}" text-anchor="middle" font-size="10" fill="{color}" font-weight="750">{svg_text(item.get("complexity", "—"))}</text>'
            f'<text x="674" y="{y + 4}" text-anchor="end" font-size="10" fill="{TEXT_MUTED}">{svg_text(item.get("time_complexity", "—"))}</text>'
            f'<circle cx="704" cy="{y - 1}" r="4" fill="{status_color}" />'
            f'<text x="716" y="{y + 3}" font-size="9" fill="{status_color}" font-weight="700">{status.upper()}</text>'
        )

    cards = "".join(
        [
            stat_card(36, 28, "built-in", len(items), ACCENT),
            stat_card(214, 28, "enabled", enabled_count, SUCCESS),
            stat_card(392, 28, "compiled-in", project.get("bot_count", enabled_count), BLUE, width=190),
        ]
    )
    content = f'''
    <rect width="{width}" height="{height}" rx="28" fill="url(#background)" />
    <rect width="{width}" height="{height}" rx="28" fill="url(#grid)" opacity="0.3" />
    <g filter="url(#softShadow)">{rounded_rect(18, 18, width - 36, height - 36, 22, fill=CARD_FILL)}</g>
    {cards}
    <text x="36" y="118" font-size="23" font-weight="750" fill="{TEXT_MAIN}">Built-in bot spectrum</text>
    {text_block(36, 142, "Compare strategy complexity and worst-case turn cost across LocalGen Bots.", max_chars=76, font_size=12, fill=TEXT_MUTED, max_lines=2)}
    {''.join(column_labels)}
    {''.join(rows)}
    <text x="674" y="170" text-anchor="end" font-size="11" fill="{TEXT_MUTED}" font-weight="700" letter-spacing="1.2">TIME</text>
    <text x="704" y="170" font-size="11" fill="{TEXT_MUTED}" font-weight="700" letter-spacing="1.2">STATUS</text>
    <text x="36" y="{height - 22}" font-size="11" fill="{TEXT_MUTED}">{len(items)} documented Bots · {enabled_count} included in v6</text>
    '''
    path = OUTPUT_DIR / "bot-spectrum.svg"
    write_svg(
        path,
        width,
        height,
        content,
        title="LocalGen built-in bot spectrum",
        description=f"Complexity and status summary for {len(items)} built-in LocalGen bots.",
    )
    return path


def generate_project_pillars(project: dict[str, Any]) -> Path:
    width, height = 820, 472
    card_y, card_width, card_height, gap = 180, 178, 198, 18
    pillars = [
        ("Local matches", "Play offline against the built-in bot roster on random or bundled maps.", ACCENT, project.get("version_line", "v6")),
        ("Map creator", "Build and edit v6 maps with metadata in the desktop application.", BLUE, f"{project.get('map_count', 0)} maps"),
        ("Bot laboratory", "Compare C++ bots with repeatable, parallel simulator runs.", WARNING, f"{project['bot_count']} compiled bots"),
        ("Qt6 foundation", "Build the active cross-platform line with Qt, CMake, and Ninja.", VIOLET, " / ".join(project.get("built_with", []))),
    ]

    cards = []
    for index, (title, summary, color, meta) in enumerate(pillars):
        x = 32 + index * (card_width + gap)
        cards.append(
            rounded_rect(x, card_y, card_width, card_height, 18, fill=CARD_RAISED)
            + f'<rect x="{x}" y="{card_y}" width="{card_width}" height="3" rx="1.5" fill="{color}" />'
            + f'<circle cx="{x + 26}" cy="{card_y + 34}" r="11" fill="{color}" fill-opacity="0.14" stroke="{color}" stroke-opacity="0.5" />'
            + f'<circle cx="{x + 26}" cy="{card_y + 34}" r="4" fill="{color}" />'
            + f'<text x="{x + 20}" y="{card_y + 68}" font-size="16" font-weight="750" fill="{TEXT_MAIN}">{svg_text(title)}</text>'
            + text_block(x + 20, card_y + 96, summary, max_chars=25, font_size=11, fill=TEXT_MUTED, line_height=17, max_lines=4)
            + f'<rect x="{x + 20}" y="{card_y + 157}" width="{card_width - 40}" height="25" rx="12.5" fill="{color}" fill-opacity="0.1" />'
            + text_block(x + card_width / 2, card_y + 174, str(meta), max_chars=20, font_size=9, fill=color, weight="750", anchor="middle", max_lines=1)
        )

    content = f'''
    <rect width="{width}" height="{height}" rx="28" fill="url(#background)" />
    <rect width="{width}" height="{height}" rx="28" fill="url(#grid)" opacity="0.32" />
    <g filter="url(#softShadow)">{rounded_rect(18, 18, width - 36, height - 36, 22, fill=CARD_FILL)}</g>
    <text x="32" y="62" font-size="12" fill="{ACCENT}" font-weight="750" letter-spacing="1.8">PROJECT PILLARS</text>
    <text x="32" y="96" font-size="27" font-weight="750" fill="{TEXT_MAIN}">Why LocalGen feels different</text>
    {text_block(32, 124, "A local-first strategy game, an open bot laboratory, and a cleaner Qt-based future.", max_chars=88, font_size=12, fill=TEXT_MUTED, line_height=18, max_lines=2)}
    {''.join(cards)}
    <text x="32" y="438" font-size="11" fill="{TEXT_MUTED}">{svg_text(project.get('license', 'GPL-3.0-or-later'))} · {svg_text(project.get('organization', 'SZXC-WG'))}</text>
    '''
    path = OUTPUT_DIR / "project-pillars.svg"
    write_svg(
        path,
        width,
        height,
        content,
        title="LocalGen project pillars",
        description="Four LocalGen pillars: offline matches, map creation, bot research, and the Qt6 foundation.",
    )
    return path


def generate_hero_board(project: dict[str, Any]) -> Path:
    width, height = 820, 460
    tile_size, tile_gap = 46, 8
    origin_x, origin_y = 42, 194
    tiles = []
    tile_accents = [ACCENT, BLUE, WARNING, VIOLET]
    for row in range(4):
        for column in range(7):
            x = origin_x + column * (tile_size + tile_gap)
            y = origin_y + row * (tile_size + tile_gap)
            color = tile_accents[(row + column) % len(tile_accents)]
            emphasized = (row, column) in {(0, 0), (1, 3), (2, 5), (3, 2)}
            fill = color if emphasized else CARD_RAISED
            opacity = 0.18 if emphasized else 1
            tiles.append(
                f'<rect x="{x}" y="{y}" width="{tile_size}" height="{tile_size}" rx="15" '
                f'fill="{fill}" fill-opacity="{opacity}" stroke="{CARD_BORDER}" />'
            )

    routes = [
        (88, 217, 250, 217, ACCENT),
        (250, 217, 358, 325, WARNING),
        (142, 379, 358, 379, BLUE),
    ]
    route_paths = []
    for x1, y1, x2, y2, color in routes:
        route_paths.append(
            f'<path d="M {x1} {y1} C {(x1 + x2) / 2:.1f} {y1 - 25}, {(x1 + x2) / 2:.1f} {y2 + 25}, {x2} {y2}" '
            f'stroke="{color}" stroke-width="3" stroke-linecap="round" />'
            f'<circle cx="{x2}" cy="{y2}" r="5" fill="{color}" stroke="{BG}" stroke-width="2" />'
        )

    stats = [
        ("VERSION", project.get("version", "v6"), ACCENT),
        ("TOOLCHAIN", " · ".join(project.get("built_with", [])), BLUE),
        ("PROJECT SCALE", f"{project['release_count']} releases · {project['bot_count']} bots", WARNING),
        ("MAINTAINED BY", project.get("organization", "SZXC-WG"), TEXT_MAIN),
    ]
    stat_rows = []
    for index, (label, value, color) in enumerate(stats):
        y = 184 + index * 60
        stat_rows.append(
            f'<text x="556" y="{y}" font-size="9" fill="{TEXT_MUTED}" font-weight="750" letter-spacing="1.1">{label}</text>'
            + text_block(556, y + 22, str(value), max_chars=24, font_size=15, fill=color, line_height=17, weight="750", max_lines=2)
        )

    content = f'''
    <rect width="{width}" height="{height}" rx="28" fill="url(#background)" />
    <rect width="{width}" height="{height}" rx="28" fill="url(#grid)" opacity="0.3" />
    <g filter="url(#softShadow)">{rounded_rect(18, 18, width - 36, height - 36, 22, fill=CARD_FILL)}</g>
    <text x="38" y="60" font-size="12" fill="{ACCENT}" font-weight="750" letter-spacing="1.8">OFFLINE-FIRST</text>
    <text x="38" y="96" font-size="30" fill="{TEXT_MAIN}" font-weight="750">Play, study, and extend</text>
    <text x="38" y="128" font-size="30" fill="{TEXT_MAIN}" font-weight="750">Local Generals.io.</text>
    {text_block(38, 158, "Local matches, bundled maps, a map creator, and a parallel bot simulator on Qt 6.", max_chars=74, font_size=12, fill=TEXT_MUTED, max_lines=2)}
    {''.join(tiles)}
    {''.join(route_paths)}
    <circle cx="250" cy="298" r="38" fill="{CARD_FILL}" stroke="{ACCENT}" stroke-width="2" />
    <path d="M 250 264 L 260 285 L 283 288 L 266 304 L 270 327 L 250 316 L 230 327 L 234 304 L 217 288 L 240 285 Z" fill="{WARNING}" />
    <line x1="520" y1="174" x2="520" y2="408" stroke="{CARD_BORDER}" />
    {''.join(stat_rows)}
    '''
    path = OUTPUT_DIR / "hero-board.svg"
    write_svg(
        path,
        width,
        height,
        content,
        title="LocalGen project signal board",
        description="A strategy board with LocalGen project branch, toolchain, scale, and maintainer information.",
    )
    return path


def generate_favicon() -> Path:
    content = f'''
    <rect width="64" height="64" rx="18" fill="{ACCENT}" />
    <rect x="7" y="7" width="50" height="50" rx="14" fill="{BG}" />
    <path d="M 16 21 H 48 M 16 32 H 48 M 16 43 H 48 M 21 16 V 48 M 32 16 V 48 M 43 16 V 48" stroke="{CARD_BORDER}" />
    <path d="M32 15 L37 25 L48 27 L40 35 L42 47 L32 41 L22 47 L24 35 L16 27 L27 25 Z" fill="{WARNING}" stroke="{BG}" stroke-width="1.5" />
    '''
    path = STATIC_DIR / "favicon.svg"
    write_svg(
        path,
        64,
        64,
        content,
        title="LocalGen",
        description="LocalGen strategy grid and star mark.",
    )
    return path


def generate_webmanifest(project: dict[str, Any]) -> Path:
    manifest = {
        "name": project.get("project_name", "Local Generals.io"),
        "short_name": project.get("short_name", "LocalGen"),
        "description": project.get("description", "Play generals.io completely offline."),
        "icons": [
            {"src": "/favicon.svg", "sizes": "any", "type": "image/svg+xml", "purpose": "any"},
            {"src": "/favicon.png", "sizes": "512x512", "type": "image/png", "purpose": "any"},
        ],
        "theme_color": BG,
        "background_color": BG,
        "display": "standalone",
        "start_url": "/",
    }
    path = STATIC_DIR / "site.webmanifest"
    path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    return path


def relative_paths(paths: Iterable[Path]) -> str:
    return "\n".join(f"  - {path.relative_to(ROOT).as_posix()}" for path in paths)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate LocalGen website diagrams from repository data.")
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Do not print the generated file list.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    project = load_json("project.json")
    releases = require_items(load_json("releases.json"), "data/releases.json")
    bots = require_items(load_json("bots.json"), "data/bots.json")

    # Data files can be refreshed independently; derive counts from the actual
    # collections so illustrations never show stale totals.
    project["release_count"] = len(releases)
    project["documented_bot_count"] = len(bots)
    project["bot_count"] = sum(bool(bot.get("enabled")) for bot in bots)

    generated = [
        generate_release_journey(releases),
        generate_bot_spectrum(project, bots),
        generate_project_pillars(project),
        generate_hero_board(project),
        generate_favicon(),
        generate_webmanifest(project),
    ]
    if not args.quiet:
        print(f"Generated {len(generated)} LocalGen visual assets:")
        print(relative_paths(generated))


if __name__ == "__main__":
    main()
