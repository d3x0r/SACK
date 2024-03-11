

#include <libssh2.h>
#include <libssh2_sftp.h>


enum ssh_states {
	SSH_STATE_RESET,
	SSH_STATE_CONNECTING,
	SSH_STATE_HANDSHAKE,
	SSH_STATE_AUTH_PW,
	SSH_STATE_AUTH_PK,
	SSH_STATE_OPEN_CHANNEL,
	SSH_STATE_REQUEST_PTY,
	SSH_STATE_SETENV,
	SSH_STATE_SHELL,
	SSH_STATE_EXEC,
	SSH_STATE_SFTP,
	SSH_STATE_LISTEN,  // listening for connections on local box
	SSH_STATE_CONNECT, // connect to local address from remote listener

	SSH_STATE_FORWARD, // forward socket connection to remote (libssh2_direct)

};

struct pending_state {
	enum ssh_states state;
	uintptr_t state_data[8];
};

struct data_buffer {
	uint8_t* buffer;
	size_t length;
	size_t used;
};

struct ssh_session {
	enum ssh_states authState; // state to go to after handshake
	PDATALIST buffers;
	PDATAQUEUE pdqStates;
	PCLIENT pc;
	LIBSSH2_SESSION *session;
	uintptr_t psv;

	struct pending_state pending;
	CTEXTSTR user;
	size_t user_len;
	CTEXTSTR pass;
	size_t pass_len;
	struct keyparts* bincert;
	uint8_t* pubKey;
	size_t pubKey_len;
	uint8_t* privKey;
	size_t privKey_len;

	ssh_session_error_cb error;
	ssh_handshake_cb handshake_complete;
	ssh_auth_cb auth_complete;
	ssh_forward_listen_cb forward_listen_cb;
	ssh_forward_connect_cb forward_connect_cb;
	ssh_open_cb channel_open;
	pw_change_cb pw_change;
	ssh_sftp_open_cb sftp_open;

};

struct ssh_channel {
	LIBSSH2_CHANNEL *channel;
	uintptr_t psv;
	struct ssh_session* session;

	ssh_channel_error_cb error;
	ssh_channel_data_cb channel_data;
	ssh_channel_eof_cb channel_eof;
	ssh_channel_close_cb channel_close;
	ssh_pty_cb pty_open;
	ssh_shell_cb shell_open;
	ssh_exec_cb exec_done;
	ssh_setenv_cb setenv_done;
	ssh_forward_connect_cb direct_connect;
};

struct ssh_sftp {
	LIBSSH2_SFTP* sftp;
	struct ssh_session* session;
	uintptr_t psv;
};

struct ssh_listener {
	//PCLIENT pc;
	struct ssh_session* session;
	LIBSSH2_LISTENER* listener;
	uintptr_t psv;
	ssh_forward_listen_cb forward_listen_cb;
	ssh_forward_listen_accept_cb listen_connect_cb;
	ssh_listen_error_cb error;
};

struct ssh_director_listener {
	PCLIENT pc; // listen socket
	struct ssh_session* session;
	ssh_forward_connect_cb connected;
	CTEXTSTR localAddr;
	int localPort;
	CTEXTSTR remoteAddr;
	int remotePort;
};

// used to track state to foward local to remote
struct ssh_director {
	PCLIENT pc;  // local client connection

	struct ssh_session* session;
	struct ssh_channel* channel;
	// listener this director came from
	struct ssh_director_listener* director;
};

struct keyparts {
	// some information from https://coolaj86.com/articles/the-openssh-private-key-format/
	// also from https://github.com/openssh/openssh-portable/blob/master/PROTOCOL.key 
	uint8_t* data;
	// this points to the beginning of the binary data
	// it should be 'openssh-key-v1'
	// it's also what needs to be deallocated to release all other pointers
	char* leadin;
	struct {
		// 0 = cipher = 'none' (for no password)
		// 1 = kdfname = 'none' (for no password)
		// 2 = 0 length name, no data
		uint32_t textlen;
		uint8_t* text;
	} parts[3];

	int numkeys; // hard coded '1'

	struct {
		uint32_t pubkeylen; // public key length (padded block?)
		uint8_t* pubkey;
		uint32_t privkeylen; // public key length (padded block?)
		uint8_t* privkey;
		/*
		uint32_t textlen; // text type of pub key
		char* text;
		uint32_t rndlen;
		uint8_t* rnd;
		uint32_t pubkeylen1;
		uint8_t* pubkey1;

		uint32_t textlen2;  // text type of priv key
		uint8_t* text2;
		uint32_t datalen2; // pub0
		uint8_t* data2;

		uint32_t rndlen2; // pub1
		uint8_t* rnd2;

		uint32_t datalen3;
		uint8_t* data3;

		uint32_t datalen4;
		uint8_t* data4;

		uint32_t datalen5;
		uint8_t* data5;

		uint32_t datalen6;
		uint8_t* data6;

		uint32_t commentlen;
		uint8_t* comment;

		uint8_t* finalpad;
		uint32_t padlen; // this is supposedly %256
		*/
	} keys[1];
};


