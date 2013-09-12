

enum 
{
	NET_STATE_RESET = 0
,NET_STATE_COMMAND = 10
, NET_STATE_GET_TASKNAME_LENGTH
,NET_STATE_GET_TASKNAME_DATA

, NET_STATE_GET_DEPEND_COUNT
, NET_STATE_GET_DEPEND_DATA
, NET_STATE_GET_TASKID
, NET_STATE_GET_SYSTEM_STATUS

}network_states;

enum {
	NL_BUFFER
	  , NL_STATE
     , NL_READSTATE
     , NL_COMMAND
};

enum {
	LIST_TASKS   // command from avatar to query list of tasks
	  , TASK_NAME // result taskname to avatar from summoner
	  , TASK_LIST_DONE // taskname is done - in case there are none.
	  , LIST_TASK_DEPENDS
	  , TASK_DEPEND
	  , SYSTEM_STATUS
	  , SUMMONER_SYSTEM_RESUME  // continues loading tasks
	  , SUMMONER_SYSTEM_SUSPEND // suspends summoner load operation
	  , SUMMONER_SYSTEM_STOP  // stops all tasks, and suspends the summoner
	  , SUMMONER_TASK_START  // continues loading tasks
	  , SUMMONER_TASK_SUSPEND // suspends summoner load operation
	  , SUMMONER_TASK_STOP  // stops all tasks, and suspends the summoner
}messages;


