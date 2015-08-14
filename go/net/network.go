package net

/*
#cgo LDFLAGS: -lbag
#include <network.h>

void ListenNotify_cgo( PCLIENT listener, PCLIENT newClient );
extern int PTRSZVAL_sz;
*/
import "C"
import "sync"
import "unsafe"
import "sack/dispose"
//import "reflect"

type (
	IUDPNetwork interface {
		Create( ) *Network, IUDPNetwork
		OnReadPacket( buffer []byte, source *IPAddress )
		OnWritePacket( buffer []byte, source *IPAddress )
		
	}
	IServerNetwork interface {
		Create() *Network, IServerNetwork
		OnAccept()
		OnClose()
		OnRead(buffer []byte)
		//OnReadPacket( buffer []byte, source_addr *IPAddress )
		OnWrite( buffer []byte )

	}
	IClientNetwork interface {
		Create() *Network, IClientNetwork
		OnConnect()
		OnClose()
		OnRead(buffer []byte)
		//OnReadPacket( buffer []byte, source_addr *IPAddress )
		//OnWrite( buffer []byte )

	}
	Network struct {
		IDispose
		pc               C.PCLIENT
		is_server        bool
		server_callbacks IServerNetwork
		client_callbacks IClientNetwork
		udp_callbacks IUDPNetwork
		read_buffer []byte
	}
	IPAddress      struct{ addr *C.SOCKADDR }
	networkPrivate struct {
		init_flag   sync.Once
		max_clients int
	}
)

var (
	local networkPrivate
)

func init() {
	local.init_flag = sync.Once{}
	local.max_clients = 256
}

func (net Network)Dispose() {
	net.server_callbacks = nil
	net.client_callbacks = nil
	net.udp_callbacks = nil
}

func SetMaxClients(max int) {
	local.max_clients = max
}

func doEnable() {
	C.NetworkWait(nil, C._16(local.max_clients), C.int(int(unsafe.Sizeof(&Network{}))/int(C.PTRSZVAL_sz)))
}

func enable() {
	local.init_flag.Do(doEnable)
}

func (n *Network) SetCallbacks(callbacks IServerNetwork) {
	n.server_callbacks = callbacks
}

//export ListenCallback
func ListenCallback(l unsafe.Pointer, new_pclient C.PCLIENT) {
	var listener *Network  = (*Network)(l)

	//new_client.callbacks = listener.callbacks
	var new_client *Network
	var new_iClient IServerNetwork
	new_client, new_iClient = listener.server_callbacks.Create( )
	new_client <- c
	new_client.pc = new_pclient
	C.SetNetworkLong(new_pclient, 0, C.PTRSZVAL(uintptr(unsafe.Pointer(&new_client))))
	
	new_iClient.OnAccept()

}

func (sock Network) OnRead(data []byte) {
}

func (sock Network) OnReadPacket(data []byte, addr *IPAddress) {
}

func (sock Network) OnClose() {
}

func (sock Network) OnWrite( buffer []byte ) {
}

func (sock Network) OnWritePacket( buffer []byte ) {
}

func (sock Network) OnAccept() {
}

func (sock Network) OnConnect() {
}

func Listen(listener IServerNetwork, port int) *Network {
	n := Network{server_callbacks: listener}
	n.pc = C.OpenTCPListenerExx(C._16(port), (*[0]byte)(C.ListenNotify_cgo))
	C.SetNetworkLong(n.pc, 0, C.PTRSZVAL(uintptr(unsafe.Pointer(&n))))
	return &n
}

func ListenAt(listener IServerNetwork, addr *IPAddress) *Network {
	n := Network{server_callbacks: listener}

	C.OpenTCPListenerAddrExx((*C.SOCKADDR)(addr.addr), (*[0]byte)(C.ListenNotify_cgo))
	C.SetNetworkLong(n.pc, 0, C.PTRSZVAL(uintptr(unsafe.Pointer(&n))))
	return &n
}

func NewAddress(addr string, port int) *IPAddress {
	//C.char
	var result IPAddress = IPAddress{C.CreateSockAddress(C.CString(addr), C._16(port))}
	return &result
}

func GetLocalAddresses() []*IPAddress {
	return nil

}

func (ip IPAddress) ToString() string {
	return ""
}

func (net Network) ToString() string {
	return ""
}

func (n Network) Write(buf []byte) {

}

func (n Network) WriteTo(buf []byte, addr *IPAddress ) {
	
}

func (n Network) Read(buf []byte) {

}

