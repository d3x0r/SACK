

#include <libssh2.h>
#include <libssh2_sftp.h>


enum ssh_states {
	SSH_STATE_RESET,
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
	CTEXTSTR pubKey;
	size_t pubKey_len;
	CTEXTSTR privKey;
	size_t privKey_len;

	ssh_handshake_cb handshake_complete;
	ssh_auth_cb auth_complete;
	ssh_listen_cb listen_cb;
	ssh_open_cb channel_open;
	pw_change_cb pw_change;
	ssh_sftp_open_cb sftp_open;

	ssh_pty_cb pty_open;
	//void (*error)(int err, const char*f, ...);

	//void (*auth)( void );
};

struct ssh_channel {
	LIBSSH2_CHANNEL *channel;
	uintptr_t psv;
	struct ssh_session* session;
	ssh_channel_data_cb channel_data;
	ssh_channel_eof_cb channel_eof;
	ssh_channel_close_cb channel_close;
	ssh_pty_cb pty_open;
	ssh_shell_cb shell_open;
	ssh_exec_cb exec_done;
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
	ssh_listen_connect_cb connect_cb;
};
