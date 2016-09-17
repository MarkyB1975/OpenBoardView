## Open Board Viewer [![Build Status](https://travis-ci.org/inflex/OpenBoardView.svg?branch=inflex-ui-features)](https://travis-ci.org/inflex/OpenBoardView)

Linux SDL/ImGui edition software for viewing .brd files, intended as a drop-in
replacement for the "Test_Link" software.


[![Demo Video](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot.png)](https://www.youtube.com/watch?v=rObeatsf660)


![Common net pin halo](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot-halo.png)

![Part searching](https://github.com/inflex/OpenBoardView/blob/inflex-ui-features/asset/screenshot-partsearch.png)


### Features

- Annotations (per board database file)
- Part and pin sizes better represented
- Better outlining of irregular objects (such as connectors)
- Drag and drop
- Recently used file history
- Non-orthagonally orientated caps/resistors/diodes now drawn more realistically
- Adjustable DPI (for working on 2K/4K screens)
- Works with multiple concurrent instances


### TODO

- Decode more board formats
- Compound project/file format


### Prerequisites

For Ubuntu developers, you'll need the following packages at a minium;

	$ apt-get install git build-essential cmake libsdl2-dev libgtk-3-dev

### Installation

1. Clone the project

    $ git clone --recursive 'https://github.com/inflex/OpenBoardView'

2. Build it

    $ ./build.sh

3. Run it!

	$ ./bin/openboardview
	...or...
	$ ./openboardview.sh


### Usage

- N: Search by power net
- C: Search by component name

- w/a/s/d: pan viewport over board
- x: Reset zoom and center
- Mouse scroll, -/=: Zoom out/in
- Mouse click-hold-drag, Numeric pad up/down/left/right: pan viewport over board
- Numeric pad +/-: zoom board
- Numeric pad 5: Reset zoom and center
- Space, Middle mouse click: Flip board
- R/./Numpad-Del: Rotate clockwise
- ,/Numpad-Ins: Rotate counter-clockwise
- L: Show net list
- K: Show part list


### Discussion

IRC Freenode channel: **#openboardview**
