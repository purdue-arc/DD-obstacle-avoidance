# DroneDelivery

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
6) To confirm compilation, navigate to "C:\git\librealsense\build". There should be a "DEBUG" and "x64" folder here.

### Installing OpenCV

### Configuring Project Files
