# Chip-8

Chip-8 emulator in C++ using SDL2 for graphics, audio and input.

### Requirements
- g++ (C++23)
- SDL2 library
  ```bash
  sudo apt install g++ libsdl2-dev make
  ```

# Run emulator

From terminal, run the below command line

```bash
make build
./run.out "roms/IBM Logo.ch8"
```

![IBM Logo](images/IBM%20Logo.png)

All available ROMs are located in the `roms/` folder.