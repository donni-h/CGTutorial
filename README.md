## How to clone
Use `git clone --recursive git@github.com:RicoGroth/CGTutorial.git` to clone. Otherwise, the submodules (glew, glfw, glm) won't be cloned.

## How to build
Your current directory has to be `<Your-clone-path>/CGTutorial/`.  
1. Create the Makefile: `cmake -S . -B build`
2. Use make to build the project: `cd build && make`

You should end up with an executable `<Your-clone-path>/CGTutorial/build/CGTutorial`.