///////////////////////////////////////////////////////////////////////////
//
// Filename    -  network_address.C
//
// Description -  Network address support
//
// Author      -  James Buckeyne
//
// Create Date -  2019-06-26
//
///////////////////////////////////////////////////////////////////////////

//
//  DEBUG FLAGS are mostly in netstruc.h
//
//#define DEBUG_ADDRESSES
//#define DEBUG_MAC_ADDRESS_LOOKUP

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE  // for features.h
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#define FIX_RELEASE_COM_COLLISION
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <stddef.h>
#include <ctype.h>
#include <sack_types.h>
#include <deadstart.h>
#include <sqlgetoption.h>

#include "netstruc.h"
#include <network.h>

//#define DO_LOGGING // override no _DEBUG def to do loggings...
//#define NO_LOGGING // force neverlog....

#include <logging.h>
#include <procreg.h>
#ifdef __LINUX__
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <asm/types.h>
#  ifdef __cplusplus
extern "C" {
#  endif
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#  ifdef __cplusplus
}
#  endif
#endif

#include <sharemem.h>
#include <timers.h>
#include <idle.h>

//for GetMacAddress
#ifdef __LINUX__
#include <net/if.h>
//#include <sys/timeb.h>

//*******************8
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/un.h>
#include <arpa/inet.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#else
#include "android_ifaddrs.h"
#define EPOLLRDHUP EPOLLHUP
#define EPOLL_CLOEXEC 0
#endif
#ifdef __MAC__
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
//*******************8

#endif
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#ifdef __CYGWIN__
#include <mingw/tchar.h>
#else
#include <tchar.h>
#endif
#include <wincrypt.h>
#include <iphlpapi.h>
#endif

SACK_NETWORK_NAMESPACE

typedef uint8_t hwaddr_bytes[6];
struct addressNode {
	hwaddr_bytes localHw;
	hwaddr_bytes remoteHw;
	SOCKADDR *remote;
};

static struct mac_data {
	int interfaceCount;
	int *ifIndexes;  // interface indexes
	hwaddr_bytes *hwaddrs; // interface hardware addresses;

	int addressCount;
	uint8_t **netmasks; // interface hardware addresses;
	// indexed by address count, with netmasks
	struct addressNode **addresses;
	int *addr_ifIndexes;  // interface indexes

#ifdef __LINUX__
	// used to get interface information
	char ifbuf[512];
#endif	
	PTREEROOT pbtAddresses;
} mac_data;

//----------------------------------------------------------------------------

#if !defined( __MAC__ ) && !defined( __EMSCRIPTEN__ )
#  define INCLUDE_MAC_SUPPORT
#endif

static void deleteAddress( CPOINTER node, uintptr_t a )
{
	struct addressNode *an = (struct addressNode*)node;
	ReleaseAddress( an->remote );
	Deallocate( struct addressNode *, an );
}

static int compareAddress( uintptr_t a, uintptr_t b )
{
	SOCKADDR *an = (SOCKADDR *)a;
	SOCKADDR *bn = (SOCKADDR *)b;
	if( an->sa_family == AF_INET6 ){
		if( bn->sa_family == AF_INET6 ){
#ifdef _WIN32
			return MemCmp( &((SOCKADDR_IN6*)an)->sin6_addr, &((SOCKADDR_IN6*)bn)->sin6_addr, 16 );
#else			
			return MemCmp( &((struct sockaddr_in6 *)an)->sin6_addr, &((struct sockaddr_in6 *)bn)->sin6_addr, 16 );
#endif			
		} else {
			return 1;
		}
	} else {
		if( bn->sa_family == AF_INET6 ){
			return -1;
		} else {
#ifdef _WIN32			
			return MemCmp( &((SOCKADDR_IN*)an)->sin_addr, &((SOCKADDR_IN*)bn)->sin_addr, 4 );
#else
			return MemCmp( &((struct sockaddr_in*)an)->sin_addr, &((struct sockaddr_in*)bn)->sin_addr, 4 );
#endif			
		}	
	}
}


#ifdef _WIN32
static void setupInterfaces( void ) {
	PMIB_IF_TABLE2 if_table;
	MIB_IPNET_ROW2 row;
	if( mac_data.addresses ) return; // already did this work.
	MemSet( &row.InterfaceLuid, 0, sizeof( row.InterfaceLuid ) );
	GetIfTable2( &if_table );
	int ifCount = 0;
	for( unsigned int i = 0; i < if_table->NumEntries; i++ ) {
		if( !if_table->Table[i].InOctets) continue; // skip if no traffic
		ifCount++;
	}
	mac_data.interfaceCount = ifCount;
	mac_data.ifIndexes = NewArray( int, ifCount );
	mac_data.hwaddrs = NewArray( hwaddr_bytes, ifCount );
	ifCount = 0;
	for( unsigned int i = 0; i < if_table->NumEntries; i++ ) {
		if( !if_table->Table[i].InOctets) continue; // skip if no traffic
		mac_data.ifIndexes[ifCount] = if_table->Table[i].InterfaceIndex;
		memcpy( mac_data.hwaddrs[ifCount], if_table->Table[i].PhysicalAddress, 6 );
		ifCount++;
	}

	PMIB_UNICASTIPADDRESS_TABLE uip_table;
	int ipCount = 0;
	GetUnicastIpAddressTable( AF_UNSPEC, &uip_table );
	for( unsigned int i = 0; i < uip_table->NumEntries; i++ ) {
		int ifIndex = -1;
		for( int j = 0; j < mac_data.interfaceCount; j++ ) {
			if( mac_data.ifIndexes[j] == uip_table->Table[i].InterfaceIndex ) {
				ifIndex = j;
				break;
			}
		}
		if( ifIndex >= 0 ) {
			ipCount++;
		}
	}
	mac_data.addressCount = ipCount;
	mac_data.addresses = NewArray( struct addressNode*, ipCount );
	mac_data.addr_ifIndexes = NewArray( int, ipCount );
	mac_data.netmasks = NewArray( uint8_t*, ipCount );
	ipCount = 0;

	for( unsigned int i = 0; i < uip_table->NumEntries; i++ ) {
		int ifIndex = -1;
		for( int j = 0; j < mac_data.interfaceCount; j++ ) {
			if( mac_data.ifIndexes[j] == uip_table->Table[i].InterfaceIndex ) {
				for( int k = 0; k < mac_data.interfaceCount; k++ ) {
					if( mac_data.ifIndexes[k] == uip_table->Table[i].InterfaceIndex ) {
						mac_data.addr_ifIndexes[ipCount] = k;
						break;
					}
				}
				ifIndex = j;
				break;
			}
		}
		if( ifIndex >= 0 ) {
			if( uip_table->Table[i].Address.si_family == AF_INET ) {
				uint8_t *mask = mac_data.netmasks[ipCount] = NewArray( uint8_t, 4 );
				int b;
				for( b = 0; b < uip_table->Table[i].OnLinkPrefixLength; b++ ) {
					mask[b/8] |= (1<<(7-(b%8)) );
				}
				for( ; b < 32; b++ ) {
					mask[b/8] &= ~(1<<(7-(b%8)) );
				}
			} else {
				uint8_t *mask = mac_data.netmasks[ipCount] = NewArray( uint8_t, 16 );
				int b;
				for( b = 0; b < uip_table->Table[i].OnLinkPrefixLength; b++ ) {
					mask[b/8] |= (1<<(7-(b%8)) );
				}
				for( ; b < 128; b++ ) {
					mask[b/8] &= ~(1<<(7-(b%8)) );
				}
			}
			struct addressNode *newAddress = (struct addressNode*)AllocateEx( sizeof( struct addressNode ) DBG_SRC );
			newAddress->remote = AllocAddr();
			newAddress->remote->sa_family = uip_table->Table[i].Address.si_family;
			if( newAddress->remote->sa_family == AF_INET ) {
				((uint32_t*)(newAddress->remote->sa_data+2))[0] = ((uint32_t*)(&uip_table->Table[i].Address.Ipv4.sin_addr))[0];
				SET_SOCKADDR_LENGTH( newAddress->remote, IN_SOCKADDR_LENGTH );
			} else {
				((uint32_t*)( newAddress->remote->sa_data + 2))[0] = 0;
				((uint64_t*)( newAddress->remote->sa_data + 6))[0] = ((uint64_t*)(&uip_table->Table[i].Address.Ipv6.sin6_addr))[0];
				((uint64_t*)( newAddress->remote->sa_data + 6))[1] = ((uint64_t*)(&uip_table->Table[i].Address.Ipv6.sin6_addr))[1];
				SET_SOCKADDR_LENGTH( newAddress->remote, IN6_SOCKADDR_LENGTH );
			}
			mac_data.addresses[ipCount] = newAddress;
			ipCount++;
		}	
	}

	FreeMibTable( if_table );
	FreeMibTable( uip_table );
}
#endif


#ifdef __LINUX__

// largly based on the following code.
//https://codereview.stackexchange.com/questions/278848/linux-netlink-kernel-socket-arp-cache-getter-similar-to-ip-ne

#ifdef DEBUG_MAC_ADDRESS_LOOKUP
static void LogMacAddress( struct addressNode *newAddress ){
	DumpAddr( "New Address Remote", newAddress->remote );
	lprintf( "Local: %02x:%02x:%02x:%02x:%02x:%02x", newAddress->localHw[0], newAddress->localHw[1], newAddress->localHw[2], newAddress->localHw[3], newAddress->localHw[4], newAddress->localHw[5] );
	lprintf( "remote: %02x:%02x:%02x:%02x:%02x:%02x", newAddress->remoteHw[0], newAddress->remoteHw[1], newAddress->remoteHw[2], newAddress->remoteHw[3], newAddress->remoteHw[4], newAddress->remoteHw[5] );
}
#endif

