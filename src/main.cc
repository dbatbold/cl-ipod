using namespace std;

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <iostream>

#include <glib/gstdio.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <itdb.h>

void usage() {
	printf("Usage: cl-ipod {command}\n"
"Commands:\n"
"	parse  - parsing iPod database into tracks file\n"
"	update - updating iPod database and files from tracks file\n"
"	tag    - updating MPEG Tag from list file\n"
"	info   - viewing MPEG Tag from list file\n"
	);
}

Itdb_iTunesDB *
get_db() {
	GError *error = NULL;
	Itdb_iTunesDB *db = itdb_parse("/mnt/ipod", &error);
	if (db == NULL) {
		fprintf(stderr, "Failed to parse: %s\n", error->message);
		return NULL;
	}
	return db;
}

void parse() {
	Itdb_iTunesDB *db = get_db();
	if (db == NULL)
		return;
	GList *item;
	Itdb_Track *track;
	for (item = db->tracks; item; item = item->next) {
		track = (Itdb_Track *)item->data;
		printf(" %s|%s|%s|%i|%s\n",
			track->album, track->title,
			track->artist, track->track_nr, track->ipod_path);
	}
	itdb_free(db);
}

char * base_name(char *path) {
	char *base = strrchr(path, '/');
	return base ? base + 1 : path;
}

Itdb_Track *
track_from_file(const char *file_path)
{
	TagLib::MPEG::File file(file_path);
	Itdb_Track *track;
	struct stat st;
	char buf[1024];
	char *file_name = base_name((char *)file_path);

	TagLib::Tag *tag = file.tag();
	if (tag == NULL) {
		fprintf(stderr, "Couldn't read tag\n");
		return NULL;
	}
	track = itdb_track_new();
	if (g_stat(file_path, &st) == 0) {
		track->size = st.st_size;
	} else {
		fprintf(stderr, "Error: File is unavailable: %s\n", file_path);
		return NULL;
	}
	track->title = g_strdup (tag->title().toCString(true));
	track->album = g_strdup (tag->album().toCString(true));
	track->artist = g_strdup (tag->artist().toCString(true));
	track->genre = g_strdup (tag->genre().toCString(true));
	track->comment = g_strdup (tag->comment().toCString(true));
	track->filetype = g_strdup ("MP3-file");
	track->tracklen = file.audioProperties()->length() * 1000;
	track->track_nr = tag->track();
	track->bitrate = file.audioProperties()->bitrate();
	track->samplerate = file.audioProperties()->sampleRate();
	track->year = tag->year();

	// no tag?
	if (!*track->title) {
		g_free(track->title);
		if (track->track_nr) 
			snprintf(buf, sizeof(buf), "Track %i. %s", track->track_nr, file_name);
		else
			snprintf(buf, sizeof(buf), "%s", file_name);
		track->title = g_strdup(buf);
		if (!*track->album) {
			g_free(track->album);
			track->album = g_strdup("0 - NoAlbumName");
		}
	}

	return track;
}

void delete_track(Itdb_iTunesDB *db, string line) {
	GList *item;
	Itdb_Track *track;

	for (item = db->tracks; item; item = item->next) {
		track = (Itdb_Track *)item->data;
		if (track && *track->ipod_path && string::npos != line.find(track->ipod_path)) {
			printf("Removing - %s|%s|%s|%s\n", track->album, track->title, track->artist, track->ipod_path);

			// delete file from iPod
			char buf[255]; 
			snprintf(buf, sizeof(buf), "/mnt/ipod%s", track->ipod_path);
			itdb_filename_ipod2fs(buf);
			if (unlink(buf))
				fprintf(stderr, "Error: failed to delete: %s\n", buf);

			// remove from DB
			itdb_playlist_remove_track(NULL, track);
			itdb_track_remove(track);
			return;
		}
	}

	fprintf(stderr, "Couldn't locate track %s\n", line.c_str());
}

void add_track(Itdb_iTunesDB *db, const char *filename) {
	Itdb_Track *track;
	GError *error = NULL;
	gboolean copy_success;

	track = track_from_file(filename);
	if (track == NULL)
		return;
	itdb_track_add(db, track, -1);
	itdb_playlist_add_track (itdb_playlist_mpl(db), track, -1);
	copy_success = itdb_cp_track_to_ipod (track, filename, &error);
	if (!copy_success) {
		fprintf(stderr, "itdb_cp_track_to_ipod failed.\n");
	}
	if (error != NULL) {
		fprintf(stderr, "Failed to copy: %s\n", error->message);
		return;
	}
	printf("Adding - %s|%s|%s\n", track->album, track->title, track->artist);
}

TagLib::String
parse_tag_line(string line, const char *tag_name) {
	// syntax:
	// album=Album Name|title=Song Title|artist=Artist Name|...
	TagLib::String tag;
	char buf[1024];
	snprintf(buf, sizeof(buf), "%s=", tag_name);
	int start = line.find(buf);
	if (start == string::npos)
		return tag;
	start += strlen(tag_name) + 1;
	int end = line.find("|", start);
	if (end == string::npos)
		end = line.length();
	int len = end - start;
	if (len >= sizeof(buf))
		return tag;
	memset(buf, 0, sizeof(buf));
	strncpy(buf, line.c_str() + start, len);
	tag = buf;
	return tag;
}

