#ifndef _TUNABLE_H_
#define _TUNABLE_H_

#include "tunable.h"

extern int tunable_pasr_enable;
extern int tunable_port_enable;
extern unsigned int tunable_listen_port;
extern unsigned int tunable_max_clients;
extern unsigned int tunable_max_per_ip;
extern unsigned int tunable_accept_tineout;
extern unsigned int tunable_connect_tineout;
extern unsigned int tunable_idle_session_tineout;
extern unsigned int tunable_data_connection_tineout;
extern unsigned int tunable_local_unask;
extern unsigned int tunable_upload_max_rate;
extern unsigned int tunable_download_max_rate;
extern const char *tunable_listen_address;

#endif