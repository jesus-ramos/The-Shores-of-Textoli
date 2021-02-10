Implemenation of the solo game of The Shores of Tripoli by Kevin Bertram
published by Fort Circle Games.

No external dependencies. You may have to resize your terminal to get everything
to fit best on it. If your terminal does not support extended color codes the
ally pirate corsairs may not display in the correct color (orange).

Building:
`make`

Running:
`./sot`

Game Screen:

Top Row:

Year track - contains the track of years the game will play through. Current
year will be highlighted in bold text and any frigates that will be deployed in
a year will be listed next to it.

Season - simple season display of the 4 seasons the game progresses through

Destroyed US Frigates - Count of US frigates destroyed by the tripoli bot

Tripolitan Gold - How much gold the tripoli bot has taken during its pirate
raids

Game Seed - The PRNG seed for the card draws and dice rolls used in the game. If
you'd like to replay a game you can pass the seed to the executable to get the
same sequence of card draw, rolls, and bot actions.

Map:

All the locations in the game. All locations contain a harbor and some contain a
Patrol Zone where frigates can intercept pirates. During battle active battle
locations will be highlighted in red.

F - Frigates on the map, US frigates in blue and Tripoli Frigates in red. When
frigates are damaged they will be unhighlighted.

C - Tripolitan corsairs displayed in red, allies of Tripoli will be displayed in
orange.

G - US Gunboats

A - Arab Infantry that is part of Hamet's Army

M - US Marine Infantry that is part of Hamet's Army

P - Tripolitan infantry

T-Bot Log:

When you first start the game this area will be empty but during the game it
will display a log of the Tripoli Bot decisions like battle cards played, cards
discarded, and actions taken.

Core Cards:

The US core cards that are available until played and don't take up space in your hand.

Hand:

List of cards in your hand along with descriptions.

Playable cards will be highlighted in white. Battle cards are always displayed
in red and cards that do not meet the conditions for being played yet will have
their names crossed out.

Command prompt:

Input ?/h/help to get a list of commands. Some cards will have their own prompts
such as specifying movement for your ships or how many gunboats to bring to a
battle.

For game rules you can find a copy of the rulebook for the original game in the
Files section of BoardGameGeek for the game. This implementation only implements
the solo mode and follows the solo rules for card play for the tripolitan
player.
