 [![CircleCI Build Status](https://circleci.com/gh/alexmyczko/modplug-tools.svg?style=shield)]

A few audio playing tools for libopenmpt-modplug-dev or libmodplug-dev (for the command line).

modplugplay is written for the OSS audio output (/dev/dsp).
modplug123 uses libAO (from xiph.org) for better cross platform audio support.

```
Usage:

modplugplay `find | shuf`               # for OSS
modplug123 filename1.mod filename2.xm   # for ALSA

Screenshot:
$ modplug123 floppi_*
floppi_dadam6.xm [1/2] [44367]
playing dadam6 (11.8/51") (44100/2/16)    

The player runs interactively, displays
FILENAME [SONG/TOTALSONGS] [SIZE]
STATUS TITLE (POSITION/TOTALPLAYTIME") (bitrate/stereo/bits)

Keyboard control:
l - toggle loop play a song
p - toggle pause playing
n - next song
N - previous song
r - random song
f/b - forward/backward 10 seconds
```
