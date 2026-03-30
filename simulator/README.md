# Bot Simulator

`LocalGen-bot-simulator` is a lightweight CLI for bot-vs-bot evaluation.

## What it does

- creates random maps with the existing core game engine
- instantiates registered bots through `BotFactory`
- runs repeated matches without the Qt UI
- prints per-game outcomes and an aggregate summary

## Example

After building, run the executable from `build/Release`:

- `./LocalGen-bot-simulator --games 10 --width 20 --height 20 --steps 600 --bots XiaruizeBot GcBot`
- `./LocalGen-bot-simulator --games 10 --map maps/arena01.lgmp --steps 600 --bots XiaruizeBot GcBot`

## Custom maps

- `--map PATH` loads a custom map for every simulated game.
- Only v6 `.lgmp` maps are supported by this flag.
- When `--map` is provided, `--width` and `--height` are ignored.

## Notes

- This simulator uses the same core board/game logic as the app.
- Independent matches are parallelized across CPU threads.
- If `--threads` is omitted, the simulator auto-selects a worker count from the machine's available CPU concurrency.
- Per-game results are printed as soon as each match finishes, so completion order may differ from game number order.
- Aggregate summary output is printed as a table, including each bot's FFA TrueSkill rating with a 95% confidence interval as well as win-rate confidence intervals.
- No reinforcement learning assets were added for this bot.
