<?php 
/***************************** 
  File: includes/functions.php 
  Written by: Frost of Slunked.com 
  Tutorial: User Registration and Login System 
******************************/ 
 
/*********** 
  bool createAccount (string $pUsername, string $pPassword) 
    Attempt to create an account for the passed in  
    username and password. 
************/ 
function createAccount($pUsername, $pPassword) { 
  // First check we have data passed in. 
  if (!empty($pUsername) && !empty($pPassword)) { 
    $uLen = strlen($pUsername); 
    $pLen = strlen($pPassword); 
     
    // escape the $pUsername to avoid SQL Injections 
    $eUsername = mysql_real_escape_string($pUsername); 
    $sql = "SELECT username FROM users WHERE username = '" . $eUsername . "' LIMIT 1"; 
 
    // Note the use of trigger_error instead of or die. 
    $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
 
    // Error checks (Should be explained with the error) 
    if ($uLen < 4 || $uLen > 11) { 
      $_SESSION['error'] = "Username must be between 4 and 11 characters."; 
    }elseif ($pLen < 6) { 
      $_SESSION['error'] = "Password must be longer then 6 characters."; 
    }elseif (mysql_num_rows($query) == 1) { 
      $_SESSION['error'] = "Username already exists."; 
    }else { 
      // All errors passed lets 
      // Create our insert SQL by hashing the password and using the escaped Username. 
      $sql = "INSERT INTO users (`username`, `password`) VALUES ('" . $eUsername . "', '" . hashPassword($pPassword, SALT1, SALT2) . "');"; 
       
      $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
       
      if ($query) { 
        return true; 
      }   
    } 
  } 
   
  return false; 
} 
 
/*********** 
  string hashPassword (string $pPassword, string $pSalt1, string $pSalt2) 
    This will create a SHA1 hash of the password 
    using 2 salts that the user specifies. 
************/ 
function hashPassword($pPassword, $pSalt1="2345#$%@3e", $pSalt2="taesa%#@2%^#") { 
  return sha1(md5($pSalt2 . $pPassword . $pSalt1)); 
} 
 
/*********** 
  bool loggedIn 
    verifies that session data is in tack 
    and the user is valid for this session. 
************/ 
function loggedIn() { 
  // check both loggedin and username to verify user. 
  if (isset($_SESSION['loggedin']) && isset($_SESSION['username'])) { 
    return true; 
  } 
   
  return false; 
} 
 
/*********** 
  bool logoutUser  
    Log out a user by unsetting the session variable. 
************/ 
function logoutUser() { 
  // using unset will remove the variable 
  // and thus logging off the user. 
  unset($_SESSION['username']); 
  unset($_SESSION['loggedin']); 
   
  return true; 
} 
 
/*********** 
  bool validateUser 
    Attempt to verify that a username / password 
    combination are valid. If they are it will set 
    cookies and session data then return true.  
    If they are not valid it simply returns false.  
************/ 
function validateUser($pUsername, $pPassword) { 
  // See if the username and password are valid. 
  $sql = "SELECT username FROM users  
    WHERE username = '" . mysql_real_escape_string($pUsername) . "' AND password = '" . hashPassword($pPassword, SALT1, SALT2) . "' LIMIT 1"; 
  $query = mysql_query($sql) or trigger_error("Query Failed: " . mysql_error()); 
   
  // If one row was returned, the user was logged in! 
  if (mysql_num_rows($query) == 1) { 
    $row = mysql_fetch_assoc($query); 
    $_SESSION['username'] = $row['username']; 
    $_SESSION['loggedin'] = true; 
       
    return true; 
  } 
   
   
  return false; 
} 
?>