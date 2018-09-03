
/*
 * modplug123 by Konstanty Bialkowski - 2010 
 * this interface uses libAO to support a high number of audio output types
 *  based on ao_example.c
 *  modplug code based on modplug play ( gurkan@linuks.mine.nu www.linuks.mine.nu - Copyright (C) 2003 Gürkan Sengün)
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

#include <ao/ao.h>

#include <sys/time.h>			/* gettimeofday */
#include <time.h>
#include <sys/poll.h>			/* poll for keyboard input */
#include <termios.h>			/* needed to get terminal size */



#define VERSION "0.5.3"

#define BUF_SIZE 4096

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
	printf("\nmodplug123 - libAO based console player - Konstanty Bialkowski");
	printf("Based on modplugplay - Copyright (C) 2003 Gürkan Sengün\n");
	printf("Version %s compiled on %s at %s.\n",VERSION,__DATE__,__TIME__);
}

void help(char *s, int exitcode)
{
	printf("modplug123 - libAO based console player - Konstanty Bialkowski\n");
	printf("Based on modplugplay Copyright (C) 2003 Gürkan Sengün\n");
        printf("Version %s compiled on %s at %s.\n",VERSION,__DATE__,__TIME__);
	printf("\n");
	if (exitcode!=0)
		printf("%s: too few arguments\n",s);
	printf("Usage: %s" /*[OPTIONS]*/" [FILES]\n",s);
	printf("\n");

	printf("  -v/--version  print version info\n");
	printf("  -h/--help   print help\n");
	printf("  -l   start in looping mode\n");
	printf("  -ao <audio output driver> use specific libAO output driver\n");
	printf("       eg. -ao wav (file writing to output.wav)\n");
/*
	printf("  -r   randomize play sequence\n");
	printf("  -c   write to console instead of %s\n",DEVICE_NAME);
	printf("  -i   use stdin for file input\n");
	printf("  -q   be quiet\n");
*/	
	exit(exitcode);
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

int main(int argc, char* argv[])
{
    long size;
    char *filedata;
    term_size terminal;
    ModPlugFile *f2;
    int mlen, len;
    struct timeval tvstart;
    struct timeval tv;
    struct timeval tvpause, tvunpause;
    struct timeval tvptotal;
    char status[161];
    char songname[41];
    char notpaus[4];

    int loop=0; // kontest
    
    int songsplayed = 0;

    int nFiles = 0, fnOffset[100];
    int i;

    ModPlug_Settings settings;
    ModPlug_GetSettings(&settings);

    ao_device *device;
    ao_sample_format format = {0};
    int default_driver;
    
    ao_initialize();
    default_driver = ao_default_driver_id();

    for (i=1; i<argc; i++) {
     /* check if arguments need to be parsed */
     if (argv[i][0] == '-') {
      if (strstr(argv[i],"-h")) {
        printf("\n");
        help(argv[0],0);
      } else if (strstr(argv[i],"-v")) {
	versioninfo();
        exit(0);
      } else if (strstr(argv[i],"-l")) {
        loop=1;
        continue;
      } else if (strstr(argv[i],"-ao")) {
        default_driver = ao_driver_id(argv[++i]);
        continue;
      }
      if (argv[i][1] == '-') { // not a song
        if (strstr(argv[i],"--help")) {
          help(argv[0],0);
        } else if (strstr(argv[i],"--version")) {
	  versioninfo();
	  exit(0);
	}
        continue;
       }
      }
      /* "valid" filename - store it */
      fnOffset[nFiles++] = i;
    }

    format.bits = 16;
    format.channels = 2;
    format.rate = 44100;
    format.byte_format = AO_FMT_LITTLE;
//	printf("Default driver = %i\n", default_driver);

    char buffer[128];
    int result, nread;
    struct pollfd pollfds;
    int timeout = 1;            /* Timeout in msec. */
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

for (song=0; song<nFiles; song++) {

    char *filename = argv[fnOffset[song]];

/* -- Open driver -- */
    if (default_driver == ao_driver_id("wav")) {
        device = ao_open_file(default_driver, "output.wav", 1, &format, NULL /*no options*/);
    } else {
        device = ao_open_live(default_driver, &format, NULL /* no options */);
    }
    if (device == NULL) {
 	fprintf(stderr, "Error opening device. (%s)\n", filename);
	fprintf(stderr, "ERROR: %i\n", errno);
        return 1;
    }
    printf("%s ",filename);
    printf("[%d/%d]",song+1,nFiles);

    filedata = getFileData(filename, &size);
    if (filedata == NULL) continue;
    printf(" [%ld]\n",size);

    // Note: All "Basic Settings" must be set before ModPlug_Load.
    settings.mResamplingMode = MODPLUG_RESAMPLE_FIR; /* RESAMP */
    settings.mChannels = 2;
    settings.mBits = 16;
    settings.mFrequency = 44100;
    settings.mStereoSeparation = 128;
    settings.mMaxMixChannels = 256;
    /* insert more setting changes here */
    ModPlug_SetSettings(&settings);

    f2 = ModPlug_Load(filedata, size);
    if (!f2) {
	printf("could not load %s\n", filename);
	close(audio_fd);
	free(filedata); /* ? */
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
        int l = strlen(filename);
	char *st = filename;
        if (l >= 41) st = filename + l - 41;
        strncpy(songname,st,41);
        songname[41] = 0;
    }
    sprintf(status,"[1Gplaying %s (%%d.%%d/%d\") (%%d/%%d/%%d)    \b\b\b\b",songname,ModPlug_GetLength(f2)/1000);
    if (loop) sprintf(status,"[1Glooping %s (%%d.%%d/%d\") (%%d/%%d/%%d)    \b\b\b\b",songname,ModPlug_GetLength(f2)/1000);

    gettimeofday(&tvstart,NULL);
    tvptotal.tv_sec=tvptotal.tv_usec=0;
    mlen=1;
    
    while(mlen!=0) {
	if (mlen==0) { break; }

	if (!pause) {
	    gettimeofday(&tv,NULL);
	    mlen = ModPlug_Read(f2,audio_buffer,BUF_SIZE);
            if (mlen > 0 && ao_play(device, audio_buffer, mlen) == 0) {
		perror("audio write");
		exit(1);
    	    }
	    /*printf("%d %d\n",mlen,len);*/
        }
        printf(status,tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec,tv.tv_usec/100000,format.rate,format.channels,settings.mBits/*,rev,revdly,sur,surdly,bas,basrng*/);
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

		    if (buffer[0]=='q') { mlen=0; song=nFiles;  } /* quit */

		    if (buffer[0]=='f') {
			if ((tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec+10) < (ModPlug_GetLength(f2)/1000)) {
			    ModPlug_Seek(f2,(tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec)*1000+10000);
			    tvstart.tv_sec-=10;
			}
		    } /* forward 10" */

		    if (buffer[0]=='b') {
			if ((tv.tv_sec-tvstart.tv_sec-tvptotal.tv_sec-10) > 0) {
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
			if (song<argc) { mlen=0; pause=0; }
		    }

		    if (buffer[0]=='N') {
			if (song>1) { song-=2; mlen=0; pause=0; }
		    }
		    
		    if (buffer[0]=='r') {
			song=(int) ((float)(argc-1)*rand()/(RAND_MAX+1.0));
			mlen=0; pause=0;
//			ioctl(audio_fd,SNDCTL_DSP_RESET,0);
			/* printf("\n[%d?]\n",song+1); */
		    }
		    
		    /*if (buffer[0]=='R') {
			song=(int) ((float)(argc-1)*rand()/(RAND_MAX+1.0));
			mlen=0; pause=0;
		    }*/
		    
/*		    if (buffer[0]=='m') {
			// mono/stereo 
			mono^=1;
			if (mono) format.channels=1; else format.channels=2;
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
		    */
		    if (buffer[0]=='l') {
			loop^=1;
			if (loop) {
			    memcpy(status+4,"loop",4);
			} else {
			    memcpy(status+4,"play",4);
			}
		    } /* loop */
		    
		    if (buffer[0]=='p') {
			pause^=1;
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
    ao_close(device);
    fprintf(stderr, "Closing audio device.\n");
    free(filedata);
    } /* valid module */
    
} /* for */
    ao_shutdown();

    return 0;
}