static void setupInterfaces() {
	struct ifconf ifc;
	if( mac_data.ifbuf[0] ) return; // already did this work.
		ifc.ifc_len = sizeof( mac_data.ifbuf );
		ifc.ifc_buf = mac_data.ifbuf;
		int sock_handle = socket( AF_INET6, SOCK_STREAM, 0);//pc->Socket;
		ioctl( sock_handle, SIOCGIFCONF, &ifc );
		{
			int i;
			struct ifreq *IFR;
			IFR = ifc.ifc_req;
			const int len = ifc.ifc_len / sizeof( struct ifreq );
			mac_data.interfaceCount = len;
			mac_data.ifIndexes = NewArray( int, len );
			mac_data.hwaddrs = NewArray( hwaddr_bytes, len );
			
			for( i = 0; i < len; i++, IFR++ )
			{
				//LogBinary( (const uint8_t*)IFR, sizeof( *IFR));
				struct ifreq ifr;
				strcpy( ifr.ifr_name, IFR->ifr_name );
				if (ioctl(sock_handle, SIOCGIFINDEX, &ifr) == 0) {
					mac_data.ifIndexes[i] = ifr.ifr_ifindex;	
				}else {
					lprintf( "ioctl SIOCGIFINDEX error? %d", errno);
				}
				if (ioctl(sock_handle, SIOCGIFFLAGS, &ifr) == 0) {
	            	if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
    	            	if (ioctl(sock_handle, SIOCGIFHWADDR, &ifr) == 0) {
							//LogBinary( (const uint8_t*)ifr.ifr_hwaddr.sa_data, 12 );
							memcpy( mac_data.hwaddrs[i], ifr.ifr_hwaddr.sa_data, 6 );
						}  else {
							lprintf( "ioctl SIOCGIFHWADDR failed: %d", errno);
						}
					} else {
						((uint32_t*)(mac_data.hwaddrs[i]))[0] = 0;
						((uint16_t*)(mac_data.hwaddrs[i]))[2] = 0;
					}
				} else {
					lprintf( "ioctl SIOCGIFFLAGS failed: %d", errno);
				}
			}
		}
		close( sock_handle );
		{
			struct ifaddrs *ifa;
			struct ifaddrs *current_ifa;
			getifaddrs( &ifa);
			int addressCount = 0;
			for( current_ifa = ifa; current_ifa; current_ifa = current_ifa->ifa_next ) {
				if( current_ifa->ifa_addr->sa_family != AF_INET 
				   && current_ifa->ifa_addr->sa_family != AF_INET6 ) continue; // don't care about non IP addresses.
				addressCount++;
			}
			mac_data.addressCount = addressCount;
			mac_data.addresses = NewArray( struct addressNode*, addressCount );
			mac_data.netmasks = NewArray( uint8_t*, addressCount );
			mac_data.addr_ifIndexes = NewArray( int, addressCount );

			addressCount = 0;
			for( current_ifa = ifa; current_ifa; current_ifa = current_ifa->ifa_next ) {
				//if( current_ifa->ifa_addr->sa_family == AF_PACKET ) continue; // don't care about packet addresses.
				if( current_ifa->ifa_addr->sa_family != AF_INET 
				   && current_ifa->ifa_addr->sa_family != AF_INET6 ) continue; // don't care about non IP addresses.

				struct addressNode *newAddress = (struct addressNode*)AllocateEx( sizeof( struct addressNode ) DBG_SRC );
				SOCKADDR *sa = AllocAddr();
				newAddress->remote = sa;
				//lprintf( "Got ifaddr on %s %d", current_ifa->ifa_name, current_ifa->ifa_addr->sa_family );
				//LogBinary( (const uint8_t*)current_ifa->ifa_addr, sizeof( *current_ifa->ifa_addr));
				sa->sa_family = current_ifa->ifa_addr->sa_family;
				if( sa->sa_family == AF_INET ) {
					((uint32_t*)(sa->sa_data+2))[0] = ((uint32_t*)(current_ifa->ifa_addr->sa_data+2))[0];
					//lprintf( "Set address v4: %04x", ((uint32_t*)(sa->sa_data+2))[0] );
					SET_SOCKADDR_LENGTH( sa, IN_SOCKADDR_LENGTH );
				} else {
					// scopr or flow control or something
					((uint32_t*)( sa->sa_data + 2))[0] = ((uint32_t*)(current_ifa->ifa_addr->sa_data+2))[0];
					((uint64_t*)( sa->sa_data + 6))[0] = ((uint64_t*)(current_ifa->ifa_addr->sa_data+6))[0];
					((uint64_t*)( sa->sa_data + 6))[1] = ((uint64_t*)(current_ifa->ifa_addr->sa_data+6))[1];
					SET_SOCKADDR_LENGTH( sa, IN6_SOCKADDR_LENGTH );
				}
				
				{
					struct ifreq *IFR;
					IFR = (struct ifreq*)mac_data.ifbuf;
					for( int i = 0; i < mac_data.interfaceCount; i++, IFR++){
						if( strcmp( current_ifa->ifa_name, IFR->ifr_name ) == 0 ) {
							mac_data.addr_ifIndexes[addressCount] = i;
							if( sa->sa_family == AF_INET ) {
								mac_data.netmasks[addressCount] = NewArray( uint8_t, 4 );
								memcpy( mac_data.netmasks[addressCount], ((uint32_t*)(current_ifa->ifa_netmask->sa_data+2)), 4 );
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
								lprintf( "ipv4 netmask:" );
								LogBinary( mac_data.netmasks[addressCount], 4 );
#endif								
							} else {
								mac_data.netmasks[addressCount] = NewArray( uint8_t, 16 );
								memcpy( mac_data.netmasks[addressCount], ((uint32_t*)(current_ifa->ifa_netmask->sa_data+6)), 16 );
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
								lprintf( "ipv6 netmask:" );
								LogBinary( mac_data.netmasks[addressCount], 16 );
#endif								
							}
							memcpy( newAddress->localHw, mac_data.hwaddrs[i], 6);
							memcpy( newAddress->remoteHw, mac_data.hwaddrs[i], 6);
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
							LogMacAddress( newAddress );
#endif							
							if( AddBinaryNode( mac_data.pbtAddresses, (CPOINTER)newAddress, (uintptr_t)sa ) ) {
								//lprintf( "Added address to tree" );
								mac_data.addresses[addressCount] = newAddress;
							} else {
								//lprintf( "Failed to add address to tree" );
								ReleaseAddress( sa );
								Deallocate( struct addressNode *, newAddress );
							}
							break;
						}
					}
				}
				addressCount++;
			}
			//LogBinary( (const uint8_t*)ifa, sizeof( *ifa));

			freeifaddrs( ifa );
		}

#ifdef DEBUG_MAC_ADDRESS_LOOKUP
		{
			struct ifreq *IFR;
			IFR = (struct ifreq*)mac_data.ifbuf;
			for( int i = 0; IFR->ifr_name[0]; i++, IFR++ ) {
				lprintf( "IF: %d(%d) %s %02x:%02x:%02x:%02x:%02x", i, mac_data.ifIndexes[i], IFR->ifr_name, mac_data.hwaddrs[i][0], mac_data.hwaddrs[i][1], mac_data.hwaddrs[i][2], mac_data.hwaddrs[i][3], mac_data.hwaddrs[i][4], mac_data.hwaddrs[i][5] );
			}
		}
#endif		
}



PTHREAD macThread;
int macThreadEnd =0;
PLIST macWaiters;
int macTableUpdated = 0;
int threadFailed = 0;

ATEXIT( CloseMacThread ){
	macThreadEnd = TRUE;
	WakeThread( macThread );
}

