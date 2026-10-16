#ifndef NXMP_MEDIAPROBE_H
#define NXMP_MEDIAPROBE_H

#include <switch.h>
#include <string>
#include <vector>

#include "logger.h"
#include "playlist.h"

class CMediaProbe{
public:
		CMediaProbe(Playlist *_playlist);
		~CMediaProbe();
		bool needUpdate();
		void SetProbeList(std::vector<Playlist::playlist_struct> *_playlist);
		
		Playlist *playlist;
		
		bool exitThread = false;
		void StartMediaProbeThread();
		Thread t0;
	
private:
	

};

#endif