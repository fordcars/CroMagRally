# How to build Cro-Mag Rally for 3DS

1. Build and install SDL2 for 3DS using [these instructions](https://wiki.libsdl.org/SDL2/README/n3ds).
2. Build PicaGL:
```bash
cd extern/Pomme/extern/picaGL
make
```
2. Build Pomme:
```bash
cd extern/Pomme
make
```
4. Build Cro-Mag Rally:
```bash
make
```