static uintptr_t MacThread( PTHREAD thread ) {

			int stat;
			int rtnetlink_socket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
			if( rtnetlink_socket < 0 )
			{
				threadFailed = 1;
				if( errno == ESOCKTNOSUPPORT ){
					lprintf( "Socket No Support?");
				}else
					lprintf( "Unable to create netlink socket %d", errno );
				return 0;
			}
			struct sockaddr rtnetlink_addr;
			rtnetlink_addr.sa_family = AF_NETLINK;
			rtnetlink_addr.sa_data[0] = 0;
			rtnetlink_addr.sa_data[1] = 0;
			int ppid = (int)(GetMyThreadID() >> 32);
			rtnetlink_addr.sa_data[2] = ppid & 0xFF;
			rtnetlink_addr.sa_data[3] = (ppid >> 8) & 0xFF;
			rtnetlink_addr.sa_data[4] = (ppid >> 16) & 0xFF;
			rtnetlink_addr.sa_data[5] = (ppid >> 24) & 0xFF;
			int grp = RTMGRP_NEIGH;
			rtnetlink_addr.sa_data[6] = grp & 0xFF;
			rtnetlink_addr.sa_data[7] = 0;
			rtnetlink_addr.sa_data[8] = 0;
			rtnetlink_addr.sa_data[9] = 0;
									
			stat = bind(rtnetlink_socket, &rtnetlink_addr, 12 );
			if( stat < 0 )
			{
				threadFailed = 1;
				lprintf( "Unable to bind netlink socket %s", strerror( errno ) );
				return 0;
			}
			int seq;

			struct {
			struct nlmsghdr nlh;
			struct ndmsg ndm;
			char buf[256];
			} req;
			req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
			req.nlh.nlmsg_type = RTM_GETNEIGH;
			req.nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
			req.nlh.nlmsg_seq =  ++seq;
			req.ndm.ndm_family = 0;

			send( rtnetlink_socket, (struct msghdr*)&req, sizeof(req), 0);

	while( !macThreadEnd ) {
			//sendmsg( rtnetlink_socket, (struct msghdr*)&req, 0);
			ssize_t rstat;
			static uint8_t buf[8192];
			int loop = 1;
			do {
				rstat = recv( rtnetlink_socket, buf, sizeof(buf), 0/*MSG_DONTWAIT*/ );
				if( rstat < 0) {
					if( errno == EAGAIN || errno == EWOULDBLOCK ) {
						//lprintf( "No data available" );
						Relinquish();
					} else{
						lprintf( "Error: %s", strerror( errno ) );
						loop = 0;
					}
				}
				else {
					struct response {
						struct nlmsghdr nl;
						struct ndmsg rt;
					} *res;
					int msgLen;
					int priorLen = 0;
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
					lprintf( "Got some data from socket:%d", rstat);
					lprintf( "Flag Values %d %d %d",NTF_SELF, NTF_PROXY, NTF_ROUTER    );
#endif					
					while( priorLen < rstat ) {
						struct addressNode *newAddress = (struct addressNode*)AllocateEx( sizeof( struct addressNode ) DBG_SRC );
						res = (struct response*)(buf+priorLen);

#ifdef DEBUG_MAC_ADDRESS_LOOKUP
						lprintf( "Stuff: %p %d", res, priorLen );
#endif						
						struct rtattr *attr = (struct rtattr*)(res+1);
						switch( res->nl.nlmsg_type ) {
							case NLMSG_DONE:
								// end of dump; should send information here.
								break;
							case RTM_DELNEIGH:
								// should probably delete the entry in the tree here...
								break;
							case RTM_NEWNEIGH:
								//LogBinary( (uint8_t*)(res), res->nl.nlmsg_len );
								/*
								lprintf( "First message: type:%d len:%d flg:%d pid:%d seq:%d", res->nl.nlmsg_type
										, res->nl.nlmsg_len-sizeof( res)
										, res->nl.nlmsg_flags, res->nl.nlmsg_pid, res->nl.nlmsg_seq );
								lprintf( "fam:%d ifi:%d st:%d fl:%d type:%d"
											, res->rt.ndm_family, res->rt.ndm_ifindex
											, res->rt.ndm_state, res->rt.ndm_flags, res->rt.ndm_type );
								*/
								// state == NUD_INCOMPLETE, NUD_REACHABLE, NUD_STALE, NUD_DELAY, NUD_PROBE, NUD_FAILED, NUD_NORARP,NUD_PERMANENT
								// flags == NTF_PROXY, NTF_ROUTER
								// 
								{
									SOCKADDR *sa = AllocAddr();

									sa->sa_family = res->rt.ndm_family;
									sa->sa_data[0] = 0;
									sa->sa_data[1] = 0;
									newAddress->remote = sa;
									for( int i = 0; i < mac_data.interfaceCount; i++ ) {
										if( mac_data.ifIndexes[i] == res->rt.ndm_ifindex ) {
											memcpy( newAddress->localHw, mac_data.hwaddrs[i], 6);
											break;
										}
									}
									
									{
										LOGICAL duplicated = FALSE;
										size_t attLen = res->nl.nlmsg_len-sizeof( *res);
										size_t attOfs = priorLen + sizeof( *res );
										do {
											struct rtattr *attr = (struct rtattr*)(buf+attOfs);
											if( !attr->rta_len )
												break;
											//lprintf( "Attr:%d %d %d", attLen, attr->rta_len, attr->rta_type );
											switch( attr->rta_type ) {
												case NDA_DST:
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
													lprintf( "Destination Address" );
#endif													
													if( ( sa->sa_family = res->rt.ndm_family ) == AF_INET ){
														((uint32_t*)(sa->sa_data+2))[0] = ((uint32_t*)(attr+1))[0];
														SET_SOCKADDR_LENGTH( sa, IN_SOCKADDR_LENGTH );
													} else {
														((uint32_t*)( sa->sa_data + 2))[0] = 0;
														((uint64_t*)( sa->sa_data + 6))[0] = ((uint64_t*)(attr+1))[0];
														((uint64_t*)( sa->sa_data + 6))[1] = ((uint64_t*)(attr+1))[1];
														SET_SOCKADDR_LENGTH( sa, IN6_SOCKADDR_LENGTH );
													}
													if( FindInBinaryTree( mac_data.pbtAddresses, (uintptr_t)sa ) ) {
														duplicated = TRUE;
														//DumpAddr( "Duplicate address notification", sa );
														ReleaseAddress( sa );
														Deallocate( struct addressNode *, newAddress );
														break;
													}

													break;
												case NDA_UNSPEC:
													lprintf( "Unknown type" );
													break;
												case NDA_LLADDR:
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
													lprintf( "Link Layer Address" );
#endif												
													memcpy( newAddress->remoteHw, (attr+1), 6 );
													break;
												case NDA_PROBES: {
													//uint32_t *probes = (uint32_t*)(attr+1);
													//lprintf( "Probes: %d %d %d", probes[0], probes[1], probes[2] );
													break;
												}
												case NDA_CACHEINFO:{
													struct nda_cacheinfo *ci = (struct nda_cacheinfo*)(attr+1);
													
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
													lprintf( "Cache Info  conf:%d used:%d upd:%d cnt:%d", ci->ndm_confirmed, ci->ndm_used, ci->ndm_updated, ci->ndm_refcnt );
#endif													
													break;
												}
												default:
													lprintf( "Unknown attribute type: %d", attr->rta_type );
													break;
											}
											attOfs += attr->rta_len;
											attLen -= attr->rta_len;
										} while( !duplicated && attLen );
										if( duplicated ) {
											break;
										}
#ifdef DEBUG_MAC_ADDRESS_LOOKUP
										LogMacAddress( newAddress );
#endif										
										if( !AddBinaryNode( mac_data.pbtAddresses, (CPOINTER)newAddress, (uintptr_t)newAddress->remote ) ) {
											ReleaseAddress( newAddress->remote );
											Deallocate( struct addressNode *, newAddress );
										}
										//LogBinary( (uint8_t*)(buf+attOfs), res->nl.nlmsg_len-attOfs );
									}
								}

								break;
							default:
								lprintf( "Default message: type:%d len:%d flg:%d pid:%d seq:%d", res->nl.nlmsg_type, res->nl.nlmsg_len
										, res->nl.nlmsg_flags, res->nl.nlmsg_pid, res->nl.nlmsg_seq );
								break;
						}
						//lprintf( "");
						priorLen += res->nl.nlmsg_len;
					}
					//LogBinary( buf, rstat );
				}
				{
					INDEX idx;
					PTHREAD waiter;
					macTableUpdated = TRUE;
					LIST_FORALL( macWaiters, idx, PTHREAD, waiter ) {
						WakeThread( waiter );
					}
				}
			} while( loop );


		}
		close( rtnetlink_socket );
		return 0;

}

#endif

