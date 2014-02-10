<?php 
/***************************** 
  File: includes/config.php 
  Written by: Frost of Slunked.com 
  Tutorial: User Registration and Login System 
******************************/ 
// start the session before any output. 
session_start(); 
 
// Set the folder for our includes 
$sFolder = '/login';  
 
/*************** 
  Database Connection  
    You will need to change the user (user)  
    and password (password) to what your database information uses.  
    Same with the database name if you used something else. 
****************/ 
mysql_connect('localhost', 'root', 'dkc408a1f') or trigger_error("Unable to connect to the database: " . mysql_error()); 
mysql_select_db('fortunet') or trigger_error("Unable to switch to the database: " . mysql_error()); 
 
/*************** 
  password salts are used to ensure a secure password 
  hash and make your passwords much harder to be broken into 
  Change these to be whatever you want, just try and limit them to 
  10-20 characters each to avoid collisions.  
****************/ 
define('SALT1', '24859f@#$#@$'); 
define('SALT2', '^&@#_-=+Afda$#%'); 
 
// require the function file 
require_once($_SERVER['DOCUMENT_ROOT'] . $sFolder . '/includes/functions.php'); 
 
// default the error variable to empty. 
$_SESSION['error'] = ""; 
 
// declare $sOutput so we do not have to do this on each page. 
$sOutput=""; 
?>