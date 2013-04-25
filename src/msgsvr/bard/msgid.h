

#define BARD_SERVICE_NAME "bard_service"

enum messages {
	MSG_RegisterSimpleEvent = MSG_UserServiceMessages
				  , MSG_IssueSimpleEvent
				  , MSG_LASTMESSAGE
};

enum events {
	MSG_DispatchSimpleEvent = MSG_EventUser
				, MSG_LASTEVENT
};