NETWORK_PROC( int, GetMacAddress)(PCLIENT pc, uint8_t* bufLocal, size_t *bufLocalLen
                                 , uint8_t *bufRemote, size_t* bufRemoteLen )//int get_mac_addr (char *device, unsigned char *buffer)
{
	if( pc->dwFlags & CF_AVAILABLE ) 
		return FALSE;
	if( !mac_data.pbtAddresses )
		mac_data.pbtAddresses = CreateBinaryTreeExx( BT_OPT_NODUPLICATES, compareAddress, deleteAddress );
	SOCKADDR *saDup = DuplicateAddress_6to4( pc->saClient );
	// clear Port for later... 
	saDup->sa_data[0] = 0;
	saDup->sa_data[1] = 0;

#ifdef __LINUX__
	macTableUpdated = FALSE;
#endif

#ifdef DEBUG_MAC_ADDRESS_LOOKUP	
	DumpAddr( "Find address in tree", saDup );
#endif	
	struct addressNode *oldAddress = (struct addressNode *)FindInBinaryTree( mac_data.pbtAddresses, (uintptr_t)saDup );
#ifdef DEBUG_MAC_ADDRESS_LOOKUP	
	lprintf( "FindinBinaryTree: %p", oldAddress );
#endif
	if( oldAddress )
	{
		MemCpy( bufLocal, oldAddress->localHw, 6 );
		(*bufLocalLen) = 6;
		MemCpy( bufRemote, oldAddress->remoteHw, 6 );
		(*bufRemoteLen) = 6;
		ReleaseAddress( saDup );
		return TRUE;
	}

	if( ( saDup->sa_family == AF_INET6 
	     && ( ( MemCmp( saDup->sa_data+6, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16 ) == 0 )
		     || ( MemCmp( saDup->sa_data+6, "\0\0\0\0\0\0\0\0\0\0\xff\xff\x7f\0\0\x1", 16 ) == 0 )
	        || ( MemCmp( saDup->sa_data+6, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x1", 16 ) == 0 ) ) )
	  || ( saDup->sa_family == AF_INET
	     && MemCmp( saDup->sa_data+2, "\x7f\0\0\x01", 4 ) == 0 ) )
	{
		// localhost
		memset( bufLocal, 0, 6 );
		(*bufLocalLen) = 6;
		memset( bufRemote, 0, 6 );
		(*bufRemoteLen) = 6;
		ReleaseAddress( saDup );
		return TRUE;
	}
	setupInterfaces();
#ifdef __LINUX__
	if( !macThread ) macThread = ThreadTo( MacThread, 0 );
#endif
	int addr;
	struct addressNode *newAddress = (struct addressNode*)AllocateEx( sizeof( struct addressNode ) DBG_SRC );
	newAddress->remote = saDup;

	for( addr = 0; addr < mac_data.addressCount; addr++ ) {
		if( mac_data.addresses[addr] && saDup->sa_family == mac_data.addresses[addr]->remote->sa_family ) {
			if( saDup->sa_family == AF_INET ) {
				const uint32_t testIP = ((uint32_t*)(saDup->sa_data+2))[0] & ((uint32_t*)(mac_data.netmasks[addr]))[0];
				const uint32_t testAddr = ((uint32_t*)(mac_data.addresses[addr]->remote->sa_data+2))[0] & ((uint32_t*)(mac_data.netmasks[addr]))[0];
				if( testIP == testAddr ) {
					break;
				}					
			} else {
				const uint64_t testIP[2] = {
					((uint64_t*)(saDup->sa_data+6))[0] & ((uint64_t*)(mac_data.netmasks[addr]))[0]
					, ((uint64_t*)(saDup->sa_data+6))[1] & ((uint64_t*)(mac_data.netmasks[addr]))[1]
				};
				const uint64_t testAddr[2] = {
					((uint64_t*)(mac_data.addresses[addr]->remote->sa_data+6))[0] & ((uint64_t*)(mac_data.netmasks[addr]))[0]
					, ((uint64_t*)(mac_data.addresses[addr]->remote->sa_data+6))[1] & ((uint64_t*)(mac_data.netmasks[addr]))[1]
				};
				if( testIP[0] == testAddr[0] && testIP[1] == testAddr[1] ) {
					break;
				}
			}
		}	
	}
	int local_addr;
	SOCKADDR *saSource = DuplicateAddress_6to4( pc->saSource );
	for( local_addr = 0; local_addr < mac_data.addressCount; local_addr++ ) {
		if( mac_data.addresses[local_addr] && saDup->sa_family == mac_data.addresses[local_addr]->remote->sa_family ) {
			if( saDup->sa_family == AF_INET ) {
				if( ((uint32_t*)(saSource->sa_data+2))[0] == ((uint32_t*)(mac_data.addresses[local_addr]->remote->sa_data+2))[0] ) {
					MemCpy( bufLocal, mac_data.hwaddrs[mac_data.addr_ifIndexes[local_addr]], 6 );
					(*bufLocalLen) = 6;	
					break;
				}					
			} else {
				if( ((uint64_t*)(saDup->sa_data+6))[0] == ((uint64_t*)(mac_data.addresses[local_addr]->remote->sa_data+6))[0]
				  && ((uint64_t*)(saDup->sa_data+6))[1] == ((uint64_t*)(mac_data.addresses[local_addr]->remote->sa_data+6))[1] ) {
					MemCpy( bufLocal, mac_data.hwaddrs[mac_data.addr_ifIndexes[local_addr]], 6 );
					(*bufLocalLen) = 6;	
					break;
				}
			}
		}	
	}
	ReleaseAddress( saSource );
	MemCpy( newAddress->localHw, bufLocal, 6 );


	if( addr == mac_data.addressCount ) {
		//lprintf( "No matching address mask found... maybe just fake add this entry for the future?" );
		memset( newAddress->remoteHw, 0, 6 );
		(*bufRemoteLen) = 6;
		// keep this remote address for future checks...
		AddBinaryNode( mac_data.pbtAddresses, (CPOINTER)newAddress, (uintptr_t)newAddress->remote );
		memset( bufRemote, 0, 6 );
		(*bufRemoteLen) = 6;
		return TRUE;
	}

#ifdef __LINUX__
	AddLink( &macWaiters, MakeThread() );
	uint64_t waitTime = timeGetTime64() + 500;
	while( !macTableUpdated && !threadFailed ) {
		// guess I should check to see if it is even possible to resolve with netmask...
		WakeableSleep( 500 );
	}
	if( threadFailed || ( timeGetTime64() > waitTime ) ) {
		//lprintf( "Timeout waiting for mac address" );
		ReleaseAddress( saDup );
		memset( bufLocal, 0, 6 );
		(*bufLocalLen) = 6;
		memset( bufRemote, 0, 6 );
		(*bufRemoteLen) = 6;
		return TRUE;
	}
	goto retry;

	return 0;
#endif

#  ifdef WIN32
	HRESULT hr;
	MIB_IPNET_ROW2 row;
	MemSet( &row.InterfaceLuid, 0, sizeof( row.InterfaceLuid ) );
	row.Address = *((SOCKADDR_INET*)saDup);

	row.InterfaceIndex = mac_data.ifIndexes[mac_data.addr_ifIndexes[local_addr]];
	hr = ResolveIpNetEntry2( &row, (SOCKADDR_INET*)GetNetworkLong( pc, GNL_LOCAL_ADDRESS) );
	lprintf( "hr=%d", hr );
	if( hr == S_OK ) {
		MemCpy( newAddress->remoteHw, row.PhysicalAddress, row.PhysicalAddressLength );
		AddBinaryNode( mac_data.pbtAddresses, (CPOINTER)newAddress, (uintptr_t)newAddress->remote );
		MemCpy( bufRemote, row.PhysicalAddress, row.PhysicalAddressLength );
		(*bufRemoteLen) = row.PhysicalAddressLength;
		lprintf( "Resolve addr: %d  %d", local_addr, hr );
		return TRUE;
	} else {
		ReleaseAddress( saDup);
		memset( bufRemote, 0, 6 );
		(*bufRemote) = 0;
		return FALSE;
	}
	return FALSE;
#  endif
}

NETWORK_PROC( PLIST, GetMacAddresses)( void )//int get_mac_addr (char *device, unsigned char *buffer)
{
	PLIST list = NULL;
	setupInterfaces();
	for( int i = 0; i < mac_data.interfaceCount; i++ ) {
		AddLink( &list, mac_data.hwaddrs[i] );
	}
	return list;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

LOGICAL IsAddressV6( SOCKADDR *addr )
{
	if( addr->sa_family == AF_INET6 && SOCKADDR_LENGTH( addr ) == 28 )
		return TRUE;
	return FALSE;
}

const char * GetAddrString( SOCKADDR *addr )
{
	static char buf[256];
	//lprintf( "addr family is: %d", addr->sa_family );
	if( addr->sa_family == AF_INET )
		snprintf( buf, 256, "%d.%d.%d.%d"
			, *(((unsigned char *)addr) + 4),
			*(((unsigned char *)addr) + 5),
			*(((unsigned char *)addr) + 6),
			*(((unsigned char *)addr) + 7) );
	else if( addr->sa_family == AF_INET6 )
	{
		int first0 = 8;
		int last0 = 0;
		int after0 = 0;
		int n;
		int ofs = 0;
		uint32_t piece;
		for( n = 0; n < 8; n++ ) {
			piece = (*(((unsigned short *)((unsigned char*)addr + 8 + (n * 2)))));
			if( piece ) {
				if( first0 < 8 )
					after0 = 1;
				if( !ofs ) {
					ofs += snprintf( buf + ofs, 256 - ofs, "%x", ntohs( piece ) );
				}
				else {
					//lprintf( last0, n );
					if( last0 == 4 && first0 == 0 )
						if( piece == 0xFFFF ) {
							snprintf( buf, 256, "::ffff:%d.%d.%d.%d",
								(*((unsigned char*)addr + 20)),
								(*((unsigned char*)addr + 21)),
								(*((unsigned char*)addr + 22)),
								(*((unsigned char*)addr + 23)) );
							break;
						}
					ofs += snprintf( buf + ofs, 256 - ofs, ":%x", ntohs( piece ) );
				}
			}
			else {
				if( !after0 ) {
					if( first0 > n ) {
						first0 = n;
						ofs += snprintf( buf + ofs, 256 - ofs, ":" );
					}
					if( last0 < n )
						last0 = n;
				}
				if( last0 < n )
					ofs += snprintf( buf + ofs, 256 - ofs, ":%x", ntohs( piece ) );
			}
		}
		if( !after0 && first0 < 8 )
			ofs += snprintf( buf + ofs, 256 - ofs, ":" );
	}
	else
		snprintf( buf, 256, "unknown protocol" );

	return buf;
}

const char * GetAddrName( SOCKADDR *addr )
{
	char * tmp = ((char**)addr)[-1];
	if( !tmp )
	{
		const char *buf = GetAddrString( addr );
		((char**)addr)[-1] = strdup( buf );
	}
	return ((char**)addr)[-1];
}

void SetAddrName( SOCKADDR *addr, const char *name )
{
	((uintptr_t*)addr)[-1] = (uintptr_t)strdup( name );
}

//---------------------------------------------------------------------------

SOCKADDR *AllocAddrEx( DBG_VOIDPASS )
{
	SOCKADDR *lpsaAddr=(SOCKADDR*)AllocateEx( MAGIC_SOCKADDR_LENGTH + 2 * sizeof( uintptr_t ) DBG_RELAY );
#ifdef DEBUG_ADDRESSES	
	lprintf( "New Length: %d", MAGIC_SOCKADDR_LENGTH);
#endif	
	memset( lpsaAddr, 0, MAGIC_SOCKADDR_LENGTH );
	//initialize socket length to something identifiable?
	((uintptr_t*)lpsaAddr)[0] = 3;
	((uintptr_t*)lpsaAddr)[1] = 0; // string representation of address

	lpsaAddr = (SOCKADDR*)( ( (uintptr_t)lpsaAddr ) + sizeof(uintptr_t) * 2 );
	return lpsaAddr;
}

//----------------------------------------------------------------------------

int GetAddressParts( SOCKADDR *sa, uint32_t *pdwIP, uint16_t *pdwPort )
{
	int result = TRUE;
	if( sa )
	{
		if( sa->sa_family == AF_INET ) {
			if( pdwIP )
				(*pdwIP) = (uint32_t)(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr);
		}
		else if( sa->sa_family == AF_INET6 ) {
			if( pdwIP )
				memcpy( pdwIP, &(((SOCKADDR_IN*)sa)->sin_addr.S_un.S_addr), 16 ); //-V512
		}
		else
			result = FALSE;
		if( (sa->sa_family == AF_INET) || (sa->sa_family = AF_INET6) ) {
			if( pdwPort )
				(*pdwPort) = ntohs((uint16_t)( (SOCKADDR_IN*)sa)->sin_port);
		}
		else
			result = FALSE;
	}
	return result;
}

//----------------------------------------------------------------------------
// return a copy of this address... if it is a ipv6 that wraps an ipv4, return ipv4 version
SOCKADDR* DuplicateAddress_6to4_Ex( SOCKADDR *pAddr DBG_PASS ) 
{
	POINTER tmp = (POINTER)( ( (uintptr_t)pAddr ) - 2*sizeof(uintptr_t) );
	SOCKADDR *dup = AllocAddrEx( DBG_VOIDRELAY );
	POINTER tmp2 = (POINTER)( ( (uintptr_t)dup ) - 2*sizeof(uintptr_t) );
#ifdef DEBUG_ADDRESSES	
	lprintf( "Existing length: %d", SOCKADDR_LENGTH( pAddr ) );
#endif	
	MemCpy( tmp2, tmp, SOCKADDR_LENGTH( pAddr ) + 2*sizeof(uintptr_t) );
	if( pAddr->sa_family == AF_INET6 ) {
		if( memcmp( &((struct sockaddr_in6 *)pAddr)->sin6_addr, "\0\0\0\0\0\0\0\0\0\0\xff\xff", 12 ) == 0 ) {
			// convert to ipv4
			((SOCKADDR_IN*)dup)->sin_family = AF_INET;
#ifdef __LINUX__			
			((SOCKADDR_IN*)dup)->sin_addr.S_un.S_addr = ((struct sockaddr_in6 *)pAddr)->sin6_addr.s6_addr32[3];
#else
			((SOCKADDR_IN*)dup)->sin_addr.S_un.S_addr = ((struct sockaddr_in6 *)pAddr)->sin6_addr.s6_words[6] | ( ((struct sockaddr_in6 *)pAddr)->sin6_addr.s6_words[7] << 16);
#endif			
			((SOCKADDR_IN*)dup)->sin_port = ((SOCKADDR_IN*)pAddr)->sin_port;
			SOCKADDR_NAME( dup ) = strdup( GetAddrName( pAddr ) );
			SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
		}
	}
	if( ((char**)( ( (uintptr_t)pAddr ) - sizeof(char*) ))[0] )
		( (char**)( ( (uintptr_t)dup ) - sizeof( char* ) ) )[0]
				= strdup( ((char**)( ( (uintptr_t)pAddr ) - sizeof( char* ) ))[0] );
	return dup;
}

//----------------------------------------------------------------------------
// return a copy of this address...
SOCKADDR* DuplicateAddressEx( SOCKADDR *pAddr DBG_PASS ) 
{
	POINTER tmp = (POINTER)( ( (uintptr_t)pAddr ) - 2*sizeof(uintptr_t) );
	SOCKADDR *dup = AllocAddrEx( DBG_VOIDRELAY );
	POINTER tmp2 = (POINTER)( ( (uintptr_t)dup ) - 2*sizeof(uintptr_t) );
#ifdef DEBUG_ADDRESSES	
	lprintf( "Existing length: %d", SOCKADDR_LENGTH( pAddr ) );
#endif	
	MemCpy( tmp2, tmp, SOCKADDR_LENGTH( pAddr ) + 2*sizeof(uintptr_t) );
	if( ((char**)( ( (uintptr_t)pAddr ) - sizeof(char*) ))[0] )
		( (char**)( ( (uintptr_t)dup ) - sizeof( char* ) ) )[0]
				= strdup( ((char**)( ( (uintptr_t)pAddr ) - sizeof( char* ) ))[0] );
	return dup;
}

//---------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateAddress_hton)( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
	lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
	lpsaAddr->sin_addr.S_un.S_addr  = htonl(dwIP);
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//---------------------------------------------------------------------------
#if defined( __LINUX__ ) && !defined( __CYGWIN__ )
#  ifndef __LINUX__
#    define UNIX_PATH_MAX	 108
struct sockaddr_un {
#    ifdef __MAC__
	u_char   sa_len;
#    endif
	sa_family_t  sun_family;		/* AF_UNIX */
	char	       sun_path[UNIX_PATH_MAX]; /* pathname */
};
#  endif
NETWORK_PROC( SOCKADDR *,CreateUnixAddress)( CTEXTSTR path )
{
	struct sockaddr_un *lpsaAddr;
#ifdef UNICODE
	char *tmp_path = CStrDup( path );
#endif
   lpsaAddr=(struct sockaddr_un*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	((uintptr_t*)lpsaAddr)[-1] = StrLen( path ) + 1;
	lpsaAddr->sun_family = PF_UNIX;
#ifdef UNICODE
	strncpy( lpsaAddr->sun_path, tmp_path, 107 );
	Deallocate( char*, tmp_path );
#else
	strncpy( lpsaAddr->sun_path, path, 107 );
#endif

#ifdef __MAC__
	lpsaAddr->sun_len = 2+strlen( lpsaAddr->sun_path );
#endif
	return((SOCKADDR*)lpsaAddr);
}
#else
NETWORK_PROC( SOCKADDR *,CreateUnixAddress)( CTEXTSTR path )
{
	lprintf( "-- CreateUnixAddress -- not available. " );
	return NULL;
}
#endif
//---------------------------------------------------------------------------

SOCKADDR *CreateAddress( uint32_t dwIP,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if (!lpsaAddr)
		return(NULL);
	SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
	lpsaAddr->sin_family	    = AF_INET;         // InetAddress Type.
	lpsaAddr->sin_addr.S_un.S_addr  = dwIP;
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//---------------------------------------------------------------------------

SOCKADDR *CreateRemote( CTEXTSTR lpName,uint16_t nHisPort)
{
	SOCKADDR_IN *lpsaAddr;
	int conversion_success = FALSE;
	char *tmpName = NULL;
#ifdef UNICODE
	char *_lpName = CStrDup( lpName );
#  define lpName _lpName
#endif
#ifndef WIN32
	PHOSTENT phe;
	// a IP type name will never have a / in it, therefore
	// we can assume it's a unix type address....
	if( lpName && StrChr( lpName, '/' ) )
		return CreateUnixAddress( lpName );
#endif
	if( lpName[0] == '[' && lpName[StrLen( lpName ) - 1] == ']' ) {
		size_t len;
		tmpName = NewArray( char, len = StrLen( lpName ) );
		memcpy( tmpName, lpName + 1, len - 2 );
		tmpName[len - 2] = 0;
		lpName = tmpName;
	}

	lpsaAddr=(SOCKADDR_IN*)AllocAddr();
	if( !lpsaAddr )
	{
#ifdef UNICODE
		Deallocate( char *, _lpName );
#endif
		return(NULL);
	}
	SetAddrName( (SOCKADDR*)lpsaAddr, lpName );

	// if it's a numeric name... attempt to use as an address.
#ifdef __LINUX__
	if( lpName &&
		( lpName[0] >= '0' && lpName[0] <= '9' )
	  && StrChr( lpName, '.' ) )
	{
		if( inet_pton( AF_INET, lpName, &lpsaAddr->sin_addr ) > 0 )
		{
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET;         // InetAddress Type.
			conversion_success = TRUE;
		}
	}
	else if( lpName
		   && ( ( lpName[0] >= '0' && lpName[0] <= '9' )
		      || ( lpName[0] >= 'a' && lpName[0] <= 'f' )
		      || ( lpName[0] >= 'A' && lpName[0] <= 'F' )
		      || lpName[0] == ':'
		      || ( lpName[0] == '[' && lpName[StrLen( lpName ) - 1] == ']' ) )
		   && StrChr( lpName, ':' )!=StrRChr( lpName, ':' ) )
	{
		//lprintf( "Converting name:", lpName );
		if( inet_pton( AF_INET6, lpName, (struct in6_addr*)(&lpsaAddr->sin_addr+1) ) > 0 )
		{
			//lprintf( "This iset in the wrong place?" );
			((struct sockaddr_in6*)lpsaAddr)->sin6_scope_id = 0;
			SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
			lpsaAddr->sin_family       = AF_INET6;         // InetAddress Type.
			conversion_success = TRUE;
		}
	}
#endif
	if( !conversion_success )
	{
		if( lpName )
		{
#ifndef h_addr
#define h_addr h_addr_list[0]
#define H_ADDR_DEFINED
#endif
#ifdef WIN32
			{
				struct addrinfo *result;
				struct addrinfo *test;
				int error;
				//lprintf( "Or we do getaddrinfo... %s", lpName );
				error = getaddrinfo( lpName, NULL, NULL, (struct addrinfo**)&result );
				if( error == 0 )
				{
					for( test = result; test; test = test->ai_next )
					{
						//SOCKADDR *tmp;
						//AddLink( &globalNetworkData.addresses, tmp = AllocAddr() );
						//lprintf( "Copy addr: %d", test->ai_addrlen );
						MemCpy( lpsaAddr, test->ai_addr, test->ai_addrlen );
						SET_SOCKADDR_LENGTH( lpsaAddr, test->ai_addrlen );
						break;
					}
				}
				else
					lprintf( "getaddrinfo Error: %d for [%s]", error, lpName );
			}
#else //WIN32

			char *tmp = CStrDup( lpName );
#ifdef __EMSCRIPTEN__
			if(!(phe=gethostbyname(tmp) ) )
#else
         if(1)
#endif
			{
#if !defined( __EMSCRIPTEN__ )
				if( !(phe=gethostbyname2(tmp,AF_INET6) ) )
#endif
				{
#if !defined( __EMSCRIPTEN__ )
					if( !(phe=gethostbyname2(tmp,AF_INET) ) )
#endif
					{
 						// could not find the name in the host file.
						lprintf( "Could not Resolve to %s  %s", tmp, lpName );
						ReleaseAddress((SOCKADDR*)lpsaAddr);
						Deallocate( char*, tmp );
						if( tmpName ) Deallocate( char*, tmpName );
						return(NULL);
					}
#if !defined( __EMSCRIPTEN__ )
					else
					{
						//lprintf( "Strange, gethostbyname failed, but AF_INET worked... %s", tmp );
						SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
						lpsaAddr->sin_family = AF_INET;
						memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
							    phe->h_addr,
						       phe->h_length);
					}
#endif
				}
#if !defined( __EMSCRIPTEN__ )
				else
				{
					SET_SOCKADDR_LENGTH( lpsaAddr, IN6_SOCKADDR_LENGTH );
					lpsaAddr->sin_family = AF_INET6;         // InetAddress Type.
					//lprintf( "This copy:%d", phe->h_length );
#  if inline_note_never_included
					{
						__SOCKADDR_COMMON (sin6_);
						n_port_t sin6_port;        /* Transport layer port # */
						uint32_t sin6_flowinfo;     /* IPv6 flow information */
						struct in6_addr sin6_addr;  /* IPv6 address */
						uint32_t sin6_scope_id;     /* IPv6 scope-id */
					};
#  endif

					memcpy( ((struct sockaddr_in6*)lpsaAddr)->sin6_addr.s6_addr,           // save IP address from host entry.
							 phe->h_addr,
							 phe->h_length);
				}
#endif
			}
			else
			{
				Deallocate( char *, tmp );
				SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
				lpsaAddr->sin_family = AF_INET;         // InetAddress Type.
				//lprintf( "gethostbyname2:...." );
				memcpy( &lpsaAddr->sin_addr.S_un.S_addr,           // save IP address from host entry.
					 phe->h_addr,
					 phe->h_length);
			}
#endif
#ifdef H_ADDR_DEFINED
#  undef H_ADDR_DEFINED
#  undef h_addr
#endif
		}
		else
		{
			lpsaAddr->sin_family      = AF_INET;         // InetAddress Type.
			lpsaAddr->sin_addr.S_un.S_addr = 0;
			SET_SOCKADDR_LENGTH( lpsaAddr, IN_SOCKADDR_LENGTH );
		}
	}
#ifdef UNICODE
	Deallocate( char *, _lpName );
#  undef lpName
#endif
	//lprintf( "Resulting thing" );
	//DumpAddr( "RESULT ADDRESS", (SOCKADDR*)lpsaAddr );
	// put in his(destination) port number...
	if( tmpName ) Deallocate( char*, tmpName );
	lpsaAddr->sin_port         = htons(nHisPort);
	return((SOCKADDR*)lpsaAddr);
}

//----------------------------------------------------------------------------

#ifdef __cplusplus
namespace udp {
#endif
#undef FreeAddrString
#undef AddrToString
	NETWORK_PROC( void, FreeAddrString )( CTEXTSTR string DBG_PASS ) {
		PTEXT p = (PTEXT)( ( (uintptr_t)string ) - offsetof( TEXT, data.data ) );
		LineReleaseEx( p DBG_RELAY );
	}

	NETWORK_PROC( CTEXTSTR, AddrToString )( CTEXTSTR name, SOCKADDR* sa DBG_PASS ) {
		PVARTEXT pvt = VarTextCreate();

		if( !sa ) { vtprintf( pvt, "%s: NULL", name ); } else {
			BinaryToString( pvt, (uint8_t*)sa, ((sa->sa_family==AF_INET)? IN_SOCKADDR_LENGTH: IN6_SOCKADDR_LENGTH)  DBG_RELAY );
			if( sa->sa_family == AF_INET ) {
				vtprintf( pvt, "%s: (%s) %d.%d.%d.%d:%d ", name
					, ( ( (uintptr_t*)sa )[-1] & 0xFFFF0000 ) ? ( ( (char**)sa )[-1] ) : "no name"
					//*(((unsigned char *)sa)+0),
					//*(((unsigned char *)sa)+1),
					, *( ( (unsigned char*)sa ) + 4 ),
					*( ( (unsigned char*)sa ) + 5 ),
					*( ( (unsigned char*)sa ) + 6 ),
					*( ( (unsigned char*)sa ) + 7 )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 2 ) ) ) )
				);
			} else if( sa->sa_family == AF_INET6 ) {
				lprintf( "Socket address binary: %s", name );
				vtprintf( pvt, "%s: (%s) %03d %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x "
					, name
					, ( ( (uintptr_t*)sa )[-1] & 0xFFFF0000 ) ? ( ( (char**)sa )[-1] ) : "no name"
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 2 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 8 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 10 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 12 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 14 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 16 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 18 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 20 ) ) ) )
					, ntohs( *( ( (unsigned short*)( (unsigned char*)sa + 22 ) ) ) )
				);
			}
		}
		PTEXT result = VarTextGet( pvt );
		VarTextDestroy( &pvt );
		return GetText( result );
	}

