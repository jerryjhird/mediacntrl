# mediacntrl

an MPRIS/Music playback controller for systemd linux systems using libsystemd/sd-bus

basically a playerctl alternative

### usage:

```
mediacntrl [-q <query>] (play|pause|play-pause|next|previous) [-nstdout (optional)]
```

### example:

```
mediacntrl -q spotify next
```

### Compilation

to compile make sure you have libsystemd-dev sometimes named libsystemd-devel or systemd-devel and that you are running systemd as your init system

also make sure you have a modern c compiler and meson + ninja

to build:

```
meson setup build
ninja -C build
```

the output file should be stored here

```
{PROJECT_ROOT_DIR}/build/src/mediacntrl
```
