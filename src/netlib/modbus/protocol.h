

#ifdef _MSC_VER
#pragma pack (push, 1)
#endif

// encode as BIG ENDIAN

// data for Message Exception Reply
PREFIX_PACKED struct command_exception_data
{
	_8 exception_code;
} PACKED;

// data for Message ID 1,2 (command )
PREFIX_PACKED struct read_digital_command_data
{
	_8 start_address[2];  /* 0-65535 */
   _8 quantity[2];       /* 0-2000 */
} PACKED;

// data for Message ID 1,2 (reply)
PREFIX_PACKED struct read_digital_reply_data
{
	_8 byte_count;      /* inputs * 8 */
   _8 status[1];       /* status of coils  27-20;34-28;etc*/
} PACKED;

// data for Message ID 1 (command )
PREFIX_PACKED struct read_analog_command_data
{
	_8 start_address[2];  /* 0-65535 */
   _8 quantity[2];       /* 1-125 */
} PACKED;

// data for Message ID 1 (reply)
PREFIX_PACKED struct read_analog_reply_data
{
	_8 byte_count;      /* inputs * 8; bytes * 2*/
   _8 status[1][2];       /* status of coils  27-20;34-28;etc*/
} PACKED;





PREFIX_PACKED struct modbus_frame
{
	_8 msg_command;
	union {
      struct command_exception_data     command_exception;
      struct read_digital_command_data  read_digital_command;
      struct read_digital_reply_data    read_digital_reply;
      struct read_analog_command_data  read_analog_command;
      struct read_analog_reply_data    read_analog_reply;
	} data;
} PACKED;

enum modbus_protocol_commands
{
	MODBUS_PROT_EXCEPTION             = 0x80,
	MODBUS_PROT_READ_COILS            = 0x01,  // read_digital_...
   MODBUS_PROT_READ_DISCRETE         = 0x02,  // read_digital_...
	MODBUS_PROT_READ_HOLDING_REGISTER = 0x03,  // read_analog_...
	MODBUS_PROT_READ_HOLDING_REGISTER = 0x04,  // read_analog_...


};

#ifdef _MSC_VER
#pragma pack (pop)
#endif