#define FreeAddrString(s) FreeAddrString( s DBG_SRC )
#define AddrToString(n,s) AddrToString( n, s DBG_SRC )


NETWORK_PROC( void, DumpAddrEx)( CTEXTSTR name, SOCKADDR *sa DBG_PASS )
	{
		if( !sa ) { _lprintf(DBG_RELAY)( "%s: NULL", name ); return; }
		LogBinary( (uint8_t *)sa, SOCKADDR_LENGTH( sa ) );
		if( sa->sa_family == AF_INET ) {
			_lprintf(DBG_RELAY)( "%s: (%s) %d.%d.%d.%d:%d ", name
			       , ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
			       //*(((unsigned char *)sa)+0),
			       //*(((unsigned char *)sa)+1),
			       ,*(((unsigned char *)sa)+4),
			       *(((unsigned char *)sa)+5),
			       *(((unsigned char *)sa)+6),
			       *(((unsigned char *)sa)+7)
			       , ntohs( *(((unsigned short *)((unsigned char*)sa + 2))) )
			);
		} else if( sa->sa_family == AF_INET6 )
		{
			lprintf( "Socket address binary: %s", name );
			_lprintf(DBG_RELAY)( "%s: (%s) %03d %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x "
					 , name
					, ( ((uintptr_t*)sa)[-1] & 0xFFFF0000 )?( ((char**)sa)[-1] ) : "no name"
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+2))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+8))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+10))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+12))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+14))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+16))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+18))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+20))))
					 , ntohs(*(((unsigned short *)((unsigned char*)sa+22))))
					 );
		}

}
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *, SetAddressPort )( SOCKADDR *pAddr, uint16_t nDefaultPort )
{
	if( pAddr )
		((SOCKADDR_IN *)pAddr)->sin_port = htons(nDefaultPort);
	return pAddr;
}

