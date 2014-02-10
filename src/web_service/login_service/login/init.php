

<?php 

require($_SERVER['DOCUMENT_ROOT'] . '/login/includes/config.php'); 


    $sql = "create table users ( uid INT NOT NULL auto_increment, username varchar(20) NOT NULL, password char(40) NOT NULL, primary key(uid), unique(username) );"; 
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 



?>