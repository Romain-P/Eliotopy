# Eliotopy - Dofus Tactical Overlay

Eliotopy is a tactical visual assistant designed for Eliotrope players in Dofus. The project aims to simplify portal network management, distance calculation, and complex redirection planning.

**Note:** To ensure account safety, this tool does not inject code or read the game client's memory. It operates entirely through game capture and a transparent overlay.

## Discord

* [Community Discord](https://discord.gg/xYMN5yXNkj)

## Download

* [Download Eliotopy v0.0.2](https://github.com/Romain-P/Eliotopy/releases/download/0.0.2/eliotopy.zip)
* Extract zip files, run dofus, log your character
* Run Eliotopy.exe
* Start a fight and enjoy Eliotopy

## Technical Stack

* **Language:** C++
* **Computer Vision:** OpenCV (screen analysis, isometric grid detection)

## Current Features

* **Smart Grid Detection:** Analyzes the game screen to dynamically generate and align an isometric grid matching the current combat map.
* **Visual Portal Management:** Manual portal placement using custom keybinds. The overlay displays portals on the grid with strategic color-coding: portals 1 and 2 are highlighted in red/orange to indicate they will be the next to disappear from the queue.
* **Distance Calculation:** Real-time display of distances (in MP/cells) between active portals in the network.
* **Interactive Menu & Keybinds:** Integrated overlay UI allowing on-the-fly modification of shortcuts (mouse and keyboard).
* **Real-time Saving:** Configuration `.ini` file updates instantly upon any settings modification.
* **Redirection Preview:** Visual representation of a redirection through a fake portal (handles equidistances, double equidistances, etc..)
* **Redirection Helper:** Set a desired exit portal to highlight the specific cells where an entrance portal can be placed to validate the redirection.

## Roadmap
* **Portal detection:** scans pixels for portals & OCR for portal numbers

## Showcase

<img width="699" height="644" alt="Eliotopy_u5isIh4teg" src="https://github.com/user-attachments/assets/91b4e94e-a7a6-440b-b8fc-279310764a27" />
