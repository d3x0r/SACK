
enum ssh_states {
	SSH_STATE_RESET,
	SSH_STATE_HANDSHAKE,
	SSH_STATE_AUTH_PW,
	SSH_STATE_AUTH_PK,
	SSH_STATE_OPEN_CHANNEL,
	SSH_STATE_REQUEST_PTY,
	SSH_STATE_SETENV,
	SSH_STATE_SHELL,
};

struct pending_state {
	enum ssh_states state;
	uintptr_t state_data[6];
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

	void (*handshake_complete)(uintptr_t, CTEXTSTR);
	void (*auth_complete)(uintptr_t, LOGICAL success);
	uintptr_t (*channel_open)(uintptr_t, struct ssh_channel*);
	void ( *pw_change )( uintptr_t, char** newpw, int* newpw_len );

	void (*pty_open)(uintptr_t, struct ssh_channel*);
	void (*error)(int err, const char*f, ...);

	void (*auth)( void );
};

struct ssh_channel {
	LIBSSH2_CHANNEL *channel;
	uintptr_t psv;
	struct ssh_session* session;
	void (*channel_data)(uintptr_t psv, struct ssh_channel*channel, int stream, const uint8_t *data, size_t len);
	void ( *channel_eof )( uintptr_t psv, struct ssh_channel* channel );
	void (*channel_close)(uintptr_t psv, struct ssh_channel*channel);
	void ( *pty_open )( uintptr_t psv, LOGICAL success );

};
