cl-ipod - A command line tool for managing iPod content.

Required libraries for building
-------------------------------

```
apt install libgpod-dev
apt install libtag1-dev
```

Building
--------

```
$ cd cl-ipod/src
$ make
```

Installing
----------

```
$ sudo cp cl-ipod /usr/local/bin/
```

Instructions
------------

1. Mounting iPod file system.

```
$ sudo mount -o uid=username /dev/sdb1 /mnt/ipod
```

username  - your username
/dev/sdb1 - iPod file system partition
/mnt/ipod - the mount point (IPOD_MOUNT_DIR environment variable can override)

Getting iPod track list:

```
$ cl-ipod parse | sort > tracks
```

tracks - created text file lists all tracks

Adding files to iPod:

2. Create a text file containing a list of files.

Example:

```
$ cd my-music
$ find . -name \*mp3 > list
$ cat list
./song-1.mp3
./song-2.mp3
./folder1/track-5.mp3
./folder2/track-6.mp3
```

3. Add '+' symbol on front of the file names to be added.

```
$ cat list
+./song1.mp3
+./song2.mp3
+./folder1/track-5.mp3
./folder2/track-6.mp3
```

4. Run cl-ipod:

```
$ cl-ipod update < list
```

Updating MPEG Tags in iPod:

```
$ cat tracks
 album-18|song-4|Erik|3|:iPod_Control:Music:F42:libgpod481489.mp3
 piano-concertos|movement-1|Sarah|4|:iPod_Control:Music:F43:libgpod736488.mp3
```

1. Add '=' symbol on front of the track in the tracks file for updating tags.

2. Add one or more album=, title=, artist=, genre= or track= delimited by '|' as in following example.

```
$ cat tracks
 album-18|song-4|Erik|3|:iPod_Control:Music:F42:libgpod481489.mp3
=album=Piano Concerto No 12|title=1st Movement|track=2|piano-concertos|movement-1|Sarah|4|:iPod_Control:Music:F43:libgpod736488.mp3
```

3. Run cl-ipod:

```
$ cl-ipod update < tracks
```

Removing tracks from iPod:

1. Add '-' symbol on front of the track in the tracks file for removing.

```
$ cat tracks
 album-18|song-4|Erik|3|:iPod_Control:Music:F42:libgpod481489.mp3
-piano-concertos|movement-1|Sarah|4|:iPod_Control:Music:F43:libgpod736488.mp3
```

2. Run cl-ipod:

```
$ cl-ipod update < tracks
```

Updating MPEG Tags in file system:

1. Prepare a list file:

```
$ cat list
./song1.mp3
./song2.mp3
./folder1/track-5.mp3
./folder2/track-6.mp3
```

2. Edit the list file as following:

```
$ cat list
./song1.mp3
file=./song2.mp3|album=Song-23|title=Track-2|artist=Tom|track=2
file=./folder1/track-5.mp3|title=Track 5|genre=Classic
./folder2/track-6.mp3
```

3. Run cl-ipod:

```
$ cl-ipod tag < list
```

Viewing MPEG Tags:

1. Prepare a list file:

```
$ cat list
./song1.mp3
./song2.mp3
./folder1/track-5.mp3
./folder2/track-6.mp3
```

2. Run cl-ipod:

```
$ cl-ipod info < list
```