//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *, SetNonDefaultPort )( SOCKADDR *pAddr, uint16_t nDefaultPort )
{
	if( pAddr && !((SOCKADDR_IN *)pAddr)->sin_port )
		((SOCKADDR_IN *)pAddr)->sin_port = htons(nDefaultPort);
	return pAddr;
}

//----------------------------------------------------------------------------

NETWORK_PROC( SOCKADDR *,CreateSockAddress)(CTEXTSTR name, uint16_t nDefaultPort )
{
// blah... should process a ip:port - but - default port?!
	uint32_t bTmpName = 0;
	char * tmp;
	SOCKADDR *sa = NULL;
	char *port;
	uint16_t wPort;
	CTEXTSTR portName = name;

#ifdef UNICODE
	char *_name = CStrDup( name );
#  define name _name
#endif
	if( name[0] == '[' ) { //-V595
		while( portName[0] && portName[0] != ']' )
			portName++;
		if( portName[0] ) portName++;
	}
	if( name && portName[0] && ( port = (char*)strrchr( portName, ':' ) ) )
	{
		tmp = StrDup( name );
		bTmpName = 1;
		port = tmp + (port-name);
		name = tmp;
		//Log1( "Found ':' assuming %s is IP:PORT", name );
		*port = 0;
		port++;
		if( port[0] )  // a trailing : could be IPV6 abbreviation.
		{
			if( isdigit( *port ) )
			{
				wPort = (short)atoi( port ); //-V595
			}
			else
			{
				struct servent *se;
				se = getservbyname( port, NULL );
				if( !se )
				{
#ifdef UNICODE
#define FMT "S"
#else
#define FMT "s"
#endif
					Log1( "Could not resolve \"%" FMT "\" as a valid service name", port );
					//return NULL;
					wPort = nDefaultPort;
				}
				else
					wPort = htons(se->s_port);
				//Log1( "port alpha - name resolve to %d", wPort );
			}
		}
		else
			wPort = nDefaultPort;
#ifdef UNICODE
#  undef name
#endif
		sa = CreateRemote( name, wPort );
		if( port )
		{
			port[-1] = ':';  // incase we obliterated it
		}
	}
	else  // no port specification...
	{
		//Log1( "%s does not have a ':'", name );
		sa = CreateRemote( name, nDefaultPort );
	}
#ifdef UNICODE
	Deallocate( char *, _name );
#endif
	if( bTmpName ) Deallocate( char*, tmp );
	return sa;
}

