## Mediacntrl 
a cli for controlling media players

### Usage:
```  
./mediacntrl <command> [args]
./mediacntrl <player> <command> [args]
```

### Commands:
```
play            start playback
pause           pause playback
play-pause      toggle playback
stop            stop playback
next            next track
previous        previous track
position <s>    jump to position (seconds)
```

### Examples:

./mediacntrl play

./mediacntrl vlc pause

./mediacntrl librewolf position 10

./mediacntrl play-pause

### Web api

if compiled with `-DHTTP_SUPPORT` (the makefile does this) you will be able to pass --http which runs a http server in the foreground

the layout should be:
"http://localhost:port/player-query/command"

#### Examples (for interacting using curl):

curl http://localhost:8000/spotify/play-pause

curl http://localhost:8000/spotify/position/30

curl http://localhost:8000/spotify/next

curl http://localhost:8000/spotify/previous