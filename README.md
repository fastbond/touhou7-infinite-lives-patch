# Touhou 07 Infinite Lives Patch
Modifies the Touhou 07 game executable to give the player infinite lives.
This patch is meant purely as a practice tool, to avoid clearing 10+ minutes just for a few seconds of experience on a particularly fun or difficult pattern. 
To prevent cheating, all generated scores are set to zero.

#### NOTE: This was written for the official 1.00b version and older english patches.  Apparently it breaks when used in conjunction with the newer thcrap english patches, but there are no plans to go back and investigate.

## Details
* Infinite Lives - Players do not lose lives upon death. Total deaths are still tracked on final score screen.
* Scoring - Score is always set to 0. 
* Power - Power is maintained upon death. (if > 16)

## Usage
```patch inputfile outputfile```

Ex:
```patch th07.exe th07_infinite_lives.exe```

Back up game and score files before use.