//----------------------------------------------------------------------------

SOCKADDR *CreateLocal(uint16_t nMyPort)
{
	char lpHostName[HOSTNAME_LEN];
	if (gethostname(lpHostName,HOSTNAME_LEN))
	{
		return(NULL);
	}
	return CreateRemote( "0.0.0.0", nMyPort );
}

//----------------------------------------------------------------------------

LOGICAL CompareAddressEx( SOCKADDR *sa1, SOCKADDR *sa2, int method )
{
	if( method == SA_COMPARE_FULL )
	{
		if( sa1 && sa2 )
		{
			if( ((SOCKADDR_IN*)sa1)->sin_family == ((SOCKADDR_IN*)sa2)->sin_family )
			{
				switch( ((SOCKADDR_IN*)sa1)->sin_family )
				{
				case AF_INET:
					{
						SOCKADDR_IN *sin1 = (SOCKADDR_IN*)sa1;
						SOCKADDR_IN *sin2 = (SOCKADDR_IN*)sa2;
						if( MemCmp( sin1, sin2, sizeof( SOCKADDR_IN ) ) == 0 )
							return 1;
					}
					break;
				default:
					xlprintf( LOG_ALWAYS )( "unhandled address type passed to compare, resulting FAILURE" );
					return 0;
				}
			}
		}
	}
	else
	{
		if( sa1 && sa2 )
		{
			if( ((SOCKADDR_IN*)sa1)->sin_family == ((SOCKADDR_IN*)sa2)->sin_family )
			{
				switch( ((SOCKADDR_IN*)sa1)->sin_family )
				{
				case AF_INET:
					{
						if( MemCmp( &((SOCKADDR_IN*)sa1)->sin_addr, &((SOCKADDR_IN*)sa2)->sin_addr, sizeof( ((SOCKADDR_IN*)sa2)->sin_addr ) ) == 0 )
							return 1;
					}
					break;
				default:
					xlprintf( LOG_ALWAYS )( "unhandled address type passed to compare, resulting FAILURE" );
					return 0;
				}
			}
		}
	}
	return 0;
}

//----------------------------------------------------------------------------

LOGICAL CompareAddress( SOCKADDR *sa1, SOCKADDR *sa2 )
{
	return CompareAddressEx( sa1, sa2, SA_COMPARE_FULL );
}

//----------------------------------------------------------------------------

PLIST GetLocalAddresses( void )
{
	return globalNetworkData.addresses;
}

//----------------------------------------------------------------------------

