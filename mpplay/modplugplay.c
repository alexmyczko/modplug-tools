
/*
 * Example Interface for libmodplug without using XMMS
 **
commandline interface to modplugxmms library
gurkan@linuks.mine.nu www.linuks.mine.nu
Copyright (C) 2003 Gürkan Sengün

TODO
unlock /dev/dsp when in 'p'ause mode
and have a look at SNDCTL_DSP_GETxPTR
more functions
sig handling
12:23 < tp76> tarzeau: yes. I prefer `T *foo = malloc (N * sizeof *foo);'.
use stat() to get filesize
run indent -kr

14:08 < tarzeau> so the player isn't stuck when anotehr thing is using /dev/dsp
14:08 < tarzeau> if that works a msg is displayed
14:08 < tarzeau> later the song is tried to be opened
14:08 < tarzeau> then the soundcard settings are set
14:09 < tarzeau> the the module is load using libmodplug
14:09 < tarzeau> module settings being set
14:09 < tarzeau> keyboard handling set
14:09 < tarzeau> the status string set up
14:09 < tarzeau> time stuff and the main loop
14:09 < warp> ok.
14:09 < tarzeau> where keyboard input is handled
14:09 < tarzeau> that's more or less it
14:10 < warp> see, you've just described all the functions you could make.
14:10 < warp> so, start that for loop with a   init_sound_device ()   or 
      something.
14:10 < warp> etc...  make functions for all the things you just described.
14:10 < tarzeau> heh good idea, i'll copy paste this irc talk and do that for 
       the next version like 0.9 with the sighandler for people 
        trying to ctrl-c or ctlr-z

maybe add support for write a .wav file for later play or .ogg
and maybe add support for gzip -d some.mod.gz | modplugplay
add support to allow modplugplay *.IT *.it *.s3m *.xm (n)ext (N)ot next
and -r for random order (make a random played/unplayed (bit)list)
show file size

argument processing
if it's not a file, we check if it's an option if not we ignore it

action keys
-> like key (put in list/directory)
-> dislike key (remove)
-> show info

|| [] >> << >| ()

TODO
or maybe just a $HOME/.modplugplay or /etc/modplugplay
command line option handling
  -l --loop
  -m --mono
  -s --stereo (default)
  -4 --44100 (default)
  -2 --22050
  -1 --11025
  -8 --8bit (default is 16bit)
  -h --help
  -q --quiet
  -v --version (head -c1080 module |strings)
  -i --info    don't play, only show module info (songname,length,size,type)
*/

#include <stdio.h>			/* printf */
#include <string.h>			/* strcpy */
#include <stdlib.h>			/* srand/rand */
#include <unistd.h>
#include <libmodplug/modplug.h>			/* core */
#include <sys/ioctl.h>			/* control device */
#include <fcntl.h>

#ifdef __NetBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <sys/time.h>			/* gettimeofday */
#include <time.h>
#include <sys/poll.h>			/* poll for keyboard input */
#include <termios.h>			/* needed to get terminal size */

#define VERSION "1.1"

#define BUF_SIZE 4096
#define DEVICE_NAME "/dev/dsp"

static struct termios stored_settings;
int audio_fd, mixer_fd;
unsigned char audio_buffer[BUF_SIZE];

typedef struct {
	int x, y;
} term_size;

/* inquire actual terminal size */
static int get_term_size(int fd, term_size *t) {
#ifdef TIOCGSIZE
    struct ttysize win;
#elif defined(TIOCGWINSZ)
    struct winsize win;
#endif

#ifdef TIOCGSIZE
    if (ioctl(fd,TIOCGSIZE,&win)) return 0;
    if (t) { 
	t->y=win.ts_lines;
    	t->x=win.ts_cols;
    }
#elif defined TIOCGWINSZ
    if (ioctl(fd,TIOCGWINSZ,&win)) return 0;
    if (t) {
        t->y=win.ws_row;
        t->x=win.ws_col;
    }
#else
    {
	const char *s;
	s=getenv("LINES");   if (s) t->y=strtol(s,NULL,10); else t->y=25;
	s=getenv("COLUMNS"); if (s) t->x=strtol(s,NULL,10); else t->x=80;
    }
#endif
    return 1;
}

void set_keypress(void)
{
    struct termios new_settings;
    tcgetattr(0,&stored_settings);
    new_settings=stored_settings;
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_lflag &= (~ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0,TCSANOW,&new_settings);
    return;
}