void update_track(Itdb_iTunesDB *db, string line) {
	GList *item;
	Itdb_Track *track;

	for (item = db->tracks; item; item = item->next) {
		track = (Itdb_Track *)item->data;
		if (track && *track->ipod_path && string::npos != line.find(track->ipod_path)) {
			printf("Updating - %s|%s|%s|%s\n", track->album, track->title, track->artist, track->ipod_path);

			char buf[1024]; 
			snprintf(buf, sizeof(buf), "/mnt/ipod%s", track->ipod_path);
			itdb_filename_ipod2fs(buf);

			TagLib::MPEG::File file(buf);
			TagLib::Tag *tag = file.tag();
			if (tag == NULL) {
				fprintf(stderr, "Couldn't read tag: %s\n", buf);
				return;
			}
			TagLib::String album = parse_tag_line(line, "album");
			TagLib::String title = parse_tag_line(line, "title");
			TagLib::String artist = parse_tag_line(line, "artist");
			const char *album_c = album.toCString();
			const char *title_c = title.toCString();
			const char *artist_c = artist.toCString();
			int updated = 0;
			if (album.length() > 0) {
				tag->setAlbum(album);
				track->album = g_strdup(album_c);
				updated = 1;
			}
			if (title.length() > 0) {
				tag->setTitle(title);
				track->title = g_strdup(title_c);
				updated = 1;
			}
			if (artist.length() > 0) {
				tag->setArtist(artist);
				track->artist = g_strdup(artist_c);
				updated = 1;
			}
			string tr_title = track->title;
			if (tr_title.find("Track") == -1 && tr_title.find(".mp3") != -1 && track->track_nr > 0) {
				snprintf(buf, sizeof(buf), "Track %i. %s. %s", track->track_nr, track->album, track->title);
				title = buf;
				tag->setTitle(title);
				track->title = g_strdup(buf);
				title_c = title.toCString();
				updated = 1;
			}
			if (updated) {
				file.save();
				printf("\tAlbum=%s Title=%s Artist=%s\n", album_c, title_c, artist_c); 
			} else {
				printf("\tNot updated\n"); 
			}

			return;
		}
	}

	fprintf(stderr, "Couldn't locate track %s\n", line.c_str());
}

void update() {
	Itdb_iTunesDB *db = get_db();
	if (db == NULL)
		return;
	string line;
	int updated = 0;
	while (getline(cin, line)) {
		if (0 == line.find("-")) {
			delete_track(db, line);
			updated = 1;
		}
		if (0 == line.find("+")) {
			add_track(db, line.c_str() + 1);
			updated = 1;
		}
		if (0 == line.find("=")) {
			update_track(db, line);
			updated = 1;
		}
	}
	if (updated) {
		GError *error = NULL;
		itdb_write(db, &error);
		if (error != NULL) {
			fprintf(stderr, "Failed to write DB: %s\n", error->message);
		}
	}
	itdb_free(db);
}

// Update MPEG tag from Stdin
// sytax: file=/path-to-file|album=Album Name|title=Song Title|artist=Artist Name|track=Track Number
void tag() {
	string line;
	while (getline(cin, line)) {
		if (line.find("file=") == -1)
			continue;
		struct stat st;
		TagLib::String filename = parse_tag_line(line, "file");
		TagLib::String album = parse_tag_line(line, "album");
		TagLib::String title = parse_tag_line(line, "title");
		TagLib::String artist = parse_tag_line(line, "artist");
		TagLib::String genre = parse_tag_line(line, "genre");
		TagLib::String track_nr = parse_tag_line(line, "track");
		const char *file_c = filename.toCString();
		printf("Tagging - %s\n", file_c);
		if (stat(file_c, &st) == -1) {
			fprintf(stderr, "\tFailed access file: %s\n", file_c);
			continue;
		}
		TagLib::MPEG::File file(file_c);
		TagLib::Tag *tag = file.tag();
		if (tag == NULL) {
			fprintf(stderr, "\tCouldn't read tag: %s\n", file_c);
			continue;
		}
		if (album.length() > 0) {
			tag->setAlbum(album);
		}
		if (title.length() > 0) {
			tag->setTitle(title);
		}
		if (artist.length() > 0) {
			tag->setArtist(artist);
		}
		if (genre.length() > 0) {
			tag->setGenre(genre);
		}
		if (track_nr.length() > 0) {
			size_t tr_num = atoi(track_nr.toCString());
			tag->setTrack(tr_num);
		}
		file.save();
	}
}

// Show MPEG tag info from Stdin
// sytax: /path-to-file
void info() {
	string line;
	while (getline(cin, line)) {
		struct stat st;
		const char *file_c = line.c_str();
		printf("Info - %s\n", file_c);
		if (stat(file_c, &st) == -1) {
			fprintf(stderr, "\tFailed access file: %s\n", file_c);
			continue;
		}
		TagLib::MPEG::File file(file_c);
		TagLib::Tag *tag = file.tag();
		if (tag == NULL) {
			fprintf(stderr, "\tCouldn't read tag: %s\n", file_c);
			continue;
		}
		printf("\t Album: %s\n\t Title: %s\n\tArtist: %s\n"
		       "\t Track: %i\n\t Genre: %s\n\n",
			tag->album().toCString(),
			tag->title().toCString(),
			tag->artist().toCString(),
			tag->track(),
			tag->genre().toCString()
			);
	}
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		usage();
		return 1;
	}
	string cmd = argv[1];
	if (cmd == "parse")
		parse();
	else if (cmd == "update")
		update();
	else if (cmd == "tag")
		tag();
	else if (cmd == "info")
		info();
	else
		cerr << "Invalid command: " << cmd << "\n";
	return 0;
}
