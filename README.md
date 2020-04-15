
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/fd91d2aec66d4bf6b35e6d9b3f1ccc76)](https://app.codacy.com/app/alexmyczko/modplug-tools?utm_source=github.com&utm_medium=referral&utm_content=alexmyczko/modplug-tools&utm_campaign=Badge_Grade_Dashboard)

![modplug-tools Build Status](https://circleci.com/gh/alexmyczko/modplug-tools.svg?style=shield)

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
```
<kbd>l</kbd> - toggle loop play a song
<kbd>p</kbd> - toggle pause playing
<kbd>n</kbd> - next song
<kbd>N</kbd> - previous song
<kbd>r</kbd> - random song
<kbd>f</kbd> / <kbd>b</kbd> - forward/backward 10 seconds