void reset_keypress(void)
{
    tcsetattr(0,TCSANOW,&stored_settings);
    return;
}

void ansi_cursor(int visible)
{
    if (visible) {
	printf("\033[?25h");
    } else {
	printf("\033[?25l");    
    }
}

void versioninfo()
{
	printf("\nCopyright (C) 2003 Gürkan Sengün\n");
	printf("Version %s compiled on %s at %s.\n",VERSION,__DATE__,__TIME__);
}

void help(char *s, int exitcode)
{
	printf("Copyright (C) 2003 Gürkan Sengün\n");
        printf("Version %s compiled on %s at %s.\n",VERSION,__DATE__,__TIME__);
	printf("\n");
	if (exitcode!=0)
		printf("%s: too few arguments\n",s);
	printf("Usage: %s" /*[OPTIONS]*/" [FILES]\n",s);
	printf("\n");

	printf("  -v/--version  print version info\n");
	printf("  -h/--help   print help\n");
	printf("  -l   start in looping mode\n");
/*
	printf("  -r   randomize play sequence\n");
	printf("  -c   write to console instead of %s\n",DEVICE_NAME);
	printf("  -i   use stdin for file input\n");
	printf("  -q   be quiet\n");
*/	
	exit(exitcode);
}

int get_byteorder() 
{
#define sz sizeof(int)
    int ival;
    int format;
    char s[sz];
    char t[sz];
    int i, lit, big;

    for (i=0; i<sz; i++) s[i] = i;
    ival = *(int *)s;
    big = lit = 0;

    for (i=0; i<sz; i++) {
        char c = ival&0xff;
        ival >>= 8;
        if (s[i] == c) lit++;
        if (s[sz-i-1] == c) big++;
        t[i] = c;
    }
    if (lit == sz && big == 0) {
        /*printf("little endian\n");*/
	format = AFMT_S16_LE;
    } else if (big == sz && lit == 0) {
        /*printf("big endian\n");*/
	format = AFMT_S16_BE;
    } else {
	format = -1;
    }
    return(format);
}

char *getFileData(char *filename, long *size)
{
    FILE *f;
    char *data;

    f = fopen(filename, "rb");
    if (f == NULL) {
      return NULL;
    }
    fseek(f, 0L, SEEK_END);
    (*size) = ftell(f);
    rewind(f);
    data = (char*)malloc(*size);
    fread(data, *size, sizeof(char), f);
    fclose(f);

    return(data);
}

int getpcmvol()
/* get absolute pcm volume, 0 - 100 (for left+right) */
{
    int vol;
    
    if ((mixer_fd=open("/dev/mixer", O_RDONLY | O_NONBLOCK, 0)) == -1) {
	printf("can't open mixer\n");
	exit(1);
    }
    ioctl(mixer_fd,MIXER_READ(SOUND_MIXER_PCM),&vol);
    close(mixer_fd);

    vol=vol & 255;
    if (vol==100) vol=99;
    return(vol);
}

int setrelpcmvol(int newvol)
/* set relative pcm volume, -100 .. 100 */
{
    int currentvol=getpcmvol();
    currentvol+=newvol;
    if (currentvol>100) currentvol=100;
    if (currentvol<0) currentvol=0;
    newvol=currentvol+(currentvol*256);

    if ((mixer_fd=open("/dev/mixer", O_RDONLY | O_NONBLOCK, 0)) == -1) {
	printf("can't open mixer\n");
	exit(1);
    }
    ioctl(mixer_fd,MIXER_WRITE(SOUND_MIXER_PCM),&newvol);
    close(mixer_fd);
}

