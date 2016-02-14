#pragma once

typedef char juci_sid_t[32]; 

struct juci_session {
	juci_sid_t sid; 
}; 

struct juci_session *juci_session_new(); 
