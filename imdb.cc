using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

void imdb::collectFilms(char* chars, vector < film >& films, string player) const{
	chars++;
	int cntSymbols = (int)player.length();
	cntSymbols++;
	if (cntSymbols % 2) {
		chars++;
		cntSymbols++;
	}
	short cntMovies = *(short*)chars;
	chars++;
	chars++;
	cntSymbols += 2;
	if (cntSymbols % 4) {
		chars++;
		chars++;
		cntSymbols += 2;
	}
	for (short i = 0; i < cntMovies; i++) {
		film currentFilm;
		int currentMovieIndex = *(int*)(chars + i * 4);
		string currentFilmName = "";
		char* currentMovieBytes = (char*)((char*)movieFile + currentMovieIndex);
		while(*currentMovieBytes != '\0') {
			currentFilmName += *currentMovieBytes;
			currentMovieBytes++;
		}
		currentMovieBytes++;
		char currentFilmYear = *(char*)(currentMovieBytes);
		currentFilm.title = currentFilmName;
		currentFilm.year = 1900 + currentFilmYear;
		films.push_back(currentFilm);
    }
}

void imdb::collectActors(char* chars, vector < string >& players, string movie) const{
	int cntSymbols = (int)movie.length();
    cntSymbols += 2;
    chars++;
    if (cntSymbols % 2) {
    	chars++;
        cntSymbols++;
    }
    short cntActors = *(short*)chars;
    chars++;
    chars++;
    cntSymbols += 2;
    if (cntSymbols % 4) {
        chars++;
        chars++;
    }
    for (short i = 0; i < cntActors; i++) {
        string currentActor = "";
        int currentActorIndex = *(int*)(chars + i * 4);
        char* currentActorPtr = (char*)((char*)actorFile + currentActorIndex);
        while (*currentActorPtr != '\0') {
            currentActor += *currentActorPtr;
            currentActorPtr++;
        }
        players.push_back(currentActor);
    }
}

char* imdb::binarySearch(char fileName, string s, int year) const{
	const void* file;
	if (fileName == 'a')
		file = actorFile;
	else
		file = movieFile;
	int cnt = *(int*)file;
  	int l = 0;
  	int r = cnt - 1;
  	while (l <= r) {
    	int mid = (l + r) / 2;
    	int ptr = *(int*)((char*)file + (mid + 1) * 4);
    	char* chars = (char*)((char*)(file) + ptr);
    	int indx = 0;
    	while(*chars != '\0' && indx < (int)s.length()) {
      		if (s[indx] < *chars) {
        		r = mid - 1;
        		break;
      		}
      		else if (s[indx] > *chars) {
        		l = mid + 1;
        		break;
      		}
      		chars++;
      		indx++;
    	}
    	if (*chars == '\0' && indx == (int)s.length()) {
    		if (year != 0) {
    			char* newChars = chars;
    			newChars++;
      			char currentFilmYearByte = *(char*)(newChars);
      			int currentFilmYear = 1900 + currentFilmYearByte;
    			if (currentFilmYear < year) {
    				l = mid + 1;
    			}
    			else if (currentFilmYear > year) {
    				r = mid - 1;
    			}
    			else {
    				return newChars;
    			}
    		}
    		else
		    	return chars;
    	}
   	 	else if (*chars == '\0') {
      		l = mid + 1;
    	}
    	else if (indx == (int)s.length()) {
      		r = mid - 1;
    	}
  	}
  	return NULL;
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const { 
	int year = 0;
  	char* chars = binarySearch('a', player, year);
  	if (chars == NULL)
  		return false;
  	collectFilms(chars, films, player); 
 	return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
    string movieName = movie.title;
 	char* chars = binarySearch('m', movieName, movie.year);
 	if (chars == NULL) {
 		return false;
 	}
 	collectActors(chars, players, movieName);
  	return true; 
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