LOGICAL IsThisAddressMe( SOCKADDR *addr, uint16_t myport )
{
	struct interfaceAddress *test_addr;
	INDEX idx;

	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, &((SOCKADDR_IN*)test_addr->sa)->sin_addr, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return TRUE;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

LOGICAL IsBroadcastAddressForInterface( struct interfaceAddress *address, SOCKADDR *addr ) {
	if( addr->sa_family == AF_INET ) {
      //lprintf( "can test for broadcast... %08x %08x %08x", ( ((uint32_t*)(address->saMask->sa_data+2))[0] | ((uint32_t*)(addr->sa_data+2))[0] ), ((uint32_t*)address->saMask->sa_data)[0] , ((uint32_t*)addr->sa_data)[0] );
		if( ( ((uint32_t*)(address->saMask->sa_data+2))[0] | ((uint32_t*)(addr->sa_data+2))[0] ) == 0xFFFFFFFFU )
         return TRUE;
	}
   return FALSE;
}

//----------------------------------------------------------------------------

struct interfaceAddress* GetInterfaceForAddress( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();
	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return test_addr;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, 16 ) == 0 )
					{
						return test_addr;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------

SOCKADDR* GetBroadcastAddressForInterface( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();
	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, sizeof(((SOCKADDR_IN*)addr)->sin_addr)  ) == 0 )
					{
						return test_addr->saBroadcast;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->sa->sa_data + 2, 16 ) == 0 )
					{
						return test_addr->saBroadcast;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

SOCKADDR* GetInterfaceAddressForBroadcast( SOCKADDR *addr )
{
	struct interfaceAddress *test_addr;
	INDEX idx;
	if( !globalNetworkData.addresses )
		LoadNetworkAddresses();

	LIST_FORALL( globalNetworkData.addresses, idx, struct interfaceAddress *, test_addr )
	{
		if( ((SOCKADDR_IN*)addr)->sin_family == ((SOCKADDR_IN*)test_addr->sa)->sin_family )
		{
			switch( ((SOCKADDR_IN*)addr)->sin_family )
			{
			case AF_INET:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->saBroadcast->sa_data + 2, 4 ) == 0 )
					{
						return test_addr->sa;
					}
				}
				break;
			case AF_INET6:
				{
					if( MemCmp( &((SOCKADDR_IN*)addr)->sin_addr, test_addr->saBroadcast->sa_data + 2, 16  ) == 0 )
					{
						return test_addr->sa;
					}
				}
				break;
			default:
				lprintf( "Unknown comparison" );
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------

void ReleaseAddress(SOCKADDR *lpsaAddr)
{
	// sockaddr is often skewed from what I would expect it. (contains its own length)
	if( lpsaAddr )
	{
		/* strdup is used for the addr part so use free instead of release */
		free( ((POINTER*)( ( (uintptr_t)lpsaAddr ) - sizeof(uintptr_t) ))[0] );
		Deallocate(POINTER, (POINTER)( ( (uintptr_t)lpsaAddr ) - 2 * sizeof(uintptr_t) ));
	}
}

//----------------------------------------------------------------------------
// creates class C broadcast address
SOCKADDR *CreateBroadcast(uint16_t nPort)
{
	SOCKADDR_IN *bcast=(SOCKADDR_IN*)AllocAddr();
	SOCKADDR *lpMyAddr;
	if (!bcast)
		return(NULL);
	lpMyAddr = CreateLocal(0);
	SET_SOCKADDR_LENGTH( bcast, IN_SOCKADDR_LENGTH );
	bcast->sin_family	    = AF_INET;
	bcast->sin_addr.S_un.S_addr  = ((SOCKADDR_IN*)lpMyAddr)->sin_addr.S_un.S_addr;
	bcast->sin_addr.S_un.S_un_b.s_b4 = 0xFF; // Fake a subnet broadcast address
	bcast->sin_port        = htons(nPort);
	ReleaseAddress(lpMyAddr);
	return((SOCKADDR*)bcast);
}

//----------------------------------------------------------------------------

void DumpSocket( PCLIENT pc )
{
	DumpAddr( "REMOT", pc->saClient );
	DumpAddr( "LOCAL", pc->saSource );
	return;
}

#ifdef __cplusplus
namespace udp {
#endif
#undef DumpAddr
NETWORK_PROC( void, DumpAddr)( CTEXTSTR name, SOCKADDR *sa )
{
	DumpAddrEx( name, sa DBG_SRC );
}

#ifdef __cplusplus
}
#endif


//----------------------------------------------------------------------------

CTEXTSTR GetSystemName( void )
{
	// start the network with defaults... we're able to reallocate later.
#ifdef __ANDROID__
#if 0
	// dont' actually have to start winsock; but we do have to do work to get our IP
	int sock_startup = socket( AF_INET, SOCK_RAW, 0);
	if( sock_startup == -1 )
		sock_startup = socket( AF_INET, SOCK_DGRAM, 0);
	if( sock_startup == -1 )
		sock_startup = socket( AF_INET, SOCK_STREAM, 0);
	if( sock_startup >= 0 )
	{
		struct ifconf buffer;
		struct ifreq ifr[10];
		int ifc_num;
		int n;
		buffer.ifc_len = sizeof(ifr);
		buffer.ifc_ifcu.ifcu_buf = (char*)ifr;
		ioctl( sock_startup, SIOCGIFCONF, &buffer);
		ifc_num = buffer.ifc_len / sizeof(struct ifreq);
#define INT_TO_ADDR(_addr) \
(_addr & 0xFF), \
(_addr >> 8 & 0xFF), \
(_addr >> 16 & 0xFF), \
(_addr >> 24 & 0xFF)

		for( n = 0; n < ifc_num; n++ )
		{
			int sd, ifc_num, addr, bcast, mask, network;
			lprintf( "interface %d : %s", n, ifr[n].ifr_name );
			if (ifr[n].ifr_addr.sa_family != AF_INET)
			{
				continue;
			}

			/* display the interface name */
			lprintf("%d) interface: %s\n", n+1, ifr[n].ifr_name);

			/* Retrieve the IP address, broadcast address, and subnet mask. */
			if (ioctl(sd, SIOCGIFADDR, &ifr[n]) == 0)
			{
				addr = ((struct sockaddr_in *)(&ifr[n].ifr_addr))->sin_addr.s_addr;
				lprintf("%d) address: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(addr));
			}
			if (ioctl(sd, SIOCGIFBRDADDR, &ifr[n]) == 0)
			{
				bcast = ((struct sockaddr_in *)(&ifr[n].ifr_broadaddr))->sin_addr.s_addr;
				lprintf("%d) broadcast: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(bcast));
			}
			if (ioctl(sd, SIOCGIFNETMASK, &ifr[n]) == 0)
			{
				mask = ((struct sockaddr_in *)(&ifr[n].ifr_netmask))->sin_addr.s_addr;
				lprintf("%d) netmask: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(mask));
			}

			/* Compute the current network value from the address and netmask. */
			network = addr & mask;
			lprintf("%d) network: %d.%d.%d.%d\n", n+1, INT_TO_ADDR(network));
		}
		close( sock_startup );
	}
	else
#endif
		globalNetworkData.system_name = "No Name Available";
#else
	NetworkStart();
#endif
	return globalNetworkData.system_name;
}

#undef NetworkLock
#undef NetworkUnlock

NETWORK_PROC( void, GetNetworkAddressBinary )( SOCKADDR *addr, uint8_t **data, size_t *datalen ) {
	if( addr ) {
		size_t namelen;
		size_t addrlen = SOCKADDR_LENGTH( addr );
		const char * tmp = ((const char**)addr)[-1];
		if( !( (uintptr_t)tmp & 0xFFFF0000 ) )
		{
			lprintf( "corrupted sockaddr." );
			DebugBreak();
		}
		if( tmp )
		{
			namelen = StrLen( tmp );
		}
		else
			namelen = 0;
		(*datalen) = namelen + 1 + 1 + SOCKADDR_LENGTH( addr );
		(*data) = NewArray( uint8_t, (*datalen) );
		MemCpy( (*data), tmp, namelen + 1 );
		(*data)[namelen+1] = (uint8_t)addrlen;
		MemCpy( (*data) + namelen + 1, addr, addrlen );
	}
}

NETWORK_PROC( SOCKADDR *, MakeNetworkAddressFromBinary )( uintptr_t *data, size_t datalen ) {
	SOCKADDR *addr = AllocAddr();
	size_t namelen = strlen( (const char*)data );
	if( namelen ) // if empty name, don't include it.
		SetAddrName( addr, (const char*)data );
	SET_SOCKADDR_LENGTH( addr, data[1] );
	MemCpy( addr, data + 2, data[1] );
	return addr;
}


#ifdef __LINUX__

void LoadNetworkAddresses( void ) {
	struct ifaddrs *addrs, *tmp;
	struct interfaceAddress *ia;

	getifaddrs( &addrs );
	tmp = addrs;

	ia = New( struct interfaceAddress );
	ia->sa = CreateRemote( "0.0.0.0", 0 );
	ia->saMask = NULL;
	ia->saBroadcast = CreateRemote( "255.255.255.255", 0 );
	AddLink( &globalNetworkData.addresses, ia );

	for( ; tmp; tmp = tmp->ifa_next )
	{
		SOCKADDR *dup;
		if( !tmp->ifa_addr )
			continue;
#  ifndef __MAC__
		if( tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET )
			continue;
#  endif
		ia = New( struct interfaceAddress );
		dup = AllocAddr();

		if( tmp->ifa_addr->sa_family == AF_INET6 ) {
			continue;
			//memcpy( dup, tmp->ifa_addr, IN6_SOCKADDR_LENGTH );
			//SET_SOCKADDR_LENGTH( dup, IN6_SOCKADDR_LENGTH );
		}
		else {
			memcpy( dup, tmp->ifa_addr, IN_SOCKADDR_LENGTH );
			SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
		}
		ia->sa = dup;

		dup = AllocAddr();

		if( tmp->ifa_addr->sa_family == AF_INET6 ) {
			//memcpy( dup, tmp->ifa_netmask, IN6_SOCKADDR_LENGTH );
			//SET_SOCKADDR_LENGTH( dup, IN6_SOCKADDR_LENGTH );
		}
		else {
			if( tmp->ifa_netmask ) {
				memcpy( dup, tmp->ifa_netmask, IN_SOCKADDR_LENGTH );
				SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
			} else {
				memset( dup, 0, IN_SOCKADDR_LENGTH );
				SET_SOCKADDR_LENGTH( dup, IN_SOCKADDR_LENGTH );
			}
		}
		ia->saMask = dup;

		ia->saBroadcast = AllocAddr();
		ia->saBroadcast->sa_family = ia->sa->sa_family;
		ia->saBroadcast->sa_data[0] = 0;
		ia->saBroadcast->sa_data[1] = 0;
		ia->saBroadcast->sa_data[2] = (ia->sa->sa_data[2] & ia->saMask->sa_data[2]) | (~ia->saMask->sa_data[2]);
		ia->saBroadcast->sa_data[3] = (ia->sa->sa_data[3] & ia->saMask->sa_data[3]) | (~ia->saMask->sa_data[3]);
		ia->saBroadcast->sa_data[4] = (ia->sa->sa_data[4] & ia->saMask->sa_data[4]) | (~ia->saMask->sa_data[4]);
		ia->saBroadcast->sa_data[5] = (ia->sa->sa_data[5] & ia->saMask->sa_data[5]) | (~ia->saMask->sa_data[5]);
		SET_SOCKADDR_LENGTH( ia->saBroadcast, SOCKADDR_LENGTH( ia->sa ) );
		AddLink( &globalNetworkData.addresses, ia );
	}

	freeifaddrs( addrs );
}

#endif

#ifdef _WIN32
#if 0

#ifdef WIN32
	{
		struct addrinfo *result;
		struct addrinfo *test;
#ifdef _UNICODE
		char *tmp = WcharConvert( globalNetworkData.system_name );
		getaddrinfo( tmp, NULL, NULL, (struct addrinfo**)&result );
		Deallocate( char*, tmp );
#else
		getaddrinfo( globalNetworkData.system_name, NULL, NULL, (struct addrinfo**)&result );
#endif
		for( test = result; test; test = test->ai_next )
		{
			//if( test->ai_family == AF_INET )
			{
				SOCKADDR *tmp;
				AddLink( &globalNetworkData.addresses, tmp = AllocAddr() );
				((uintptr_t*)tmp)[-1] = test->ai_addrlen;
				MemCpy( tmp, test->ai_addr, test->ai_addrlen );
				//lprintf( "initialize addres..." );
				//DumpAddr( "blah", tmp );
			}
		}
	}
#endif
#endif

void LoadNetworkAddresses( void ) {
	// Declare and initialize variables
	PIP_INTERFACE_INFO pInfo;
	pInfo = (IP_INTERFACE_INFO *) malloc( sizeof(IP_INTERFACE_INFO) );
	DWORD dwRetVal = 0;

	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;

	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = New(IP_ADAPTER_INFO);
	if (pAdapterInfo == NULL) {
		lprintf("Error allocating memory needed to call GetAdaptersinfo\n");
		free( pInfo );
		return;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		Deallocate( PIP_ADAPTER_INFO, pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) NewArray(uint8_t, ulOutBufLen);
		if (pAdapterInfo == NULL) {
			lprintf("Error allocating memory needed to call GetAdaptersinfo\n");
			free( pInfo );
			return;
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {

			PIP_ADDR_STRING ipadd = &pAdapter->IpAddressList;
			for( ; ipadd; ipadd = ipadd->Next ) {
			/*
			typedef struct _IP_ADDR_STRING {
			  struct _IP_ADDR_STRING  *Next;
			  IP_ADDRESS_STRING      IpAddress;
			  IP_MASK_STRING         IpMask;
			  DWORD                  Context;
			} IP_ADDR_STRING, *PIP_ADDR_STRING;
			*/
				if( StrCmp( ipadd->IpAddress.String, "0.0.0.0" ) == 0 )
					continue;
				struct interfaceAddress *ia = New( struct interfaceAddress );
				ia->sa = CreateRemote( ipadd->IpAddress.String, 0 );
				ia->saMask = CreateRemote( ipadd->IpMask.String, 0 );
				ia->saBroadcast = AllocAddr();
				ia->saBroadcast->sa_family = ia->sa->sa_family;
				ia->saBroadcast->sa_data[0] = 0;
				ia->saBroadcast->sa_data[1] = 0;
				ia->saBroadcast->sa_data[2] = (ia->sa->sa_data[2] & ia->saMask->sa_data[2]) | (~ia->saMask->sa_data[2]);
				ia->saBroadcast->sa_data[3] = (ia->sa->sa_data[3] & ia->saMask->sa_data[3]) | (~ia->saMask->sa_data[3]);
				ia->saBroadcast->sa_data[4] = (ia->sa->sa_data[4] & ia->saMask->sa_data[4]) | (~ia->saMask->sa_data[4]);
				ia->saBroadcast->sa_data[5] = (ia->sa->sa_data[5] & ia->saMask->sa_data[5]) | (~ia->saMask->sa_data[5]);
				AddLink( &globalNetworkData.addresses, ia );
			}

			pAdapter = pAdapter->Next;
		}
	}
	else {
		lprintf( "GetAdaptersInfo failed with error: %d\n", dwRetVal );
	}
#if 0
https://msdn.microsoft.com/en-us/library/windows/desktop/aa365915%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

  do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
            GetAdaptersAddresses(AF_UNSPEC
		, GAA_FLAG_SKIP_DNS_SERVER|GAA_FLAG_SKIP_FRIENDLY_NAME|GAA_FLAG_INCLUDE_ALL_INTERFACES
		, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));
#endif

}

#endif

SACK_NETWORK_NAMESPACE_END
