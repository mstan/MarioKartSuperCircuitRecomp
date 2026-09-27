## Link-cable netplay (new in v0.1.0)

Open the launcher's Netplay tab to host or join a two-player session. Both
players need this release, the same USA game ROM and their own GBA BIOS.
Choose your save file before starting. Use the game's multi-cart link-cable
menus after connecting.

Both input-delay and rollback modes are available. This emulates a local
link cable over recomp-net Internet/LAN sessions; Wireless Adapter, Single-Pak
multiboot and cross-version Pokemon linking are not supported yet.

The Netplay **Your display** setting offers native, 16:9, 21:9, 32:9 and adaptive widescreen.
Each player can choose a different view or window size. These views preserve
the synchronized game state. Gameplay-changing single-player mods are separate
from the netplay view setting.

During netplay, Shift+F1 or closing the game once saves and leaves the entire
paired session. Closing again aborts. Session checkpoints preserve both GBAs;
they do not replace either player's original cartridge save. Checkpoint resume
currently uses the command-line option --netplay-resume.