// inline macro version of ntohl (no function all - slight improvement in performance.
//#define NTOHL(x) ((( ( ((uint8_t*)x)[0] << 8 ) | ( ((uint8_t*)x)[1] )<< 8 ) | ( ((uint8_t*)x)[2]) << 8 ) | ( ((uint8_t*)x)[3] ) )

#define NTOHL1_1(x) ( ( ((uint8_t*)&(x))[0] << 24 ) \
        | ( ((uint8_t*)&(x))[1] << 16 ) \
        | ( ((uint8_t*)&(x))[2] << 8 ) \
        | ( ((uint8_t*)&(x))[3] << 0 ) )

#define NTOHL1_2(x) ((((( (( ((uint8_t*)&(x))[0] << 8 ) \
               |  ( (uint8_t*)&(x))[1] ) << 8 ) \
               |  ( (uint8_t*)&(x))[2] ) << 8 ) \
               |  ( (uint8_t*)&(x))[3] ) )

#define NTOHL1_3(n)  ((((n) & 0xff) << 24u) |   \
                (((n) & 0xff00) << 8u) |    \
                (((n) & 0xff0000) >> 8u) |  \
                (((n) & 0xff000000) >> 24u)) 

#ifdef __cplusplus 
#pragma optimize( "st", on )
inline auto bswap( uint32_t v ) noexcept { return _byteswap_ulong( v ); }
#pragma optimize( "", on )

#pragma optimize( "st", on )

constexpr uint32_t const_bswap( uint32_t v ) noexcept {
	return ( ( v & UINT32_C( 0x0000'00FF ) ) << 24 ) |
		( ( v & UINT32_C( 0x0000'FF00 ) ) << 8 ) |
		( ( v & UINT32_C( 0x00FF'0000 ) ) >> 8 ) |
		( ( v & UINT32_C( 0xFF00'0000 ) ) >> 24 );
}

#pragma optimize( "", on )

#endif


#define NTOHL2_1(a,b) ( ( ( ((uint8_t*)&(a))[0] ) = ( ((uint8_t*)&(b))[3] ) ), \
                      ( ( ((uint8_t*)&(a))[1] ) = ( ((uint8_t*)&(b))[2] ) ), \
                      ( ( ((uint8_t*)&(a))[2] ) = ( ((uint8_t*)&(b))[1] ) ), \
                      ( ( ((uint8_t*)&(a))[3] ) = ( ((uint8_t*)&(b))[0] ) ) )

#define NTOHL2_2(a,b) do { uint8_t ab[4], bb[4]; ((uint32_t*)bb)[0]=b; \
                    ( ( ( ((uint8_t*)ab)[0] ) = ( ((uint8_t*)bb)[3] ) ), \
                      ( ( ((uint8_t*)ab)[1] ) = ( ((uint8_t*)bb)[2] ) ), \
                      ( ( ((uint8_t*)ab)[2] ) = ( ((uint8_t*)bb)[1] ) ), \
                      ( ( ((uint8_t*)ab)[3] ) = ( ((uint8_t*)bb)[0] ) ) );  a = ((uint32_t*)ab)[0]; } while(0)


#define NTOHL2_3(a,b) do { uint8_t ab[4]; const uint32_t bb=b;  ( ( ( ((uint8_t*)ab)[3] ) = (uint8_t)( bb ) ), \
                      ( ( ((uint8_t*)ab)[2] ) = (uint8_t)( bb>>8 ) ), \
                      ( ( ((uint8_t*)ab)[1] ) = (uint8_t)( bb>>16 ) ),\
                      ( ( ((uint8_t*)ab)[0] ) = (uint8_t)( bb>>24 ) ) ); \
                      a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL2_4(a,b) do { uint8_t *ab, *bb; (ab)=((uint8_t*)&a)+3; (bb)=(uint8_t*)&b; \
                    ( ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) ), ab--, bb++, \
                      ( ( ab[0] ) = ( bb[0] ) )  );  a = ((uint32_t*)ab)[0]; } while(0)

#define NTOHL2_5(a,b) do { uint8_t *ab, *bb; (ab)=((uint8_t*)&a)+3; (bb)=(uint8_t*)&b; \
                      (ab--)[0] = (bb++)[0]; \
                      (ab--)[0] = (bb++)[0]; \
                      (ab--)[0] = (bb++)[0]; \
                      ab[0] = bb[0];  a = ((uint32_t*)ab)[0]; } while(0)

#ifdef _MSC_VER
#  undef NTOHL2
#  undef NTOHL
// these do optmize to bswap in release build...
#  define NTOHL(b)  (_byteswap_ulong( b ))
#  define NTOHL2(a,b)  ((a) = _byteswap_ulong( b ))
#elif defined( __GNUC__ ) || defined( __clang__ ) || defined( __llvm__ )
#  undef NTOHL2
#  undef NTOHL
#  define NTOHL2(a,b)  ((a) = __builtin_bswap32( b ))
#  define NTOHL(b)  ( __builtin_bswap32(b))
#else
#  define HTOHL2 NTOHL2_3
#  define HTONL HTONL1_3
#endif

