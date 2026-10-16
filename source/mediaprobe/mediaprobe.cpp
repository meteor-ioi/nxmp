#include "mediaprobe.h"

void ProbeThread(void*arg) {
	CMediaProbe *ctx = (CMediaProbe *)arg;
	
	while(!ctx->exitThread){
		for(size_t i=0; i<ctx->playlist->currplaylist.size(); i++){
			if(!ctx->playlist->currplaylist[i].is_valid){
				ctx->playlist->currplaylist[i].is_valid = true;
				ctx->playlist->CalcPlaylist();
			}
		}
		svcSleepThread(1'000'000'000);
	}
}

CMediaProbe::CMediaProbe(Playlist *_playlist){
	playlist = _playlist;
}

void CMediaProbe::StartMediaProbeThread(){
	threadCreate(&t0, ProbeThread, this, NULL, 0x10000, 0x3B, -2);
	threadStart(&t0);
}

bool CMediaProbe::needUpdate(){
	bool refresh = false;
	for(size_t i=0; i<playlist->currplaylist.size(); i++){
		if(!playlist->currplaylist[i].is_valid) refresh = true;
	}
	return refresh;
}

CMediaProbe::~CMediaProbe(){
	exitThread = true;
	threadWaitForExit(&t0);
	threadClose(&t0);
}
