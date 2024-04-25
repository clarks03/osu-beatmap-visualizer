# osu! Beatmap Visualizer

A simple tool that allows you to visualize an osu! beatmap, without osu!, written in C++.

### Features

- Circles
- Accurate Circle Size and Approach Rate
- Combo Colours
- Background and music

#### TODOs

- Sliders
- Combo Numbers
- Spinners

### Requirements

- UNIX
    - Right now, the `<unistd.h>` library is being used for the `sleep()` command (on line 158). Windows equivalents should work fine.
- SFML
    - Look [here](https://www.sfml-dev.org/download.php) for installation instructions for your OS.
- libzip
- gcc (although this should come with UNIX)

### Installation

Clone the repository and change directory inside it:

```bash
git clone https://github.com/clarks03/osu-beatmap-visualizer.git && cd osu-beatmap-visualizer
```

Then, when inside the directory, run:

```bash
make
```

This assumes that SFML has already been installed.

#### "Uninstall"

To uninstall the program, `cd` into the directory and run:

```sh
make clean
```

### Setup

**Edit Apr. 25: Significant improvements to setup!!**

To visualize a beatmap properly, you need 5 things:
1. A hitcircle
2. A hitcircleoverlay
3. An approachcircle
4. Hitcircle numbers
5. A sound effect (could be hitsound or anything else)

You can copy a hitcircle, hitcircleoverlay, and approachcircle, titles `hitcircle.png`, `hitcircleoverlay.png` and `approachcircle.png` respectively, from a pre-existing osu! skin to the repository directory. 

You can also copy the hitcircle numbers to the repo directory. Most of the time these are named `default-<num>.png`, but it may vary depending on your skin. Make sure to check the `skin.ini` file first. 

**NOTE:** make sure that you are copying in the **standard resolution** versions of the aforementioned skin elements. High-resolution elements (i.e. elements named `<thing>@2x.png`) will **NOT** work (yet, at least.)

You can also copy a hitsound (or any other sound effect) to the repository directory, **ensuring that it is named `untitled.mp3`.

To load a map to be visualized, simply place the .osz file in the repo directory. The program will take care of the rest.

### Usage

To run the file, simply run:

```sh
make  # if you haven't done so already
./osu-parser
```

You will be asked which beatmap/difficulty you want to visualize, and that should be pretty straightfoward to choose.

### Roadmap

This project is heavily W.I.P and will be receiving active updates. Eventually I want to turn this into a full-fledged osu! replay viewer, where you can automatically download the map given a `.osr` (replay) file. 
