# Quick Tug Event

Author: Kenechukwu Echezona

Design: A game of tug of war, controlled by competitive quick time events.

Networking: Clients transmit directional inputs to the server via PlayMode. The inputs are handled by handle_event in PlayMode, are sent via Game, and actually polled (aka the input buffer is actually fully communicated) in PlayMode. Admittedly, there are not too many changes from the starter client code in this regard.

Much of the changes I made come from what the server does with the input. The server does two passes over the game state:
- The first pass processes all the inputs together under the current game state, to determine how both the game state and individual player data should change. I use a nextState variable to handle things like ties between the players. It's sort of like processing the inputs as one input.
- The second pass reasons on this new state and data, incrementing timers and handling state transitions from this new state.
PlayMode handles drawing based on the game state in `game`.
Without this approach, utilizing the given structure, problems would arise. For example, timers would increment faster if handled in the loop of players unless I normalized the time user player size; but why not just do that stuff outside of the loop?

The game only supports two players at once; in addition to the list of players, I keep a static `activePlayerCount` and gave every `Player` a boolean member called `activePlayer`. If more players than two players join, the game will make them spectators (basically treating them like a player whose inputs don't matter). A player dropping out will have them be replaced by the spectator who joined earliest.

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:
- WASD: Up/Left/Down/Right
- When the arrow appears, press in that direction before your opponent to start pulling on the rope.
- If you win the QTE, you'll keep pulling the rope. Press the same direction to stop pulling and prevent being countered.
- If you lose the QTE, you can initiate a counter by pressing in the opposite direction. Successfully countering before your opponent stops pulling will undo all of their progress for that particular interaction, and grant you 50% of the progress they had made!
- Be the first to pull the rope to your side!

Strategy:
- Reaction time is key here. If your reaction time is better than your opponent, you're more likely to win.
- If you win a QTE, consider how long it'll take for your opponent to 1) react to losing the QTE, and 2) determine the right button to counter. If you get too greedy and underestimate your opponent, you can lose a lot of progress.
- Not all is lost if your opponent has a better reaction time! If you're aware that your opponent reacts faster than you, then you can mentally prepare yourself to counter before they do.
- Of course, your opponent can anticipate this strategy and bait you into a false start, netting them a ton of progress!
- Be aware of the Tug Clock. It should really only matter if you're turtling (ending your pulls very quickly), but it can set you back very quickly.

Sources: (TODO: list a source URL for any assets you did not create yourself. Make sure you have a license for the asset.)

This game was built with [NEST](NEST.md).

