# DroneDelivery

## Resources
* Project Management: https://docs.google.com/spreadsheets/d/11xxZHC3XC1sGwpEqG1tOXjks8iv0NYNlaF1fCZc6Xvo/edit?usp=sharing
* Code Samples: https://dev.intelrealsense.com/docs/code-samples
* Pipeline: https://docs.google.com/drawings/d/1kvJUrOsBiEv-U5tZRwZ6jow2ZWX7vi7e-kIwDFJwo5s/edit?usp=sharing

## Obstacle Avoidance Setup

### Installing Visual Studio
1) Navigate to this webpage: "https://learn.microsoft.com/en-us/visualstudio/install/install-visual-studio?view=vs-2022"
2) Click on "Download Visual Studio" button
3) In the Visual Studio Installer, select "Desktop Development with C++"
4) Download/Install while Downloading

### Downloading RealSense Library
1) Navigate to "/c"
2) Create a folder named "git"
3) Navigate to this webpage: "https://github.com/IntelRealSense/librealsense/releases/tag/v2.51.1"
4) Download "Source Code (zip)" from the bottom of the page
5) Extract the download into "/c/git"
6) Rename the extracted folder to "librealsense"

### Installing CMake
1) Navigate to this webpage: "https://cmake.org/download/"
2) Install CMake using Binary distributions (.msi)
3) When installing, make sure to select "Add to path"

### Compiling using CMake
1) Open CMake
2) In "Where is Source Code," add this: "C:/git/librealsense"
3) Navigate to "C:/git/librealsense" and create "build" folder
4) In "C:/git/librealsense/build," add this: "C:/git/librealsense/build"

### Compiling Using Visual Studio
1) Navigate to "C:\git\librealsense\build"
2) Open VS project: "librealsense2.sln"
3) Once open, click on the green "Start without Debugging Button" on the top
4) It should take a couple of minutes
5) Do not worry about the "Access denied - ALL BUILD" error
6) To confirm compilation, navigate to "C:\git\librealsense\build"
7) Here, you should find a "DEBUG" and "x64" folder

### Installing OpenCV
1) Navigate to this webpage: "https://opencv.org/releases/"
2) Choose on appropriate OS to download
3) Open the downloaded file
4) In extract to location, paste: "C:\" and extract

### Configuring Project Files
1) Clone this repository to location of your choice
2) Then, navigate to "C:\git\librealsense\build\DEBUG\"
3) Copy files with names "realsense2d.dll" and "realsense2d.lib"
4) Place them in your git repository with relative path: "\DroneDelivery\ObstacleAvoidance\x64\Debug"
5) Navigate to "C:\opencv\build\x64\vc15\bin"
6) Copy file with name "opencv_world460d.dll"
7) Place the file in your git repository with relative path: "\DroneDelivery\ObstacleAvoidance\x64\Debug"
8) Finally, navigate within git repository to relative path: "\DroneDelivery\ObstacleAvoidance"
9) Open the "ObstacleAvoidance.sln" VS project file
10) Follow instuctions on to configure project: "https://docs.google.com/document/d/1myfsQhaJrfRXf1rcWCboaPYjYce16MSWIf7sAMnLKaw/edit?usp=sharing"
