

<?php 

// use the comon database connector
require($_SERVER['DOCUMENT_ROOT'] . '/login/includes/config.php'); 


//------------------
    $sql = "create table if not exists games_user_info ( user_id int auto_increment
                                                , system_user_id int
    						, PRIMARY KEY(user_id)
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error());
    mysql_free_result($query);

//------------------
    $sql = "create table if not exists games_platforms ( platform_id int auto_increment
                                                , platform_name varchar(64)
    						, PRIMARY KEY(platform_id)
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
    mysql_free_result($query);

//------------------
    $sql = "create table if not exists games_lobbies ( lobby_id int auto_increment
                                                , lobby_name varchar(64)
    						, PRIMARY KEY(lobby_id)
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
    mysql_free_result($query);

//------------------
    $sql = "create table if not exists games_table_info ( table_id int auto_increment
    						, PRIMARY KEY(table_id)
                                                ); engine=innodb";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
    mysql_free_result($query);

//------------------
    $sql = "create table if not exists games_platform_lobby ( platform_lobby_id int auto_increment
                                                , platform_id int
    						, lobby_id int
                                                , PRIMARY KEY(lobby_table_id)
                                                , CONSTRAINT `FK_platform` FOREIGN KEY (`platform_id`) REFERENCES `games_platforms` (`platform_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                , CONSTRAINT `FK_lobby` FOREIGN KEY (`lobby_id`) REFERENCES `games_lobbies` (`lobby_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
    mysql_free_result($query);

//------------------
    $sql = "create table if not exists games_lobby_table ( lobby_table_id int auto_increment
                                                , lobby_id int
    						, table_id int
                                                , PRIMARY KEY(lobby_table_id)
                                                , CONSTRAINT `FK_lobby` FOREIGN KEY (`lobby_id`) REFERENCES `games_lobbies` (`lobby_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                , CONSTRAINT `FK_table` FOREIGN KEY (`table_id`) REFERENCES `games_table_info` (`table_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
    mysql_free_result($query);

//------------------
    $sql = "create table if not exists games_lobby_users ( lobby_user_id int auto_increment
                                                , lobby_id int
                                                , user_id int
    						, PRIMARY KEY(lobby_id)
                                                , CONSTRAINT `FK_lobby` FOREIGN KEY (`lobby_id`) REFERENCES `games_lobbies` (`lobby_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                , CONSTRAINT `FK_user` FOREIGN KEY (`user_id`) REFERENCES `games_user_info` (`user_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
    mysql_free_result($query);


//------------------
    $sql = "create table if not exists games_table_users ( table_user_id int auto_increment
                                                , table_id int
                                                , user_id int
                                                , spectator int
    						, PRIMARY KEY(table_user_id)
                                                , CONSTRAINT `FK_table` FOREIGN KEY (`table_id`) REFERENCES `games_table_info` (`table_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                , CONSTRAINT `FK_user` FOREIGN KEY (`user_id`) REFERENCES `games_user_info` (`user_id`) ON DELETE CASCADE ON UPDATE CASCADE
                                                ) engine=innodb;";
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error());
    mysql_free_result($query);

?>



