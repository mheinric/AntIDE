# Ant IDE

Utilities for simulating a colonies of ant following custom assembly instructions. This project was heavily inspired by the [Moment Swarm](https://dev.moment.com/swarm) challenge, a puzzle 'game' where you have to design an assembly program for ants to efficiently bring back food their nest.

This project provides some tools to verify that the program are syntactically correct, and run/debug the simulation on custom maps. These tools are available through an extension for VSCode that can be downloaded [here](https://github.com/mheinric/AntIDE/releases/download/v1.0.0/antide-1.0.0.vsix).

## Usage 

If you want to try it out, I recommend installing the vscode extension you can find [here](https://github.com/mheinric/AntIDE/releases/download/v1.0.0/antide-1.0.0.vsix). It adds syntax highlighting and debugging support for ant assembly files with the .aasm extension. You can add breakpoints, step through the instructions one at a time, and visualize the current state of the map.

Alternatively, you can run the antide executable directly (though some functionalities are only available through the extension). It supports the following commands: 

```bash
antide check filename
```
Verifying that the assembly file is syntactically correct

```bash 
antide run [--map map_file] filename
```
Runs the ant simulation with the specified program and on the given map.

```bash 
antide lsp
```
Runs an LSP server (over stdio) that can be connected to a text editor to get syntax errors directly into the editor.

```bash
anide dbg
```
Runs a debugger server that can be connected to a text editor for debugging (though DAP over stdio).

## Format for custom maps

Custom maps can be passed to the run command. These maps are plaintext files such as the one below:
```
WWWWWWWWWWWWW
W  NNN  111 W
W  NIN      W
W  NNN  WWWWW
W       223 W
WWWWWWWWWWWWW
```

More precisely, each cell is represented by a single character where:

- ` `/`0`: emtpy cell
- `1`-`8`: cell containing the specified amount of food
- `N`: nest cell where the food must be dropped off.
- `I`: also a nest cell, this is the initial position of the ants. Exactly one `I` cell must be present on the map
- `W`: walls the ants have to get around of. Must be present on all the borders of the map.


## Compiling

Linux is required for compiling and running the project.

Install the nodejs dependencies for the vscode extension part of the project: 
```bash
cd vscode_extension && npm install
```

Generate the main executable and the extension package: 
```bash 
make all
```
The generated artifacts are in the `bin` folder.

## Motivation

Some time ago, I took part in the Moment Swarm challenge. It was really nice (I recommend trying it out if you are into this sort of puzzles), however I had moments of frustration due to the lack of some debugging tools such as:

- stepping through instructions one by one
- inspecting the state of specific cells (and in particular their pheromone levels)
- the simulation seemed a bit slow for something that 'just' needs to simulate 200 ants on 2000 steps.

Since then I have been wondering if I could implement these features into tools of my own, and this project was then answer: Yes, but... implementing all the other features that the original simulator already had took me quite a bit more time than expected. It still was quite interesting, so I don't regret any of it.

## Notes
While this projects provides additional debugging tools when running the simulation, it is still lagging behind the original simulator in the following aspects: 

- map generation: very little is implemented here to generate interesting procedural maps
- some UI elements are missing, such as the ability to highlight ants with a specific tag, or choosing the random seed used for the simulation/map generation.

## License
MIT