//
// Ping.h
//

#pragma pack(1)

#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8
SACK_NETWORK_NAMESPACE

// IP Header -- RFC 791
typedef struct tagIPHDR
{
	uint8_t  VIHL;			// Version and IHL
	uint8_t	TOS;			// Type Of Service
	short	TotLen;			// Total Length
	short	ID;				// Identification
	short	FlagOff;		// Flags and Fragment Offset
	uint8_t	TTL;			// Time To Live
	uint8_t	Protocol;		// Protocol
	uint16_t	Checksum;		// Checksum
	struct	in_addr iaSrc;	// Internet Address - Source
	struct	in_addr iaDst;	// Internet Address - Destination
}IPHDR, *PIPHDR;


// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	uint8_t	Type;			// Type
	uint8_t	Code;			// Code
	uint16_t	Checksum;		// Checksum
	uint16_t	ID;				// Identification
	uint16_t	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, *PICMPHDR;


#define REQ_DATASIZE 32		// Echo Request Data size

// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	uint64_t		dwTime;
	char	cData[REQ_DATASIZE];
}ECHOREQUEST, *PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, *PECHOREPLY;

SACK_NETWORK_NAMESPACE_END

#pragma pack()

// $Log: $
