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

./mediacntrl firefox position 10

./mediacntrl play-pause