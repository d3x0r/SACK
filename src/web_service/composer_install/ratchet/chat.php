<?php
use Ratchet\MessageComponentInterface;
use Ratchet\ConnectionInterface;
use Ratchet\Server\IoServer;
use Ratchet\WebSocket\WsServer;

    require __DIR__ . '/vendor/autoload.php';

/**
 * chat.php
 * Send any incoming messages to all connected clients (except sender)
 */
class Chat implements MessageComponentInterface {
    protected $clients;

    public function __construct() {
        //error_log( "created?\n" );
        $this->clients = new \SplObjectStorage;
    }

    public function onOpen(ConnectionInterface $conn) {
    	//error_log( "Connected" );
        $this->clients->attach($conn);
    }

    public function onMessage(ConnectionInterface $from, $msg) {
    	//error_log( "Received" );
        foreach ($this->clients as $client) {
            if ($from != $client) {
	    	//error_log( "Send to someone" );
                $client->send($msg);
            }
        }
    }

    public function onClose(ConnectionInterface $conn) {
        $this->clients->detach($conn);
    }

    public function onError(ConnectionInterface $conn, \Exception $e) {
        //error_log( "error?\n" );
        $conn->close();
    }
}

    // Run the server application through the WebSocket protocol on port 8080
    $server = IoServer::factory(new Chat(), 8080);
    $server->run();
?>
