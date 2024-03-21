#ifndef DUELCLIENT_H
#define DUELCLIENT_H

#include "config.h"
#include <vector>
#include <set>
#include <utility>
#include "network.h"
#include "data_manager.h"
#include "deck_manager.h"
#include "../ocgcore/mtrandom.h"

namespace ygo {

class DuelClient {
private:
	static unsigned int connect_state;
	static unsigned char response_buf[SIZE_RETURN_VALUE];
	static unsigned int response_len;
	static unsigned int watching;
	static unsigned char selftype;
	static bool is_host;
	static unsigned char duel_client_read[SIZE_NETWORK_BUFFER];
	static unsigned char duel_client_write[SIZE_NETWORK_BUFFER];
	static bool is_closing;
	static bool is_swapping;
	static int select_hint;
	static int select_unselect_hint;
	static int last_select_hint;
	static unsigned char last_successful_msg[0x2000];
	static unsigned int last_successful_msg_length;
	static wchar_t event_string[256];
	static mt19937 rnd;
public:
	static bool StartClient(unsigned int ip, unsigned short port, bool create_game = true);
	static void StopClient(bool is_exiting = false);
	static int ClientThread();
	static void HandleSTOCPacketLan(unsigned char* data, unsigned int len);
	static int ClientAnalyze(unsigned char* msg, unsigned int len);
	static void SwapField();
	static void SetResponseI(int respI);
	static void SetResponseB(void* respB, unsigned int len);
	static void SendResponse();
	static void SendPacketToServer(unsigned char proto) {
	}
	template<typename ST>
	static void SendPacketToServer(unsigned char proto, ST& st) {
	}
	static void SendBufferToServer(unsigned char proto, void* buffer, size_t len) {
	}
	
protected:
	static bool is_refreshing;
	static int match_kill;
	static std::set<std::pair<unsigned int, unsigned short>> remotes;
public:
	static std::vector<HostPacket> hosts;
	static void BeginRefreshHost();
};

}

#endif //DUELCLIENT_H