int main(int argc, char* argv[])
{
    long size;
    char *d;
    term_size terminal;
    ModPlugFile *f2;
    int mlen, len;
    struct timeval tvstart;
    struct timeval tv;
    struct timeval tvpause;
    struct timeval tvunpause;
    struct timeval tvptotal;
    char status[161];
    char songname[41];
    char notpaus[4];
    
    int vol=getpcmvol();
    int songsplayed = 0;

    ModPlug_Settings settings;
    ModPlug_GetSettings(&settings);

    int format;
    int channels = 2;
    int speed = 44100;

    int c; // kontest
    char buffer[128];
    int result, nread;
    struct pollfd pollfds;
    int timeout = 1;            /* Timeout in msec. */
    int loop=0; // kontest
    int pause=0;
    int mono=0;
    int bits=0;
    int song;
    int millisecond;
    char *randplayed; /* randomly played songs
			 songs that are n/N'd are done or not? */
    /* what about N if the previous song is not playable? */
    /* maybe mark it played in randplayed */

    // [rev--dly--] [sur--dly--] [bas--rng--]
    int rev=0;    // a
    int revdly=0; // s
    int sur=0;    // d
    int surdly=0; // y
    int bas=0;    // x
    int basrng=0; // c

    if ((format = get_byteorder()) == -1) {
        return 1;
    }

    /* Initialize pollfds; we're looking at input, stdin */
    pollfds.fd = 0;             /* stdin */
    pollfds.events = POLLIN;    /* Wait for input */

    if (argc==1) {
	help(argv[0],1);
    }

    if (!get_term_size(STDIN_FILENO,&terminal)) {
	fprintf(stderr,"warning: failed to get terminal size\n");
    }
    
    srand(time(NULL));

for (song=1; song<argc; song++) {

/* check if arguments need to be parsed */
    if (argv[song][0] == '-') {
      if (!songsplayed && strstr(argv[song],"-h")) {
        printf("\n");
        help(argv[0],0);
      } else if (!songsplayed && strstr(argv[song],"-v")) {
	versioninfo();
        exit(0);
      } else if (strstr(argv[song],"-l")) {
        loop=1;
        continue;
      }
      if (argv[song][1] == '-') { // not a song
        if (strstr(argv[song],"--help")) {
          help(argv[0],0);
        } else if (strstr(argv[song],"--version")) {
	  versioninfo();
	  exit(0);
	}
        continue;
       }
      }

    /* O_NONBLOCK gave me the problem 
    that after about 5 seconds writing audiobuffer to DEVICE_NAME
    was not completed, like 3072 bytes instead of the full buffer 4096
    which made the sound get faster (and wrong)
    */
    if ((audio_fd=open(DEVICE_NAME, O_WRONLY | O_NONBLOCK, 0)) == -1) {
	fprintf(stderr,"%s (%s): ",DEVICE_NAME,argv[song]);
	perror("");
	exit(1);
    }
    close(audio_fd);

    if ((audio_fd=open(DEVICE_NAME, O_WRONLY /*| O_NONBLOCK*/, 0)) == -1) {
	perror(DEVICE_NAME);
	exit(1);
    } else {
	printf("opened %s for playing ",DEVICE_NAME);
    }
    printf("%s ",argv[song]);
    printf("[%d/%d]",song,argc-1);

    d = getFileData(argv[song], &size);
    if (d == NULL) continue;
    printf(" [%ld]\n",size);

    if (ioctl(audio_fd,SNDCTL_DSP_SETFMT, &format) == -1) {
	perror("SND_CTL_DSP_SETFMT");
	exit(1);
    }

    if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
	perror("SNDCTL_DSP_CHANNELS");
	exit(1);
    }
    /* int mChannels; */   /* Number of channels - 1 for mono or 2 for stereo */
    /* int mBits; */       /* Bits per sample - 8, 16, or 32 */
    /* int mFrequency; */  /* Sampling rate - 11025, 22050, or 44100 */

    if (ioctl(audio_fd,SNDCTL_DSP_SPEED,&speed) == -1) {
	perror("SNDCTL_DSP_SPEED");
	exit(1);
    }

    /* Note: Basic settings must be set BEFORE ModPlug_Load */
    settings.mResamplingMode = MODPLUG_RESAMPLE_FIR; /* RESAMP */
    settings.mChannels = 2;
    settings.mBits = 16;
    settings.mFrequency = 44100;
    /* insert more setting changes here */
    ModPlug_SetSettings(&settings);

    f2 = ModPlug_Load(d, size);
    if (!f2) {
	printf("could not load %s\n", argv[song]);
	close(audio_fd);
	free(d); /* ? */
    } else {
      songsplayed++;
/*    settings.mFlags=MODPLUG_ENABLE_OVERSAMPLING | \
                    MODPLUG_ENABLE_NOISE_REDUCTION | \
		    MODPLUG_ENABLE_REVERB | \
		    MODPLUG_ENABLE_MEGABASS | \
		    MODPLUG_ENABLE_SURROUND;*/

//    settings.mReverbDepth = 100; /* 0 - 100 */ *   [REV--DLY--]
//    settings.mReverbDelay = 200; /* 40 - 200 ms  00-FF */ 
//    settings.mSurroundDepth = 100; /* 0 - 100 */   [SUR--DLY--]
//    settings.mSurroundDelay = 40; /* 5 - 40 ms */  
//    settings.mBassAmount  = 100; /* 0 - 100 */     [BAS--RNG--]
//    settings.mBassRange   = 100; /* 10 - 100 hz */ 
// [REV--DLY--] [SUR--DLY--] [BAS--RNG--]
// [rev--dly--] [sur--dly--] [bas--rng--]

    set_keypress();
    strcpy(songname, ModPlug_GetName(f2));

    /* if no modplug "name" - use last 41 characters of filename */
    if (strlen(songname)==0) {
        int l = strlen(argv[song]);
	char *st = argv[song];
        if (l >= 41) st = argv[song] + l - 41;
        strncpy(songname,st,41);
        songname[41] = 0;
    }
    sprintf(status,"[1Gplaying %s (%%d.%%d/%d\") (%%d/%%d/%%d%%s)    \b\b\b\b",songname,ModPlug_GetLength(f2)/1000);
    if (loop) sprintf(status,"[1Glooping %s (%%d.%%d/%d\") (%%d/%%d/%%d%%s)    \b\b\b\b",songname,ModPlug_GetLength(f2)/1000);

    gettimeofday(&tvstart,NULL);
    tvptotal.tv_sec=tvptotal.tv_usec=0;
    mlen=1;
    
    while(mlen!=0) {
	if (mlen==0) { break; }

	if (!pause) {
	    gettimeofday(&tv,NULL);
	    mlen=ModPlug_Read(f2,audio_buffer,BUF_SIZE);
	    if ((len=write(audio_fd,audio_buffer,mlen)) == -1) {
		perror("audio write");
		exit(1);
    	    }
	    /*printf("%d %d\n",mlen,len);*/
        }
	printf(status,tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec,tv.tv_usec/100000,speed,channels,settings.mBits/*,rev,revdly,sur,surdly,bas,basrng*/);
	fflush(stdout);

	if ((mlen==0) && (loop==1)) {
	    /*printf("LOOPING NOW\n");*/
	    ModPlug_Seek(f2,0);
	    gettimeofday(&tvstart,NULL);
	    mlen=ModPlug_Read(f2,audio_buffer,BUF_SIZE);
	    tvptotal.tv_sec=tvptotal.tv_usec=0;
	}

        result = poll(&pollfds, 1, timeout);
        switch (result) {
        case 0:
            /*printf(".");*/
            break;
        case -1:
            perror("select");
            exit(1);

        default:
            if (pollfds.revents && POLLIN) {
	        nread = read(0, buffer, 1); /* s/nread/1/2 */
                if (nread == 0) {
                    printf("keyboard done\n");
                    exit(0);
               } else {
                    buffer[nread] = 0;
                    /* printf("%s", buffer); */

		    if (buffer[0]=='q') { mlen=0; song=argc; ioctl(audio_fd,SNDCTL_DSP_RESET,0); } /* quit */

		    if (buffer[0]=='f') {
			if ((tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec+10) < (ModPlug_GetLength(f2)/1000)) {
			    ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			    ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
			    tvstart.tv_sec-=10;
			}
		    } /* forward 10" */

		    if (buffer[0]=='b') {
			if ((tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec-10) > 0) {
			    ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			    ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000-10000);
			    tvstart.tv_sec+=10;
			}
		    } /* backward 10" */
		    
		    /*
		    if (buffer[0]=='i') {
			printf("\n");
		    } */

/*
		    if (buffer[0]=='a') {
			rev++; settings.mReverbDepth=rev;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='A') {
			rev--; settings.mReverbDepth=rev;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='s') {
			revdly++; settings.mReverbDelay=revdly;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='S') {
			revdly--; settings.mReverbDelay=revdly;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='d') {
			sur++; settings.mSurroundDepth=sur;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='D') {
			sur--; settings.mSurroundDepth=sur;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='y') {
			surdly++; settings.mSurroundDelay=surdly;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='Y') {
			surdly--; settings.mSurroundDelay=surdly;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='x') {
			bas++; settings.mBassAmount=bas;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='X') {
			bas--; settings.mBassAmount=bas;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='c') {
			basrng++; settings.mBassRange=basrng;
			ModPlug_SetSettings(&settings);
		    }
		    if (buffer[0]=='C') {
			basrng--; settings.mBassRange=basrng;
			ModPlug_SetSettings(&settings);
		    }
*/
		    
		    if (buffer[0]=='n') {
			if (song<argc) { mlen=0; pause=0; ioctl(audio_fd,SNDCTL_DSP_RESET,0); }
		    }

		    if (buffer[0]=='N') {
			if (song>1) { song-=2; mlen=0; pause=0; ioctl(audio_fd,SNDCTL_DSP_RESET,0); }
		    }
		    
		    if (buffer[0]=='r') {
			song=(int) ((float)(argc-1)*rand()/(RAND_MAX+1.0));
			mlen=0; pause=0;
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			/* printf("\n[%d?]\n",song+1); */
		    }
		    
		    /*if (buffer[0]=='R') {
			song=(int) ((float)(argc-1)*rand()/(RAND_MAX+1.0));
			mlen=0; pause=0;
		    }*/
		    
		    if (buffer[0]=='1') {
			/* 11025 hertz */
			speed=11025;
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			if (ioctl(audio_fd,SNDCTL_DSP_SPEED,&speed) == -1) {
			    perror("SNDCTL_DSP_SPEED");
			    exit(1);
			}
			settings.mFrequency = 11025;
			ModPlug_SetSettings(&settings);
			f2=ModPlug_Load(d,size);
			ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
		    }
		    
		    if (buffer[0]=='2') {
			/* 22050 hertz */
			speed=22050;
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			if (ioctl(audio_fd,SNDCTL_DSP_SPEED,&speed) == -1) {
			    perror("SNDCTL_DSP_SPEED");
			    exit(1);
			}
			settings.mFrequency = 22050;
			ModPlug_SetSettings(&settings);
			f2=ModPlug_Load(d,size);
			ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
		    }
		    
		    if (buffer[0]=='4') {
			/* 44100 hertz */
			speed=44100;
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			if (ioctl(audio_fd,SNDCTL_DSP_SPEED,&speed) == -1) {
			    perror("SNDCTL_DSP_SPEED");
			    exit(1);
			}
			settings.mFrequency = 44100;
			ModPlug_SetSettings(&settings);
			f2=ModPlug_Load(d,size);
			ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
		    }

		    if (buffer[0]=='8') {
			/* 8/16 bit */
			/*format=;*/
			bits^=1;
			if (bits) { format=AFMT_U8; } else {
                            switch (get_byteorder()) {
                              case 0: format=AFMT_S16_LE; break;
                              case 1: format=AFMT_S16_BE; break;
                              default:
				printf("could not determine byte order.\n");
				return 1;
			    }
			}
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			if (ioctl(audio_fd,SNDCTL_DSP_SETFMT, &format) == -1) {
			    perror("SND_CTL_DSP_SETFMT");
			    exit(1);
			}
			if (bits) settings.mBits=8; else settings.mBits=16;
			ModPlug_SetSettings(&settings);
			f2=ModPlug_Load(d,size);
			ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
		    }

		    if (buffer[0]=='m') {
			/* mono/stereo */
			mono^=1;
			if (mono) channels=1; else channels=2;
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
			    perror("SNDCTL_DSP_CHANNELS");
			    exit(1);
			}
			if (mono) settings.mChannels=1; else settings.mChannels=2;
			ModPlug_SetSettings(&settings);
			f2=ModPlug_Load(d,size);
			ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
		    }
		    
		    if (buffer[0]=='l') {
			loop^=1;
			if (loop) {
			    memcpy(status+4,"loop",4);
			} else {
			    memcpy(status+4,"play",4);
			}
		    } /* loop */
		    
		    if (buffer[0]=='+') {
			setrelpcmvol(4);
		    }

		    if (buffer[0]=='-') {
			setrelpcmvol(-4);
		    }

		    if (buffer[0]=='p') {
			pause^=1;
			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			if (pause) {
			    gettimeofday(&tvpause,NULL);
			    memcpy(notpaus,status+4,4);
			    memcpy(status+4,"paus",4);
			} else {
			    gettimeofday(&tvunpause,NULL);
			    memcpy(status+4,notpaus,4);
			    tvptotal.tv_sec+=tvunpause.tv_sec-tvpause.tv_sec;
			    tvptotal.tv_usec+=tvunpause.tv_usec-tvpause.tv_usec;
			    /* printf(status,tv.tv_sec-tvstart.tv_sec,tv.tv_usec/100000); */
			}
		    } /* pause */
                }
            }
        }
    }
    printf("\n");

    reset_keypress();
    ModPlug_Unload(f2);
    close(audio_fd);
    free(d);
    } /* valid module */
    
} /* for */

    return 0;
}

