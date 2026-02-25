# FoP-Project-G66
FoP Project G66
Scratch Engine
A Scratch-like visual programming environment built with C++ and SDL2.

Team Members
•	Amirhossein Abolhassani
•	Amir Foroughi Esfahani

Course
Fundamentals of Programming (FoP) — Group 66

Features
•	Visual block-based programming workspace
•	Motion, Looks, Sound, Control, Events, Operators, Variables, Sensing blocks
•	Custom function (My Blocks) definition and calling with parameters
•	Pen drawing tools with Erase All support
•	Sprite management with costume editor (draw, erase, fill, line, circle, rect)
•	Backdrop system with built-in and custom images
•	Project save/load (.scratch format) with full serialization
•	Step-by-step debugging and logger
•	Run with Enter key, Pause/Resume/Stop controls
•	Keyboard and mouse sensing
•	Timer and reset timer
•	Touching edge detection
•	Variable display during execution

Built With
•	C++17
•	SDL2
•	SDL2_ttf
•	SDL2_mixer
•	SDL2_image
•	CMake

How to Build
1. Clone the repository
2. Open with CLion or run CMake manually:
    mkdir build && cd build && cmake .. && make
3. Run the executable from the project root directory

How to Use
•	Select block categories from the left sidebar
•	Drag blocks to the workspace and snap them together
•	Click Run or press Enter to execute your program
•	Use Pause/Resume/Stop buttons to control execution
•	Use Step button for step-by-step debugging
•	Click the + button to add sprites
•	Click the edit icon on a sprite to open the costume editor
•	Use File > Save Project / Load Project to manage your work
•	Use My Blocks category to define custom functions

