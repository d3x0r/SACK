
<?php 

function get_lobbies( )
{
    $sql = "select lobby_id from games_platform_lobbies join games_platforms using(platform_id) where platform_name=$_SESSION['active_platform']";

    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 

    while ($row = mysql_fetch_assoc($query)) {
        echo $row['lobby_id'];
    }

    // Free the resources associated with the result set
    // This is done automatically at the end of the script
    mysql_free_result($query);
}

function set_platform( $platform )
{
    $_SESSION['active_platform'] = $platform;
}



?>
